#pragma once

#include <cmath>
#include <cstdint>

#include "trueswing/Vec3.h"

namespace trueswing::rebuild::runtime {

inline constexpr std::uint16_t kSwingForwardScanCode = 0x11U;
inline constexpr std::uint16_t kSwingLeftScanCode = 0x1EU;
inline constexpr std::uint16_t kSwingBackwardScanCode = 0x1FU;
inline constexpr std::uint16_t kSwingRightScanCode = 0x20U;

inline constexpr std::uint32_t kSwingForwardKeyBit = 1U << 0U;
inline constexpr std::uint32_t kSwingLeftKeyBit = 1U << 1U;
inline constexpr std::uint32_t kSwingBackwardKeyBit = 1U << 2U;
inline constexpr std::uint32_t kSwingRightKeyBit = 1U << 3U;
inline constexpr double kSwingSteeringAcceleration = 8.0;

struct SwingMovementInput final {
    bool forward{};
    bool left{};
    bool backward{};
    bool right{};
};

[[nodiscard]] constexpr std::uint32_t SwingMovementKeyBit(
    std::uint16_t scanCode, bool extended = false) noexcept {
    if (extended) {
        return 0U;
    }
    switch (scanCode) {
    case kSwingForwardScanCode:
        return kSwingForwardKeyBit;
    case kSwingLeftScanCode:
        return kSwingLeftKeyBit;
    case kSwingBackwardScanCode:
        return kSwingBackwardKeyBit;
    case kSwingRightScanCode:
        return kSwingRightKeyBit;
    default:
        return 0U;
    }
}

// Pure transition used by the Raw Input bridge. Repeated make/break events are
// deliberately idempotent, and unrelated/extended keys leave the mask intact.
[[nodiscard]] constexpr std::uint32_t ApplySwingMovementKeyTransition(
    std::uint32_t currentMask, std::uint16_t scanCode, bool down,
    bool extended = false) noexcept {
    const std::uint32_t bit = SwingMovementKeyBit(scanCode, extended);
    if (bit == 0U) {
        return currentMask;
    }
    return down ? currentMask | bit : currentMask & ~bit;
}

[[nodiscard]] constexpr SwingMovementInput DecodeSwingMovementInput(
    std::uint32_t mask) noexcept {
    return {
        (mask & kSwingForwardKeyBit) != 0U,
        (mask & kSwingLeftKeyBit) != 0U,
        (mask & kSwingBackwardKeyBit) != 0U,
        (mask & kSwingRightKeyBit) != 0U,
    };
}

// Steering is an acceleration, not a velocity rotation or speed cap. At useful
// horizontal speed, W/S follow the current momentum and A/D form its lateral
// axis. Near rest, the caller's already-corrected character-right vector is the
// authoritative fallback. The helper never mirrors that vector itself.
[[nodiscard]] inline Vec3 ComputeSwingSteeringAcceleration(
    const SwingMovementInput& input, const Vec3& velocity,
    const Vec3& correctedCharacterRight,
    double accelerationMagnitude = kSwingSteeringAcceleration,
    double momentumDirectionMinimumSpeed = 0.5) noexcept {
    const double forwardAxis = static_cast<double>(input.forward) -
                               static_cast<double>(input.backward);
    const double rightAxis = static_cast<double>(input.right) -
                             static_cast<double>(input.left);
    const double inputLength = std::hypot(forwardAxis, rightAxis);
    if (!std::isfinite(inputLength) || inputLength <= 1.0e-12) {
        return {};
    }
    if (!velocity.IsFinite() || !correctedCharacterRight.IsFinite() ||
        !std::isfinite(accelerationMagnitude) ||
        accelerationMagnitude < 0.0 ||
        !std::isfinite(momentumDirectionMinimumSpeed) ||
        momentumDirectionMinimumSpeed < 0.0) {
        return {};
    }

    Vec3 characterRight{correctedCharacterRight.x, 0.0,
                        correctedCharacterRight.z};
    const double characterRightLength = characterRight.Length();
    if (!std::isfinite(characterRightLength) ||
        characterRightLength <= 1.0e-9) {
        return {};
    }
    characterRight = characterRight / characterRightLength;

    Vec3 forward{velocity.x, 0.0, velocity.z};
    const double horizontalSpeed = forward.Length();
    Vec3 right{};
    if (std::isfinite(horizontalSpeed) &&
        horizontalSpeed >= momentumDirectionMinimumSpeed &&
        horizontalSpeed > 1.0e-9) {
        forward = forward / horizontalSpeed;
        right = Vec3{0.0, 1.0, 0.0}.Cross(forward);
        if (right.Dot(characterRight) < 0.0) {
            right = -right;
        }
    } else {
        right = characterRight;
        forward = right.Cross(Vec3{0.0, 1.0, 0.0});
    }

    const Vec3 requested =
        forward * forwardAxis + right * rightAxis;
    const double requestedLength = requested.Length();
    if (!requested.IsFinite() || !std::isfinite(requestedLength) ||
        requestedLength <= 1.0e-12) {
        return {};
    }
    return requested * (accelerationMagnitude / requestedLength);
}

} // namespace trueswing::rebuild::runtime
