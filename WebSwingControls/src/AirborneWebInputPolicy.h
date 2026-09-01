#pragma once

#include <cstdint>

namespace trueswing::rebuild::runtime {

enum class WebSide : std::uint8_t {
    Left,
    Right,
};

enum class MouseButtonTransition : std::uint8_t {
    LeftDown,
    LeftUp,
    RightDown,
    RightUp,
};

struct WebInputEligibility final {
    bool foreground{};
    bool runtimeReady{};
    bool airborneProven{};
};

struct WebInputDecision final {
    bool consume{};
    bool leftAttach{};
    bool leftRelease{};
    bool rightAttach{};
    bool rightRelease{};
    bool nativeSwingPress{};
    bool nativeSwingRelease{};
};

// Pure state machine for the Raw Input adapter. A mouse down is captured only
// when every gameplay gate is valid. Once a down is captured, its matching up
// is always consumed so a synthetic/native Swing hold cannot stick or leak an
// unmatched mouse edge. One native rope exists, so the first held mouse button
// owns it; a second simultaneous button is swallowed until released.
class AirborneWebInputPolicy final {
public:
    [[nodiscard]] WebInputDecision Update(
        MouseButtonTransition transition,
        const WebInputEligibility& eligibility) noexcept {
        switch (transition) {
        case MouseButtonTransition::LeftDown:
            return Down(WebSide::Left, eligibility);
        case MouseButtonTransition::LeftUp:
            return Up(WebSide::Left);
        case MouseButtonTransition::RightDown:
            return Down(WebSide::Right, eligibility);
        case MouseButtonTransition::RightUp:
            return Up(WebSide::Right);
        }
        return {};
    }

    // Stops the pending native hold when identity or runtime ownership is
    // lost. This is input ownership only; native Submit/Enter remain the sole
    // authority for whether a rope actually exists. Capture latches remain
    // set until physical ups unless the adapter resets them at a focus epoch.
    [[nodiscard]] WebInputDecision CancelNativeHold() noexcept {
        WebInputDecision decision{};
        const bool hadAny = leftOwnsNativeHold_ || rightOwnsNativeHold_;
        decision.leftRelease = leftOwnsNativeHold_;
        decision.rightRelease = rightOwnsNativeHold_;
        decision.nativeSwingRelease = hadAny;
        leftOwnsNativeHold_ = false;
        rightOwnsNativeHold_ = false;
        return decision;
    }

    void Reset() noexcept {
        leftCaptured_ = false;
        rightCaptured_ = false;
        leftOwnsNativeHold_ = false;
        rightOwnsNativeHold_ = false;
    }

    [[nodiscard]] bool LeftCaptured() const noexcept { return leftCaptured_; }
    [[nodiscard]] bool RightCaptured() const noexcept { return rightCaptured_; }
    [[nodiscard]] bool LeftOwnsNativeHold() const noexcept {
        return leftOwnsNativeHold_;
    }
    [[nodiscard]] bool RightOwnsNativeHold() const noexcept {
        return rightOwnsNativeHold_;
    }

private:
    [[nodiscard]] WebInputDecision Down(
        WebSide side, const WebInputEligibility& eligibility) noexcept {
        bool& captured = side == WebSide::Left ? leftCaptured_ : rightCaptured_;
        bool& ownsNativeHold = side == WebSide::Left
                                   ? leftOwnsNativeHold_
                                   : rightOwnsNativeHold_;
        if (captured) {
            WebInputDecision duplicate{};
            duplicate.consume = true;
            return duplicate;
        }

        const bool hadAny = leftOwnsNativeHold_ || rightOwnsNativeHold_;
        if (!hadAny &&
            (!eligibility.foreground || !eligibility.runtimeReady ||
             !eligibility.airborneProven)) {
            return {};
        }
        captured = true;

        WebInputDecision decision{};
        decision.consume = true;
        if (hadAny) {
            return decision;
        }
        ownsNativeHold = true;
        decision.leftAttach = side == WebSide::Left;
        decision.rightAttach = side == WebSide::Right;
        decision.nativeSwingPress = true;
        return decision;
    }

    [[nodiscard]] WebInputDecision Up(WebSide side) noexcept {
        bool& captured = side == WebSide::Left ? leftCaptured_ : rightCaptured_;
        bool& ownsNativeHold = side == WebSide::Left
                                   ? leftOwnsNativeHold_
                                   : rightOwnsNativeHold_;
        if (!captured) {
            return {};
        }

        const bool hadAny = leftOwnsNativeHold_ || rightOwnsNativeHold_;
        WebInputDecision decision{};
        decision.consume = true;
        decision.leftRelease = side == WebSide::Left && ownsNativeHold;
        decision.rightRelease = side == WebSide::Right && ownsNativeHold;
        captured = false;
        ownsNativeHold = false;
        decision.nativeSwingRelease =
            hadAny && !leftOwnsNativeHold_ && !rightOwnsNativeHold_;
        return decision;
    }

    bool leftCaptured_{};
    bool rightCaptured_{};
    bool leftOwnsNativeHold_{};
    bool rightOwnsNativeHold_{};
};

} // namespace trueswing::rebuild::runtime
