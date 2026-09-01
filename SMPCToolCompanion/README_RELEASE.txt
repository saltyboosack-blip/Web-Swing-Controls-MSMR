WEB SWING CONTROLS - SMPCTOOL COMPANION
Version 1.1.0 RC2

This is an unofficial standalone companion for jedijosh920's Spider-Man PC
Modding Tool (SMPCTool) v1.1.1. It is not a modified copy of SMPCTool and does
not contain, patch, or redistribute SMPCTool files.

INSTALLATION

1. Download and extract the official SMPCTool v1.1.1 from Nexus Mods page 51.
2. Open SMPCTool and select this folder when it asks for the asset archive:

   ...\Marvel's Spider-Man Remastered\asset_archive

3. Close SMPCTool and close the game.
4. Extract this companion folder into the folder containing SMPCTool.exe.
   Keep the companion as its own adjacent subfolder, or place its contents
   directly beside SMPCTool.exe. Both layouts work.
5. Run WebSwingControls_SMPCTool_Companion.exe.
6. Confirm the detected SMPCTool, asset archive, and game paths.
7. Choose Install.
8. Launch the game normally through Steam after the success message.

The companion reads SMPCTool's assetArchiveDir.txt setting. It does not change
SMPCTool itself, the game's toc, or asset archives. It installs only:

  scripts\TrueSwing.dll
  scripts.txt
  winmm.dll
  commandline.txt

Every replaced file receives a verified backup under the game folder's
"WSC Backups" directory. Unknown game builds, loaders, and
TrueSwing DLLs are refused. The companion never closes running programs.

CONTROLLER LAYOUT

  Airborne L2/LT = left-side swing
  Airborne R2/RT = right-side swing
  L1/LB = former L2/LT zoom action
  L1/LB + R1/RB = native gameplay chord; zoom is suspended

No L1/LB hold is required for swinging. Mouse controls remain unchanged.

SUPPORTED GAME

Marvel's Spider-Man Remastered Steam build 23986256
Spider-Man.exe version 4.0630.0.0

SOURCE

https://github.com/saltyboosack-blip/Web-Swing-Controls-MSMR/tree/main/SMPCToolCompanion
