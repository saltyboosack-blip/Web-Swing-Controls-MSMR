# Modding Tool - MSMR Script Add-on

This is an unofficial GPL add-on for Tkachov's Modding Tool 1.4.5. It keeps
the original asset extraction, replacement, `.stage`, and `.modular` features
and adds one menu command:

`Mod > Install MSMR .script...`

The command installs a dependency-free MSMR `.script` package directly into a
validated Marvel's Spider-Man Remastered game folder. It is an optional
alternative installation path for Web Swing Controls; it does not convert
native code into an asset mod.

## Safety behavior

- Validates the exact supported `Spider-Man.exe` SHA-256 before writing.
- Validates the embedded host-gated script proxy SHA-256.
- Refuses unknown game-directory `winmm.dll` loaders.
- Rejects malformed archives, traversal paths, links, oversized packages, and
  unresolved script dependencies.
- Preserves existing script-list entries and command-line arguments.
- Copies every replaced file into a unique `ModdingTool Script Backups` folder.
- Uses staged writes, post-write hashes, and rollback on failure.
- Detects a running game or video helper and stops with instructions; it never
  closes either process automatically.

## Supported build

- Marvel's Spider-Man Remastered Steam build `23986256`
- `Spider-Man.exe` version `4.0630.0.0`
- `Spider-Man.exe` SHA-256
  `E297D4D94F1FFE4FEBF289745E79E7B6FA233A788E7A00F480FC77C55DB81AD1`

Unsupported executable builds are refused without a force-install option.

## Source provenance

The copied Modding Tool, DAT1, GDeflateWrapper, and OverstrikeShared source is
based on Tkachov/Overstrike commit
`8c3a655d7516bbba20ec2fac6bb16ecb3d13c85c` and remains GPL-3.0-or-later.
The host-gated script proxy is built from the `ScriptsProxy/` source in the Web
Swing Controls repository. See that component's `PATCH_NOTES.md` for its
Overstrike 1.8.0 base and MSMR helper-process repair.

## Build and test

Place the verified proxy at:

`ModdingTool/NativePayload/scripts_proxy.dll`

Required proxy SHA-256:

`FFB6AD902798D7DF87F6D3FDE3B5A4BAE1E9212A40971B346452A5064768E111`

Then run:

```powershell
dotnet run --project .\ModdingToolScriptSupportTests\ModdingToolScriptSupportTests.csproj -c Release
dotnet publish .\ModdingTool\ModdingTool.csproj -c Release -r win-x64 --self-contained false -p:PublishSingleFile=true -p:PublishReadyToRun=false
```

`Directory.Build.props` disables release debug records so the distributable
single-file executable contains no developer-machine source paths.

The UI project follows upstream and targets the x64 .NET 7 Windows Desktop
runtime. The distributable therefore requires the x64 .NET 7 Windows Desktop
Runtime, matching Modding Tool 1.4.5. The test suite targets the same runtime.
