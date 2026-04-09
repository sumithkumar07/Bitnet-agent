@echo off
setlocal

:: Check if cl.exe is in PATH
where cl >nul 2>nul
if %errorlevel% EQU 0 goto :skip_msvc

echo cl.exe not found. Attempting to initialize MSVC environment...
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do set "VS_DIR=%%i"

if not defined VS_DIR (
    echo [ERROR] Visual Studio installation not found.
    exit /b 1
)

if not exist "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" (
    echo [ERROR] Could not find vcvars64.bat
    exit /b 1
)

call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul

:skip_msvc

echo Cleaning old payloads...
del *.exe *.obj >nul 2>&1

echo Compiling Sovereign Kernel (AVX2 Enabled)...
cl src\main.cpp /I include /EHsc /O2 /arch:AVX2 /Fe: bitnet_daemon.exe

if %errorlevel% neq 0 (
    echo [ERROR] Native Hardware compile failure.
    exit /b %errorlevel%
)

echo Initializing Mesh...
bitnet_daemon.exe
