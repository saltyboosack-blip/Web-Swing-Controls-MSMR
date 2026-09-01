#include <Windows.h>
#include <mmsystem.h>

#include <array>
#include <atomic>
#include <iostream>
#include <thread>

namespace {

[[nodiscard]] bool LoadedLocalProxy() {
    const HMODULE proxy = GetModuleHandleW(L"winmm.dll");
    if (proxy == nullptr) {
        return false;
    }
    wchar_t path[32768]{};
    const DWORD length = GetModuleFileNameW(
        proxy, path, static_cast<DWORD>(std::size(path)));
    if (length == 0 || length >= std::size(path)) {
        return false;
    }
    std::wcout << L"PROXY_PATH " << path << L'\n';
    const wchar_t* basename = wcsrchr(path, L'\\');
    return wcsstr(path, L"ProxyHarness\\") != nullptr &&
           basename != nullptr &&
           _wcsicmp(basename + 1, L"winmm.dll") == 0;
}

} // namespace

int wmain() {
    if (!LoadedLocalProxy()) {
        std::wcerr << L"Local proxy was not loaded.\n";
        return 1;
    }

    TIMECAPS caps{};
    if (timeGetDevCaps(&caps, sizeof(caps)) != TIMERR_NOERROR) {
        std::wcerr << L"timeGetDevCaps forwarding failed.\n";
        return 2;
    }
    const UINT requestedPeriod = caps.wPeriodMin == 0 ? 1U : caps.wPeriodMin;
    const MMRESULT periodResult = timeBeginPeriod(requestedPeriod);

    std::atomic_bool failed{false};
    const auto timeWorker = [&failed]() {
        DWORD previous = timeGetTime();
        for (int index = 0; index < 10000; ++index) {
            const DWORD current = timeGetTime();
            if (current + 10000U < previous) {
                failed.store(true, std::memory_order_relaxed);
                return;
            }
            previous = current;
        }
    };
    const auto capsWorker = [&failed]() {
        for (int index = 0; index < 5000; ++index) {
            TIMECAPS localCaps{};
            if (timeGetDevCaps(&localCaps, sizeof(localCaps)) !=
                TIMERR_NOERROR) {
                failed.store(true, std::memory_order_relaxed);
                return;
            }
        }
    };
    const auto joyWorker = []() {
        for (int index = 0; index < 50; ++index) {
            JOYINFOEX info{};
            info.dwSize = sizeof(info);
            info.dwFlags = JOY_RETURNALL;
            (void)joyGetPosEx(JOYSTICKID1, &info);
        }
    };

    std::array<std::thread, 4> workers{
        std::thread(timeWorker),
        std::thread(timeWorker),
        std::thread(capsWorker),
        std::thread(joyWorker),
    };
    for (std::thread& worker : workers) {
        worker.join();
    }

    if (periodResult == TIMERR_NOERROR) {
        (void)timeEndPeriod(requestedPeriod);
    }
    if (failed.load(std::memory_order_relaxed)) {
        std::wcerr << L"Forwarding stress check failed.\n";
        return 3;
    }

    const HMODULE proxy = GetModuleHandleW(L"winmm.dll");
    if (GetProcAddress(proxy, "joyGetPosEx") == nullptr ||
        GetProcAddress(proxy, "joySetCapture") == nullptr ||
        GetProcAddress(proxy, "timeGetTime") == nullptr ||
        GetProcAddress(proxy, "timeGetDevCaps") == nullptr) {
        std::wcerr << L"Required helper exports are missing.\n";
        return 4;
    }

    wchar_t processPath[32768]{};
    GetModuleFileNameW(nullptr, processPath,
                       static_cast<DWORD>(std::size(processPath)));
    std::wcout << L"PROXY_HELPER_OK host=" << processPath << L'\n';
    return 0;
}
