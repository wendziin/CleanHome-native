#include "platform.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include "app_ids.h"

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

static int try_asset_root(const char *candidate) {
    if (!candidate || candidate[0] == '\0') return 0;
    char resolved[PATH_MAX];
    if (!realpath(candidate, resolved)) return 0;
    snprintf(asset_root, sizeof(asset_root), "%s", resolved);
    return 1;
}

int platform_init(void) {
    const char *env_root = getenv("CLEANHOME_ASSETS");
    const char *candidates[] = {
        env_root,
        "assets",
        "../assets",
        "../../assets",
        "res/assets",
        "../res/assets",
        NULL
    };
    int found = 0;
    for (int i = 0; candidates[i]; ++i) {
        if (try_asset_root(candidates[i])) {
            found = 1;
            break;
        }
    }
    if (!found) {
        snprintf(asset_root, sizeof(asset_root), "assets");
    }

    const char *home = getenv("HOME");
    if (!home || home[0] == '\0') home = "/tmp";
    snprintf(data_root, sizeof(data_root), "%s/.config/cleanhome", home);
    mkdir(data_root, 0700);
    fprintf(stderr, "assets: %s\ndata: %s\n", asset_root, data_root);
    return 0;
}

void platform_shutdown(void) {
}

const char *platform_asset_root(void) {
    return asset_root;
}

const char *platform_data_root(void) {
    return data_root;
}

uint32_t platform_ticks_ms(void) {
    return SDL_GetTicks();
}

int platform_list_apps(PlatformApp *apps, int max_apps) {
    static const struct {
        const char *id;
        const char *name;
        const char *icon;
    } demo[] = {
        {LIVETV_APP_ID, "TV ao Vivo", ""},
        {SETTINGS_APP_ID, "Configurações", ""},
        {"com.samsung.tv.coba.netflix", "Netflix", "icons/apps/netflix.png"},
        {"com.samsung.tv.coba.youtube", "YouTube", ""},
        {"com.samsung.tv.coba.spotify", "Spotify", "icons/apps/spotify.png"},
        {"com.samsung.tv.coba.plex", "Plex", "icons/apps/plex.png"},
        {"com.samsung.tv.coba.appletv", "Apple TV", ""},
        {"com.samsung.tv.coba.primevideo", "Prime Video", ""},
        {"com.samsung.tv.coba.disneyplus", "Disney+", ""},
        {"com.samsung.tv.coba.dstv", "DStv", "icons/apps/dstv.png"},
        {"com.samsung.tv.coba.internet", "Internet", "icons/apps/internet.png"},
    };
    int count = 0;
    int total = (int)(sizeof(demo) / sizeof(demo[0]));
    for (int i = 0; i < total && count < max_apps; ++i) {
        PlatformApp *app = &apps[count++];
        snprintf(app->id, sizeof(app->id), "%s", demo[i].id);
        snprintf(app->name, sizeof(app->name), "%s", demo[i].name);
        if (demo[i].icon[0]) {
            join_path(app->icon, sizeof(app->icon), asset_root, demo[i].icon);
        } else {
            app->icon[0] = '\0';
        }
    }
    return count;
}

int platform_get_status(PlatformStatus *status) {
    if (!status) return -1;
    memset(status, 0, sizeof(*status));
    status->wifi_on = 1;
    status->network_type = 1;
    snprintf(status->wifi_ssid, sizeof(status->wifi_ssid), "Casa");
    for (int i = 1; i <= 4; ++i) {
        status->hdmi_known[i] = 1;
        status->hdmi_connected[i] = (i == 1) ? 1 : 0;
    }
    return 0;
}

int platform_launch_app(const char *id) {
    printf("launch app: %s\n", id ? id : "(null)");
    fflush(stdout);
    return 0;
}

int platform_launch_source(const char *source) {
    printf("launch source: %s\n", source ? source : "(null)");
    fflush(stdout);
    return 0;
}

int platform_launch_store(void) {
    printf("launch store\n");
    fflush(stdout);
    return 0;
}

int platform_launch_settings(void) {
    printf("launch settings\n");
    fflush(stdout);
    return 0;
}

int platform_launch_home(void) {
    printf("launch home\n");
    fflush(stdout);
    return 0;
}

int platform_launch_network_settings(void) {
    printf("launch network settings\n");
    fflush(stdout);
    return 0;
}

int platform_launch_ambient(void) {
    printf("launch ambient\n");
    fflush(stdout);
    return 0;
}
