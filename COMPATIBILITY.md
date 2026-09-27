# Compatibility and release status

These are Windows x64 releases. The owner confirmed completion of the in-game checks on 27 September 2026. The installed game identity is recorded in GAME_BUILD.json; no game binaries or API dumps are redistributed.

The loader forwards 92 WinHTTP exports to the System32 library. It does not initiate telemetry or network requests; the game and separately installed mods may use WinHTTP normally. Missing system exports fail explicitly rather than invent an ABI-incompatible return value. Mod loading runs on a worker after DllMain; there is no hot-unload support and DllMain never waits for that worker.

Colourable Aquariums dispatches Unity setup and updates on the game thread. Its native hooks check known prologue bytes before patching and refuse a mismatching target. The six UI patches are checked as a batch with rollback on installation failure. These checks reduce risk; they do not establish universal update compatibility or guarantee rollback when Windows refuses protection changes. The lighting consumer and update hook are separately guarded.

Expanded Colour Picker resolves named IL2CPP/UI APIs and uses the verified 64-bit, one-dimensional array layout (data at 0x20). Missing required APIs prevent setup, but there is no runtime whole-game hash gate: an update that preserves names while changing semantics/layout may still be incompatible. The centralized array helper documents this assumption; it does not make the DLL portable. Revalidate after game updates.

The two colour mods discover each other through optional runtime exports. Neither imports the other's DLL. Mod Engine is a separate, binary-only project and is not supplied or modified here. All function-key bindings in these three source releases are removed/absent; this does not change controls in separately installed software. Ctrl sampling, Tab/Shift+Tab and Escape remain available where applicable.

## In-game checks

- Loader with no mods, then each colour mod alone, then both together.
- Aquarium Choose Colour, wheel, swatches and reset/close buttons; F1–F12 cause no mod action.
- Picker RGB/HEX, Tab/Shift+Tab, 35 swatches, tooltips in both drawer states, eyedropper button and Ctrl/cancel.
- Save/load, quit/relaunch and switching save scopes retain the correct independent colours.

The owner confirmed that the in-game checks have been completed. Licensing remains undecided by the owner's request; no MIT or other licence is assigned. Artwork provenance is recorded separately in SOURCE_NOTES.md.
