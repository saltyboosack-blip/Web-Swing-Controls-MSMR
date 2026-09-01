#include "ControlsOnlyRuntime.h"

#include <Windows.h>

#include <MinHook.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <string>

#include "GameBuild.h"
#include "Logger.h"
#include "ManualWebRuntime.h"

namespace trueswing::rebuild::runtime {
namespace {

std::atomic_int g_startState{0}; // 0 idle, 1 installing, 2 active

void LogHookFailure(const char* stage, void* target,
                    MH_STATUS status) noexcept {
    char line[320]{};
    sprintf_s(line,
              "WebSwingControls hook failure: stage=%s target=0x%llX status=%s (%d).",
              stage,
              static_cast<unsigned long long>(
                  reinterpret_cast<std::uintptr_t>(target)),
              MH_StatusToString(status), static_cast<int>(status));
    Logger::Write(line);
}

[[nodiscard]] bool InstallControlsOnlyHooks() {
    const HMODULE module = GetModuleHandleW(nullptr);
    std::string reason;
    if (module == nullptr || !GameBuild::IsSupported(reason)) {
        Logger::Write(("WebSwingControls refused: " + reason).c_str());
        return false;
    }
    const std::uintptr_t moduleBase =
        reinterpret_cast<std::uintptr_t>(module);
    if (!PrepareManualWebRuntime(moduleBase, reason)) {
        Logger::Write(("WebSwingControls refused: " + reason).c_str());
        return false;
    }

    const MH_STATUS initialize = MH_Initialize();
    if (initialize != MH_OK) {
        LogHookFailure("initialize", nullptr, initialize);
        return false;
    }

    struct HookState final {
        ManualWebHookSpec spec{};
        void* target{};
        bool created{};
    };
    const auto specs = GetManualWebHookSpecs();
    std::array<HookState, kManualWebHookCount> hooks{};
    for (std::size_t index = 0; index < specs.size(); ++index) {
        hooks[index].spec = specs[index];
        hooks[index].target = reinterpret_cast<void*>(
            moduleBase + specs[index].targetRva);
    }

    const auto rollback = [&hooks]() noexcept {
        SetManualWebRuntimeReady(false);
        for (HookState& hook : hooks) {
            if (!hook.created) {
                continue;
            }
            (void)MH_DisableHook(hook.target);
            (void)MH_RemoveHook(hook.target);
            hook.created = false;
        }
        (void)MH_Uninitialize();
    };

    for (HookState& hook : hooks) {
        const MH_STATUS status =
            MH_CreateHook(hook.target, hook.spec.detour, hook.spec.original);
        if (status != MH_OK || *hook.spec.original == nullptr) {
            LogHookFailure(hook.spec.name, hook.target, status);
            rollback();
            return false;
        }
        hook.created = true;
    }
    for (HookState& hook : hooks) {
        const MH_STATUS status = MH_QueueEnableHook(hook.target);
        if (status != MH_OK) {
            LogHookFailure(hook.spec.name, hook.target, status);
            rollback();
            return false;
        }
    }
    const MH_STATUS applied = MH_ApplyQueued();
    if (applied != MH_OK) {
        LogHookFailure("apply", nullptr, applied);
        rollback();
        return false;
    }

    SetManualWebRuntimeReady(true);
    Logger::Write(
        "WebSwingControls active: airborne LMB/RMB select character-left/right; airborne L1+L2/R2 select character-left/right through the game-selected controller; vanilla Swing owns movement, velocity, collision, and release.");
    return true;
}

} // namespace

bool StartControlsOnlyRuntime() {
    int expected = 0;
    if (!g_startState.compare_exchange_strong(expected, 1,
                                               std::memory_order_acq_rel)) {
        return expected == 2;
    }
    const bool installed = InstallControlsOnlyHooks();
    g_startState.store(installed ? 2 : 0, std::memory_order_release);
    return installed;
}

} // namespace trueswing::rebuild::runtime
