@echo off
REM Build the Dungeon Crawler on Windows (double-click or run from cmd).
setlocal
cd /d "%~dp0"
set PATH=C:\raylib\w64devkit\bin;%PATH%
set RAYLIB=C:\raylib\raylib\src

windres app.rc -O coff -o app_icon.o
if %errorlevel% neq 0 (echo windres FAILED & endlocal & exit /b 1)

gcc -O2 -Wall -DCHALLENGES_ONLY -ffunction-sections -fdata-sections -o dungeon_crawler_challenges.exe ^
  main.c globals.c game_items.c advancements.c game_utils.c game_particles.c ^
  game_map.c skins.c game_save.c game_shop.c game_settings.c ^
  game_player.c game_entities.c game_boss.c game_ui.c game_challenges.c game_audio.c game_sfx.c ^
  app_icon.o ^
  -I"%RAYLIB%" -L"%RAYLIB%" -Wl,--gc-sections -lraylib -lopengl32 -lgdi32 -lwinmm

if %errorlevel% neq 0 (echo Build FAILED & endlocal & exit /b 1)
echo Build OK -^> dungeon_crawler_challenges.exe
endlocal
