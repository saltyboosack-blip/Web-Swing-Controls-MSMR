# Build payloads

Release builds require two locally built payload files in this directory:

- `TrueSwing.dll` — Web Swing Controls v0.2.1, SHA-256
  `1E0AC0AE9FA82B59F06B5CBCD2A915FECA8FAEEBD924A1306A5471810CE0EBC8`
- `winmm.dll` — host-gated script proxy, SHA-256
  `FFB6AD902798D7DF87F6D3FDE3B5A4BAE1E9212A40971B346452A5064768E111`

These build products are intentionally not tracked here. Use `build.ps1` with
explicit paths to verified local builds. Their complete source is already in
the repository under `WebSwingControls/` and `ScriptsProxy/`.
