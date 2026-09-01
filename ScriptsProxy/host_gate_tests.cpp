#include "host_gate.h"

#include <iostream>
#include <iterator>

namespace {

struct Case final {
    const wchar_t* name;
    bool expected;
};

} // namespace

int wmain() {
    const Case cases[] = {
        {L"Spider-Man.exe", true},
        {L"SPIDER-MAN.EXE", true},
        {L"spider-man.exe", true},
        {L"crs-video.exe", false},
        {L"crs-handler.exe", false},
        {L"Spider-Man2.exe", false},
        {L"MilesMorales.exe", false},
        {L"RiftApart.exe", false},
        {L"Spider-Man.exe.bak", false},
        {L"C:\\Games\\Spider-Man.exe", false},
        {L"", false},
        {nullptr, false},
    };

    int failures = 0;
    for (const Case& testCase : cases) {
        const bool actual =
            scripts_proxy::IsMsmrExecutableBasename(testCase.name);
        if (actual != testCase.expected) {
            ++failures;
            std::wcerr << L"Host gate mismatch for "
                       << (testCase.name == nullptr ? L"<null>" : testCase.name)
                       << L": expected=" << testCase.expected
                       << L" actual=" << actual << L'\n';
        }
    }

    if (failures != 0) {
        return 1;
    }
    std::wcout << L"HOST_GATE_OK cases=" << std::size(cases) << L'\n';
    return 0;
}
