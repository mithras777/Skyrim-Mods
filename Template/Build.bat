@echo off
setlocal EnableExtensions
cd /d "%~dp0"

for %%I in ("%CD%") do set "PROJECT_NAME=%%~nxI"

where cmake >nul 2>&1
if errorlevel 1 (
	echo [ERROR] CMake was not found in PATH. Install CMake 3.24 or newer.
	exit /b 1
)

where git >nul 2>&1
if errorlevel 1 (
	echo [ERROR] Git was not found in PATH. Git is needed if the shared CommonLibSSE-NG cache is unavailable.
	exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
	echo [ERROR] Visual Studio Installer's vswhere.exe was not found.
	exit /b 1
)

set "VSINSTALL="
for /f "usebackq tokens=* delims=" %%V in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%V"
if not defined VSINSTALL (
	echo [ERROR] The MSVC C++ build tools were not found. Install the Visual C++ x64/x86 build tools.
	exit /b 1
)

set "VSDEVCMD=%VSINSTALL%\Common7\Tools\VsDevCmd.bat"
set "VCPKG_ROOT=%VSINSTALL%\VC\vcpkg"
set "NINJA_DIR=%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
if not exist "%VSDEVCMD%" (
	echo [ERROR] VsDevCmd.bat was not found under %VSINSTALL%.
	exit /b 1
)
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
	echo [ERROR] Visual Studio's vcpkg component is missing. Add it in Visual Studio Installer.
	exit /b 1
)
if not exist "%NINJA_DIR%\ninja.exe" (
	echo [ERROR] Visual Studio's Ninja executable was not found.
	exit /b 1
)

call "%VSDEVCMD%" -no_logo -arch=amd64
if errorlevel 1 (
	echo [ERROR] Could not initialize the MSVC command-line environment.
	exit /b 1
)
set "PATH=%NINJA_DIR%;%PATH%"
where cl >nul 2>&1
if errorlevel 1 (
	echo [ERROR] cl.exe was not found after initializing the MSVC environment.
	exit /b 1
)
where ninja >nul 2>&1
if errorlevel 1 (
	echo [ERROR] ninja.exe was not found after updating PATH.
	exit /b 1
)

echo.
echo [INFO] Configuring %PROJECT_NAME% with CMake, Ninja, and CommonLibSSE-NG...
cmake --preset windows-ninja
if errorlevel 1 (
	echo [ERROR] CMake configuration failed.
	exit /b 1
)

echo.
echo [INFO] Building the release DLL and packaging the Nexus ZIP...
cmake --build --preset windows-ninja-release --parallel
if errorlevel 1 (
	echo [ERROR] Build or ZIP packaging failed.
	exit /b 1
)

if not exist "Nexus Release\%PROJECT_NAME%.zip" (
	echo [ERROR] The build finished, but Nexus Release\%PROJECT_NAME%.zip was not created.
	exit /b 1
)

echo.
echo [SUCCESS] Created Nexus Release\%PROJECT_NAME%.zip
exit /b 0
