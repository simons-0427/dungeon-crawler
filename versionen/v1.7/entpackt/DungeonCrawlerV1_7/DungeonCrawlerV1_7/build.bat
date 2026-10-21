@echo off
REM Build the Dungeon Crawler on Windows (double-click or run from cmd).
setlocal
pushd "%~dp0" || (endlocal & exit /b 1)
set PATH=C:\raylib\w64devkit\bin;%PATH%
set ICON_CHANGED=0
mingw32-make -q obj/app_icon.o >nul
if errorlevel 2 goto failed
if errorlevel 1 set ICON_CHANGED=1
mingw32-make -q >nul
if errorlevel 2 goto failed
if not errorlevel 1 goto up_to_date
mingw32-make -j4
if errorlevel 1 goto failed
if "%ICON_CHANGED%"=="1" powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0refresh_icon_cache.ps1" "%~dp0dungeon_crawler.exe"
echo Build OK -^> dungeon_crawler.exe
popd
endlocal
exit /b 0

:up_to_date
echo Build up to date -^> dungeon_crawler.exe
popd
endlocal
exit /b 0

:failed
echo Build FAILED
popd
endlocal
exit /b 1
