#include "MemoryAccess.h"

#include <Windows.h>

#include <cmath>

namespace trueswing::rebuild::runtime {

bool TryReadPointer(std::uintptr_t address, std::uintptr_t& value) {
    __try {
        value = *reinterpret_cast<const std::uintptr_t*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadU16(std::uintptr_t address, std::uint16_t& value) {
    __try {
        value = *reinterpret_cast<const std::uint16_t*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadU32(std::uintptr_t address, std::uint32_t& value) {
    __try {
        value = *reinterpret_cast<const std::uint32_t*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadByte(std::uintptr_t address, std::uint8_t& value) {
    __try {
        value = *reinterpret_cast<const std::uint8_t*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadFloat(std::uintptr_t address, float& value) {
    __try {
        value = *reinterpret_cast<const float*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadGameVec3(std::uintptr_t address, GameVec3& value) {
    __try {
        value = *reinterpret_cast<const GameVec3*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryWriteGameVec3(std::uintptr_t address, const GameVec3& value) {
    __try {
        *reinterpret_cast<GameVec3*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsFinite(const GameVec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

trueswing::rebuild::Vec3 ToPhysics(const GameVec3& value) {
    return {static_cast<double>(value.x), static_cast<double>(value.y),
            static_cast<double>(value.z)};
}

bool ToGame(const trueswing::rebuild::Vec3& value, GameVec3& result) {
    if (!value.IsFinite()) {
        return false;
    }
    const GameVec3 candidate{static_cast<float>(value.x),
                             static_cast<float>(value.y),
                             static_cast<float>(value.z)};
    if (!IsFinite(candidate)) {
        return false;
    }
    result = candidate;
    return true;
}

} // namespace trueswing::rebuild::runtime
