#include "launcher.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "app_ids.h"
#include "config.h"
#include "platform.h"

#define MAX_APPS 512
#define MAX_DOCK 9
#define MAX_GRID MAX_APPS

#define OVERLAY_NONE 0
#define OVERLAY_CONTROLS 1
#define OVERLAY_WALLPAPER 2

#define SCREEN_W 1920.0f
#define SCREEN_H 1080.0f

typedef struct {
    char id[256];
    char name[128];
    char icon[1024];
} App;

typedef struct {
    const char *id;
    const char *name;
    const char *file;
    const char *thumb;
} Wallpaper;

typedef struct {
    const char *key;
    const char *icon;
    uint32_t color;
    int card;
    int dark;
    int letter;
} Brand;

typedef enum {
    CTRL_TV,
    CTRL_HDMI1,
    CTRL_HDMI2,
    CTRL_HDMI3,
    CTRL_HDMI4,
    CTRL_AMBIENT,
    CTRL_WIFI,
    CTRL_SETTINGS,
    CTRL_THEME,
    CTRL_FAMILY,
    CTRL_DIAG,
    CTRL_HOME
} ControlKind;

typedef struct {
    const char *label;
    const char *icon;
    ControlKind kind;
} Control;

static const Wallpaper WALLPAPERS[] = {
    {"off", "Desativado (Tema Puro)", NULL, NULL},
    {"espaco-profundo", "Espaço Profundo", "wallpapers/espaco-profundo.jpg", "wallpapers/thumbs/espaco-profundo.jpg"},
    {"aurora-boreal", "Aurora Boreal", "wallpapers/aurora-boreal.jpg", "wallpapers/thumbs/aurora-boreal.jpg"},
    {"cidade-cyberpunk", "Cidade Cyberpunk", "wallpapers/cidade-cyberpunk.jpg", "wallpapers/thumbs/cidade-cyberpunk.jpg"},
    {"floresta-neblina", "Floresta Neblina", "wallpapers/floresta-neblina.jpg", "wallpapers/thumbs/floresta-neblina.jpg"},
    {"montanhas-sunset", "Montanhas Sunset", "wallpapers/montanhas-sunset.jpg", "wallpapers/thumbs/montanhas-sunset.jpg"},
    {"ondas-neon", "Ondas Neon", "wallpapers/ondas-neon.jpg", "wallpapers/thumbs/ondas-neon.jpg"},
    {"ceu-estrelado", "Céu Estrelado", "wallpapers/ceu-estrelado.jpg", "wallpapers/thumbs/ceu-estrelado.jpg"},
    {"deserto-minimalista", "Deserto Minimalista", "wallpapers/deserto-minimalista.jpg", "wallpapers/thumbs/deserto-minimalista.jpg"},
};
#define WALLPAPER_COUNT ((int)(sizeof(WALLPAPERS) / sizeof(WALLPAPERS[0])))

static const char *THEME_IDS[] = {"obsidian", "aurora", "six"};
static const char *THEME_NAMES[] = {"Obsidiana", "Aurora", "Seis Colunas"};
#define THEME_COUNT ((int)(sizeof(THEME_IDS) / sizeof(THEME_IDS[0])))

static const char *UTIL_LABELS[] = {
    "Todos os apps", "Atualizar", "Editar Início", "Loja de Apps",
    "Planos de Fundo", NULL
};
#define UTIL_COUNT 6
#define UTIL_GEAR_INDEX 5

static const Control CONTROLS[] = {
    {"TV / Antena", "icons/tv.png", CTRL_TV},
    {"HDMI 1", "icons/hdmi.png", CTRL_HDMI1},
    {"HDMI 2", "icons/hdmi.png", CTRL_HDMI2},
    {"HDMI 3", "icons/hdmi.png", CTRL_HDMI3},
    {"HDMI 4", "icons/hdmi.png", CTRL_HDMI4},
    {"Modo Ambiente", "icons/ambient.png", CTRL_AMBIENT},
    {"Wi-Fi", "icons/wifi-on.png", CTRL_WIFI},
    {"Configurações da TV", "icons/gear.png", CTRL_SETTINGS},
    {"Tema da Interface", "icons/theme.png", CTRL_THEME},
    {"Troca Automática", "icons/theme.png", CTRL_FAMILY},
    {"Diagnóstico", "icons/diag.png", CTRL_DIAG},
    {"Smart Hub Original", "icons/home.png", CTRL_HOME},
};
#define CONTROL_COUNT ((int)(sizeof(CONTROLS) / sizeof(CONTROLS[0])))

static const Brand BRANDS[] = {
    {"netflix", "icons/netflix.png", 0xE50914, 0, 0, 0},
    {"youtube", "icons/youtube-card.png", 0xFF0000, 1, 0, 0},
    {"spotify", "icons/spotify.png", 0x1DB954, 0, 0, 0},
    {"plex", "icons/plex.png", 0xE5A00D, 0, 0, 0},
    {"apple tv", "icons/appletv.png", 0xFFFFFF, 0, 1, 0},
    {"prime video", "icons/primevideo-card.png", 0x00A8E1, 1, 0, 0},
    {"dstv", "icons/dstv.png", 0x00A8E1, 0, 0, 0},
    {"disney", NULL, 0x6C8CFF, 0, 0, 1},
    {"tv ao vivo", "icons/tv.png", 0xFFFFFF, 0, 1, 0},
    {"configura", "icons/gear.png", 0xFFFFFF, 0, 1, 0},
};
#define BRAND_COUNT ((int)(sizeof(BRANDS) / sizeof(BRANDS[0])))

struct Launcher {
    Renderer *renderer;
    Fonts *fonts;
    Images *images;

    Config config;
    char config_path[1200];
    char asset_root[1024];

    App apps[MAX_APPS];
    int app_count;
    int dock[MAX_APPS];
    int dock_count;
    int grid[MAX_APPS];
    int grid_count;

    int view_grid;
    int grid_editing;
    int zone_utils;
    int overlay;
    int dock_index;
    int util_index;
    int grid_index;
    int controls_index;
    int wallpaper_index;

    int theme;
    int columns;
    int wallpaper;

    int reordering;
    int reorder_original[MAX_APPS];
    int reorder_original_count;

    int suppress_activate;
    int ok_held;
    int hold_fired;
    uint32_t ok_down_ms;

    uint32_t toast_until;
    char toast[256];

    PlatformStatus status;
    uint32_t last_status_ms;
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

static Color white(float a) {
    return color_rgba(1.0f, 1.0f, 1.0f, a);
}

static Color black(float a) {
    return color_rgba(0.0f, 0.0f, 0.0f, a);
}

static Color transparent(void) {
    return color_rgba(0.0f, 0.0f, 0.0f, 0.0f);
}

static char *trim_inplace(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' ||
                       end[-1] == '\r' || end[-1] == '\n')) {
        end--;
    }
    *end = '\0';
    return s;
}

static int contains_ci(const char *haystack, const char *needle) {
    if (!haystack || !needle || !needle[0]) return 0;
    size_t n = strlen(needle);
    for (const char *p = haystack; *p; ++p) {
        size_t i = 0;
        while (i < n && p[i] &&
               tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) {
            i++;
        }
        if (i == n) return 1;
    }
    return 0;
}

static int is_blocked(const App *app) {
    return contains_ci(app->name, "emanual") ||
           contains_ci(app->name, "e-manual") ||
           contains_ci(app->id, "emanual");
}

static int app_name_cmp(const void *a, const void *b) {
    const App *aa = (const App *)a;
    const App *bb = (const App *)b;
    return strcasecmp(aa->name, bb->name);
}

static int find_app_index(const Launcher *l, const char *key) {
    if (!key || !key[0]) return -1;
    for (int i = 0; i < l->app_count; ++i) {
        if (strcmp(l->apps[i].id, key) == 0) return i;
    }
    for (int i = 0; i < l->app_count; ++i) {
        if (strcasecmp(l->apps[i].name, key) == 0) return i;
    }
    for (int i = 0; i < l->app_count; ++i) {
        if (contains_ci(l->apps[i].name, key)) return i;
    }
    return -1;
}

static int dock_contains(const Launcher *l, int app_index) {
    for (int i = 0; i < l->dock_count; ++i) {
        if (l->dock[i] == app_index) return 1;
    }
    return 0;
}

static int add_to_dock(Launcher *l, int app_index) {
    if (app_index < 0 || app_index >= l->app_count) return 0;
    if (dock_contains(l, app_index)) return 0;
    if (l->dock_count >= MAX_DOCK) return 0;
    l->dock[l->dock_count++] = app_index;
    return 1;
}

static void remove_from_dock(Launcher *l, int app_index) {
    for (int i = 0; i < l->dock_count; ++i) {
        if (l->dock[i] == app_index) {
            memmove(&l->dock[i], &l->dock[i + 1],
                    sizeof(l->dock[0]) * (size_t)(l->dock_count - i - 1));
            l->dock_count--;
            return;
        }
    }
}

static void build_grid(Launcher *l) {
    l->grid_count = 0;
    for (int i = 0; i < l->app_count && l->grid_count < MAX_GRID; ++i) {
        if (!is_blocked(&l->apps[i])) {
            l->grid[l->grid_count++] = i;
        }
    }
}

static void default_dock(Launcher *l) {
    static const char *ids[] = {
        "com.samsung.tv.coba.netflix",
        "com.samsung.tv.coba.youtube",
        "com.samsung.tv.coba.spotify",
        "com.samsung.tv.coba.plex",
        "com.samsung.tv.coba.appletv",
        "com.samsung.tv.coba.primevideo",
        "com.samsung.tv.coba.disneyplus",
        "com.samsung.tv.coba.dstv",
        NULL
    };
    static const char *names[] = {
        "Netflix", "YouTube", "Spotify", "Plex", "Apple TV",
        "Prime Video", "Disney+", "DStv", NULL
    };
    l->dock_count = 0;
    for (int i = 0; ids[i] && l->dock_count < 8; ++i) {
        int index = find_app_index(l, ids[i]);
        if (index < 0) index = find_app_index(l, names[i]);
        if (index >= 0 && !is_blocked(&l->apps[index])) {
            add_to_dock(l, index);
        }
    }
    for (int i = 0; i < l->app_count && l->dock_count < 8; ++i) {
        if (strcmp(l->apps[i].id, SETTINGS_APP_ID) == 0) continue;
        if (strcmp(l->apps[i].id, LIVETV_APP_ID) == 0) continue;
        if (is_blocked(&l->apps[i])) continue;
        add_to_dock(l, i);
    }
    l->dock_index = 0;
}

static void dock_from_config(Launcher *l) {
    l->dock_count = 0;
    char buffer[2048];
    snprintf(buffer, sizeof(buffer), "%s", l->config.dock);
    char *save = NULL;
    for (char *token = strtok_r(buffer, ",", &save); token;
         token = strtok_r(NULL, ",", &save)) {
        char *key = trim_inplace(token);
        if (!key[0]) continue;
        int index = find_app_index(l, key);
        if (index >= 0 && !is_blocked(&l->apps[index])) {
            add_to_dock(l, index);
        }
    }
}

static void save_config(Launcher *l) {
    snprintf(l->config.wallpaper, sizeof(l->config.wallpaper), "%s",
             WALLPAPERS[l->wallpaper].id);
    snprintf(l->config.theme, sizeof(l->config.theme), "%s",
             THEME_IDS[l->theme]);
    l->config.dock[0] = '\0';
    size_t offset = 0;
    for (int i = 0; i < l->dock_count; ++i) {
        const char *id = l->apps[l->dock[i]].id;
        int written = snprintf(l->config.dock + offset,
                               sizeof(l->config.dock) - offset, "%s%s",
                               i > 0 ? "," : "", id);
        if (written < 0 || (size_t)written >= sizeof(l->config.dock) - offset) {
            break;
        }
        offset += (size_t)written;
    }
    l->config.seeded = 1;
    config_save(&l->config, l->config_path);
}

static int theme_from_id(const char *id) {
    for (int i = 0; i < THEME_COUNT; ++i) {
        if (strcmp(id, THEME_IDS[i]) == 0) return i;
    }
    return 0;
}

static int wallpaper_from_id(const char *id) {
    for (int i = 0; i < WALLPAPER_COUNT; ++i) {
        if (strcmp(id, WALLPAPERS[i].id) == 0) return i;
    }
    return 1;
}

static void set_theme(Launcher *l, int theme) {
    if (theme < 0) theme = THEME_COUNT - 1;
    if (theme >= THEME_COUNT) theme = 0;
    l->theme = theme;
    l->columns = theme == 2 ? 6 : 5;
    if (l->grid_count > 0 && l->grid_index >= l->grid_count) {
        l->grid_index = l->grid_count - 1;
    }
}

static void toast(Launcher *l, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(l->toast, sizeof(l->toast), fmt, args);
    va_end(args);
    l->toast_until = platform_ticks_ms() + 2400;
}

static const Brand *find_brand(const App *app) {
    if (strcmp(app->id, LIVETV_APP_ID) == 0) return &BRANDS[8];
    if (strcmp(app->id, SETTINGS_APP_ID) == 0) return &BRANDS[9];
    for (int i = 0; i < BRAND_COUNT; ++i) {
        if (contains_ci(app->id, BRANDS[i].key) ||
            contains_ci(app->name, BRANDS[i].key)) {
            return &BRANDS[i];
        }
    }
    return NULL;
}

static int app_is_dark(const App *app) {
    const Brand *brand = find_brand(app);
    return brand && brand->dark;
}

static int app_is_card(const App *app) {
    const Brand *brand = find_brand(app);
    return brand && brand->card;
}

static GLuint icon_texture(Launcher *l, const char *relative, int *width,
                           int *height) {
    if (!relative || !relative[0]) return 0;
    char path[1200];
    join_path(path, sizeof(path), l->asset_root, relative);
    return image_get(l->images, path, width, height);
}

static GLuint app_icon_texture(Launcher *l, const App *app, int *width,
                               int *height) {
    if (app->icon[0]) {
        return image_get(l->images, app->icon, width, height);
    }
    const Brand *brand = find_brand(app);
    if (brand && brand->icon) {
        return icon_texture(l, brand->icon, width, height);
    }
    return 0;
}

static GLuint wallpaper_texture(Launcher *l, int index, int thumb, int *width,
                                int *height) {
    if (index < 0 || index >= WALLPAPER_COUNT) return 0;
    const Wallpaper *wallpaper = &WALLPAPERS[index];
    const char *relative = thumb ? wallpaper->thumb : wallpaper->file;
    if (!relative) return 0;
    return icon_texture(l, relative, width, height);
}

static void draw_text(Launcher *l, const char *text, int weight, int px,
                      float x, float y, Color color) {
    if (!text || !text[0]) return;
    TextTexture tex = text_get(l->fonts, text, weight, px, 0);
    if (tex.texture) {
        render_image(l->renderer, tex.texture, x, y, (float)tex.width,
                     (float)tex.height, 0.0f, color);
    }
}

static void draw_text_fit(Launcher *l, const char *text, int weight, int px,
                          float x, float y, int max_width, Color color) {
    if (!text || !text[0]) return;
    TextTexture tex = text_get(l->fonts, text, weight, px, max_width);
    if (tex.texture) {
        render_image(l->renderer, tex.texture, x, y, (float)tex.width,
                     (float)tex.height, 0.0f, color);
    }
}

static void draw_text_centered(Launcher *l, const char *text, int weight,
                               int px, float center_x, float y, int max_width,
                               Color color) {
    if (!text || !text[0]) return;
    TextTexture tex = text_get(l->fonts, text, weight, px, max_width);
    if (tex.texture) {
        render_image(l->renderer, tex.texture,
                     center_x - (float)tex.width * 0.5f, y,
                     (float)tex.width, (float)tex.height, 0.0f, color);
    }
}

static void draw_text_right(Launcher *l, const char *text, int weight, int px,
                            float right, float y, int max_width, Color color) {
    if (!text || !text[0]) return;
    TextTexture tex = text_get(l->fonts, text, weight, px, max_width);
    if (tex.texture) {
        render_image(l->renderer, tex.texture, right - (float)tex.width, y,
                     (float)tex.width, (float)tex.height, 0.0f, color);
    }
}

static void format_date_pt(const struct tm *tm, char *out, size_t out_size) {
    static const char *weekdays[] = {
        "domingo", "segunda-feira", "terça-feira", "quarta-feira",
        "quinta-feira", "sexta-feira", "sábado"
    };
    static const char *months[] = {
        "janeiro", "fevereiro", "março", "abril", "maio", "junho",
        "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"
    };
    int weekday = tm->tm_wday;
    int month = tm->tm_mon;
    if (weekday < 0 || weekday > 6) weekday = 0;
    if (month < 0 || month > 11) month = 0;
    snprintf(out, out_size, "%s, %d de %s", weekdays[weekday], tm->tm_mday,
             months[month]);
}

static void render_background(Launcher *l) {
    const Wallpaper *wallpaper = &WALLPAPERS[l->wallpaper];
    if (wallpaper->file) {
        int width = 0;
        int height = 0;
        GLuint texture = wallpaper_texture(l, l->wallpaper, 0, &width, &height);
        if (texture) {
            render_image_cover(l->renderer, texture, width, height, 0.0f, 0.0f,
                               SCREEN_W, SCREEN_H);
        } else {
            render_rect(l->renderer, 0.0f, 0.0f, SCREEN_W, SCREEN_H, black(1));
        }
        render_gradient_rect(l->renderer, 0.0f, 0.0f, SCREEN_W, SCREEN_H * 0.42f,
                             0.0f, black(0.38f), black(0.04f));
        render_gradient_rect(l->renderer, 0.0f, SCREEN_H * 0.42f, SCREEN_W,
                             SCREEN_H * 0.58f, 0.0f, black(0.04f),
                             black(0.66f));
        return;
    }

    if (l->theme == 1) {
        render_rect(l->renderer, 0.0f, 0.0f, SCREEN_W, SCREEN_H,
                    color_hex(0x0B1220, 1.0f));
        render_radial_rect(l->renderer, 0.0f, 0.0f, SCREEN_W, SCREEN_H, 0.0f,
                           SCREEN_W * 0.20f, SCREEN_H * 0.08f, 900.0f,
                           color_hex(0x2DD4BF, 0.22f),
                           color_hex(0x2DD4BF, 0.0f));
        render_radial_rect(l->renderer, 0.0f, 0.0f, SCREEN_W, SCREEN_H, 0.0f,
                           SCREEN_W * 0.82f, SCREEN_H * 0.04f, 900.0f,
                           color_hex(0x8B5CF6, 0.26f),
                           color_hex(0x8B5CF6, 0.0f));
    } else {
        render_rect(l->renderer, 0.0f, 0.0f, SCREEN_W, SCREEN_H, black(1));
    }
}

static void render_clock(Launcher *l) {
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);

    const char *greeting = "BOA NOITE";
    if (tm.tm_hour < 12) greeting = "BOM DIA";
    else if (tm.tm_hour < 18) greeting = "BOA TARDE";

    char clock_text[16];
    strftime(clock_text, sizeof(clock_text), "%H:%M", &tm);
    char date_text[128];
    format_date_pt(&tm, date_text, sizeof(date_text));

    draw_text(l, greeting, 500, 20, 96.0f, 64.0f, white(0.42f));
    draw_text(l, clock_text, 300, 52, 96.0f, 102.0f, white(0.95f));
    draw_text(l, date_text, 300, 24, 96.0f, 168.0f, white(0.55f));
}

static void render_utils(Launcher *l) {
    float widths[UTIL_COUNT];
    float total = 0.0f;
    for (int i = 0; i < UTIL_COUNT; ++i) {
        if (i == UTIL_GEAR_INDEX) {
            widths[i] = 78.0f;
        } else {
            int width = 0;
            int height = 0;
            text_measure(l->fonts, UTIL_LABELS[i], 500, 24, 0, &width, &height);
            widths[i] = (float)width + 52.0f;
        }
        total += widths[i];
    }
    total += 14.0f * (UTIL_COUNT - 1);

    float x = SCREEN_W - 96.0f - total;
    float y = 64.0f;
    for (int i = 0; i < UTIL_COUNT; ++i) {
        int focused = !l->zone_utils ? 0 : (l->util_index == i);
        if (focused) {
            render_rounded_rect_border(l->renderer, x, y, widths[i], 64.0f,
                                       32.0f, 1.0f, white(0.30f),
                                       white(0.36f));
        }
        if (i == UTIL_GEAR_INDEX) {
            int width = 0;
            int height = 0;
            GLuint texture = icon_texture(l, "icons/gear.png", &width, &height);
            if (texture) {
                float size = 28.0f;
                render_image(l->renderer, texture,
                             x + (widths[i] - size) * 0.5f,
                             y + (64.0f - size) * 0.5f, size, size, 0.0f,
                             focused ? white(0.95f) : white(0.82f));
            }
        } else {
            int width = 0;
            int height = 0;
            text_measure(l->fonts, UTIL_LABELS[i], 500, 24, 0, &width, &height);
            draw_text(l, UTIL_LABELS[i], 500, 24, x + 26.0f,
                      y + (64.0f - (float)height) * 0.5f,
                      focused ? white(0.95f) : white(0.78f));
        }
        x += widths[i] + 14.0f;
    }
}

static const char *shelf_title(Launcher *l, char *buffer, size_t size) {
    if (l->overlay == OVERLAY_WALLPAPER) {
        snprintf(buffer, size, "Planos de Fundo");
        return buffer;
    }
    if (l->overlay == OVERLAY_CONTROLS) {
        snprintf(buffer, size, "%s", CONTROLS[l->controls_index].label);
        return buffer;
    }
    if (l->view_grid) {
        if (l->grid_count == 0) snprintf(buffer, size, "Nenhum aplicativo encontrado");
        else snprintf(buffer, size, "%s", l->apps[l->grid[l->grid_index]].name);
        return buffer;
    }
    if (l->zone_utils) {
        if (l->util_index == UTIL_GEAR_INDEX) snprintf(buffer, size, "Ajustes rápidos");
        else snprintf(buffer, size, "%s", UTIL_LABELS[l->util_index]);
        return buffer;
    }
    if (l->dock_count == 0) {
        snprintf(buffer, size, "Nenhum app no Início");
    } else {
        snprintf(buffer, size, "%s", l->apps[l->dock[l->dock_index]].name);
    }
    return buffer;
}

static void render_shelf(Launcher *l) {
    const char *eyebrow = "APLICATIVOS";
    const char *sub = "Pressione OK para abrir";

    if (l->overlay == OVERLAY_WALLPAPER) {
        eyebrow = "APARÊNCIA";
        sub = "Escolha um plano de fundo para a sua TV";
    } else if (l->overlay == OVERLAY_CONTROLS) {
        eyebrow = "AJUSTES RÁPIDOS";
        sub = "Pressione OK para abrir";
    } else if (l->view_grid) {
        eyebrow = l->grid_editing ? "EDITAR INÍCIO" : "APLICATIVOS";
        sub = l->grid_editing ? "OK adiciona ou remove · Voltar para sair"
                              : "Pressione OK para abrir";
    } else if (l->zone_utils) {
        eyebrow = "CONFIGURAÇÕES";
        sub = "Pressione OK para abrir";
    } else if (l->reordering) {
        eyebrow = "REORGANIZAR";
        sub = "Esquerda / Direita para mover · OK para fixar";
    } else if (l->dock_count == 0) {
        sub = "Pressione OK para atualizar a lista";
    }

    char title[256];
    shelf_title(l, title, sizeof(title));

    draw_text(l, eyebrow, 500, 20, 96.0f, 300.0f, white(0.42f));
    draw_text_fit(l, title, 300, 64, 96.0f, 342.0f, 1200, white(0.96f));
    draw_text_fit(l, sub, 300, 24, 96.0f, 428.0f, 1400, white(0.55f));
}

static void render_app_contents(Launcher *l, const App *app, float x, float y,
                                float size) {
    const Brand *brand = find_brand(app);
    int width = 0;
    int height = 0;
    GLuint texture = app_icon_texture(l, app, &width, &height);

    if (app->icon[0] && texture) {
        render_image(l->renderer, texture, x, y, size, size, 16.0f,
                     white(1.0f));
        return;
    }
    if (app_is_card(app) && texture) {
        render_image_cover(l->renderer, texture, width, height, x, y, size,
                           size);
        return;
    }
    float scale = size / 160.0f;
    if (texture) {
        float icon_size = 76.0f * scale;
        render_image(l->renderer, texture, x + (size - icon_size) * 0.5f,
                     y + (size - icon_size) * 0.5f, icon_size, icon_size,
                     12.0f, white(1.0f));
        return;
    }
    if (brand && brand->letter) {
        char letter[4] = {app->name[0] ? app->name[0] : '?', '+', '\0', '\0'};
        if (letter[0] >= 'a' && letter[0] <= 'z') letter[0] -= 32;
        int text_px = (int)(48.0f * scale);
        int tw = 0;
        int th = 0;
        text_measure(l->fonts, letter, 500, text_px, 0, &tw, &th);
        draw_text_centered(l, letter, 500, text_px, x + size * 0.5f,
                           y + (size - (float)th) * 0.5f, 0,
                           color_hex(brand->color, 1.0f));
        return;
    }
    char initial[2] = {app->name[0] ? app->name[0] : '?', '\0'};
    if (initial[0] >= 'a' && initial[0] <= 'z') initial[0] -= 32;
    int text_px = (int)(48.0f * scale);
    int tw = 0;
    int th = 0;
    text_measure(l->fonts, initial, 500, text_px, 0, &tw, &th);
    draw_text_centered(l, initial, 500, text_px, x + size * 0.5f,
                       y + (size - (float)th) * 0.5f, 0, white(0.82f));
}

static void render_dock(Launcher *l) {
    if (l->dock_count == 0 || l->view_grid) return;

    const float tile = 160.0f;
    const float gap = 14.0f;
    const float pad = 24.0f;
    const float height = 208.0f;
    const float y = 752.0f;
    float width = pad * 2.0f + tile * (float)l->dock_count +
                  gap * (float)(l->dock_count - 1);
    float x = (SCREEN_W - width) * 0.5f;

    render_gradient_rect(l->renderer, x, y, width, height, 40.0f,
                         white(0.085f), white(0.035f));
    render_rounded_rect_border(l->renderer, x, y, width, height, 40.0f, 1.0f,
                               transparent(), white(0.10f));

    for (int i = 0; i < l->dock_count; ++i) {
        const App *app = &l->apps[l->dock[i]];
        int focused = !l->zone_utils && l->overlay == OVERLAY_NONE &&
                      l->dock_index == i;
        int dark = app_is_dark(app);
        int card = app_is_card(app);
        int full_icon = app->icon[0] != 0;
        float tx = x + pad + (tile + gap) * (float)i;
        float ty = y + pad;
        float draw_x = tx;
        float draw_y = ty;
        float draw_size = tile;
        if (focused) {
            draw_size = tile * 1.04f;
            draw_x = tx - (draw_size - tile) * 0.5f;
            draw_y = ty - 8.0f - (draw_size - tile) * 0.5f;
        }

        if (focused && !dark && !card && !full_icon) {
            render_rounded_rect(l->renderer, draw_x, draw_y, draw_size,
                                draw_size, 18.0f, white(0.42f));
        }
        if (dark) {
            render_rounded_rect(l->renderer, draw_x, draw_y, draw_size,
                                draw_size, 18.0f, black(1.0f));
        }
        render_app_contents(l, app, draw_x, draw_y, draw_size);
        if (focused && (dark || card || full_icon)) {
            render_rounded_rect_border(l->renderer, draw_x, draw_y, draw_size,
                                       draw_size, 18.0f, 2.0f, transparent(),
                                       white(0.88f));
        }
    }
}

static void render_grid(Launcher *l) {
    if (!l->view_grid) return;

    const float x0 = 96.0f;
    const float y0 = 240.0f;
    const float tile_w = 190.0f;
    const float tile_h = 140.0f;
    const float gap_x = 18.0f;
    const float gap_y = 18.0f;
    const float view_h = 720.0f;

    int focused_row = l->grid_count > 0 ? l->grid_index / l->columns : 0;
    float scroll = focused_row > 2
                       ? (float)(focused_row - 2) * (tile_h + gap_y)
                       : 0.0f;

    render_scissor(l->renderer, x0, y0, 1728.0f, view_h);
    for (int i = 0; i < l->grid_count; ++i) {
        int column = i % l->columns;
        int row = i / l->columns;
        float x = x0 + (tile_w + gap_x) * (float)column;
        float y = y0 + (tile_h + gap_y) * (float)row - scroll;
        if (y + tile_h < y0 || y > y0 + view_h) continue;

        const App *app = &l->apps[l->grid[i]];
        int focused = !l->zone_utils && l->overlay == OVERLAY_NONE &&
                      l->grid_index == i;
        Color background = focused ? color_hex(0x2A2A2E, 1.0f)
                                   : color_hex(0x1C1C1E, 1.0f);
        render_rounded_rect(l->renderer, x, y, tile_w, tile_h, 20.0f,
                            background);
        const Brand *brand = find_brand(app);
        int width = 0;
        int height = 0;
        GLuint texture = app_icon_texture(l, app, &width, &height);
        int full_bleed = 0;
        if (app->icon[0] && texture) {
            full_bleed = 1;
            render_image(l->renderer, texture, x + (tile_w - 140.0f) * 0.5f,
                         y, 140.0f, 140.0f, 16.0f, white(1.0f));
        } else if (brand && brand->card && texture) {
            full_bleed = 1;
            render_image_cover(l->renderer, texture, width, height, x, y,
                               tile_w, tile_h);
        } else if (texture) {
            render_image(l->renderer, texture, x + (tile_w - 84.0f) * 0.5f,
                         y + 14.0f, 84.0f, 84.0f, 12.0f, white(1.0f));
        } else if (brand && brand->letter) {
            char letter[4] = {app->name[0] ? app->name[0] : '?', '+', '\0', '\0'};
            if (letter[0] >= 'a' && letter[0] <= 'z') letter[0] -= 32;
            int tw = 0;
            int th = 0;
            text_measure(l->fonts, letter, 500, 48, 0, &tw, &th);
            draw_text_centered(l, letter, 500, 48, x + tile_w * 0.5f,
                               y + 30.0f, 0, color_hex(brand->color, 1.0f));
        } else {
            char initial[2] = {app->name[0] ? app->name[0] : '?', '\0'};
            if (initial[0] >= 'a' && initial[0] <= 'z') initial[0] -= 32;
            draw_text_centered(l, initial, 500, 48, x + tile_w * 0.5f,
                               y + 30.0f, 0, white(0.82f));
        }

        if (full_bleed) {
            render_gradient_rect(l->renderer, x, y + tile_h - 70.0f, tile_w,
                                 70.0f, 20.0f, black(0.0f), black(0.72f));
        }
        draw_text_centered(l, app->name, 500, 22, x + tile_w * 0.5f,
                           y + tile_h - 30.0f, (int)tile_w - 16,
                           full_bleed ? white(0.95f)
                                      : (focused ? white(0.95f) : white(0.72f)));
        if (focused) {
            render_rounded_rect_border(l->renderer, x, y, tile_w, tile_h,
                                       20.0f, 2.0f, transparent(),
                                       white(0.88f));
        }

        if (dock_contains(l, l->grid[i])) {
            render_rounded_rect(l->renderer, x + tile_w - 22.0f, y + 12.0f,
                                14.0f, 14.0f, 7.0f, color_hex(0x0B1220, 1.0f));
            render_rounded_rect(l->renderer, x + tile_w - 20.0f, y + 14.0f,
                                10.0f, 10.0f, 5.0f, color_hex(0x34D399, 1.0f));
        }
    }
    render_scissor_disable(l->renderer);
}

static const char *control_sub(Launcher *l, int index, char *buffer,
                               size_t size) {
    const Control *control = &CONTROLS[index];
    switch (control->kind) {
        case CTRL_TV:
            return "Canais";
        case CTRL_HDMI1:
        case CTRL_HDMI2:
        case CTRL_HDMI3:
        case CTRL_HDMI4: {
            int hdmi = (int)control->kind - (int)CTRL_HDMI1 + 1;
            if (!l->status.hdmi_known[hdmi]) return "Verificando...";
            return l->status.hdmi_connected[hdmi] ? "Conectado" : "Não conectado";
        }
        case CTRL_AMBIENT:
            return "Galeria";
        case CTRL_WIFI:
            if (l->status.network_type == 1 && l->status.wifi_ssid[0]) {
                snprintf(buffer, size, "%s", l->status.wifi_ssid);
                return buffer;
            }
            if (l->status.network_type == 2) return "Rede Cabeada";
            return "Não conectado";
        case CTRL_SETTINGS:
            return "Abrir";
        case CTRL_THEME:
            return THEME_NAMES[l->theme];
        case CTRL_FAMILY:
            return "Desativado";
        case CTRL_DIAG:
            return "Indisponível";
        case CTRL_HOME:
            return "Smart Hub";
    }
    return "";
}

static void render_controls(Launcher *l) {
    if (l->overlay != OVERLAY_CONTROLS) return;

    const float x = 1444.0f;
    const float y = 150.0f;
    const float w = 380.0f;
    const float h = 732.0f;

    render_rounded_rect_border(l->renderer, x, y, w, h, 24.0f, 1.0f,
                               color_rgba(16.0f / 255.0f, 21.0f / 255.0f,
                                          29.0f / 255.0f, 0.94f),
                               white(0.14f));
    draw_text(l, "CONFIGURAÇÕES", 500, 15, x + 30.0f, y + 24.0f, white(0.42f));

    float row_y = y + 60.0f;
    for (int i = 0; i < CONTROL_COUNT; ++i) {
        int focused = l->controls_index == i;
        if (focused) {
            render_rounded_rect(l->renderer, x + 18.0f, row_y, w - 36.0f,
                                48.0f, 14.0f, white(0.94f));
        }
        Color foreground = focused ? black(0.92f) : white(0.82f);
        Color secondary = focused ? black(0.55f) : white(0.50f);

        int iw = 0;
        int ih = 0;
        GLuint texture = icon_texture(l, CONTROLS[i].icon, &iw, &ih);
        if (texture) {
            render_image(l->renderer, texture, x + 32.0f, row_y + 9.0f, 30.0f,
                         30.0f, 0.0f, foreground);
        }

        int tw = 0;
        int th = 0;
        text_measure(l->fonts, CONTROLS[i].label, 500, 24, 0, &tw, &th);
        draw_text(l, CONTROLS[i].label, 500, 24, x + 76.0f,
                  row_y + (48.0f - (float)th) * 0.5f, foreground);

        char buffer[128];
        const char *sub = control_sub(l, i, buffer, sizeof(buffer));
        int sw = 0;
        int sh = 0;
        text_measure(l->fonts, sub, 300, 18, 0, &sw, &sh);
        draw_text_right(l, sub, 300, 18, x + w - 32.0f,
                        row_y + (48.0f - (float)sh) * 0.5f, 170, secondary);

        row_y += 56.0f;
    }
}

static void render_wallpaper_picker(Launcher *l) {
    if (l->overlay != OVERLAY_WALLPAPER) return;

    render_rect(l->renderer, 0.0f, 0.0f, SCREEN_W, SCREEN_H, black(0.38f));
    render_gradient_rect(l->renderer, 0.0f, 440.0f, SCREEN_W, 640.0f, 36.0f,
                         color_hex(0x0E121A, 0.98f),
                         color_hex(0x0A0D12, 1.0f));

    draw_text(l, "Planos de Fundo", 500, 34, 72.0f, 476.0f, white(0.96f));
    draw_text_right(l, "OK para aplicar · Voltar para fechar", 300, 20,
                    1848.0f, 486.0f, 600, white(0.60f));

    const float tile_w = 277.0f;
    const float tile_h = 160.0f;
    const float gap = 22.0f;
    const float x0 = 72.0f;
    const float y0 = 560.0f;

    for (int i = 0; i < WALLPAPER_COUNT; ++i) {
        int column = i % 6;
        int row = i / 6;
        float x = x0 + (tile_w + gap) * (float)column;
        float y = y0 + (tile_h + gap) * (float)row;
        int focused = l->wallpaper_index == i;

        int width = 0;
        int height = 0;
        GLuint texture = wallpaper_texture(l, i, 1, &width, &height);
        if (texture) {
            render_image_cover(l->renderer, texture, width, height, x, y,
                               tile_w, tile_h);
            render_gradient_rect(l->renderer, x, y, tile_w, tile_h, 18.0f,
                                 black(0.0f), black(0.68f));
        } else {
            render_rounded_rect(l->renderer, x, y, tile_w, tile_h, 18.0f,
                                color_hex(0x1A1F29, 1.0f));
            int tw = 0;
            int th = 0;
            text_measure(l->fonts, "Desativado", 500, 20, 0, &tw, &th);
            draw_text_centered(l, "Desativado", 500, 20, x + tile_w * 0.5f,
                               y + (tile_h - (float)th) * 0.5f, 0,
                               white(0.70f));
        }

        draw_text_fit(l, WALLPAPERS[i].name, 500, 18, x + 14.0f,
                      y + tile_h - 30.0f, (int)tile_w - 28, white(0.92f));

        if (i == l->wallpaper) {
            render_rounded_rect(l->renderer, x + tile_w - 62.0f, y + 10.0f,
                                50.0f, 24.0f, 12.0f, white(0.92f));
            draw_text_centered(l, "ATIVO", 500, 12,
                               x + tile_w - 37.0f, y + 15.0f, 0, black(0.90f));
        }
        if (focused) {
            render_rounded_rect_border(l->renderer, x, y, tile_w, tile_h,
                                       20.0f, 3.0f, transparent(),
                                       white(0.95f));
        }
    }

    draw_text(l, "Wallpapers locais · sem downloads", 300, 18, 72.0f, 980.0f,
              white(0.42f));
}

static void render_toast(Launcher *l) {
    uint32_t now = platform_ticks_ms();
    if (!l->toast[0] || now >= l->toast_until) return;

    int width = 0;
    int height = 0;
    text_measure(l->fonts, l->toast, 500, 20, 900, &width, &height);
    float w = (float)width + 48.0f;
    float h = (float)height + 28.0f;
    float x = (SCREEN_W - w) * 0.5f;
    float y = 1000.0f;
    render_rounded_rect_border(l->renderer, x, y, w, h, 18.0f, 1.0f,
                               color_rgba(10.0f / 255.0f, 13.0f / 255.0f,
                                          18.0f / 255.0f, 0.94f),
                               white(0.16f));
    draw_text_centered(l, l->toast, 500, 20, SCREEN_W * 0.5f,
                       y + 14.0f, 900, white(0.88f));
}

static void render_empty(Launcher *l) {
    if (l->app_count != 0) return;
    draw_text_centered(l, "Nenhum aplicativo encontrado", 500, 32,
                       SCREEN_W * 0.5f, 360.0f, 1200, white(0.75f));
    draw_text_centered(l, "Pressione OK para tentar novamente", 300, 22,
                       SCREEN_W * 0.5f, 410.0f, 1200, white(0.45f));
}

static void load_apps(Launcher *l) {
    PlatformApp platform_apps[MAX_APPS];
    int count = platform_list_apps(platform_apps, MAX_APPS);
    if (count < 0) count = 0;
    if (count > MAX_APPS) count = MAX_APPS;

    l->app_count = 0;
    for (int i = 0; i < count; ++i) {
        App *app = &l->apps[l->app_count++];
        snprintf(app->id, sizeof(app->id), "%s", platform_apps[i].id);
        snprintf(app->name, sizeof(app->name), "%s", platform_apps[i].name);
        snprintf(app->icon, sizeof(app->icon), "%s", platform_apps[i].icon);
    }
    qsort(l->apps, (size_t)l->app_count, sizeof(App), app_name_cmp);
    build_grid(l);

    if (l->config.seeded) {
        dock_from_config(l);
        if (l->dock_count == 0) {
            default_dock(l);
            save_config(l);
        }
    } else {
        default_dock(l);
        save_config(l);
    }

    if (l->dock_index >= l->dock_count) l->dock_index = 0;
    if (l->grid_index >= l->grid_count) l->grid_index = 0;
    if (l->util_index < 0) l->util_index = 0;
    if (l->util_index >= UTIL_COUNT) l->util_index = UTIL_COUNT - 1;
}

static void reload_apps(Launcher *l) {
    load_apps(l);
    toast(l, "Lista de apps atualizada");
}

static void launch_app(Launcher *l, const App *app) {
    int result;
    if (strcmp(app->id, SETTINGS_APP_ID) == 0) {
        result = platform_launch_settings();
    } else {
        result = platform_launch_app(app->id);
    }
    if (result != 0) {
        toast(l, "Não foi possível abrir %s", app->name);
    }
}

static void toggle_home(Launcher *l, int app_index) {
    if (app_index < 0 || app_index >= l->app_count) return;
    const App *app = &l->apps[app_index];
    if (is_blocked(app)) {
        toast(l, "Este app não pode ser fixado");
        return;
    }
    if (dock_contains(l, app_index)) {
        remove_from_dock(l, app_index);
        if (l->dock_index >= l->dock_count) {
            l->dock_index = l->dock_count > 0 ? l->dock_count - 1 : 0;
        }
        save_config(l);
        toast(l, "Removido do Início");
    } else if (l->dock_count >= MAX_DOCK) {
        toast(l, "Limite de 9 apps no Início");
    } else {
        add_to_dock(l, app_index);
        save_config(l);
        toast(l, "Adicionado ao Início");
    }
}

static void activate_util(Launcher *l) {
    switch (l->util_index) {
        case 0:
            l->view_grid = 1;
            l->grid_editing = 0;
            l->zone_utils = 0;
            l->grid_index = 0;
            break;
        case 1:
            reload_apps(l);
            break;
        case 2:
            l->view_grid = 1;
            l->grid_editing = 1;
            l->zone_utils = 0;
            l->grid_index = 0;
            break;
        case 3:
            platform_launch_store();
            break;
        case 4:
            l->overlay = OVERLAY_WALLPAPER;
            l->wallpaper_index = l->wallpaper;
            break;
        case 5:
            l->overlay = OVERLAY_CONTROLS;
            l->controls_index = 0;
            break;
        default:
            break;
    }
}

static void close_overlay(Launcher *l) {
    l->overlay = OVERLAY_NONE;
}

static void activate_control(Launcher *l) {
    const Control *control = &CONTROLS[l->controls_index];
    int result = 0;
    switch (control->kind) {
        case CTRL_TV:
            result = platform_launch_app(LIVETV_APP_ID);
            close_overlay(l);
            break;
        case CTRL_HDMI1:
            result = platform_launch_source("HDMI1");
            close_overlay(l);
            break;
        case CTRL_HDMI2:
            result = platform_launch_source("HDMI2");
            close_overlay(l);
            break;
        case CTRL_HDMI3:
            result = platform_launch_source("HDMI3");
            close_overlay(l);
            break;
        case CTRL_HDMI4:
            result = platform_launch_source("HDMI4");
            close_overlay(l);
            break;
        case CTRL_AMBIENT:
            result = platform_launch_ambient();
            close_overlay(l);
            break;
        case CTRL_WIFI:
            result = platform_launch_network_settings();
            close_overlay(l);
            break;
        case CTRL_SETTINGS:
            result = platform_launch_settings();
            close_overlay(l);
            break;
        case CTRL_THEME:
            set_theme(l, l->theme + 1);
            save_config(l);
            toast(l, "Tema: %s", THEME_NAMES[l->theme]);
            break;
        case CTRL_FAMILY:
            toast(l, "Troca automática indisponível");
            break;
        case CTRL_DIAG:
            toast(l, "Diagnóstico indisponível");
            break;
        case CTRL_HOME:
            result = platform_launch_home();
            close_overlay(l);
            break;
    }
    if (result != 0) {
        toast(l, "Não foi possível abrir %s", control->label);
    }
}

static void apply_wallpaper(Launcher *l) {
    l->wallpaper = l->wallpaper_index;
    save_config(l);
    close_overlay(l);
    toast(l, "Plano de fundo: %s", WALLPAPERS[l->wallpaper].name);
}

static void enter_reorder(Launcher *l) {
    if (l->dock_count == 0) return;
    l->reordering = 1;
    l->reorder_original_count = l->dock_count;
    memcpy(l->reorder_original, l->dock,
           sizeof(l->dock[0]) * (size_t)l->dock_count);
    l->zone_utils = 0;
    l->view_grid = 0;
    l->overlay = OVERLAY_NONE;
    l->hold_fired = 1;
    l->suppress_activate = 1;
}

static void finish_reorder(Launcher *l) {
    if (!l->reordering) return;
    l->reordering = 0;
    save_config(l);
    toast(l, "Ordem dos apps salva");
}

static void cancel_reorder(Launcher *l) {
    if (!l->reordering) return;
    memcpy(l->dock, l->reorder_original,
           sizeof(l->dock[0]) * (size_t)l->reorder_original_count);
    l->dock_count = l->reorder_original_count;
    l->reordering = 0;
    l->dock_index = 0;
    save_config(l);
    toast(l, "Alterações descartadas");
}

static void reorder_move(Launcher *l, int direction) {
    if (!l->reordering || l->dock_count < 2) return;
    int target = l->dock_index + direction;
    if (target < 0) target = 0;
    if (target >= l->dock_count) target = l->dock_count - 1;
    if (target == l->dock_index) return;
    int tmp = l->dock[l->dock_index];
    l->dock[l->dock_index] = l->dock[target];
    l->dock[target] = tmp;
    l->dock_index = target;
    save_config(l);
}

static void handle_back(Launcher *l) {
    l->ok_held = 0;
    l->hold_fired = 0;
    l->suppress_activate = 0;
    if (l->overlay != OVERLAY_NONE) {
        close_overlay(l);
        return;
    }
    if (l->reordering) {
        cancel_reorder(l);
        return;
    }
    if (l->view_grid) {
        l->view_grid = 0;
        l->grid_editing = 0;
        l->zone_utils = 0;
        l->dock_index = 0;
        return;
    }
    if (l->zone_utils) {
        l->zone_utils = 0;
        return;
    }
    toast(l, "Pressione Home da TV para o Smart Hub");
}

static void handle_nav(Launcher *l, LauncherKey key) {
    if (l->overlay == OVERLAY_WALLPAPER) {
        if (key == CH_KEY_LEFT) {
            l->wallpaper_index = l->wallpaper_index > 0 ? l->wallpaper_index - 1 : 0;
        } else if (key == CH_KEY_RIGHT) {
            l->wallpaper_index = l->wallpaper_index + 1 < WALLPAPER_COUNT
                                     ? l->wallpaper_index + 1
                                     : WALLPAPER_COUNT - 1;
        } else if (key == CH_KEY_UP) {
            l->wallpaper_index -= 6;
            if (l->wallpaper_index < 0) l->wallpaper_index = 0;
        } else if (key == CH_KEY_DOWN) {
            l->wallpaper_index += 6;
            if (l->wallpaper_index >= WALLPAPER_COUNT) {
                l->wallpaper_index = WALLPAPER_COUNT - 1;
            }
        }
        return;
    }

    if (l->overlay == OVERLAY_CONTROLS) {
        if (key == CH_KEY_UP && l->controls_index > 0) l->controls_index--;
        if (key == CH_KEY_DOWN && l->controls_index + 1 < CONTROL_COUNT) {
            l->controls_index++;
        }
        return;
    }

    if (l->reordering) {
        if (key == CH_KEY_LEFT) reorder_move(l, -1);
        if (key == CH_KEY_RIGHT) reorder_move(l, 1);
        return;
    }

    if (l->view_grid) {
        if (key == CH_KEY_LEFT && l->grid_index > 0) {
            l->grid_index--;
        } else if (key == CH_KEY_RIGHT && l->grid_index + 1 < l->grid_count) {
            l->grid_index++;
        } else if (key == CH_KEY_UP) {
            if (l->grid_index < l->columns) {
                l->view_grid = 0;
                l->grid_editing = 0;
                l->zone_utils = 0;
                l->dock_index = 0;
            } else {
                l->grid_index -= l->columns;
            }
        } else if (key == CH_KEY_DOWN &&
                   l->grid_index + l->columns < l->grid_count) {
            l->grid_index += l->columns;
        }
        return;
    }

    if (l->zone_utils) {
        if (key == CH_KEY_LEFT && l->util_index > 0) l->util_index--;
        if (key == CH_KEY_RIGHT && l->util_index + 1 < UTIL_COUNT) {
            l->util_index++;
        }
        if (key == CH_KEY_DOWN) {
            l->zone_utils = 0;
            if (l->dock_index >= l->dock_count) l->dock_index = 0;
        }
        return;
    }

    if (key == CH_KEY_LEFT && l->dock_index > 0) {
        l->dock_index--;
    } else if (key == CH_KEY_RIGHT && l->dock_index + 1 < l->dock_count) {
        l->dock_index++;
    } else if (key == CH_KEY_UP) {
        l->zone_utils = 1;
        l->util_index = 0;
    }
}

static void activate(Launcher *l) {
    if (l->view_grid) {
        if (l->grid_count == 0) {
            toast(l, "Nenhum aplicativo encontrado");
            return;
        }
        if (l->grid_editing) {
            toggle_home(l, l->grid[l->grid_index]);
        } else {
            launch_app(l, &l->apps[l->grid[l->grid_index]]);
        }
        return;
    }
    if (l->zone_utils) {
        activate_util(l);
        return;
    }
    if (l->dock_count == 0) {
        toast(l, "Nenhum app no Início");
        return;
    }
    launch_app(l, &l->apps[l->dock[l->dock_index]]);
}

int launcher_init(Launcher **out, Renderer *renderer, Fonts *fonts,
                  Images *images) {
    Launcher *l = (Launcher *)calloc(1, sizeof(Launcher));
    if (!l) return -1;
    l->renderer = renderer;
    l->fonts = fonts;
    l->images = images;
    snprintf(l->asset_root, sizeof(l->asset_root), "%s", platform_asset_root());
    snprintf(l->config_path, sizeof(l->config_path), "%s/config.ini",
             platform_data_root());
    config_load(&l->config, l->config_path);
    l->theme = theme_from_id(l->config.theme);
    l->wallpaper = wallpaper_from_id(l->config.wallpaper);
    if (l->wallpaper < 0 || l->wallpaper >= WALLPAPER_COUNT) l->wallpaper = 1;
    l->columns = l->theme == 2 ? 6 : 5;
    l->wallpaper_index = l->wallpaper;
    l->view_grid = 0;
    l->grid_editing = 0;
    l->zone_utils = 0;
    l->overlay = OVERLAY_NONE;
    l->dock_index = 0;
    l->util_index = 0;
    l->grid_index = 0;
    l->controls_index = 0;
    l->status.network_type = 0;
    l->last_status_ms = platform_ticks_ms();

    load_apps(l);
    platform_get_status(&l->status);
    *out = l;
    return 0;
}

void launcher_shutdown(Launcher *l) {
    free(l);
}

void launcher_tick(Launcher *l) {
    if (!l) return;
    uint32_t now = platform_ticks_ms();
    if (now - l->last_status_ms >= 5000) {
        platform_get_status(&l->status);
        l->last_status_ms = now;
    }
    if (l->ok_held && !l->hold_fired && !l->reordering && !l->view_grid &&
        !l->zone_utils && l->overlay == OVERLAY_NONE && l->dock_count > 0 &&
        now - l->ok_down_ms >= 600) {
        enter_reorder(l);
    }
}

void launcher_render(Launcher *l) {
    if (!l) return;
    render_begin(l->renderer);
    render_background(l);
    render_clock(l);
    render_utils(l);
    render_shelf(l);
    render_dock(l);
    render_grid(l);
    render_controls(l);
    render_wallpaper_picker(l);
    render_empty(l);
    render_toast(l);
    render_end(l->renderer);
}

void launcher_key(Launcher *l, LauncherKey key, int down, int repeat) {
    if (!l) return;
    uint32_t now = platform_ticks_ms();

    if (!down) {
        if (key == CH_KEY_OK) {
            int was_held = l->ok_held;
            l->ok_held = 0;
            if (l->suppress_activate) {
                l->suppress_activate = 0;
                l->hold_fired = 0;
                return;
            }
            if (l->reordering) {
                l->hold_fired = 0;
                return;
            }
            if (l->overlay != OVERLAY_NONE) {
                l->hold_fired = 0;
                return;
            }
            if (was_held && !l->hold_fired) {
                activate(l);
            }
            l->hold_fired = 0;
        }
        return;
    }

    if (key == CH_KEY_OK) {
        if (repeat) return;
        if (l->reordering) {
            l->suppress_activate = 1;
            finish_reorder(l);
            return;
        }
        if (l->overlay == OVERLAY_CONTROLS) {
            activate_control(l);
            return;
        }
        if (l->overlay == OVERLAY_WALLPAPER) {
            apply_wallpaper(l);
            return;
        }
        if (l->app_count == 0) {
            reload_apps(l);
            return;
        }
        l->ok_held = 1;
        l->ok_down_ms = now;
        l->hold_fired = 0;
        return;
    }

    if (key == CH_KEY_BACK) {
        if (!repeat) handle_back(l);
        return;
    }

    if (key == CH_KEY_LEFT || key == CH_KEY_RIGHT || key == CH_KEY_UP ||
        key == CH_KEY_DOWN) {
        handle_nav(l, key);
        return;
    }

    if (key == CH_KEY_MEDIA_PLAY || key == CH_KEY_MEDIA_NEXT ||
        key == CH_KEY_MEDIA_PREV) {
        if (!repeat) toast(l, "Controle de mídia indisponível");
    }
}
