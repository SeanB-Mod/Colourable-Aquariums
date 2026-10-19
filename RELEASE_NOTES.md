# Colourable Aquariums v1.0.1

Fixes a missing aquarium colour wheel caused by an overly strict UpdateInput hook guard. The guard now verifies the instruction bytes and validates the live RIP-relative address inside readable GameAssembly image memory instead of accepting only two hard-coded address displacements. Other hook checks and original-byte rollback remain intact.

Includes all Steam Workshop v1.0 changes: Workshop/local loading support, corrected Dredge glow and fish transitions, and the existing per-museum colour persistence and optional Expanded Colour Picker integration. The compatibility fix changes no rendering, lighting, save format or controls relative to v1.0.

## Validation

- 32 isolated checks passed, including guard rejection cases, production rollback and native relocation tests.
- Fresh source-package rebuild matches the tested DLL exactly.
- Owner confirmed v1.0.1 works in-game on 19 October 2026. Confirmation from the originally affected user is pending; compatibility with every game build is not claimed.
- This release uses the exact tested DLL; internal diagnostic labels still say `1.0.1-test`.

## Downloads and updating

- Windows-x64 ZIP: local installation; replace Mods/TPM-AquaColour.dll with the game closed.
- Workshop-Upload ZIP: staging content for a Workshop upload; contains DLL Mod/TPM-AquaColour.dll. Publishing this GitHub release does not update Steam Workshop automatically.
- GitHub-Pack ZIP: source, tests and both runtime archives.

Back up your existing DLL and Mods/Saves before updating. Preserve saves and avoid loading duplicate local and Workshop copies. The compatible loader gives a successfully loaded local copy priority. See README.md and TEST_BUILD.md for installation and rollback details.
