#pragma once

#include <cstdint>
#include <optional>

namespace trueswing::rebuild::runtime {

struct ControllerSourceRouteState final {
    std::optional<std::uintptr_t> device{};
    std::uint64_t sourceEpoch{};
    bool changed{};
};

// Tracks only the game-resolved logical-player-0 device. Selection and
// fallback policy deliberately remain outside this type.
class ControllerSourceRoute final {
public:
    using Device = std::uintptr_t;

    [[nodiscard]] ControllerSourceRouteState Update(
        const std::optional<Device>& logicalPlayerZeroDevice) noexcept {
        std::optional<Device> normalized = logicalPlayerZeroDevice;
        if (normalized.has_value() && *normalized == 0U) {
            normalized.reset();
        }

        changed_ = normalized != currentDevice_;
        if (changed_) {
            currentDevice_ = normalized;
            AdvanceEpoch();
        }
        return State();
    }

    // A raw address has no proof that it belongs to logical player 0.
    ControllerSourceRouteState Update(Device) noexcept = delete;

    [[nodiscard]] ControllerSourceRouteState State() const noexcept {
        return {currentDevice_, sourceEpoch_, changed_};
    }

    [[nodiscard]] std::optional<Device> CurrentDevice() const noexcept {
        return currentDevice_;
    }

    [[nodiscard]] std::uint64_t SourceEpoch() const noexcept {
        return sourceEpoch_;
    }

    [[nodiscard]] bool Changed() const noexcept { return changed_; }

    void Reset() noexcept {
        currentDevice_.reset();
        changed_ = false;
        // Preserve the counter. A later reconnect must not reuse an ownership
        // epoch that existed before reset.
    }

private:
    void AdvanceEpoch() noexcept {
        ++sourceEpoch_;
        if (sourceEpoch_ == 0U) {
            ++sourceEpoch_;
        }
    }

    std::optional<Device> currentDevice_{};
    std::uint64_t sourceEpoch_{};
    bool changed_{};
};

} // namespace trueswing::rebuild::runtime
