#pragma once

#include "ControllerWebInputPolicy.h"

namespace trueswing::rebuild::runtime {

struct ControllerTriggerBridgeDecision final {
    bool writeLeftTrigger{};
    bool writeRightTrigger{};
    float leftTrigger{};
    float rightTrigger{};
};

// Converts physical layer ownership into the normalized game-facing trigger
// state. Both physical triggers are hidden while the layer owns them. Either
// side drives only the game's native right-trigger Swing input.
[[nodiscard]] constexpr ControllerTriggerBridgeDecision
ResolveControllerTriggerBridge(
    const ControllerWebInputDecision& decision,
    bool nativeSwingHeld) noexcept {
    return {
        .writeLeftTrigger = decision.consumeLeftTrigger,
        .writeRightTrigger =
            decision.consumeRightTrigger || nativeSwingHeld,
        .leftTrigger = 0.0F,
        .rightTrigger = nativeSwingHeld ? 1.0F : 0.0F,
    };
}

} // namespace trueswing::rebuild::runtime
