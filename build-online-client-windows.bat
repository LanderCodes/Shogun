@echo off
setlocal EnableExtensions

where winget >nul 2>nul
if errorlevel 1 (
    echo winget was not found. Install Windows 11 or an updated Windows 10, then run this again.
    exit /b 1
)

if exist "%~dp0online\CMakeLists.txt" (
    cd /d "%~dp0"
) else if exist "%~dp0CMakeLists.txt" (
    cd /d "%~dp0.."
) else (
    cd /d "%~dp0"
    winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
    if errorlevel 1 exit /b 1
    set "PATH=%ProgramFiles%\Git\cmd;%ProgramFiles(x86)%\Git\cmd;%PATH%"
    if not exist "Shogun" git clone --filter=blob:none --sparse https://github.com/LanderCodes/Shogun.git Shogun
    if errorlevel 1 exit /b 1
    cd /d "Shogun"
    git sparse-checkout set online
    if errorlevel 1 exit /b 1
)

winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
if errorlevel 1 exit /b 1
winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements
if errorlevel 1 exit /b 1
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-source-agreements --accept-package-agreements --override "--wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
if errorlevel 1 exit /b 1

set "PATH=%ProgramFiles%\Git\cmd;%ProgramFiles(x86)%\Git\cmd;%ProgramFiles%\CMake\bin;%ProgramFiles(x86)%\CMake\bin;%PATH%"

where git >nul 2>nul
if errorlevel 1 (
    echo Git was not found after installation.
    exit /b 1
)
where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake was not found after installation.
    exit /b 1
)

if "%VCPKG_ROOT%"=="" set "VCPKG_ROOT=%LOCALAPPDATA%\vcpkg"
if not exist "%VCPKG_ROOT%" git clone https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%"
if errorlevel 1 exit /b 1
if not exist "%VCPKG_ROOT%\vcpkg.exe" call "%VCPKG_ROOT%\bootstrap-vcpkg.bat" -disableMetrics
if errorlevel 1 exit /b 1
"%VCPKG_ROOT%\vcpkg.exe" install sdl2:x64-windows-static
if errorlevel 1 exit /b 1

if exist "online\build-client-static" rmdir /s /q "online\build-client-static"
cmake -S online -B online\build-client-static -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
if errorlevel 1 exit /b 1
cmake --build online\build-client-static --config Release --target shogun --parallel
if errorlevel 1 exit /b 1

if not exist "online\build-client-static\Release\shogun.exe" (
    echo Build did not produce shogun.exe.
    exit /b 1
)

if not exist "online\windows-client" mkdir "online\windows-client"
copy /Y "online\build-client-static\Release\shogun.exe" "online\windows-client\shogun.exe" >nul

echo Single-file Windows client build ready in online\windows-client
