# Building the proxy

Requirements: Visual Studio 2022 Build Tools with MSVC v143, Windows SDK, MASM,
PowerShell, and vcpkg manifest support.

From `ScriptsProxy/`:

```bat
build-release.cmd
build-host-gate-tests.cmd
build-proxy-helper-tests.cmd
```

Then compare the release DLL with an exact stock Overstrike 1.8.0 proxy:

```powershell
.\verify-proxy.ps1 -BaselinePath 'C:\path\to\stock\scripts_proxy.dll'
```

`build-release.cmd` first regenerates `winmm.cpp`, `winmm.h`, and
`winmm_forwarders.asm` from canonical `winmm.def`, then performs an x64 Release
rebuild. It fails if the definition is not exactly 180 unique contiguous
exports. The tested release links the static runtime and uses `/Brepro` plus
`/PDBALTPATH:%_PDB%`.
