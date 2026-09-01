# Web Swing Controls for Marvel's Spider-Man Remastered

Controls-only native mod that lets the player choose which side Spider-Man
uses for an ordinary web swing.

- Hold **left mouse** while airborne to request a valid left-side anchor.
- Hold **right mouse** while airborne to request a valid right-side anchor.
- On controller while airborne, use **L2/LT** for left or **R2/RT** for right.
  No shoulder-button hold is required. **L1/LB** performs the former native
  **L2/LT** zoom action. Native **L1/LB + R1/RB** gameplay chords take priority
  over zoom, including environmental item pickup.
- Releasing the owning button releases the web.

The mod retains the game's native swing action, anchors, animations, physics,
collision, combat, saves, and assets. It does not include the separate
TrueSwing physics project.

[Nexus Mods page](https://www.nexusmods.com/marvelsspidermanremastered/mods/6306)

## Supported version

- Marvel's Spider-Man Remastered on Steam
- Game build `23986256`
- `Spider-Man.exe` version `4.0630.0.0`
- Exact Overstrike 1.8.0 build documented in the release manifest

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

Two supported install methods exist. Use only one.

### jedijosh920's Spider-Man PC Modding Tool v1.1.1

Web Swing Controls cannot be packaged as a functional `.smpcmod`: that format
replaces archive assets, while this mod requires native script files. The
GitHub release therefore includes an unofficial standalone companion for the
official tool. It contains no SMPCTool code or binary and never changes the
tool, game `toc`, or archive files.

1. Download and run the official SMPCTool v1.1.1 from Nexus Mods page 51.
2. Select the game's `asset_archive` folder, then close SMPCTool and the game.
3. Extract the companion folder into the folder containing `SMPCTool.exe`.
4. Run `WebSwingControls_SMPCTool_Companion.exe`, review the detected paths,
   then choose **Install**.

See [`SMPCToolCompanion/README.md`](SMPCToolCompanion/README.md) for safety,
source, and build details.

### Overstrike 1.8.0

1. Download `Web_Swing_Controls_MSMR_v0.2.1-beta_FULL_INSTALL.zip` from the
   GitHub release or Nexus Mods and extract the entire archive. This is the
   complete mod; no earlier version or separate controller update is needed.
2. Close the game and Overstrike.
3. Run `1_INSTALL_PROXY_FIX.cmd` and select your `Overstrike.exe` when asked.
   The installer validates the selected folder, backs up Overstrike's original
   proxy, and installs the host-gated replacement.
4. Open Overstrike and enable `.script` support.
5. Add `mod/Web_Swing_Controls_MSMR_v0.2.1-beta.script` from the extracted
   folder.
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
| Overstrike v0.3.0 script | `68AB2F6D952DD2F299ED2BF8B8367AE5C9BAE800C95124DB7582EDA7A74BD845` |
| Complete v0.2.1 ZIP | `C3110C70995EE132F644E7A2AF7BECD68B5CB4F578120B55A653065FA9BD37C4` |
| SMPCTool Companion v1.0.0 ZIP | `DF16A8924AC3B346AEBD6F7672854400601B0C6A68690C13889A835ACEB574DB` |
| SMPCTool Companion EXE | `0B4693731B83318C24E89D9851CE6FC1D35633BF0032203D5CD1F5A35137813F` |
| Controls DLL | `1E0AC0AE9FA82B59F06B5CBCD2A915FECA8FAEEBD924A1306A5471810CE0EBC8` |
| Controls DLL v0.3.0 RC2 | `10F25A79F541731BAF898F28316B4FBE444E96E07C6B47F46E49C55F9AB691FC` |
| Compatibility proxy | `FFB6AD902798D7DF87F6D3FDE3B5A4BAE1E9212A40971B346452A5064768E111` |

Version 0.2.1 passed all seven native test executables. The user then
installed the complete package and confirmed the controller mechanic works in
game on a PS5 controller.

Version 0.3.0 RC2 passed all four current native test executables and was
confirmed in game with direct L2/R2 swinging, L1 zoom, and native L1+R1 item
pickup working together on a PS5 controller.

The standalone SMPCTool companion passed twelve installer fixture tests. It
does not modify or redistribute SMPCTool and does not convert the native mod
into a misleading `.smpcmod` asset package.

These are unsigned native modding DLLs that use MinHook and Windows memory
protection APIs. That behavior can attract generic antivirus heuristics. The
complete source and reproducible build instructions are provided for review.

## License

This repository is distributed under the GNU General Public License v3.0. See
`LICENSE` and the component-specific third-party license files.
