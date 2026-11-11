#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' ||
                       end[-1] == '\r' || end[-1] == '\n')) {
        end--;
    }
    *end = '\0';
    return s;
}

void config_defaults(Config *config) {
    memset(config, 0, sizeof(*config));
    snprintf(config->wallpaper, sizeof(config->wallpaper), "espaco-profundo");
    snprintf(config->theme, sizeof(config->theme), "obsidian");
    config->dock[0] = '\0';
    config->seeded = 0;
}

void config_load(Config *config, const char *path) {
    config_defaults(config);
    FILE *file = fopen(path, "r");
    if (!file) return;
    char line[4096];
    while (fgets(line, sizeof(line), file)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim(line);
        char *value = trim(eq + 1);
        if (strcmp(key, "wallpaper") == 0) {
            snprintf(config->wallpaper, sizeof(config->wallpaper), "%s", value);
        } else if (strcmp(key, "theme") == 0) {
            snprintf(config->theme, sizeof(config->theme), "%s", value);
        } else if (strcmp(key, "dock") == 0) {
            snprintf(config->dock, sizeof(config->dock), "%s", value);
        } else if (strcmp(key, "seeded") == 0) {
            config->seeded = atoi(value) != 0;
        }
    }
    fclose(file);
}

void config_save(const Config *config, const char *path) {
    FILE *file = fopen(path, "w");
    if (!file) return;
    fprintf(file, "wallpaper=%s\n", config->wallpaper);
    fprintf(file, "theme=%s\n", config->theme);
    fprintf(file, "dock=%s\n", config->dock);
    fprintf(file, "seeded=%d\n", config->seeded);
    fclose(file);
}
