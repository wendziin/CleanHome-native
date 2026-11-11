#ifndef CLEANHOME_TEXT_H
#define CLEANHOME_TEXT_H

#include "render.h"

typedef struct Fonts Fonts;

typedef struct {
    GLuint texture;
    int width;
    int height;
} TextTexture;

int text_init(Fonts **out, const char *asset_root);
void text_shutdown(Fonts *fonts);
TextTexture text_get(Fonts *fonts, const char *text, int weight, int px,
                     int max_width);
void text_measure(Fonts *fonts, const char *text, int weight, int px,
                  int max_width, int *width, int *height);

#endif
