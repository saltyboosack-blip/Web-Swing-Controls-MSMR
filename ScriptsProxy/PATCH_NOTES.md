# MSMR helper-process safety patch

Base source: Tkachov/Overstrike `OS/v1.8.0`, commit
`192dbe1487cf9a2201c97c25633412bfe7975133`, `ScriptsProxy/`.

The stock proxy initializes real System32 WinMM forwarding, then pattern-scans
every process that loads the game-directory proxy. `crs-video.exe` imports
WinMM and contains a no-access `.retplne` section at RVA `0x171000`. Two
recorded crashes reached that page in `ScanModule` at proxy RVA `0x64A6`.

This MSMR-specific repair preserves the complete stock forwarding/export
surface. After forwarding is initialized, it permits pattern scanning and
MinHook installation only when the current executable basename is exactly
`Spider-Man.exe` (case-insensitive). Every other host remains forwarding-only.

The stock proxy also routed every export through one shared mutable `PA`
pointer. Concurrent calls to different WinMM functions could overwrite that
pointer between selection and dispatch, sending a call to a function with the
wrong signature. This repair generates 180 direct MASM tail-jump thunks, each
backed by its own target slot. Arguments, return values, and the caller's return
address pass through unchanged, without shared dispatch state.

Additional narrow hardening:

- Fail DLL attachment if System32 WinMM cannot be loaded.
- Fail DLL attachment unless all 180 System32 WinMM exports resolve.
- Generate the export-name table and direct thunks from canonical `winmm.def`.
- Link the release DLL with `/Brepro` so repeated clean builds are byte-stable,
  and use `/PDBALTPATH:%_PDB%` so its debug record contains no local path.

The stock parser hook and its deferred `-scripts` loading behavior are
otherwise unchanged. No game or Overstrike installation files are modified by
this source tree.
