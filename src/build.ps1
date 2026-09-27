param([Parameter(Mandatory=$true)][string]$Toolchain,[string]$BuildDirectory)
$ErrorActionPreference='Stop'
if(-not $BuildDirectory){$BuildDirectory=Join-Path $PSScriptRoot 'build'}
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$clang=Join-Path $Toolchain 'bin\clang++.exe'
$linker=Join-Path $Toolchain 'bin\ld.lld.exe'
$object=Join-Path $BuildDirectory 'AquariumColour.obj'
$dll=Join-Path $BuildDirectory 'TPM-AquaColour.dll'
& $clang --target=x86_64-pc-windows-msvc -O2 -ffreestanding -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -funwind-tables -c (Join-Path $PSScriptRoot 'AquariumColour_source.cpp') -o $object
if($LASTEXITCODE -ne 0){throw 'Compile failed'}
& $linker -flavor link /dll /entry:DllMain /nodefaultlib /machine:x64 /timestamp:0 "/out:$dll" $object
if($LASTEXITCODE -ne 0){throw 'Link failed'}
& $clang --version
Get-FileHash -LiteralPath $dll -Algorithm SHA256
