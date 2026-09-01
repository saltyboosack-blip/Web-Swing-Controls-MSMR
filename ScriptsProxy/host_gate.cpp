#include "host_gate.h"

#include <Windows.h>

#include <cwchar>

namespace scripts_proxy {

bool IsMsmrExecutableBasename(const wchar_t* basename) noexcept {
    return basename != nullptr && basename[0] != L'\0' &&
           _wcsicmp(basename, L"Spider-Man.exe") == 0;
}

bool IsMsmrHostProcess() noexcept {
    wchar_t executablePath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(
        nullptr, executablePath, static_cast<DWORD>(_countof(executablePath)));
    if (length == 0 || length >= _countof(executablePath)) {
        return false;
    }

    const wchar_t* basename = executablePath;
    if (const wchar_t* backslash = std::wcsrchr(executablePath, L'\\')) {
        basename = backslash + 1;
    }
    if (const wchar_t* slash = std::wcsrchr(basename, L'/')) {
        basename = slash + 1;
    }
    return IsMsmrExecutableBasename(basename);
}

} // namespace scripts_proxy
