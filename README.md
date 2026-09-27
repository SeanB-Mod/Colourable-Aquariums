# Colourable Aquariums

Version **0.7.81.5** · Windows x64 · Two Point Museum

Adds a Choose Colour action to supported aquarium rooms. Aquarium colours are saved separately under Mods/Saves. Works independently of Expanded Colour Picker; installing both enables its 35 swatches, RGB/HEX entry and eyedropper.

<img width="1277" height="827" alt="Screenshot 2026-09-27 135130" src="https://github.com/user-attachments/assets/3b221bf0-76cb-4ae0-b722-99d23bed21f7" />

## Video

https://www.youtube.com/watch?v=VCXSuzt28XY

## Installation

Install the Two Point Museum Mod Loader separately. Close the game, then extract the release ZIP into the folder containing TPM.exe so the DLL lands in Mods. Add TPM-AquaColour.dll once to the numbered [Mods] list in Mods/loader.ini if it is not already present. The other colour mod and Mod Engine are not required.

To update, replace only this mod's DLL while the game is closed. Preserve Mods/Saves. To uninstall, remove this mod's INI entry and DLL; keep its saved data if you may reinstall.

## Usage

Select an aquarium room and use its Choose Colour action. Use the colour wheel and swatches to set its colour. With Expanded Colour Picker installed, the same window gains RGB/HEX entry, 35 swatches and screen colour sampling. This DLL exposes optional compatibility entry points; it has no required import of Colour Picker.

## Requirements and compatibility

Requires an installed Windows x64 copy of Two Point Museum. This is an unofficial mod, not a macOS/Linux build or a Workshop-only installation. Game updates can change native APIs; validation is against the local retail game installation used on 27 September 2026. Game binaries and SDK dumps are not distributed.

Each colour mod can run without the other. Both require a compatible native DLL loader; the approved predecessor was live-tested with the WinHTTP loader setup. This release has the packaged automated validation results and owner-confirmed in-game checks described in COMPATIBILITY.md.

## Data and troubleshooting

Check Mods/Logs/AquariumColour.log. Aquarium snapshots and backups remain under Mods/Saves. No player saves, personal settings or logs are included in the release. Keep backups of your own saves before changing your mod setup.

## Source and release files

See BUILD.md for reproducible build commands, VALIDATION.md for checks and RELEASE_NOTES.md for the GitHub release description. VERSION records this release number. Source filenames retain historical internal names; DLL basenames are intentionally unchanged for loader compatibility. No new source license has been selected in this packaging. See SOURCE_NOTES.md for asset and dependency provenance.

## Release status

Remove all legacy F8–F12 bindings and the diagnostic checkpoint. Preserve UI button actions and Escape-to-close. Remove unreachable metadata/render/glow probes and obsolete diagnostic helpers. Optional Colour Picker support remains independent.

See COMPATIBILITY.md and GAME_BUILD.json. Licensing remains undecided; source availability is not a grant of an open-source licence.
