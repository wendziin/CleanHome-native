#ifndef CLEANHOME_LAUNCHER_H
#define CLEANHOME_LAUNCHER_H

#include "image.h"
#include "render.h"
#include "text.h"

typedef enum {
    CH_KEY_LEFT,
    CH_KEY_RIGHT,
    CH_KEY_UP,
    CH_KEY_DOWN,
    CH_KEY_OK,
    CH_KEY_BACK,
    CH_KEY_MEDIA_PLAY,
    CH_KEY_MEDIA_NEXT,
    CH_KEY_MEDIA_PREV
} LauncherKey;

typedef struct Launcher Launcher;

int launcher_init(Launcher **out, Renderer *renderer, Fonts *fonts,
                  Images *images);
void launcher_shutdown(Launcher *launcher);
void launcher_tick(Launcher *launcher);
void launcher_render(Launcher *launcher);
void launcher_key(Launcher *launcher, LauncherKey key, int down, int repeat);

#endif
