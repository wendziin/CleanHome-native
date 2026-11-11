#ifndef CLEANHOME_PLATFORM_H
#define CLEANHOME_PLATFORM_H

#include <stdint.h>

typedef struct {
    char id[256];
    char name[128];
    char icon[1024];
} PlatformApp;

typedef struct {
    int wifi_on;
    int network_type;
    char wifi_ssid[128];
    int hdmi_known[5];
    int hdmi_connected[5];
} PlatformStatus;

int platform_init(void);
void platform_shutdown(void);
const char *platform_asset_root(void);
const char *platform_data_root(void);
uint32_t platform_ticks_ms(void);

int platform_list_apps(PlatformApp *apps, int max_apps);
int platform_get_status(PlatformStatus *status);

int platform_launch_app(const char *id);
int platform_launch_source(const char *source);
int platform_launch_store(void);
int platform_launch_settings(void);
int platform_launch_home(void);
int platform_launch_network_settings(void);
int platform_launch_ambient(void);

#endif
