#!/usr/bin/env bash
# Build the Dungeon Crawler (Git Bash / MSYS, using the bundled w64devkit gcc).
set -e
case "$0" in */*) cd "${0%/*}" ;; esac

# w64devkit gcc + assembler must be on PATH.
export PATH="/c/raylib/w64devkit/bin:$PATH"

# raylib headers (raylib.h, raymath.h) and libraylib.a both live here.
RAYLIB_SRC="/c/raylib/raylib/src"

mkdir -p obj
windres app.rc -O coff -o obj/app_icon.o
gcc -O2 -Wall -Wextra -o dungeon_crawler.exe \
    main.c globals.c game_items.c advancements.c game_utils.c game_particles.c \
    game_map.c skins.c game_save.c game_shop.c game_settings.c \
    game_player.c game_entities.c game_boss.c game_ui.c game_challenges.c game_audio.c game_sfx.c app_icon_win32.c game_console_win32.c \
    obj/app_icon.o \
    -I"$RAYLIB_SRC" -L"$RAYLIB_SRC" \
    -mwindows -lraylib -lopengl32 -lgdi32 -luser32 -lwinmm -lshell32

echo "Build OK -> dungeon_crawler.exe"
