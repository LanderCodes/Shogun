param(
    [string]$MingwBin = "C:\msys64\ucrt64\bin",
    [string]$OutputDir = "",
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param(
        [string]$Executable,
        [string[]]$Arguments
    )
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Executable failed with exit code $LASTEXITCODE"
    }
}

function Resolve-Tool {
    param(
        [string]$Directory,
        [string]$Name
    )
    $candidate = Join-Path $Directory $Name
    if (Test-Path -LiteralPath $candidate) {
        return $candidate
    }
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    throw "Could not find $Name. Pass -MingwBin with a MinGW-w64 bin directory."
}

if (-not (Test-Path -LiteralPath $MingwBin)) {
    throw "MinGW bin directory not found: $MingwBin"
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$onlineDir = $PSScriptRoot
$buildDir = Join-Path $onlineDir "build-windows"
if (-not $OutputDir) {
    $OutputDir = Join-Path $onlineDir "windows-release"
}

$gcc = Resolve-Tool -Directory $MingwBin -Name "gcc.exe"
$cmake = Resolve-Tool -Directory $MingwBin -Name "cmake.exe"
$ctest = Resolve-Tool -Directory $MingwBin -Name "ctest.exe"
$make = Resolve-Tool -Directory $MingwBin -Name "mingw32-make.exe"
$sdlDll = Join-Path $MingwBin "SDL2.dll"
if (-not (Test-Path -LiteralPath $sdlDll)) {
    throw "SDL2.dll not found in $MingwBin"
}

$libDir = [System.IO.Path]::GetFullPath((Join-Path $MingwBin "..\lib"))
$pkgConfigDir = Join-Path $libDir "pkgconfig"
$includeDir = [System.IO.Path]::GetFullPath((Join-Path $MingwBin "..\include"))
if (-not (Test-Path -LiteralPath (Join-Path $includeDir "SDL2\SDL.h"))) {
    throw "SDL2 headers not found in $includeDir"
}
if (-not (Test-Path -LiteralPath $pkgConfigDir)) {
    throw "pkg-config files not found in $pkgConfigDir"
}

$env:PATH = "$MingwBin;$env:PATH"
$env:PKG_CONFIG_PATH = "$pkgConfigDir;$env:PKG_CONFIG_PATH"

$configureArgs = @(
    "-S", $onlineDir,
    "-B", $buildDir,
    "-G", "MinGW Makefiles",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_MAKE_PROGRAM=$make",
    "-DCMAKE_C_COMPILER=$gcc"
)
Invoke-Checked -Executable $cmake -Arguments $configureArgs
Invoke-Checked -Executable $cmake -Arguments @("--build", $buildDir, "--config", "Release", "--parallel")

if (-not $SkipTests) {
    Invoke-Checked -Executable $ctest -Arguments @("--test-dir", $buildDir, "--output-on-failure")
}

$clientExe = Join-Path $buildDir "shogun.exe"
$serverExe = Join-Path $buildDir "shogun-server.exe"
if (-not (Test-Path -LiteralPath $clientExe)) {
    throw "Build did not produce $clientExe"
}
if (-not (Test-Path -LiteralPath $serverExe)) {
    throw "Build did not produce $serverExe"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
Copy-Item -LiteralPath $clientExe -Destination (Join-Path $OutputDir "shogun.exe") -Force
Copy-Item -LiteralPath $serverExe -Destination (Join-Path $OutputDir "shogun-server.exe") -Force
Copy-Item -LiteralPath $sdlDll -Destination (Join-Path $OutputDir "SDL2.dll") -Force

Write-Host "Windows build ready in $OutputDir"
