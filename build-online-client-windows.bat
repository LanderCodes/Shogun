@echo off
setlocal EnableExtensions

set "LOG=%~dp0windows-build.log"
call :main > "%LOG%" 2>&1
set "RC=%ERRORLEVEL%"
type "%LOG%"
if not "%RC%"=="0" (
    echo.
    echo Build failed. Full output saved to:
    echo %LOG%
    pause
)
exit /b %RC%

:main
where winget >nul 2>nul
if errorlevel 1 (
    set "FAILMSG=winget was not found. Install Windows 11 or an updated Windows 10, then run this again."
    goto :fail
)

if exist "%~dp0online\CMakeLists.txt" (
    cd /d "%~dp0"
) else if exist "%~dp0CMakeLists.txt" (
    cd /d "%~dp0.."
) else (
    cd /d "%~dp0"
    winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
    if errorlevel 1 (
        set "FAILMSG=Git installation failed."
        goto :fail
    )
    set "PATH=%ProgramFiles%\Git\cmd;%ProgramFiles(x86)\Git\cmd;%PATH%"
    if not exist "Shogun" git clone --filter=blob:none --sparse https://github.com/LanderCodes/Shogun.git Shogun
    if errorlevel 1 (
        set "FAILMSG=Repository clone failed."
        goto :fail
    )
    cd /d "Shogun"
    git sparse-checkout set online
    if errorlevel 1 (
        set "FAILMSG=Sparse checkout failed."
        goto :fail
    )
)

git rev-parse --is-inside-work-tree >nul 2>nul
if not errorlevel 1 (
    git pull --ff-only
    if errorlevel 1 (
        set "FAILMSG=Git pull failed."
        goto :fail
    )
)

winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
if errorlevel 1 (
    set "FAILMSG=Git installation failed."
    goto :fail
)
winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements
if errorlevel 1 (
    set "FAILMSG=CMake installation failed."
    goto :fail
)
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-source-agreements --accept-package-agreements --override "--wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
if errorlevel 1 (
    set "FAILMSG=Visual Studio Build Tools installation failed."
    goto :fail
)

set "PATH=%ProgramFiles%\Git\cmd;%ProgramFiles(x86)\Git\cmd;%ProgramFiles%\CMake\bin;%ProgramFiles(x86)\CMake\bin;%PATH%"

where git >nul 2>nul
if errorlevel 1 (
    set "FAILMSG=Git was not found after installation."
    goto :fail
)
where cmake >nul 2>nul
if errorlevel 1 (
    set "FAILMSG=CMake was not found after installation."
    goto :fail
)

if "%VCPKG_ROOT%"=="" set "VCPKG_ROOT=%LOCALAPPDATA%\vcpkg"
if not exist "%VCPKG_ROOT%" git clone https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%"
if errorlevel 1 (
    set "FAILMSG=vcpkg clone failed."
    goto :fail
)
if not exist "%VCPKG_ROOT%\vcpkg.exe" call "%VCPKG_ROOT%\bootstrap-vcpkg.bat" -disableMetrics
if errorlevel 1 (
    set "FAILMSG=vcpkg bootstrap failed."
    goto :fail
)
"%VCPKG_ROOT%\vcpkg.exe" install sdl2:x64-windows-static
if errorlevel 1 (
    set "FAILMSG=SDL2 installation failed."
    goto :fail
)

if exist "online\build-client-static" rmdir /s /q "online\build-client-static"
cmake -S online -B online\build-client-static -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
if errorlevel 1 (
    set "FAILMSG=CMake configure failed."
    goto :fail
)
cmake --build online\build-client-static --config Release --target shogun --parallel
if errorlevel 1 (
    set "FAILMSG=Client build failed."
    goto :fail
)

if not exist "online\build-client-static\Release\shogun.exe" (
    set "FAILMSG=Build did not produce shogun.exe."
    goto :fail
)

if not exist "online\windows-client" mkdir "online\windows-client"
copy /Y "online\build-client-static\Release\shogun.exe" "online\windows-client\shogun.exe" >nul
if errorlevel 1 (
    set "FAILMSG=Copying shogun.exe failed."
    goto :fail
)

echo Single-file Windows client build ready in online\windows-client
exit /b 0

:fail
echo ERROR: %FAILMSG%
exit /b 1
