CC ?= cc
PKG_CONFIG ?= pkg-config

CFLAGS ?= -O2 -g
CFLAGS += -std=c99 -Wall -Wextra -Wno-unused-parameter -Isrc

SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 SDL2_image SDL2_ttf)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 SDL2_image SDL2_ttf) -lGLESv2 -lm

SRC := \
	src/main_desktop.c \
	src/platform_desktop.c \
	src/render.c \
	src/text.c \
	src/image.c \
	src/launcher.c \
	src/config.c

OBJ := $(SRC:.c=.o)

.PHONY: desktop clean tizen-assets tpk

desktop: cleanhome-desktop

cleanhome-desktop: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(SDL_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) cleanhome-desktop

tpk:
	bash tools/tpk.sh

tizen-assets:
	rm -rf res/assets
	mkdir -p res/assets shared/res
	cp -a assets/. res/assets/
	cp -f tizen/shared/res/icon.png shared/res/icon.png
