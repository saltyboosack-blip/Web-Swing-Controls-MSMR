#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace trueswing::rebuild::runtime {

enum class ControllerWebOwner : std::uint8_t {
    None,
    Left,
    Right,
};

struct ControllerWebInputSample final {
    std::uint64_t sourceEpoch{};
    bool connected{};
    bool focused{};
    bool runtimeReady{};
    bool airborneProven{};
    bool leftShoulderHeld{};
    bool nativeSwingAvailable{};
    double leftTrigger{};
    double rightTrigger{};
};

struct ControllerWebInputDecision final {
    // While layerActive, both trigger axes belong to the mod. The adapter
    // neutralizes them in the game-facing device state, then drives only the
    // game's normalized native Swing trigger. Outside the layer, unowned axes
    // remain native and unchanged.
    bool layerActive{};
    bool consumeLeftTrigger{};
    bool consumeRightTrigger{};
    bool leftAttach{};
    bool leftRelease{};
    bool rightAttach{};
    bool rightRelease{};
    bool nativeSwingPress{};
    bool nativeSwingRelease{};
};

// Pure controller policy. It owns input edges only; it has no movement,
// velocity, collision, timing, or rope-solver state.
class ControllerWebInputPolicy final {
public:
    // Inclusive Schmitt thresholds. A captured trigger remains held throughout
    // the band, preventing analog jitter from producing repeated Swing edges.
    static constexpr double kTriggerPressThreshold = 0.50;
    static constexpr double kTriggerReleaseThreshold = 0.25;

    [[nodiscard]] ControllerWebInputDecision Update(
        const ControllerWebInputSample& sample) noexcept {
        ControllerWebInputDecision decision{};

        if (!sample.connected) {
            CancelOwner(decision);
            ResetSource();
            return decision;
        }

        if (!sourceKnown_ || sample.sourceEpoch != sourceEpoch_) {
            CancelOwner(decision);
            ResetSource();
            sourceKnown_ = true;
            sourceEpoch_ = sample.sourceEpoch;
        }

        if (!sample.focused) {
            decision.consumeLeftTrigger = left_.captured;
            decision.consumeRightTrigger = right_.captured;
            CancelOwner(decision);
            left_.armed = false;
            right_.armed = false;
            return decision;
        }

        if (!std::isfinite(sample.leftTrigger) ||
            !std::isfinite(sample.rightTrigger)) {
            // Intercept only axes already owned by this policy. Keep those
            // captures quarantined until valid neutral samples arrive; an
            // unowned axis remains entirely under the native input layer.
            decision.consumeLeftTrigger = left_.captured;
            decision.consumeRightTrigger = right_.captured;
            CancelOwner(decision);
            left_.armed = false;
            right_.armed = false;
            return decision;
        }

        const double leftValue = Normalize(sample.leftTrigger);
        const double rightValue = Normalize(sample.rightTrigger);
        const TriggerTransition leftTransition =
            ObserveTrigger(left_, leftValue);
        const TriggerTransition rightTransition =
            ObserveTrigger(right_, rightValue);

        decision.consumeLeftTrigger = left_.captured;
        decision.consumeRightTrigger = right_.captured;

        const bool gate = sample.runtimeReady && sample.airborneProven &&
                          sample.leftShoulderHeld &&
                          sample.nativeSwingAvailable;
        decision.layerActive = gate;
        if (gate) {
            // L1/LB is a true input layer: neither physical trigger may leak
            // into its ordinary action while the airborne layer is active.
            decision.consumeLeftTrigger = true;
            decision.consumeRightTrigger = true;
        }
        if (owner_ != ControllerWebOwner::None && !gate) {
            CancelOwner(decision);
        }

        // Matching releases are consumed even after gate loss. This prevents a
        // captured high analog state from leaking into vanilla input mid-hold.
        if (leftTransition.released && left_.captured) {
            decision.consumeLeftTrigger = true;
            if (owner_ == ControllerWebOwner::Left) {
                CancelOwner(decision);
            }
            left_.captured = false;
        }
        if (rightTransition.released && right_.captured) {
            decision.consumeRightTrigger = true;
            if (owner_ == ControllerWebOwner::Right) {
                CancelOwner(decision);
            }
            right_.captured = false;
        }

        const bool freshLeft = leftTransition.pressed &&
                               leftTransition.wasArmed &&
                               !left_.captured;
        const bool freshRight = rightTransition.pressed &&
                                rightTransition.wasArmed &&
                                !right_.captured;
        if (!gate || (!freshLeft && !freshRight)) {
            return decision;
        }

        if (decision.nativeSwingRelease) {
            // Do not synthesize a Shift up/down restart in one native update.
            // A fresh opposite trigger is quarantined until release, then the
            // user can deliberately press it again for the next web.
            if (freshLeft) {
                left_.captured = true;
                decision.consumeLeftTrigger = true;
            }
            if (freshRight) {
                right_.captured = true;
                decision.consumeRightTrigger = true;
            }
            return decision;
        }

        if (owner_ == ControllerWebOwner::None && freshLeft && freshRight) {
            // One snapshot cannot prove which trigger crossed first. Swallow
            // both until release rather than choosing an arbitrary web side.
            left_.captured = true;
            right_.captured = true;
            decision.consumeLeftTrigger = true;
            decision.consumeRightTrigger = true;
            return decision;
        }

        if (owner_ == ControllerWebOwner::None) {
            if (freshLeft) {
                CaptureOwner(ControllerWebOwner::Left, decision);
            } else {
                CaptureOwner(ControllerWebOwner::Right, decision);
            }
            return decision;
        }

        // The first trigger owns the game's single native rope. A later trigger
        // is suppressed until its own release and never steals or inherits it.
        if (freshLeft) {
            left_.captured = true;
            decision.consumeLeftTrigger = true;
        }
        if (freshRight) {
            right_.captured = true;
            decision.consumeRightTrigger = true;
        }
        return decision;
    }

    // Runtime shutdown uses the same exact-once release path before hooks or
    // native input injection are disabled.
    [[nodiscard]] ControllerWebInputDecision CancelAndReset() noexcept {
        ControllerWebInputDecision decision{};
        CancelOwner(decision);
        ResetSource();
        return decision;
    }

    [[nodiscard]] ControllerWebOwner CurrentOwner() const noexcept {
        return owner_;
    }
    [[nodiscard]] bool NativeSwingHeld() const noexcept {
        return nativeSwingHeld_;
    }
    [[nodiscard]] bool LeftCaptured() const noexcept {
        return left_.captured;
    }
    [[nodiscard]] bool RightCaptured() const noexcept {
        return right_.captured;
    }
    [[nodiscard]] bool LeftTriggerDown() const noexcept {
        return left_.down;
    }
    [[nodiscard]] bool RightTriggerDown() const noexcept {
        return right_.down;
    }

private:
    struct TriggerState final {
        bool down{};
        bool captured{};
        bool armed{};
    };

    struct TriggerTransition final {
        bool pressed{};
        bool released{};
        bool wasArmed{};
    };

    [[nodiscard]] static double Normalize(double value) noexcept {
        return std::clamp(value, 0.0, 1.0);
    }

    [[nodiscard]] static TriggerTransition ObserveTrigger(
        TriggerState& trigger, double value) noexcept {
        TriggerTransition transition{};
        transition.wasArmed = trigger.armed;
        if (!trigger.down && value >= kTriggerPressThreshold) {
            trigger.down = true;
            transition.pressed = true;
        } else if (trigger.down && value <= kTriggerReleaseThreshold) {
            trigger.down = false;
            transition.released = true;
        }
        if (!trigger.down && value <= kTriggerReleaseThreshold) {
            trigger.armed = true;
        }
        return transition;
    }

    void CaptureOwner(ControllerWebOwner owner,
                      ControllerWebInputDecision& decision) noexcept {
        owner_ = owner;
        nativeSwingHeld_ = true;
        decision.nativeSwingPress = true;
        if (owner == ControllerWebOwner::Left) {
            left_.captured = true;
            decision.consumeLeftTrigger = true;
            decision.leftAttach = true;
        } else {
            right_.captured = true;
            decision.consumeRightTrigger = true;
            decision.rightAttach = true;
        }
    }

    void CancelOwner(ControllerWebInputDecision& decision) noexcept {
        if (owner_ == ControllerWebOwner::Left) {
            decision.leftRelease = true;
        } else if (owner_ == ControllerWebOwner::Right) {
            decision.rightRelease = true;
        }
        if (nativeSwingHeld_) {
            decision.nativeSwingRelease = true;
        }
        owner_ = ControllerWebOwner::None;
        nativeSwingHeld_ = false;
    }

    void ResetSource() noexcept {
        sourceKnown_ = false;
        sourceEpoch_ = 0U;
        left_ = {};
        right_ = {};
        owner_ = ControllerWebOwner::None;
        nativeSwingHeld_ = false;
    }

    bool sourceKnown_{};
    std::uint64_t sourceEpoch_{};
    TriggerState left_{};
    TriggerState right_{};
    ControllerWebOwner owner_{ControllerWebOwner::None};
    bool nativeSwingHeld_{};
};

} // namespace trueswing::rebuild::runtime
