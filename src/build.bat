@echo off
setlocal

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat"
) else (
    echo [WARNING] vcvars32.bat not found automatically. Using fallback path...
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
)


cl.exe /O2 /MT /LD /Fe:dinput.dll main.cpp /link /DEF:dinput.def

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================
    echo Build successful! dinput.dll is ready.
    echo ====================================
) else (
    echo.
    echo [ERROR] Build failed.
)

if exist dinput.obj del dinput.obj
if exist dinput.exp del dinput.exp
if exist dinput.lib del dinput.lib

pause
