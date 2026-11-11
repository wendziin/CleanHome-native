#ifndef CLEANHOME_CONFIG_H
#define CLEANHOME_CONFIG_H

typedef struct {
    char wallpaper[64];
    char theme[32];
    char dock[2048];
    int seeded;
} Config;

void config_defaults(Config *config);
void config_load(Config *config, const char *path);
void config_save(const Config *config, const char *path);

#endif
