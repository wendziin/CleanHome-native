#ifndef CLEANHOME_RENDER_H
#define CLEANHOME_RENDER_H

#include <stdint.h>
#include "gles.h"

typedef struct {
    float r, g, b, a;
} Color;

typedef struct Renderer Renderer;

Color color_rgba(float r, float g, float b, float a);
Color color_hex(uint32_t rgb, float a);

int render_init(Renderer **out);
void render_shutdown(Renderer *r);
void render_set_size(Renderer *r, int width, int height);
void render_begin(Renderer *r);
void render_end(Renderer *r);
void render_clear(Renderer *r, Color c);
void render_rect(Renderer *r, float x, float y, float w, float h, Color c);
void render_rounded_rect(Renderer *r, float x, float y, float w, float h,
                         float radius, Color c);
void render_rounded_rect_border(Renderer *r, float x, float y, float w, float h,
                                float radius, float border, Color fill,
                                Color border_color);
void render_gradient_rect(Renderer *r, float x, float y, float w, float h,
                          float radius, Color top, Color bottom);
void render_radial_rect(Renderer *r, float x, float y, float w, float h,
                        float radius, float cx, float cy, float radial_radius,
                        Color inner, Color outer);
void render_image(Renderer *r, GLuint texture, float x, float y, float w, float h,
                  float radius, Color tint);
void render_image_cover(Renderer *r, GLuint texture, int tex_w, int tex_h,
                        float x, float y, float w, float h);
void render_scissor(Renderer *r, float x, float y, float w, float h);
void render_scissor_disable(Renderer *r);

#endif
