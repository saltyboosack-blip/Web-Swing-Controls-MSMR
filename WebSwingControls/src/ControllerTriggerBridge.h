#pragma once

#include "ControllerWebInputPolicy.h"

namespace trueswing::rebuild::runtime {

struct ControllerTriggerBridgeDecision final {
    bool writeLeftShoulder{};
    std::uint32_t leftShoulderState{};
    bool writeLeftTrigger{};
    bool writeRightTrigger{};
    float leftTrigger{};
    float rightTrigger{};
};

// Converts physical controls into game-facing state. L1/LB alone becomes
// native L2/LT (zoom), but the native L1/LB state passes through while R1/RB
// is also held so gameplay chords such as L1+R1 keep working. Physical L2/LT
// is therefore reserved for left-side swings. Either physical trigger drives
// only the game's native right-trigger Swing input while a web is owned.
[[nodiscard]] constexpr ControllerTriggerBridgeDecision
ResolveControllerTriggerBridge(
    const ControllerWebInputDecision& decision,
    bool nativeSwingHeld,
    bool leftShoulderHeld,
    bool rightShoulderHeld) noexcept {
    const bool nativeShoulderChord =
        leftShoulderHeld && rightShoulderHeld;
    return {
        .writeLeftShoulder = !nativeShoulderChord,
        .leftShoulderState = 0U,
        .writeLeftTrigger = true,
        .writeRightTrigger =
            decision.consumeRightTrigger || nativeSwingHeld,
        .leftTrigger =
            leftShoulderHeld && !nativeShoulderChord ? 1.0F : 0.0F,
        .rightTrigger = nativeSwingHeld ? 1.0F : 0.0F,
    };
}

} // namespace trueswing::rebuild::runtime
