#include <Windows.h>

#include <filesystem>
#include <string>

#include "ControlsOnlyRuntime.h"
#include "GameBuild.h"
#include "Logger.h"

namespace {

[[nodiscard]] bool InitializeLog() {
    wchar_t executablePath[32768]{};
    const DWORD length = GetModuleFileNameW(
        nullptr, executablePath, static_cast<DWORD>(std::size(executablePath)));
    if (length == 0 || length >= std::size(executablePath)) {
        return false;
    }
    const std::filesystem::path gameDirectory =
        std::filesystem::path(executablePath).parent_path();
    return trueswing::rebuild::runtime::Logger::Initialize(
        gameDirectory / "WebSwingControls", "WebSwingControls.log",
        "WebSwingControls 0.2.0-test controls-only session");
}

} // namespace

extern "C" __declspec(dllexport) DWORD WINAPI TrueSwingInitialize(void*) {
    using namespace trueswing::rebuild::runtime;
    if (!InitializeLog()) {
        return 0;
    }
    std::string reason;
    if (!GameBuild::IsSupported(reason)) {
        Logger::Write(("WebSwingControls refused: " + reason).c_str());
        return 0;
    }
    Logger::Write(
        "WebSwingControls initialized: 0.2.0-test; mouse and normalized-controller input feed native Swing with character-local anchor selection; no custom physics.");
    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI TrueSwingStartMod(void*) {
    using namespace trueswing::rebuild::runtime;
    return StartControlsOnlyRuntime() ? 1U : 0U;
}

extern "C" __declspec(dllexport) void script_enable() {
    if (TrueSwingInitialize(nullptr) != 0U) {
        (void)TrueSwingStartMod(nullptr);
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
