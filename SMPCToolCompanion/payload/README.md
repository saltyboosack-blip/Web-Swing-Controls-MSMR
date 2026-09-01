# Build payloads

Release builds require two locally built payload files in this directory:

- `TrueSwing.dll` — Web Swing Controls v0.3.0-rc2, SHA-256
  `10F25A79F541731BAF898F28316B4FBE444E96E07C6B47F46E49C55F9AB691FC`
- `winmm.dll` — host-gated script proxy, SHA-256
  `FFB6AD902798D7DF87F6D3FDE3B5A4BAE1E9212A40971B346452A5064768E111`

These build products are intentionally not tracked here. Use `build.ps1` with
explicit paths to verified local builds. Their complete source is already in
the repository under `WebSwingControls/` and `ScriptsProxy/`.
