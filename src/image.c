#include "image.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IMAGES 512
#define PATH_SIZE 1024

typedef struct {
    char path[PATH_SIZE];
    GLuint texture;
    int width;
    int height;
} ImageEntry;

struct Images {
    ImageEntry entries[MAX_IMAGES];
    int count;
};

int image_init(Images **out) {
    int flags = IMG_INIT_JPG | IMG_INIT_PNG;
    int initialized = IMG_Init(flags);
    if ((initialized & flags) != flags) {
        fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        return -1;
    }
    Images *images = (Images *)calloc(1, sizeof(Images));
    if (!images) {
        IMG_Quit();
        return -1;
    }
    *out = images;
    return 0;
}

void image_shutdown(Images *images) {
    if (!images) return;
    for (int i = 0; i < images->count; ++i) {
        if (images->entries[i].texture) {
            glDeleteTextures(1, &images->entries[i].texture);
        }
    }
    free(images);
    IMG_Quit();
}

GLuint image_get(Images *images, const char *path, int *width, int *height) {
    if (width) *width = 0;
    if (height) *height = 0;
    if (!images || !path || path[0] == '\0') return 0;

    for (int i = 0; i < images->count; ++i) {
        if (strcmp(images->entries[i].path, path) == 0) {
            if (width) *width = images->entries[i].width;
            if (height) *height = images->entries[i].height;
            return images->entries[i].texture;
        }
    }
    if (images->count >= MAX_IMAGES) return 0;

    SDL_Surface *surface = IMG_Load(path);
    if (!surface) {
        fprintf(stderr, "image load failed: %s (%s)\n", path, IMG_GetError());
        return 0;
    }
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return 0;

    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (!texture) {
        SDL_FreeSurface(rgba);
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    ImageEntry *entry = &images->entries[images->count++];
    snprintf(entry->path, sizeof(entry->path), "%s", path);
    entry->texture = texture;
    entry->width = rgba->w;
    entry->height = rgba->h;

    if (width) *width = rgba->w;
    if (height) *height = rgba->h;
    SDL_FreeSurface(rgba);
    return texture;
}
