param(
    [string]$DefinitionPath = (Join-Path $PSScriptRoot 'winmm.def'),
    [string]$HeaderPath = (Join-Path $PSScriptRoot 'winmm.h'),
    [string]$SourcePath = (Join-Path $PSScriptRoot 'winmm.cpp'),
    [string]$AssemblyPath = (Join-Path $PSScriptRoot 'winmm_forwarders.asm')
)

$ErrorActionPreference = 'Stop'

$entries = foreach ($line in Get-Content -LiteralPath $DefinitionPath) {
    if ($line -match '^\s*([^=\s]+)=([^\s]+)\s+@(\d+)\s*$') {
        [pscustomobject]@{
            ExportName = $Matches[1]
            ThunkName = $Matches[2]
            Ordinal = [int]$Matches[3]
        }
    }
}

if ($entries.Count -ne 180) {
    throw "Expected 180 exports in winmm.def; found $($entries.Count)."
}

for ($index = 0; $index -lt $entries.Count; ++$index) {
    $expectedOrdinal = $index + 1
    if ($entries[$index].Ordinal -ne $expectedOrdinal) {
        throw "Expected ordinal $expectedOrdinal at index $index; found $($entries[$index].Ordinal)."
    }
}

if (($entries.ExportName | Sort-Object -Unique).Count -ne $entries.Count) {
    throw 'Duplicate export name in winmm.def.'
}
if (($entries.ThunkName | Sort-Object -Unique).Count -ne $entries.Count) {
    throw 'Duplicate thunk name in winmm.def.'
}

$license = @'
// Overstrike -- an open-source mod manager for PC ports of Insomniac Games' games.
// This program is free software, and can be redistributed and/or modified by you. It is provided 'as-is', without any warranty.
// For more details, terms and conditions, see GNU General Public License.
// A copy of the that license should come with this program (LICENSE.txt). If not, see <http://www.gnu.org/licenses/>.
'@

$header = @"
$license

#pragma once

#include <Windows.h>

#include <cstddef>

inline constexpr std::size_t kWinmmFunctionCount = $($entries.Count);

// Each exported MASM thunk jumps through one dedicated slot. The slots are
// populated once during process attach and are never changed afterward.
extern "C" FARPROC g_winmmFunctions[kWinmmFunctionCount];

struct winmm_dll {
	HMODULE dll = nullptr;
};

extern winmm_dll winmm;

[[nodiscard]] bool setupFunctions() noexcept;
"@

$nameLines = foreach ($entry in $entries) {
    "`t`"$($entry.ExportName)`","
}

$source = @"
$license

#include "winmm.h"

extern "C" {
FARPROC g_winmmFunctions[kWinmmFunctionCount]{};
}

winmm_dll winmm{};

namespace {

constexpr const char* kWinmmExportNames[kWinmmFunctionCount]{
$($nameLines -join "`r`n")
};

static_assert(
	(sizeof(kWinmmExportNames) / sizeof(kWinmmExportNames[0])) ==
		kWinmmFunctionCount);

} // namespace

bool setupFunctions() noexcept {
	if (winmm.dll == nullptr) {
		return false;
	}

	for (std::size_t index = 0; index < kWinmmFunctionCount; ++index) {
		g_winmmFunctions[index] =
			GetProcAddress(winmm.dll, kWinmmExportNames[index]);
		if (g_winmmFunctions[index] == nullptr) {
			return false;
		}
	}

	return true;
}
"@

$assemblyLines = [System.Collections.Generic.List[string]]::new()
$assemblyLines.Add('option casemap:none')
$assemblyLines.Add('')
$assemblyLines.Add('EXTERN g_winmmFunctions:QWORD')
$assemblyLines.Add('')
$assemblyLines.Add('.code')
for ($index = 0; $index -lt $entries.Count; ++$index) {
    $entry = $entries[$index]
    $offset = $index * 8
    $assemblyLines.Add("PUBLIC $($entry.ThunkName)")
    $assemblyLines.Add("$($entry.ThunkName) PROC")
    $assemblyLines.Add("    jmp QWORD PTR [g_winmmFunctions + $offset]")
    $assemblyLines.Add("$($entry.ThunkName) ENDP")
    $assemblyLines.Add('')
}
$assemblyLines.Add('END')

$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($HeaderPath, $header, $utf8NoBom)
[System.IO.File]::WriteAllText($SourcePath, $source, $utf8NoBom)
[System.IO.File]::WriteAllText(
    $AssemblyPath,
    ($assemblyLines -join "`r`n") + "`r`n",
    $utf8NoBom)

Write-Host "Generated $($entries.Count) dedicated WinMM forwarders."
