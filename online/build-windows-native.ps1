param(
    [string]$VcpkgRoot = "",
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

function Install-Winget {
    param(
        [string]$Id,
        [string]$Override = ""
    )
    if ($Override) {
        & winget install --id $Id -e --accept-source-agreements --accept-package-agreements --override $Override
    } else {
        & winget install --id $Id -e --accept-source-agreements --accept-package-agreements
    }
    if ($LASTEXITCODE -ne 0) {
        throw "winget failed to install $Id with exit code $LASTEXITCODE"
    }
}

$programFilesX86 = ${env:ProgramFiles(x86)}
$pathAdditions = @(
    (Join-Path $env:ProgramFiles "Git\cmd"),
    (Join-Path $env:ProgramFiles "Git\bin"),
    (Join-Path $env:ProgramFiles "CMake\bin")
)
if ($programFilesX86) {
    $pathAdditions += (Join-Path $programFilesX86 "Git\cmd")
    $pathAdditions += (Join-Path $programFilesX86 "CMake\bin")
}
foreach ($addition in $pathAdditions) {
    if (Test-Path -LiteralPath $addition) {
        $env:PATH = "$addition;$env:PATH"
    }
}

if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    throw "winget is required but was not found"
}

Install-Winget -Id "Git.Git"
Install-Winget -Id "Kitware.CMake"
Install-Winget -Id "Microsoft.VisualStudio.2022.BuildTools" -Override "--wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

$pathAdditions = @(
    (Join-Path $env:ProgramFiles "Git\cmd"),
    (Join-Path $env:ProgramFiles "Git\bin"),
    (Join-Path $env:ProgramFiles "CMake\bin")
)
if ($programFilesX86) {
    $pathAdditions += (Join-Path $programFilesX86 "Git\cmd")
    $pathAdditions += (Join-Path $programFilesX86 "CMake\bin")
}
foreach ($addition in $pathAdditions) {
    if (Test-Path -LiteralPath $addition) {
        $env:PATH = "$addition;$env:PATH"
    }
}

$git = (Get-Command git.exe -ErrorAction SilentlyContinue).Source
$cmake = (Get-Command cmake.exe -ErrorAction SilentlyContinue).Source
$ctest = (Get-Command ctest.exe -ErrorAction SilentlyContinue).Source
if (-not $git) { throw "git.exe not found after installing Git" }
if (-not $cmake) { throw "cmake.exe not found after installing CMake" }
if (-not $ctest) { throw "ctest.exe not found after installing CMake" }

if (-not $VcpkgRoot) {
    $VcpkgRoot = Join-Path $env:LOCALAPPDATA "vcpkg"
}
if (-not (Test-Path -LiteralPath $VcpkgRoot)) {
    Invoke-Checked -Executable $git -Arguments @("clone", "https://github.com/microsoft/vcpkg.git", $VcpkgRoot)
}

$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path -LiteralPath $vcpkgExe)) {
    Invoke-Checked -Executable (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -Arguments @("-disableMetrics")
}
Invoke-Checked -Executable $vcpkgExe -Arguments @("install", "sdl2:x64-windows")

$vcpkgBin = Join-Path $VcpkgRoot "installed\x64-windows\bin"
$sdlDll = Join-Path $vcpkgBin "SDL2.dll"
if (-not (Test-Path -LiteralPath $sdlDll)) {
    throw "SDL2.dll not found in $vcpkgBin"
}
$env:PATH = "$vcpkgBin;$env:PATH"

$onlineDir = $PSScriptRoot
$buildDir = Join-Path $onlineDir "build-windows-native"
if (-not $OutputDir) {
    $OutputDir = Join-Path $onlineDir "windows-release-native"
}

$toolchain = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
Invoke-Checked -Executable $cmake -Arguments @(
    "-S", $onlineDir,
    "-B", $buildDir,
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    "-DVCPKG_TARGET_TRIPLET=x64-windows"
)
Invoke-Checked -Executable $cmake -Arguments @("--build", $buildDir, "--config", "Release", "--parallel")

if (-not $SkipTests) {
    Invoke-Checked -Executable $ctest -Arguments @("--test-dir", $buildDir, "-C", "Release", "--output-on-failure")
}

$clientExe = Join-Path $buildDir "Release\shogun.exe"
$serverExe = Join-Path $buildDir "Release\shogun-server.exe"
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
