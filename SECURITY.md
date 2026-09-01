# Security notes

Web Swing Controls is an unsigned native mod. It hooks functions inside the
supported `Spider-Man.exe` process to read airborne input, feed the game's
native Swing action, and influence the native left/right anchor request.

The compatibility proxy loads the real System32 `winmm.dll`, forwards its
exports, and enables Overstrike script loading only in the supported game host.
It deliberately refuses to initialize the script loader inside Steam helper
processes such as `crs-video.exe`.

The project does not contain network communication, telemetry collection,
automatic updating, credential access, persistence services, or background
processes. Installer scripts operate only on the user-selected Overstrike
folder, create a backup of the stock proxy, and support restoration.

Only the executable version documented in `README.md` is supported. Stop using
the mod if the game is updated until compatibility has been reconfirmed.

