# Changelog

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
