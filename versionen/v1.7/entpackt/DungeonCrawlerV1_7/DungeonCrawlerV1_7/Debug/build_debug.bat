@echo off
REM Build the Dungeon Crawler debug executable on Windows.
REM Debug controls: F9 cycles room types + Coin Rush; arrow keys change rounds; move with WASD.
setlocal
pushd "%~dp0.." || (endlocal & exit /b 1)
set PATH=C:\raylib\w64devkit\bin;%PATH%
set RAYLIB=C:\raylib\raylib\src
if not exist Debug\obj mkdir Debug\obj

windres app.rc -O coff -o Debug\obj\app_icon.o
if errorlevel 1 goto failed

gcc -O0 -g3 -fno-omit-frame-pointer -Wall -Wextra -DDEBUG_ROOMS ^
  -o Debug\dungeon_crawler_debug_build.exe ^
  main.c globals.c game_items.c advancements.c game_utils.c game_particles.c ^
  game_map.c skins.c game_save.c game_shop.c game_settings.c ^
  game_player.c game_entities.c game_boss.c game_ui.c game_challenges.c game_audio.c game_sfx.c ^
  app_icon_win32.c game_console_win32.c Debug\obj\app_icon.o ^
  -mwindows -I"%RAYLIB%" -L"%RAYLIB%" -lraylib -lopengl32 -lgdi32 -luser32 -lwinmm -lshell32

if errorlevel 1 goto failed

REM Keep source-level debug information in a sidecar instead of the playable EXE.
objcopy --only-keep-debug Debug\dungeon_crawler_debug_build.exe Debug\dungeon_crawler_debug.dbg
if errorlevel 1 goto failed
strip --strip-debug Debug\dungeon_crawler_debug_build.exe
if errorlevel 1 goto failed
objcopy --add-gnu-debuglink=Debug\dungeon_crawler_debug.dbg Debug\dungeon_crawler_debug_build.exe
if errorlevel 1 goto failed

copy /Y Debug\dungeon_crawler_debug_build.exe Debug\dungeon_crawler_debug.exe >nul
if errorlevel 1 (
  echo Debug build OK -^> Debug\dungeon_crawler_debug_build.exe
  echo The running Debug\dungeon_crawler_debug.exe is locked. Close it before replacing it.
) else (
  powershell -NoProfile -ExecutionPolicy Bypass -File "refresh_icon_cache.ps1" "Debug\dungeon_crawler_debug.exe"
  del /Q Debug\dungeon_crawler_debug_build.exe
  echo Debug build OK -^> Debug\dungeon_crawler_debug.exe
)
popd
endlocal
exit /b 0

:failed
echo Debug build FAILED
popd
endlocal
exit /b 1
