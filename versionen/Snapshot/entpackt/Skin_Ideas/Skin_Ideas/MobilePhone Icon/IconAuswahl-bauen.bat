@echo off
setlocal
cd /d "%~dp0"

set "RAYLIB_SRC=C:\raylib\raylib\src"
set "GCC_EXE=C:\raylib\w64devkit\bin\gcc.exe"

if not exist "%GCC_EXE%" (
    echo FEHLER: GCC wurde nicht unter "%GCC_EXE%" gefunden.
    pause
    exit /b 1
)

if not exist "%RAYLIB_SRC%\libraylib.a" (
    echo FEHLER: raylib wurde nicht unter "%RAYLIB_SRC%" gefunden.
    pause
    exit /b 1
)

"%GCC_EXE%" -B"C:/raylib/w64devkit/bin/" -std=c99 -O2 -Wall -Wextra -Wpedantic -mwindows ^
    icon_selector.c -o IconAuswahl.exe ^
    -I"%RAYLIB_SRC%" -L"%RAYLIB_SRC%" ^
    -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32

if errorlevel 1 (
    echo.
    echo Build FEHLGESCHLAGEN.
    pause
    exit /b 1
)

echo.
echo Build OK: IconAuswahl.exe
echo Du kannst die EXE jetzt per Doppelklick starten.
pause
