#pragma once

#include <cstdint>
#include <optional>

namespace trueswing::rebuild::runtime {

struct MoverBinding final {
    std::uintptr_t stateHost{};
    std::uintptr_t manager{};
    std::uintptr_t mover{};
    std::uint32_t handle{};
    std::uint16_t registryIndex{};
};

[[nodiscard]] std::optional<MoverBinding> ResolveHeroMover(
    std::uintptr_t stateHost, std::uintptr_t moduleBase);
[[nodiscard]] bool RevalidateHeroMover(const MoverBinding& binding,
                                       std::uintptr_t moduleBase);

} // namespace trueswing::rebuild::runtime
