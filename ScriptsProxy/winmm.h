// Overstrike -- an open-source mod manager for PC ports of Insomniac Games' games.
// This program is free software, and can be redistributed and/or modified by you. It is provided 'as-is', without any warranty.
// For more details, terms and conditions, see GNU General Public License.
// A copy of the that license should come with this program (LICENSE.txt). If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <Windows.h>

#include <cstddef>

inline constexpr std::size_t kWinmmFunctionCount = 180;

// Each exported MASM thunk jumps through one dedicated slot. The slots are
// populated once during process attach and are never changed afterward.
extern "C" FARPROC g_winmmFunctions[kWinmmFunctionCount];

struct winmm_dll {
	HMODULE dll = nullptr;
};

extern winmm_dll winmm;

[[nodiscard]] bool setupFunctions() noexcept;