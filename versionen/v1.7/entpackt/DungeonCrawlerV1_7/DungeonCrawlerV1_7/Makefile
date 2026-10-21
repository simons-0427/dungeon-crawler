# Dungeon Crawler — multi-file build (w64devkit gcc + raylib).
# Usage:  make           build dungeon_crawler.exe
#         make clean     remove objects + exe
# Ensure C:\raylib\w64devkit\bin is on PATH (for gcc + as + mingw32-make).

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra
RAYLIB  = C:/raylib/raylib/src
INCS    = -I$(RAYLIB)
LIBS    = -mwindows -L$(RAYLIB) -lraylib -lopengl32 -lgdi32 -luser32 -lwinmm -lshell32

SRCS    = main.c globals.c game_items.c advancements.c game_utils.c game_particles.c \
          game_map.c skins.c game_save.c game_shop.c game_settings.c \
          game_player.c game_entities.c game_boss.c game_ui.c game_challenges.c game_audio.c game_sfx.c app_icon_win32.c game_console_win32.c
OBJS    = $(patsubst %.c,obj/%.o,$(SRCS))
RES     = obj/app_icon.o
TARGET  = dungeon_crawler.exe

$(TARGET): $(OBJS) $(RES)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(RES) $(LIBS)

obj/%.o: %.c Makefile | obj
	$(CC) $(CFLAGS) $(INCS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

obj:
	mkdir -p obj

$(RES): app.rc app_icon_resource.h assets/app_icon.ico | obj
	windres app.rc -O coff -o $(RES)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(RES) $(TARGET)

.PHONY: clean
-include $(OBJS:.o=.d)
