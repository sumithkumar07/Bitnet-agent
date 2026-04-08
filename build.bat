@echo off
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
