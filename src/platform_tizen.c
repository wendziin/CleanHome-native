#include "platform.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>

#include <SDL2/SDL.h>

#include <app_common.h>
#include <app_control.h>
#include <app_manager.h>
#include <dlog.h>

#include "app_ids.h"

#undef LOG_TAG
#define LOG_TAG "CleanHome"

static char asset_root[1024];
static char data_root[1024];

static void join_path(char *out, size_t out_size, const char *root,
                      const char *relative) {
    size_t len = strlen(root);
    if (len > 0 && root[len - 1] == '/') {
        snprintf(out, out_size, "%s%s", root, relative);
    } else {
        snprintf(out, out_size, "%s/%s", root, relative);
    }
}

static int contains_ci(const char *haystack, const char *needle) {
    if (!haystack || !needle || !needle[0]) return 0;
    size_t len = strlen(needle);
    for (const char *p = haystack; *p; ++p) {
        if (strncasecmp(p, needle, len) == 0) return 1;
    }
    return 0;
}

static const char *icon_for(const char *id, const char *label) {
    if (contains_ci(id, "netflix") || contains_ci(label, "netflix")) {
        return "icons/apps/netflix.png";
    }
    if (contains_ci(id, "spotify") || contains_ci(label, "spotify")) {
        return "icons/apps/spotify.png";
    }
    if (contains_ci(id, "plex") || contains_ci(label, "plex")) {
        return "icons/apps/plex.png";
    }
    if (contains_ci(id, "dstv") || contains_ci(label, "dstv")) {
        return "icons/apps/dstv.png";
    }
    if (contains_ci(id, "internet") || contains_ci(label, "internet") ||
        contains_ci(label, "navegador")) {
        return "icons/apps/internet.png";
    }
    return NULL;
}

static int skip_app(const char *id, const char *label) {
    if (!id || !id[0]) return 1;
    if (strcmp(id, "com.cleanhome.native") == 0) return 1;
    if (contains_ci(label, "emanual") || contains_ci(label, "e-manual")) {
        return 1;
    }
    return 0;
}

int platform_init(void) {
    if (SDL_Init(0) != 0) {
        dlog_print(DLOG_ERROR, LOG_TAG, "SDL_Init failed: %s", SDL_GetError());
    }

    char *resource = app_get_resource_path();
    if (resource) {
        join_path(asset_root, sizeof(asset_root), resource, "assets");
        free(resource);
    } else {
        snprintf(asset_root, sizeof(asset_root), "res/assets");
    }

    char *data = app_get_data_path();
    if (data) {
        snprintf(data_root, sizeof(data_root), "%s", data);
        free(data);
        size_t len = strlen(data_root);
        while (len > 1 && data_root[len - 1] == '/') {
            data_root[--len] = '\0';
        }
    } else {
        snprintf(data_root, sizeof(data_root), "/tmp/cleanhome");
    }
    mkdir(data_root, 0700);

    dlog_print(DLOG_INFO, LOG_TAG, "assets=%s data=%s", asset_root, data_root);
    return 0;
}

void platform_shutdown(void) {
    SDL_Quit();
}

const char *platform_asset_root(void) {
    return asset_root;
}

const char *platform_data_root(void) {
    return data_root;
}

uint32_t platform_ticks_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000u +
                      (uint64_t)ts.tv_nsec / 1000000u);
}

typedef struct {
    PlatformApp *apps;
    int max_apps;
    int count;
} AppListContext;

static bool app_info_callback(app_info_h info, void *user_data) {
    AppListContext *ctx = (AppListContext *)user_data;
    if (ctx->count >= ctx->max_apps) return false;

    char *id = NULL;
    char *label = NULL;
    if (app_info_get_app_id(info, &id) != APP_MANAGER_ERROR_NONE || !id) {
        return true;
    }
    if (app_info_get_label(info, &label) != APP_MANAGER_ERROR_NONE || !label) {
        free(id);
        return true;
    }
    if (skip_app(id, label)) {
        free(id);
        free(label);
        return true;
    }

    PlatformApp *app = &ctx->apps[ctx->count];
    snprintf(app->id, sizeof(app->id), "%s", id);
    snprintf(app->name, sizeof(app->name), "%s", label);
    const char *icon = icon_for(id, label);
    if (icon) {
        join_path(app->icon, sizeof(app->icon), asset_root, icon);
    } else {
        app->icon[0] = '\0';
    }
    ctx->count++;

    free(id);
    free(label);
    return true;
}

int platform_list_apps(PlatformApp *apps, int max_apps) {
    if (!apps || max_apps <= 0) return 0;
    AppListContext ctx = {apps, max_apps, 0};
    int result = app_manager_foreach_app_info(app_info_callback, &ctx);
    if (result != APP_MANAGER_ERROR_NONE) {
        dlog_print(DLOG_ERROR, LOG_TAG, "app_manager_foreach_app_info: %d",
                   result);
    }
    return ctx.count;
}

int platform_get_status(PlatformStatus *status) {
    if (!status) return -1;
    memset(status, 0, sizeof(*status));
    status->wifi_on = 1;
    status->network_type = 1;
    snprintf(status->wifi_ssid, sizeof(status->wifi_ssid), "Wi-Fi");
    for (int i = 1; i <= 4; ++i) {
        status->hdmi_known[i] = 1;
        status->hdmi_connected[i] = 0;
    }
    return 0;
}

static int launch_app_id(const char *app_id, const char *key,
                         const char *value) {
    if (!app_id || !app_id[0]) return -1;

    app_control_h control = NULL;
    if (app_control_create(&control) != APP_CONTROL_ERROR_NONE || !control) {
        dlog_print(DLOG_ERROR, LOG_TAG, "app_control_create failed");
        return -1;
    }
    int result = app_control_set_operation(control, APP_CONTROL_OPERATION_DEFAULT);
    if (result == APP_CONTROL_ERROR_NONE) {
        result = app_control_set_app_id(control, app_id);
    }
    if (result == APP_CONTROL_ERROR_NONE && key && value) {
        result = app_control_add_extra_data(control, key, value);
    }
    if (result == APP_CONTROL_ERROR_NONE) {
        result = app_control_send_launch_request(control, NULL, NULL);
    }
    app_control_destroy(control);

    if (result != APP_CONTROL_ERROR_NONE) {
        dlog_print(DLOG_ERROR, LOG_TAG, "launch %s failed: %d", app_id, result);
        return -1;
    }
    return 0;
}

int platform_launch_app(const char *id) {
    return launch_app_id(id, NULL, NULL);
}

int platform_launch_source(const char *source) {
    return launch_app_id(SOURCE_APP_ID, "source", source);
}

int platform_launch_store(void) {
    return launch_app_id(STORE_APP_ID, NULL, NULL);
}

int platform_launch_settings(void) {
    return launch_app_id(SETTINGS_APP_ID, NULL, NULL);
}

int platform_launch_home(void) {
    return launch_app_id(HOME_APP_ID, NULL, NULL);
}

int platform_launch_network_settings(void) {
    return launch_app_id(NETWORK_SETTINGS_APP_ID, NULL, NULL);
}

int platform_launch_ambient(void) {
    return launch_app_id(AMBIENT_APP_ID, NULL, NULL);
}
