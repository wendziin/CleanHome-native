#include <stdbool.h>
#include <string.h>

#include <Elementary.h>
#include <Evas_GL.h>
#include <app.h>
#include <dlog.h>

#include "gles.h"
#include "image.h"
#include "launcher.h"
#include "platform.h"
#include "render.h"
#include "text.h"

#undef LOG_TAG
#define LOG_TAG "CleanHome"

Evas_GL_API *g_evas_gl_api = NULL;

typedef struct {
    Evas_Object *win;
    Evas_Object *glview;
    Renderer *renderer;
    Fonts *fonts;
    Images *images;
    Launcher *launcher;
    int width;
    int height;
    int ready;
    int paused;
} AppState;

static AppState g_app;

static void cleanup(void) {
    if (!g_app.ready) return;
    g_app.ready = 0;
    launcher_shutdown(g_app.launcher);
    g_app.launcher = NULL;
    image_shutdown(g_app.images);
    g_app.images = NULL;
    text_shutdown(g_app.fonts);
    g_app.fonts = NULL;
    render_shutdown(g_app.renderer);
    g_app.renderer = NULL;
    platform_shutdown();
}

static void glview_del_cb(Evas_Object *obj) {
    (void)obj;
    cleanup();
    g_app.glview = NULL;
}

static void glview_resize_cb(Evas_Object *obj) {
    int width = 0;
    int height = 0;
    elm_glview_size_get(obj, &width, &height);
    if (width <= 0) width = 1920;
    if (height <= 0) height = 1080;
    g_app.width = width;
    g_app.height = height;
    if (g_app.renderer) {
        render_set_size(g_app.renderer, width, height);
    }
}

static void glview_render_cb(Evas_Object *obj) {
    if (!g_app.ready || g_app.paused || !g_app.launcher) return;

    int width = 0;
    int height = 0;
    elm_glview_size_get(obj, &width, &height);
    if (width > 0 && height > 0 &&
        (width != g_app.width || height != g_app.height)) {
        g_app.width = width;
        g_app.height = height;
        render_set_size(g_app.renderer, width, height);
    }

    launcher_tick(g_app.launcher);
    launcher_render(g_app.launcher);
}

static void glview_init_cb(Evas_Object *obj) {
    if (g_app.ready) return;

    Evas_GL *evas_gl = elm_glview_evas_gl_get(obj);
    if (!evas_gl) {
        dlog_print(DLOG_ERROR, LOG_TAG, "elm_glview_evas_gl_get failed");
        return;
    }
    g_evas_gl_api = evas_gl_api_get(evas_gl);
    if (!g_evas_gl_api) {
        dlog_print(DLOG_ERROR, LOG_TAG, "evas_gl_api_get failed");
        return;
    }

    if (platform_init() != 0) {
        dlog_print(DLOG_ERROR, LOG_TAG, "platform_init failed");
        return;
    }
    if (render_init(&g_app.renderer) != 0) {
        dlog_print(DLOG_ERROR, LOG_TAG, "render_init failed");
        platform_shutdown();
        return;
    }

    glview_resize_cb(obj);
    if (g_app.width <= 0) g_app.width = 1920;
    if (g_app.height <= 0) g_app.height = 1080;
    render_set_size(g_app.renderer, g_app.width, g_app.height);

    if (text_init(&g_app.fonts, platform_asset_root()) != 0) {
        dlog_print(DLOG_ERROR, LOG_TAG, "text_init failed");
        render_shutdown(g_app.renderer);
        g_app.renderer = NULL;
        platform_shutdown();
        return;
    }
    if (image_init(&g_app.images) != 0) {
        dlog_print(DLOG_ERROR, LOG_TAG, "image_init failed");
        text_shutdown(g_app.fonts);
        g_app.fonts = NULL;
        render_shutdown(g_app.renderer);
        g_app.renderer = NULL;
        platform_shutdown();
        return;
    }
    if (launcher_init(&g_app.launcher, g_app.renderer, g_app.fonts,
                      g_app.images) != 0) {
        dlog_print(DLOG_ERROR, LOG_TAG, "launcher_init failed");
        image_shutdown(g_app.images);
        g_app.images = NULL;
        text_shutdown(g_app.fonts);
        g_app.fonts = NULL;
        render_shutdown(g_app.renderer);
        g_app.renderer = NULL;
        platform_shutdown();
        return;
    }

    g_app.ready = 1;
    dlog_print(DLOG_INFO, LOG_TAG, "ready %dx%d", g_app.width, g_app.height);
}

static int map_key(const char *keyname, LauncherKey *out) {
    if (!keyname) return 0;
    if (strcmp(keyname, "Left") == 0) {
        *out = CH_KEY_LEFT;
        return 1;
    }
    if (strcmp(keyname, "Right") == 0) {
        *out = CH_KEY_RIGHT;
        return 1;
    }
    if (strcmp(keyname, "Up") == 0) {
        *out = CH_KEY_UP;
        return 1;
    }
    if (strcmp(keyname, "Down") == 0) {
        *out = CH_KEY_DOWN;
        return 1;
    }
    if (strcmp(keyname, "Return") == 0 || strcmp(keyname, "KP_Enter") == 0 ||
        strcmp(keyname, "space") == 0) {
        *out = CH_KEY_OK;
        return 1;
    }
    if (strcmp(keyname, "Escape") == 0 || strcmp(keyname, "XF86Back") == 0 ||
        strcmp(keyname, "BackSpace") == 0) {
        *out = CH_KEY_BACK;
        return 1;
    }
    if (strcmp(keyname, "XF86AudioPlay") == 0 ||
        strcmp(keyname, "XF86AudioPause") == 0) {
        *out = CH_KEY_MEDIA_PLAY;
        return 1;
    }
    if (strcmp(keyname, "XF86AudioNext") == 0) {
        *out = CH_KEY_MEDIA_NEXT;
        return 1;
    }
    if (strcmp(keyname, "XF86AudioPrev") == 0) {
        *out = CH_KEY_MEDIA_PREV;
        return 1;
    }
    return 0;
}

static void key_down_cb(void *data, Evas *evas, Evas_Object *obj,
                        void *event_info) {
    (void)data;
    (void)evas;
    (void)obj;
    Evas_Event_Key_Down *event = (Evas_Event_Key_Down *)event_info;
    LauncherKey key;
    if (map_key(event->keyname, &key)) {
        launcher_key(g_app.launcher, key, 1, 0);
    }
}

static void key_up_cb(void *data, Evas *evas, Evas_Object *obj,
                      void *event_info) {
    (void)data;
    (void)evas;
    (void)obj;
    Evas_Event_Key_Up *event = (Evas_Event_Key_Up *)event_info;
    LauncherKey key;
    if (map_key(event->keyname, &key)) {
        launcher_key(g_app.launcher, key, 0, 0);
    }
}

static void win_del_cb(void *data, Evas *evas, Evas_Object *obj,
                       void *event_info) {
    (void)data;
    (void)evas;
    (void)obj;
    (void)event_info;
    g_app.win = NULL;
}

static bool app_create(void *data) {
    (void)data;

    g_app.win = elm_win_add(NULL, "CleanHome", ELM_WIN_BASIC);
    if (!g_app.win) return false;
    elm_win_title_set(g_app.win, "CleanHome");
    elm_win_autodel_set(g_app.win, EINA_TRUE);
    elm_win_fullscreen_set(g_app.win, EINA_TRUE);
    elm_win_focus_highlight_enabled_set(g_app.win, EINA_FALSE);
    evas_object_event_callback_add(g_app.win, EVAS_CALLBACK_DEL, win_del_cb,
                                   NULL);
    evas_object_event_callback_add(g_app.win, EVAS_CALLBACK_KEY_DOWN,
                                   key_down_cb, NULL);
    evas_object_event_callback_add(g_app.win, EVAS_CALLBACK_KEY_UP, key_up_cb,
                                   NULL);

    g_app.glview = elm_glview_add(g_app.win);
    if (!g_app.glview) {
        evas_object_del(g_app.win);
        g_app.win = NULL;
        return false;
    }
    elm_glview_mode_set(g_app.glview, ELM_GLVIEW_ALPHA);
    elm_glview_resize_policy_set(g_app.glview,
                                 ELM_GLVIEW_RESIZE_POLICY_RECREATE);
    elm_glview_render_policy_set(g_app.glview,
                                 ELM_GLVIEW_RENDER_POLICY_ALWAYS);
    elm_glview_init_func_set(g_app.glview, glview_init_cb);
    elm_glview_del_func_set(g_app.glview, glview_del_cb);
    elm_glview_resize_func_set(g_app.glview, glview_resize_cb);
    elm_glview_render_func_set(g_app.glview, glview_render_cb);

    evas_object_resize(g_app.glview, 1920, 1080);
    elm_win_resize_object_add(g_app.win, g_app.glview);
    evas_object_show(g_app.glview);
    evas_object_show(g_app.win);
    evas_object_focus_set(g_app.win, EINA_TRUE);
    elm_win_activate(g_app.win);
    return true;
}

static void app_terminate(void *data) {
    (void)data;
    if (g_app.win) {
        Evas_Object *win = g_app.win;
        g_app.win = NULL;
        evas_object_del(win);
    } else {
        cleanup();
    }
}

static void app_pause(void *data) {
    (void)data;
    g_app.paused = 1;
}

static void app_resume(void *data) {
    (void)data;
    g_app.paused = 0;
    if (g_app.glview) {
        elm_glview_changed_set(g_app.glview);
    }
    if (g_app.win) {
        elm_win_activate(g_app.win);
        evas_object_focus_set(g_app.win, EINA_TRUE);
    }
}

static void app_control(app_control_h app_control, void *data) {
    (void)app_control;
    (void)data;
    if (g_app.win) {
        evas_object_show(g_app.win);
        elm_win_activate(g_app.win);
        evas_object_focus_set(g_app.win, EINA_TRUE);
    }
}

int main(int argc, char *argv[]) {
    ui_app_lifecycle_callback_s callbacks = {0,};
    callbacks.create = app_create;
    callbacks.terminate = app_terminate;
    callbacks.pause = app_pause;
    callbacks.resume = app_resume;
    callbacks.app_control = app_control;
    return ui_app_main(argc, argv, &callbacks, NULL);
}
