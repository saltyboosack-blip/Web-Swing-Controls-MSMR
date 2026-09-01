#include "MoverBinding.h"

#include "MemoryAccess.h"

namespace trueswing::rebuild::runtime {
namespace {

constexpr std::uintptr_t kHeroMoverManagerVtableRva = 0x038B2C98;
constexpr std::uintptr_t kMoverStandardVtableRva = 0x04F70168;
constexpr std::uintptr_t kHandleTablePointerRva = 0x07A44320;
constexpr std::uintptr_t kHandleTableCountRva = 0x07A44340;
constexpr std::uintptr_t kMoverHandleOffset = 0xDB4;
constexpr std::size_t kMaximumRegistryEntries = 1024U;

[[nodiscard]] std::optional<MoverBinding> ResolveManager(
    std::uintptr_t stateHost, std::uintptr_t manager,
    std::uintptr_t moduleBase, std::uint16_t registryIndex) {
    std::uintptr_t managerVtable = 0;
    std::uint32_t handle = 0;
    std::uintptr_t table = 0;
    std::uint32_t count = 0;
    if (manager == 0 ||
        !TryReadPointer(manager, managerVtable) ||
        managerVtable != moduleBase + kHeroMoverManagerVtableRva ||
        !TryReadU32(manager + kMoverHandleOffset, handle) ||
        !TryReadPointer(moduleBase + kHandleTablePointerRva, table) ||
        !TryReadU32(moduleBase + kHandleTableCountRva, count) || table == 0) {
        return std::nullopt;
    }

    const std::uint32_t generation = handle >> 20U;
    const std::uint32_t index = handle & 0xFFFFFU;
    if (generation == 0 || index >= count || index >= 0x100000U) {
        return std::nullopt;
    }

    const std::uintptr_t entry = table + static_cast<std::uintptr_t>(index) * 16U;
    std::uint32_t liveGeneration = 0;
    std::uintptr_t mover = 0;
    std::uintptr_t moverVtable = 0;
    if (!TryReadU32(entry + 8U, liveGeneration) ||
        liveGeneration != generation || !TryReadPointer(entry, mover) ||
        mover == 0 || !TryReadPointer(mover, moverVtable) ||
        moverVtable != moduleBase + kMoverStandardVtableRva) {
        return std::nullopt;
    }

    return MoverBinding{stateHost, manager, mover, handle, registryIndex};
}

} // namespace

std::optional<MoverBinding> ResolveHeroMover(std::uintptr_t stateHost,
                                             std::uintptr_t moduleBase) {
    if (stateHost == 0 || moduleBase == 0) {
        return std::nullopt;
    }

    std::uintptr_t registry = 0;
    std::uint16_t capacity = 0;
    if (!TryReadPointer(stateHost + 0x80U, registry) ||
        !TryReadU16(stateHost + 0x88U, capacity) || registry == 0 ||
        capacity == 0 || capacity > kMaximumRegistryEntries) {
        return std::nullopt;
    }

    std::optional<MoverBinding> unique;
    const std::uintptr_t expectedVtable =
        moduleBase + kHeroMoverManagerVtableRva;
    for (std::uint16_t index = 0; index < capacity; ++index) {
        const std::uintptr_t entry =
            registry + static_cast<std::uintptr_t>(index) * 16U;
        std::uintptr_t manager = 0;
        std::uintptr_t vtable = 0;
        if (!TryReadPointer(entry + 8U, manager) || manager == 0 ||
            !TryReadPointer(manager, vtable) || vtable != expectedVtable) {
            continue;
        }
        const auto candidate =
            ResolveManager(stateHost, manager, moduleBase, index);
        if (!candidate.has_value()) {
            continue;
        }
        if (unique.has_value() && unique->manager != candidate->manager) {
            return std::nullopt;
        }
        unique = candidate;
    }
    return unique;
}

bool RevalidateHeroMover(const MoverBinding& binding,
                         std::uintptr_t moduleBase) {
    if (binding.stateHost == 0 || binding.manager == 0 ||
        binding.mover == 0 || binding.handle == 0 || moduleBase == 0) {
        return false;
    }

    const auto current = ResolveManager(binding.stateHost, binding.manager,
                                        moduleBase, binding.registryIndex);
    if (!current.has_value() || current->mover != binding.mover ||
        current->handle != binding.handle) {
        return false;
    }

    std::uintptr_t registry = 0;
    std::uint16_t capacity = 0;
    if (!TryReadPointer(binding.stateHost + 0x80U, registry) ||
        !TryReadU16(binding.stateHost + 0x88U, capacity) || registry == 0 ||
        capacity == 0 || capacity > kMaximumRegistryEntries) {
        return false;
    }
    if (binding.registryIndex >= capacity) {
        return false;
    }
    std::uintptr_t manager = 0;
    return TryReadPointer(
               registry +
                   static_cast<std::uintptr_t>(binding.registryIndex) * 16U +
                   8U,
               manager) &&
           manager == binding.manager;
}

} // namespace trueswing::rebuild::runtime
