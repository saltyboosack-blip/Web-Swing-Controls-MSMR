# Web Swing Controls for Marvel's Spider-Man Remastered

Controls-only native mod that lets the player choose which side Spider-Man
uses for an ordinary web swing.

- Hold **left mouse** while airborne to request a valid left-side anchor.
- Hold **right mouse** while airborne to request a valid right-side anchor.
- On controller, hold **L1** and use **L2** for left or **R2** for right.
- Releasing the owning button releases the web.

The mod retains the game's native swing action, anchors, animations, physics,
collision, combat, saves, and assets. It does not include the separate
TrueSwing physics project.

[Nexus Mods page](https://www.nexusmods.com/marvelsspidermanremastered/mods/6306)

## Supported version

- Marvel's Spider-Man Remastered on Steam
- Game build `23986256`
- `Spider-Man.exe` version `4.0630.0.0`
- Overstrike 1.8.0 or newer

Unsupported executable builds are rejected rather than hooked.

## Why the Overstrike proxy update is included

Overstrike's stock `winmm.dll` script proxy can also be loaded by Steam helper
processes located beside the game. On the tested installation, initializing the
script loader inside `crs-video.exe` caused an access-violation crash.

The included compatibility proxy adds a host-process gate. Script loading is
enabled only in the supported `Spider-Man.exe`; all normal Windows Multimedia
exports continue to forward to the real System32 `winmm.dll` in helper
processes. The proxy update does not change game assets or swing physics.

## Installation

1. Download `Web_Swing_Controlsv0.2.0-beta.zip` from the GitHub release or the
   Nexus Mods page and extract the entire archive.
2. Close the game and Overstrike.
3. Run `1_INSTALL_PROXY_FIX.cmd` and select your `Overstrike.exe` when asked.
   The installer validates the selected folder, backs up Overstrike's original
   proxy, and installs the host-gated replacement.
4. Open Overstrike and enable `.script` support.
5. Add `Web_Swing_Controlsv0.2.0-beta.script` from the extracted `MOD` folder.
6. Install mods through Overstrike, close Overstrike, and launch the game
   normally through Steam.

Do not manually copy either DLL into the game directory. The included restore
script returns Overstrike to its original proxy when needed.

Keep the game's ordinary Swing action bound to **Left Shift** and use
hold-to-swing mode. The mod feeds that native action internally; Shift is not
required for the left/right mouse mechanic itself.

## Source layout

- `WebSwingControls/` - controls-only DLL and policy tests.
- `ScriptsProxy/` - host-gated Overstrike 1.8.0 proxy source and tests.
- `SOURCE-MANIFEST.json` - file hashes for the source release prepared for
  Nexus review.

MinHook 1.3.4 and its license are included with each component that uses it.
The proxy is based on Overstrike commit
`192dbe1487cf9a2201c97c25633412bfe7975133`.

## Build Web Swing Controls

Requirements:

- Windows 10 or 11
- Visual Studio 2022 Build Tools with MSVC v143 and the Windows SDK
- CMake 3.24 or newer

From a Visual Studio 2022 Developer PowerShell:

```powershell
cmake -S WebSwingControls -B build-controls -G "Visual Studio 17 2022" -A x64
cmake --build build-controls --config Release --target WebSwingControls
ctest --test-dir build-controls -C Release --output-on-failure
```

Output: `build-controls/Release/WebSwingControls.dll`. The release packager
stores this DLL as `TrueSwing.dll` inside the Overstrike `.script` archive.

For the proxy, follow `ScriptsProxy/BUILDING.md` and
`ScriptsProxy/PATCH_NOTES.md`.

## Release hashes

| Artifact | SHA-256 |
| --- | --- |
| Working ZIP | `5E848088537A6AB66E349C50F82CC658B8D380ECB0C1BF7D45C178CCC232273A` |
| Controls DLL | `61C2537DD141D803B177B2988119A284CCC8B30CCAA812947528F3E0B014A223` |
| Compatibility proxy | `FFB6AD902798D7DF87F6D3FDE3B5A4BAE1E9212A40971B346452A5064768E111` |
| Source ZIP | `2A1766057C705D4B585E2A37EC665B06554418178B5D2DCA1A9015017BEC9363` |

These are unsigned native modding DLLs that use MinHook and Windows memory
protection APIs. That behavior can attract generic antivirus heuristics. The
complete source and reproducible build instructions are provided for review.

## License

This repository is distributed under the GNU General Public License v3.0. See
`LICENSE` and the component-specific third-party license files.

