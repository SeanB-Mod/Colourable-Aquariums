# Validation

28 isolated checks passed, including persistence, UI dispatch, main-thread bootstrap, optional bridges, stale target rejection, input focus and material lifecycle. All eight native hook guards match the recorded GameAssembly.dll.

Final source archives are extracted into a fresh directory, rebuilt and compared by SHA-256 with the player DLLs. Archive members and nested release assets are checked against explicit manifests; all player payload files have hashes. See the pack-set audit report for the recorded results.

The owner has also confirmed completion of the in-game checks in COMPATIBILITY.md. The approved fallbacks were not changed.
