#include "text.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FONTS 64
#define MAX_TEXT_CACHE 1024
#define KEY_SIZE 600

typedef struct {
    int weight;
    int px;
    TTF_Font *font;
} FontEntry;

typedef struct {
    char key[KEY_SIZE];
    GLuint texture;
    int width;
    int height;
    unsigned int last_used;
} TextEntry;

struct Fonts {
    char asset_root[1024];
    FontEntry fonts[MAX_FONTS];
    int font_count;
    TextEntry cache[MAX_TEXT_CACHE];
    int cache_count;
    unsigned int frame;
};

static void join_path(char *out, size_t out_size, const char *root,
                      const char *relative) {
    size_t len = strlen(root);
    if (len > 0 && root[len - 1] == '/') {
        snprintf(out, out_size, "%s%s", root, relative);
    } else {
        snprintf(out, out_size, "%s/%s", root, relative);
    }
}

static void utf8_trim_last(char *s) {
    size_t len = strlen(s);
    if (len == 0) return;
    size_t i = len - 1;
    while (i > 0 && (((unsigned char)s[i]) & 0xc0) == 0x80) i--;
    s[i] = '\0';
}

static void fit_text(TTF_Font *font, const char *text, int max_width,
                     char *out, size_t out_size) {
    snprintf(out, out_size, "%s", text);
    if (max_width <= 0) return;

    int width = 0;
    int height = 0;
    if (TTF_SizeUTF8(font, out, &width, &height) == 0 && width <= max_width) {
        return;
    }

    const char *ellipsis = "...";
    int ellipsis_width = 0;
    int ellipsis_height = 0;
    TTF_SizeUTF8(font, ellipsis, &ellipsis_width, &ellipsis_height);

    char trimmed[1024];
    snprintf(trimmed, sizeof(trimmed), "%s", text);
    while (strlen(trimmed) > 0) {
        utf8_trim_last(trimmed);
        if (TTF_SizeUTF8(font, trimmed, &width, &height) == 0 &&
            width + ellipsis_width <= max_width) {
            break;
        }
    }
    snprintf(out, out_size, "%s%s", trimmed, ellipsis);
}

static TTF_Font *get_font(Fonts *fonts, int weight, int px) {
    if (px < 8) px = 8;
    for (int i = 0; i < fonts->font_count; ++i) {
        if (fonts->fonts[i].weight == weight && fonts->fonts[i].px == px) {
            return fonts->fonts[i].font;
        }
    }
    if (fonts->font_count >= MAX_FONTS) return NULL;

    const char *file = weight == 500 ? "fonts/outfit-500.ttf" : "fonts/outfit-300.ttf";
    char path[1200];
    join_path(path, sizeof(path), fonts->asset_root, file);
    TTF_Font *font = TTF_OpenFont(path, px);
    if (!font) {
        fprintf(stderr, "font open failed: %s (%s)\n", path, TTF_GetError());
        return NULL;
    }
    TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
    TTF_SetFontKerning(font, 1);
    FontEntry *entry = &fonts->fonts[fonts->font_count++];
    entry->weight = weight;
    entry->px = px;
    entry->font = font;
    return font;
}

int text_init(Fonts **out, const char *asset_root) {
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        return -1;
    }
    Fonts *fonts = (Fonts *)calloc(1, sizeof(Fonts));
    if (!fonts) {
        TTF_Quit();
        return -1;
    }
    snprintf(fonts->asset_root, sizeof(fonts->asset_root), "%s", asset_root);
    *out = fonts;
    return 0;
}

void text_shutdown(Fonts *fonts) {
    if (!fonts) return;
    for (int i = 0; i < fonts->cache_count; ++i) {
        if (fonts->cache[i].texture) {
            glDeleteTextures(1, &fonts->cache[i].texture);
        }
    }
    for (int i = 0; i < fonts->font_count; ++i) {
        if (fonts->fonts[i].font) TTF_CloseFont(fonts->fonts[i].font);
    }
    free(fonts);
    TTF_Quit();
}

TextTexture text_get(Fonts *fonts, const char *text, int weight, int px,
                     int max_width) {
    TextTexture result = {0, 0, 0};
    if (!fonts || !text || text[0] == '\0') return result;
    TTF_Font *font = get_font(fonts, weight, px);
    if (!font) return result;

    char fitted[1024];
    fit_text(font, text, max_width, fitted, sizeof(fitted));
    if (fitted[0] == '\0') return result;

    char key[KEY_SIZE];
    snprintf(key, sizeof(key), "%d|%d|%d|%s", weight, px, max_width, fitted);

    fonts->frame++;
    for (int i = 0; i < fonts->cache_count; ++i) {
        if (strcmp(fonts->cache[i].key, key) == 0) {
            fonts->cache[i].last_used = fonts->frame;
            result.texture = fonts->cache[i].texture;
            result.width = fonts->cache[i].width;
            result.height = fonts->cache[i].height;
            return result;
        }
    }

    int slot = fonts->cache_count;
    if (slot >= MAX_TEXT_CACHE) {
        slot = 0;
        unsigned int oldest = fonts->cache[0].last_used;
        for (int i = 1; i < MAX_TEXT_CACHE; ++i) {
            if (fonts->cache[i].last_used < oldest) {
                oldest = fonts->cache[i].last_used;
                slot = i;
            }
        }
        if (fonts->cache[slot].texture) {
            glDeleteTextures(1, &fonts->cache[slot].texture);
        }
    } else {
        fonts->cache_count++;
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, fitted, white);
    if (!surface) {
        return result;
    }
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!rgba) {
        return result;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (!texture) {
        SDL_FreeSurface(rgba);
        return result;
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    TextEntry *entry = &fonts->cache[slot];
    snprintf(entry->key, sizeof(entry->key), "%s", key);
    entry->texture = texture;
    entry->width = rgba->w;
    entry->height = rgba->h;
    entry->last_used = fonts->frame;

    result.texture = texture;
    result.width = rgba->w;
    result.height = rgba->h;
    SDL_FreeSurface(rgba);
    return result;
}

void text_measure(Fonts *fonts, const char *text, int weight, int px,
                  int max_width, int *width, int *height) {
    if (width) *width = 0;
    if (height) *height = 0;
    if (!fonts || !text || text[0] == '\0') return;
    TTF_Font *font = get_font(fonts, weight, px);
    if (!font) return;
    char fitted[1024];
    fit_text(font, text, max_width, fitted, sizeof(fitted));
    int w = 0;
    int h = 0;
    if (TTF_SizeUTF8(font, fitted, &w, &h) == 0) {
        if (width) *width = w;
        if (height) *height = h;
    }
}
