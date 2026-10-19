# Colourable Aquariums v1.0.1 TEST BUILD - 19 October 2026

Compatibility test instructions retained for v1.0.1. Owner confirmed this exact DLL works in-game on 19 October 2026; confirmation from the originally affected user is pending. The GitHub update does not itself publish a Steam Workshop update.

## Change
Only the UpdateInput hook guard changes at runtime. It matches all ten instruction bytes exactly, then calculates the signed RIP-relative operand from the four live displacement bytes. The complete 8-byte referenced slot must lie inside GameAssembly.dll and a committed, readable MEM_IMAGE region whose allocation base is GameAssembly.dll. VirtualQuery failure, guard/no-access memory, changed opcodes and out-of-image operands abort UI installation. The existing relocation and original-byte rollback remain in use. The other five UI signatures remain exact. Existing v1.0 original and October signatures are covered by tests. Rendering, lighting, colour values, persistence and controls are unchanged.

This checks instruction shape and address accessibility, not the semantic identity of arbitrary future game builds. The affected user's complete method bytes and game binary were not available. The test with a displacement beginning 02 is synthetic, not a reproduction of their full build.

## Install for testing
1. Exit the game. Back up the currently working v1.0 DLL and Mods/Saves folder.
2. For a local installation, replace Mods/TPM-AquaColour.dll with the DLL in the Windows-x64 ZIP. Retain the existing loader entry.
3. If using a Workshop-only installation, use a temporary local Mods/TPM-AquaColour.dll and add its entry to the loader's numbered [Mods] list following the existing format. The supplied v1.0 README says the local copy takes precedence. Check the test-version messages below to confirm which copy actually loaded. Do not arrange for both copies to load.
4. Do not change or delete save files or update the public Workshop item during this test.

## Verify on both machines
First test on the machine where v1.0 already works, then on the affected user's machine.
- Open an aquarium's Choose Colour action; confirm wheel, live colour changes, swatches, reset and close.
- Check normal game input after closing, and another aquarium.
- Save, reload and restart; check colours persist and remain independent between tanks.
- Where available, check a standard tank, a Dredge tank and optional Expanded Colour Picker integration.
- Read Mods/Logs/AquariumColour.log. Successful installation should include:
  `1.0.1-test UI: UpdateInput signature accepted; RIP bytes=...; operand RVA=... validated.`
  `1.0.1-test UI: six guarded hooks installed on game thread; native aquarium button enabled.`
- If the wheel is still absent, return the complete fresh startup log, game build/Steam branch and mod list. The operand log may precede a later hook failure, so look for the six-hooks success message too.

## Roll back
Exit the game. Restore the backed-up v1.0 DLL, or remove the temporary local test DLL and its temporary loader entry to resume Workshop loading. Restore the save backup only if needed; the test changes no save format.

## Validation performed
- Rebuilt unmodified v1.0 with clang 22.1.8; its SHA-256 exactly matched the supplied DLL: d15a0e8a1f3edaed22a92d5d2e8fe78f3b9a91fd81ea3f7a8ac4e80ac0cc22a8.
- Test DLL compiled successfully using the same toolchain.
- All 30 existing isolated checks passed, including 10,000 executed relocated native calls.
- New input guard check passed: original/October/synthetic 02 variants; every prefix byte mutation; positive/negative displacement; module/region edges; unreadable, guard, uncommitted, non-image and wrong-allocation rejection.
- New rollback check executes the production capture/rollback code and verifies live bytes after simulated installation failure following each of hooks 1 through 5.
- 32 isolated checks total. Owner confirmed working in-game on 19 October 2026; originally affected user confirmation pending.

The GitHub pack includes the source, tests and CHANGES.patch. GAME_BUILD.json is inherited historical v1.0 provenance, not a record of testing the affected user's installation.
