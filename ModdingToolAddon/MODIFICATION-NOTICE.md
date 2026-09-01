# Modification notice

Unofficial derivative prepared for Web Swing Controls on 2026-09-01.

Changes from upstream Modding Tool 1.4.5:

- added `Mod > Install MSMR .script...`;
- resolved a loaded `asset_archive/toc` back to the MSMR game root;
- added an embedded, hash-pinned, helper-safe MSMR script proxy;
- added exact game-build validation and safe `.script` archive parsing;
- added non-destructive merge behavior for `scripts.txt` and
  `commandline.txt`;
- added verified backups, atomic writes, rollback, and install manifests;
- added a running-game guard that never terminates a process;
- added a dependency-free core library and disposable fixture test program.

Original project: <https://github.com/Tkachov/Overstrike>

Original copied commit: `8c3a655d7516bbba20ec2fac6bb16ecb3d13c85c`

