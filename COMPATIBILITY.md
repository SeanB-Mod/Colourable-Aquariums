# Test-build compatibility

The UpdateInput guard described in TEST_BUILD.md supersedes the original two-signature allowlist. No universal game-version compatibility is claimed. Owner confirmed v1.0.1 works in-game on 19 October 2026. Confirmation from the originally affected user is pending. The following v1.0 notes are historical context.

# Compatibility

Windows x64, Two Point Museum, TPM Mod Loader v1.2.5 or later. The same DLL supports local `Mods` and Workshop `DLL Mod` installations. Expanded Colour Picker integration is optional; Mod Engine is not required.

GAME_BUILD.json records the installed game files inspected on 10 October 2026. Native hooks resolve through runtime metadata and check exact known instruction sequences, including the accepted October UI input variant. The default Dredge lighting reader additionally checks its native call sequence and runtime field types. Missing or incompatible APIs fail closed; these checks do not promise compatibility with future game updates.

All Unity work uses the game thread. There is no hot-unload support; close the game before changing local DLLs. Both installed copies should not be used independently: the loader prioritises a successful local load.

The user confirmed 100+ FPS, save/load/reset and fish transitions on the preceding candidates and approved the 25% Dredge output of 0.7.81.14. v1.0 keeps that rendering and removes temporary diagnostics. Automated checks are listed in VALIDATION.md; the final DLL has not been separately live-tested by the build agent.
