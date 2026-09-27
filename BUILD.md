# Build and test

Use Windows x64, PowerShell and LLVM-MinGW (verified: clang 22.1.8, 20260616 UCRT x86_64). Python 3 is required for loader export generation and Aquarium test generation. Pillow is only needed to regenerate the Colour Picker embedded image header; existing headers suffice for compilation.

From the repository root:

```powershell
.\src\build.ps1 -Toolchain C:\Tools\llvm-mingw
```

Output: src/build/TPM-AquaColour.dll. The scripts do not install into the game. No Unity SDK or game assembly is needed to compile. Optional API/hook verification scripts require your own local dump/game assembly, neither of which is included.

## Isolated tests

```powershell
python src/run_tests.py --compiler C:\Tools\llvm-mingw\bin\clang++.exe --output src/build/tests
```

Loader fixtures execute dummy mods in temporary test folders only. Do not place test_host.exe or test_mod.dll in the game directory.

## Packaging

Run `python package_release.py --binary src/build/TPM-AquaColour.dll` after a successful build. The reviewed PACKAGE_MANIFEST.json is the only source allowlist. Update it deliberately when adding files. Runtime ZIPs contain only installable DLL/configuration files; documentation and file hashes remain in the source pack and accompanying audit report. No documentation is installed beside TPM.exe.
