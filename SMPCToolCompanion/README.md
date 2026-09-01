# SMPCTool companion installer

This is an unofficial standalone companion for jedijosh920's Spider-Man PC
Modding Tool (SMPCTool) v1.1.1. It is not a modified SMPCTool build and does
not contain, patch, inject into, or redistribute any SMPCTool file.

SMPCTool's `.smpcmod` format can only replace assets inside the game's asset
archives. Web Swing Controls is a native script mod, so it cannot be installed
as a functional `.smpcmod`. This companion instead reads the
`assetArchiveDir.txt` file created by SMPCTool and safely derives the game
folder from that setting.

## User workflow

1. Download the official SMPCTool v1.1.1 from Nexus Mods page 51.
2. Run SMPCTool once and select the game's `asset_archive` folder.
3. Close SMPCTool and the game.
4. Extract the companion release folder into the folder containing
   `SMPCTool.exe`. You may keep it as its own adjacent subfolder or place its
   contents directly beside `SMPCTool.exe`.
5. Run `WebSwingControls_SMPCTool_Companion.exe` and review the detected paths.
6. Choose **Install**.

The companion never launches, closes, or modifies SMPCTool. It never changes
the game's `toc` or archive files.

## Safety behavior

- Requires `SMPCTool.exe` and its own `assetArchiveDir.txt` in the companion
  folder.
- Requires exact Steam build `23986256` of Marvel's Spider-Man Remastered.
- Verifies the controls DLL and host-gated proxy with exact SHA-256 values.
- Refuses unknown `winmm.dll` loaders.
- Refuses an unknown `scripts\TrueSwing.dll`, which may belong to the separate
  TrueSwing physics project.
- Preserves existing `scripts.txt` entries and `commandline.txt` arguments.
- Backs up and verifies every replaced file before writing.
- Stages writes on the game drive, verifies them afterward, and attempts a
  rollback on failure.
- Refuses to write while the game, `crs-video.exe`, or `SMPCTool.exe` is
  running. It never closes those processes.
- Contains no networking or updater code.

## Build and test

The companion targets the Windows .NET Framework 4.x runtime used by
SMPCTool. No separate .NET 7 installation is required.

From PowerShell:

```powershell
.\SMPCToolCompanion\build.ps1 `
  -ControlsDllPath C:\path\to\TrueSwing.dll `
  -ProxyDllPath C:\path\to\scripts_proxy.dll `
  -OutputDirectory C:\new\empty\output
```

The build refuses an existing output directory, verifies both payload hashes,
compiles the installer and tests with Microsoft's installed .NET Framework
compiler, runs twelve isolated fixture tests, and copies the two verified
payloads into the release folder.

## Provenance and permissions

The companion behavior was designed after read-only inspection of the public
SMPCTool v1.1.1 source snapshot at commit
`515f11fa40eb051d5ceb8bc88cbe493eab699a1c`. No SMPCTool source or binary is
included here because its Nexus page does not grant modification or reupload
permission and the public source snapshot contains no explicit license file.

This companion's source is licensed under this repository's GPL-3.0 license.
