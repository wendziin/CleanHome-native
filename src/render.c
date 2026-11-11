#include "render.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIRTUAL_W 1920.0f
#define VIRTUAL_H 1080.0f

struct Renderer {
    int width;
    int height;
    int scissor_active;

    GLuint rect_program;
    GLuint image_program;
    GLuint rect_vbo;
    GLuint image_vbo;

    GLint rect_a_pos;
    GLint rect_u_res;
    GLint rect_u_rect;
    GLint rect_u_radius;
    GLint rect_u_color;
    GLint rect_u_color2;
    GLint rect_u_mode;
    GLint rect_u_center;
    GLint rect_u_radial;
    GLint rect_u_border;
    GLint rect_u_border_color;

    GLint image_a_pos;
    GLint image_a_uv;
    GLint image_u_res;
    GLint image_u_rect;
    GLint image_u_radius;
    GLint image_u_tex;
    GLint image_u_tint;
};

static const char *RECT_VERT =
    "precision mediump float;\n"
    "attribute vec2 a_pos;\n"
    "uniform vec2 u_res;\n"
    "uniform vec4 u_rect;\n"
    "varying vec2 v_local;\n"
    "void main() {\n"
    "  vec2 p = u_rect.xy + a_pos * u_rect.zw;\n"
    "  v_local = a_pos * u_rect.zw;\n"
    "  vec2 clip = p / u_res * 2.0 - 1.0;\n"
    "  gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);\n"
    "}\n";

static const char *RECT_FRAG =
    "precision mediump float;\n"
    "uniform vec4 u_color;\n"
    "uniform vec4 u_color2;\n"
    "uniform vec4 u_rect;\n"
    "uniform float u_radius;\n"
    "uniform float u_mode;\n"
    "uniform vec2 u_center;\n"
    "uniform float u_radial;\n"
    "uniform float u_border;\n"
    "uniform vec4 u_border_color;\n"
    "varying vec2 v_local;\n"
    "float sd_round_box(vec2 p, vec2 b, float r) {\n"
    "  vec2 q = abs(p) - b + vec2(r);\n"
    "  return min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r;\n"
    "}\n"
    "void main() {\n"
    "  vec2 half_size = u_rect.zw * 0.5;\n"
    "  float d = sd_round_box(v_local - half_size, half_size, u_radius);\n"
    "  if (d > 0.0) discard;\n"
    "  vec4 fill = u_color;\n"
    "  if (u_mode > 1.5) {\n"
    "    float dist = length(v_local - u_center) / max(u_radial, 1.0);\n"
    "    fill = mix(u_color, u_color2, smoothstep(0.0, 1.0, dist));\n"
    "  } else if (u_mode > 0.5) {\n"
    "    float t = clamp(v_local.y / max(u_rect.w, 1.0), 0.0, 1.0);\n"
    "    fill = mix(u_color, u_color2, t);\n"
    "  }\n"
    "  if (u_border > 0.0) {\n"
    "    float t = smoothstep(-u_border - 1.0, -u_border + 1.0, d);\n"
    "    fill = mix(fill, u_border_color, t);\n"
    "  }\n"
    "  gl_FragColor = fill;\n"
    "}\n";

static const char *IMAGE_VERT =
    "precision mediump float;\n"
    "attribute vec2 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "uniform vec2 u_res;\n"
    "uniform vec4 u_rect;\n"
    "varying vec2 v_local;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "  vec2 p = u_rect.xy + a_pos * u_rect.zw;\n"
    "  v_local = a_pos * u_rect.zw;\n"
    "  v_uv = a_uv;\n"
    "  vec2 clip = p / u_res * 2.0 - 1.0;\n"
    "  gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);\n"
    "}\n";

static const char *IMAGE_FRAG =
    "precision mediump float;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec4 u_rect;\n"
    "uniform float u_radius;\n"
    "uniform vec4 u_tint;\n"
    "varying vec2 v_local;\n"
    "varying vec2 v_uv;\n"
    "float sd_round_box(vec2 p, vec2 b, float r) {\n"
    "  vec2 q = abs(p) - b + vec2(r);\n"
    "  return min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r;\n"
    "}\n"
    "void main() {\n"
    "  vec2 half_size = u_rect.zw * 0.5;\n"
    "  if (sd_round_box(v_local - half_size, half_size, u_radius) > 0.0) discard;\n"
    "  vec4 c = texture2D(u_tex, v_uv) * u_tint;\n"
    "  if (c.a <= 0.001) discard;\n"
    "  gl_FragColor = c;\n"
    "}\n";

Color color_rgba(float r, float g, float b, float a) {
    Color c = {r, g, b, a};
    return c;
}

Color color_hex(uint32_t rgb, float a) {
    Color c;
    c.r = ((rgb >> 16) & 0xff) / 255.0f;
    c.g = ((rgb >> 8) & 0xff) / 255.0f;
    c.b = (rgb & 0xff) / 255.0f;
    c.a = a;
    return c;
}

static GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "shader compile failed: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint link_program(const char *vert_src, const char *frag_src,
                           const char *pos_name, const char *uv_name) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if (!vert || !frag) {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glBindAttribLocation(program, 0, pos_name);
    if (uv_name) glBindAttribLocation(program, 1, uv_name);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        fprintf(stderr, "program link failed: %s\n", log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

int render_init(Renderer **out) {
    Renderer *r = (Renderer *)calloc(1, sizeof(Renderer));
    if (!r) return -1;

    r->rect_program = link_program(RECT_VERT, RECT_FRAG, "a_pos", NULL);
    r->image_program = link_program(IMAGE_VERT, IMAGE_FRAG, "a_pos", "a_uv");
    if (!r->rect_program || !r->image_program) {
        render_shutdown(r);
        return -1;
    }

    r->rect_a_pos = glGetAttribLocation(r->rect_program, "a_pos");
    r->rect_u_res = glGetUniformLocation(r->rect_program, "u_res");
    r->rect_u_rect = glGetUniformLocation(r->rect_program, "u_rect");
    r->rect_u_radius = glGetUniformLocation(r->rect_program, "u_radius");
    r->rect_u_color = glGetUniformLocation(r->rect_program, "u_color");
    r->rect_u_color2 = glGetUniformLocation(r->rect_program, "u_color2");
    r->rect_u_mode = glGetUniformLocation(r->rect_program, "u_mode");
    r->rect_u_center = glGetUniformLocation(r->rect_program, "u_center");
    r->rect_u_radial = glGetUniformLocation(r->rect_program, "u_radial");
    r->rect_u_border = glGetUniformLocation(r->rect_program, "u_border");
    r->rect_u_border_color = glGetUniformLocation(r->rect_program, "u_border_color");

    r->image_a_pos = glGetAttribLocation(r->image_program, "a_pos");
    r->image_a_uv = glGetAttribLocation(r->image_program, "a_uv");
    r->image_u_res = glGetUniformLocation(r->image_program, "u_res");
    r->image_u_rect = glGetUniformLocation(r->image_program, "u_rect");
    r->image_u_radius = glGetUniformLocation(r->image_program, "u_radius");
    r->image_u_tex = glGetUniformLocation(r->image_program, "u_tex");
    r->image_u_tint = glGetUniformLocation(r->image_program, "u_tint");

    glGenBuffers(1, &r->rect_vbo);
    glGenBuffers(1, &r->image_vbo);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    *out = r;
    return 0;
}

void render_shutdown(Renderer *r) {
    if (!r) return;
    if (r->rect_vbo) glDeleteBuffers(1, &r->rect_vbo);
    if (r->image_vbo) glDeleteBuffers(1, &r->image_vbo);
    if (r->rect_program) glDeleteProgram(r->rect_program);
    if (r->image_program) glDeleteProgram(r->image_program);
    free(r);
}

void render_set_size(Renderer *r, int width, int height) {
    if (!r) return;
    r->width = width > 0 ? width : 1;
    r->height = height > 0 ? height : 1;
    glViewport(0, 0, r->width, r->height);
}

void render_begin(Renderer *r) {
    if (!r) return;
    glDisable(GL_SCISSOR_TEST);
    r->scissor_active = 0;
    glViewport(0, 0, r->width, r->height);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void render_end(Renderer *r) {
    (void)r;
}

void render_clear(Renderer *r, Color c) {
    (void)r;
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void draw_rect(Renderer *r, float x, float y, float w, float h,
                      float radius, Color fill, Color fill2, float mode,
                      float center_x, float center_y, float radial,
                      float border, Color border_color) {
    if (!r || w <= 0.0f || h <= 0.0f) return;
    glUseProgram(r->rect_program);
    glUniform2f(r->rect_u_res, VIRTUAL_W, VIRTUAL_H);
    glUniform4f(r->rect_u_rect, x, y, w, h);
    glUniform1f(r->rect_u_radius, radius);
    glUniform4f(r->rect_u_color, fill.r, fill.g, fill.b, fill.a);
    glUniform4f(r->rect_u_color2, fill2.r, fill2.g, fill2.b, fill2.a);
    glUniform1f(r->rect_u_mode, mode);
    glUniform2f(r->rect_u_center, center_x, center_y);
    glUniform1f(r->rect_u_radial, radial);
    glUniform1f(r->rect_u_border, border);
    glUniform4f(r->rect_u_border_color, border_color.r, border_color.g,
                border_color.b, border_color.a);

    static const float verts[12] = {
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
    };
    glBindBuffer(GL_ARRAY_BUFFER, r->rect_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glEnableVertexAttribArray((GLuint)r->rect_a_pos);
    glVertexAttribPointer((GLuint)r->rect_a_pos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render_rect(Renderer *r, float x, float y, float w, float h, Color c) {
    draw_rect(r, x, y, w, h, 0.0f, c, c, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c);
}

void render_rounded_rect(Renderer *r, float x, float y, float w, float h,
                         float radius, Color c) {
    draw_rect(r, x, y, w, h, radius, c, c, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c);
}

void render_rounded_rect_border(Renderer *r, float x, float y, float w, float h,
                                float radius, float border, Color fill,
                                Color border_color) {
    draw_rect(r, x, y, w, h, radius, fill, fill, 0.0f, 0.0f, 0.0f, 0.0f,
              border, border_color);
}

void render_gradient_rect(Renderer *r, float x, float y, float w, float h,
                          float radius, Color top, Color bottom) {
    draw_rect(r, x, y, w, h, radius, top, bottom, 1.0f, 0.0f, 0.0f, 0.0f,
              0.0f, top);
}

void render_radial_rect(Renderer *r, float x, float y, float w, float h,
                        float radius, float cx, float cy, float radial_radius,
                        Color inner, Color outer) {
    draw_rect(r, x, y, w, h, radius, inner, outer, 2.0f, cx, cy, radial_radius,
              0.0f, inner);
}

static void draw_image_uv(Renderer *r, GLuint texture, float x, float y,
                          float w, float h, float radius, Color tint,
                          float u0, float v0, float u1, float v1) {
    if (!r || !texture || w <= 0.0f || h <= 0.0f) return;
    glUseProgram(r->image_program);
    glUniform2f(r->image_u_res, VIRTUAL_W, VIRTUAL_H);
    glUniform4f(r->image_u_rect, x, y, w, h);
    glUniform1f(r->image_u_radius, radius);
    glUniform4f(r->image_u_tint, tint.r, tint.g, tint.b, tint.a);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(r->image_u_tex, 0);

    float verts[24] = {
        0.0f, 0.0f, u0, v0,
        1.0f, 0.0f, u1, v0,
        0.0f, 1.0f, u0, v1,
        1.0f, 0.0f, u1, v0,
        1.0f, 1.0f, u1, v1,
        0.0f, 1.0f, u0, v1
    };
    glBindBuffer(GL_ARRAY_BUFFER, r->image_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glEnableVertexAttribArray((GLuint)r->image_a_pos);
    glVertexAttribPointer((GLuint)r->image_a_pos, 2, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 4, 0);
    glEnableVertexAttribArray((GLuint)r->image_a_uv);
    glVertexAttribPointer((GLuint)r->image_a_uv, 2, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 4, (void *)(sizeof(float) * 2));
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render_image(Renderer *r, GLuint texture, float x, float y, float w,
                  float h, float radius, Color tint) {
    draw_image_uv(r, texture, x, y, w, h, radius, tint, 0.0f, 0.0f, 1.0f, 1.0f);
}

void render_image_cover(Renderer *r, GLuint texture, int tex_w, int tex_h,
                        float x, float y, float w, float h) {
    if (!texture || tex_w <= 0 || tex_h <= 0 || w <= 0.0f || h <= 0.0f) return;
    float tex_aspect = (float)tex_w / (float)tex_h;
    float dst_aspect = w / h;
    float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
    if (tex_aspect > dst_aspect) {
        float crop = dst_aspect / tex_aspect;
        u0 = (1.0f - crop) * 0.5f;
        u1 = u0 + crop;
    } else {
        float crop = tex_aspect / dst_aspect;
        v0 = (1.0f - crop) * 0.5f;
        v1 = v0 + crop;
    }
    draw_image_uv(r, texture, x, y, w, h, 0.0f, color_rgba(1, 1, 1, 1),
                  u0, v0, u1, v1);
}

void render_scissor(Renderer *r, float x, float y, float w, float h) {
    if (!r) return;
    int sx = (int)(x * (float)r->width / VIRTUAL_W);
    int sy = (int)(y * (float)r->height / VIRTUAL_H);
    int sw = (int)(w * (float)r->width / VIRTUAL_W);
    int sh = (int)(h * (float)r->height / VIRTUAL_H);
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sw < 0) sw = 0;
    if (sh < 0) sh = 0;
    if (sx + sw > r->width) sw = r->width - sx;
    if (sy + sh > r->height) sh = r->height - sy;
    glEnable(GL_SCISSOR_TEST);
    glScissor(sx, r->height - (sy + sh), sw, sh);
    r->scissor_active = 1;
}

void render_scissor_disable(Renderer *r) {
    if (!r) return;
    glDisable(GL_SCISSOR_TEST);
    r->scissor_active = 0;
}
