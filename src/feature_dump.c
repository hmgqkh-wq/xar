// src/feature_dump.c
#include "feature_dump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *paths_to_try[] = {
    "/sdcard/xclipse/logs",
    "/storage/emulated/0/xclipse/logs",
    NULL
};

static void ensure_dir_recursive(const char *path) {
    char tmp[512];
    strncpy(tmp, path, sizeof(tmp));
    tmp[sizeof(tmp)-1] = 0;
    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0775);
            *p = '/';
        }
    }
    mkdir(tmp, 0775);
}

static FILE* open_fallback(const char *filename) {
    for (int i = 0; paths_to_try[i]; ++i) {
        ensure_dir_recursive(paths_to_try[i]);
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", paths_to_try[i], filename);
        FILE *f = fopen(full, "w");
        if (f) return f;
    }

    // fallback to current dir
    return fopen(filename, "w");
}

void fdump_append(const char *filename, const char *line) {
    FILE *f = open_fallback(filename);
    if (!f) return;
    fprintf(f, "%s\n", line);
    fflush(f);
    fclose(f);
}

void fdump_features(const char *filename) {
    FILE *f = open_fallback(filename);
    if (!f) return;

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);

    fprintf(f, "Xclipse feature dump\n");
    fprintf(f, "Timestamp: %04d-%02d-%02d %02d:%02d:%02d\n",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);

    // Environment
    char *user = getenv("USER");
    if (!user) user = getenv("LOGNAME");
    if (!user) user = "unknown";
    fprintf(f, "USER: %s\n", user);

    // Basic system info heuristics
    fprintf(f, "OS: Android (detected by wrapper)\n");
    fprintf(f, "Filesystem checks:\n");

    // List presence of common files
    const char *check[] = {
        "/system/build.prop",
        "/vendor/build.prop",
        "/proc/cpuinfo",
        "/proc/version",
        NULL
    };
    for (int i=0; check[i]; ++i) {
        struct stat st;
        if (stat(check[i], &st) == 0) {
            fprintf(f, "  %s : exists\n", check[i]);
        } else {
            fprintf(f, "  %s : missing\n", check[i]);
        }
    }

    fprintf(f, "\nNote: Vulkan-specific detection requires loader present and may be produced later at runtime.\n");

    fflush(f);
    fclose(f);
}
