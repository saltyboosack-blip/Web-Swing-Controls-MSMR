#pragma once

#include <cstdint>

#include "trueswing/Vec3.h"

namespace trueswing::rebuild::runtime {

struct GameVec3 final {
    float x{};
    float y{};
    float z{};
};

static_assert(sizeof(GameVec3) == 12U);

[[nodiscard]] bool TryReadPointer(std::uintptr_t address,
                                  std::uintptr_t& value);
[[nodiscard]] bool TryReadU16(std::uintptr_t address, std::uint16_t& value);
[[nodiscard]] bool TryReadU32(std::uintptr_t address, std::uint32_t& value);
[[nodiscard]] bool TryReadByte(std::uintptr_t address, std::uint8_t& value);
[[nodiscard]] bool TryReadFloat(std::uintptr_t address, float& value);
[[nodiscard]] bool TryReadGameVec3(std::uintptr_t address, GameVec3& value);
[[nodiscard]] bool TryWriteGameVec3(std::uintptr_t address,
                                    const GameVec3& value);

[[nodiscard]] bool IsFinite(const GameVec3& value);
[[nodiscard]] trueswing::rebuild::Vec3 ToPhysics(const GameVec3& value);
[[nodiscard]] bool ToGame(const trueswing::rebuild::Vec3& value,
                          GameVec3& result);

} // namespace trueswing::rebuild::runtime
