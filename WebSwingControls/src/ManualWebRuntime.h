#pragma once

#include <array>
#include <cstdint>
#include <string>

#if !defined(TRUESWING_CONTROLS_ONLY)
#include "SwingSteeringPolicy.h"
#endif

namespace trueswing::rebuild::runtime {

struct ManualWebHookSpec final {
    std::uintptr_t targetRva{};
    void* detour{};
    void** original{};
    const char* name{};
};

#if defined(TRUESWING_CONTROLS_ONLY)
inline constexpr std::size_t kManualWebHookCount = 4U;
#else
inline constexpr std::size_t kManualWebHookCount = 6U;
#endif

#if !defined(TRUESWING_CONTROLS_ONLY)
enum class ManualWebEnterKind : std::uint8_t {
    Vanilla,
    ActiveRequest,
    CanceledRequest,
};

struct ManualWebEnterClaim final {
    ManualWebEnterKind kind{ManualWebEnterKind::Vanilla};
    std::uint64_t token{};
};

[[nodiscard]] inline ManualWebEnterKind ClassifyManualWebEnter(
    std::uint64_t queuedSubmitToken, bool queuedSubmitCanceled,
    bool hasRequest,
    bool requestCanceling, std::uint64_t requestToken,
    bool requestEligible, bool queuedSubmitAmbiguous = false) noexcept {
    if (queuedSubmitAmbiguous) {
        return ManualWebEnterKind::CanceledRequest;
    }
    if (queuedSubmitToken != 0U) {
        return !queuedSubmitCanceled && hasRequest && !requestCanceling &&
                       requestToken == queuedSubmitToken && requestEligible
                   ? ManualWebEnterKind::ActiveRequest
                   : ManualWebEnterKind::CanceledRequest;
    }
    return hasRequest ? ManualWebEnterKind::CanceledRequest
                      : ManualWebEnterKind::Vanilla;
}
#endif

[[nodiscard]] bool PrepareManualWebRuntime(std::uintptr_t moduleBase,
                                           std::string& reason);
[[nodiscard]] std::array<ManualWebHookSpec, kManualWebHookCount>
GetManualWebHookSpecs() noexcept;
#if !defined(TRUESWING_CONTROLS_ONLY)
[[nodiscard]] ManualWebEnterClaim ClaimManualWebSwingEnter(
    const void* payload) noexcept;
[[nodiscard]] bool CompleteManualWebSwingEnter(
    const ManualWebEnterClaim& claim) noexcept;
[[nodiscard]] SwingMovementInput ReadSwingMovementInput() noexcept;
#endif
void SetManualWebRuntimeReady(bool ready) noexcept;

} // namespace trueswing::rebuild::runtime
