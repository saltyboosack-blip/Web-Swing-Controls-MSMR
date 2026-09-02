# Changelog

## 0.3.1

- Preserved native Space + LMB Ground Strike input while airborne.
- Preserved native LMB Swing Kick input during an RMB-owned swing.
- Kept ordinary airborne LMB/RMB side selection and controller behavior
  unchanged.
- Documented that a held LMB cannot simultaneously act as both the left-web
  owner and a separate Swing Kick press.

## 0.3.0-rc2

- Preserved native L1/LB + R1/RB gameplay chords, including environmental
  item pickup, by suspending the L1/LB-to-zoom remap while both shoulders are
  held.
- Kept direct airborne L2/LT left swing and R2/RT right swing unchanged.
- Confirmed the right-shoulder state field and mapping against exact Steam
  build 23986256 before adding the chord exception.
- Confirmed the complete controller layout working in game on a PS5
  controller, including native L1+R1 item pickup.

## 0.3.0-rc1

- Removed the controller L1/LB swing-layer requirement.
- Made airborne L2/LT directly request a left-side swing and R2/RT directly
  request a right-side swing.
- Remapped the former native L2/LT zoom action to L1/LB and suppressed L1/LB's
  original action.
- Kept mouse controls, native anchors, native swinging, and physics unchanged.
- Added direct-trigger, remap, ownership, disconnect, focus-loss, and exhaustive
  controller policy coverage.

## SMPCTool Companion 1.0.0

- Added an unofficial standalone companion for jedijosh920's Spider-Man PC
  Modding Tool v1.1.1.
- Read the official tool's `assetArchiveDir.txt` to locate the correct game
  folder without modifying or redistributing SMPCTool.
- Added exact payload and game-build verification, conflict refusal, verified
  backups, staged writes, rollback, and twelve fixture tests.
- Documented why this native script mod cannot work as an `.smpcmod` archive
  replacement package.

## 0.2.1-beta

- Integrated the controller repair into the complete install package; no
  earlier version or separate update is required.
- Added Sony SCE, Steam Input, XInput, and Windows.Gaming.Input controller
  routing.
- Made airborne L1/LB a controller swing layer that reserves both triggers.
- Mapped L2/LT to the character-left web and R2/RT to the character-right web
  through the game's normalized native Swing trigger.
- Prevented default trigger actions from leaking through while the layer is
  active.
- Confirmed the complete package working in game with a PS5 controller.

## 0.2.0-beta

- Added airborne left-mouse and right-mouse side-specific swing requests.
- Added controller input: hold L1, then L2 for left or R2 for right.
- Kept the mod controls-only with no custom swing physics.
- Added an Overstrike 1.8.0 proxy host gate for Steam helper compatibility.
- Added backup, restoration, verification, source, and build documentation.
