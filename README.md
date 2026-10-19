# Colourable Aquariums - Steam Workshop v1.0.1

Windows x64 mod for Two Point Museum. Adds a Choose Colour action to aquarium rooms, with a colour wheel, swatches, reset and saved per-museum colours. Standard tanks retain normal lighting; Dredge tanks use 25% of the corrected normal aquarium glow brightness. Adding or removing Dredge fish retains the selected tank colour.

## v1.0.1 compatibility fix

Fixes a missing aquarium colour wheel when the game changes the address displacement in ItemColorPickerMenu.UpdateInput. The instruction bytes and live address are validated before installing the hook. Retains the Steam Workshop v1.0 features, lighting and save behaviour.

Owner confirmed v1.0.1 works in-game on 19 October 2026. Confirmation from the originally affected user is pending. See VALIDATION.md for the scope of verification.

[Download v1.0.1](https://github.com/SeanB-Mod/Colourable-Aquariums/releases/tag/v1.0.1)

<img width="1277" height="827" alt="Aquarium colour picker" src="https://github.com/user-attachments/assets/3b221bf0-76cb-4ae0-b722-99d23bed21f7" />

## Video

https://www.youtube.com/watch?v=VCXSuzt28XY

## Steam Workshop installation

Install TPM Mod Loader v1.2.5 or later separately in the game folder containing TPM.exe. The loader cannot run directly from a Workshop subscription: open the loader download location and copy its installation files into the game folder as described by its instructions.

Subscribe to this mod and allow Steam to finish downloading. Keep its DLL in the Workshop item's `DLL Mod` folder. The compatible loader discovers it under the game's Workshop content folder (game ID 2185060); the numbered item ID can change without editing this mod. Restart the game after subscribing or updating.

## Local / Nexus installation

Close the game. Extract the Windows-x64 ZIP into the game folder so `TPM-AquaColour.dll` lands in `Mods`. Add it once to the numbered `[Mods]` list in `Mods/loader.ini` if missing. The same DLL supports both installation methods. A successfully loaded local copy takes priority over its Workshop copy; remove an older local DLL when switching to Workshop.

## Use and optional integration

Select an aquarium room and click Choose Colour. Use the wheel or swatches, then close the picker. Reset restores the tank's original appearance. Colours are stored separately from the game's museum save.

Expanded Colour Picker is optional. Installing it adds its expanded swatches, RGB/HEX entry and eyedropper support. This mod does not require Mod Engine or Expanded Colour Picker.

## Updates, data and troubleshooting

Close the game before replacing a local DLL. Logs and saved colour data are stored under the game's `Mods/Logs` and `Mods/Saves`, including when the DLL loads from Workshop. The log is `AquariumColour.log`. Preserve these folders when updating or switching installation methods. If migrating from an older version that wrote data beside its Workshop DLL, keep that data backed up; this release uses the game's Mods folder.

Do not distribute your generated logs or saves with a Workshop upload. The prepared upload ZIP contains only `DLL Mod/TPM-AquaColour.dll`. The mod is Windows x64 only; native game updates may require a compatibility update.

## Release validation

All 32 isolated checks passed. The packaged source rebuilds to the exact DLL tested by the owner. The release retains the same DLL bytes and diagnostic `1.0.1-test` labels as that tested build. Original and October signatures are covered; confirmation on the originally reported user's build is still pending. See VALIDATION.md.

BUILD.md documents compilation; WORKSHOP_DESCRIPTION.md contains ready-to-paste listing text. No source licence has been assigned. See SOURCE_NOTES.md for provenance.
