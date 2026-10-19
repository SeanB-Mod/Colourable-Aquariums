## Validation performed
- Rebuilt unmodified v1.0 with clang 22.1.8; its SHA-256 exactly matched the supplied DLL: d15a0e8a1f3edaed22a92d5d2e8fe78f3b9a91fd81ea3f7a8ac4e80ac0cc22a8.
- Test DLL compiled successfully using the same toolchain.
- All 30 existing isolated checks passed, including 10,000 executed relocated native calls.
- New input guard check passed: original/October/synthetic 02 variants; every prefix byte mutation; positive/negative displacement; module/region edges; unreadable, guard, uncommitted, non-image and wrong-allocation rejection.
- New rollback check executes the production capture/rollback code and verifies live bytes after simulated installation failure following each of hooks 1 through 5.
- 32 isolated checks total. Owner confirmed v1.0.1 works in-game on 19 October 2026. This was a general functionality confirmation, not a separately recorded pass of every item in the test checklist. Confirmation from the originally affected user is pending.
- GitHub release packaging retains the exact owner-tested DLL, including its diagnostic test labels.

The GitHub pack includes the source, tests and CHANGES.patch. GAME_BUILD.json is inherited historical v1.0 provenance, not a record of testing the affected user's installation.
