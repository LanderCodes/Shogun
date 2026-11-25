#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void config_path(char *out, size_t size)
{
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        snprintf(out, size, "%s/tcksoft/shogun.cfg", xdg);
        return;
    }
    const char *home = getenv("HOME");
    if (!home || !*home) home = ".";
    snprintf(out, size, "%s/.config/tcksoft/shogun.cfg", home);
}

static void make_dirs(const char *path)
{
    char buf[1024];
    if (strlen(path) >= sizeof(buf)) return;
    strcpy(buf, path);
    for (char *p = buf + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(buf, 0700);
            *p = '/';
        }
    }
    mkdir(buf, 0700);
}

int config_load(void)
{
    char path[1024];
    config_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int value = -1;
    if (fscanf(f, "%d", &value) != 1) value = -1;
    fclose(f);
    return value;
}

void config_save(const Settings *settings)
{
    int bits = (settings->show_moves ? 1 : 0) |
               (settings->show_terrain ? 2 : 0) |
               (settings->improved_ai ? 4 : 0);

    char path[1024];
    char dir[1024];
    config_path(path, sizeof(path));
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (!slash) return;
    *slash = '\0';
    make_dirs(dir);

    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "%d\n", bits);
    fclose(f);
}

void settings_from_bits(Settings *settings, int bits)
{
    if (bits < 0) {
        settings->show_moves = 0;
        settings->show_terrain = 0;
        settings->improved_ai = 0;
        return;
    }
    settings->show_moves = (bits & 1) != 0;
    settings->show_terrain = (bits & 2) != 0;
    settings->improved_ai = (bits & 4) != 0;
}
