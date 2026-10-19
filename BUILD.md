# Build and test

Windows x64, LLVM-MinGW clang 22.1.8 and Python 3. From the repository root:

```powershell
.\src\build.ps1 -Toolchain C:\Tools\llvm-mingw
python src/run_tests.py --compiler C:\Tools\llvm-mingw\bin\clang++.exe --output src/build/tests
python package_release.py --binary src/build/TPM-AquaColour.dll
```

Output DLL: `src/build/TPM-AquaColour.dll`. No game assembly or Unity SDK is needed to compile. Isolated fixtures do not install mods or launch the game. Packaging uses PACKAGE_MANIFEST.json and produces local, Workshop and source ZIPs with hashes and an audit. Scripts do not upload anything or copy files into the game.

The legacy verify_game_hooks.py utility records older fixed RVAs and is historical; current hooks resolve their targets via runtime metadata and guarded instruction signatures. Revalidate compatibility after game updates.
