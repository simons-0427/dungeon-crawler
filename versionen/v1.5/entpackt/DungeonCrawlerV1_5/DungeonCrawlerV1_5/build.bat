@echo off
REM Build the Dungeon Crawler on Windows (double-click or run from cmd).
setlocal
set PATH=C:\raylib\w64devkit\bin;%PATH%
set RAYLIB=C:\raylib\raylib\src

gcc -O2 -Wall -o dungeon_crawler.exe ^
  main.c globals.c game_items.c advancements.c game_utils.c game_particles.c ^
  game_map.c skins.c game_save.c game_shop.c game_settings.c ^
  game_player.c game_entities.c game_ui.c game_audio.c game_sfx.c ^
  -I"%RAYLIB%" -L"%RAYLIB%" -lraylib -lopengl32 -lgdi32 -lwinmm

if %errorlevel%==0 (echo Build OK -^> dungeon_crawler.exe) else (echo Build FAILED)
endlocal
