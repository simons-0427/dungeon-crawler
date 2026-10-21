# Dungeon Crawler — multi-file build (w64devkit gcc + raylib).
# Usage:  make           build dungeon_crawler.exe
#         make clean     remove objects + exe
# Ensure C:\raylib\w64devkit\bin is on PATH (for gcc + as + mingw32-make).

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra
RAYLIB  = C:/raylib/raylib/src
INCS    = -I$(RAYLIB)
LIBS    = -L$(RAYLIB) -lraylib -lopengl32 -lgdi32 -lwinmm

SRCS    = main.c globals.c game_items.c advancements.c game_utils.c game_particles.c \
          game_map.c skins.c game_save.c game_shop.c game_settings.c \
          game_player.c game_entities.c game_boss.c game_ui.c game_audio.c game_sfx.c
OBJS    = $(SRCS:.c=.o)
TARGET  = dungeon_crawler.exe

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
