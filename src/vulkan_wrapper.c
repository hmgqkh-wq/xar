// src/vulkan_wrapper.c
// Minimal Vulkan loader wrapper that:
// - loads real libvulkan (dlopen)
// - exports vkGetInstanceProcAddr and vkGetDeviceProcAddr
// - on library load performs feature detection and logging to files
// - safe for Android (writes to /sdcard/xclipse/logs and Android/data locations when available)

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "feature_dump.h"

typedef void* (*PFN_vkGetInstanceProcAddr)(void);
typedef void* (*PFN_vkGetDeviceProcAddr)(void);

static void *real_vulkan = NULL;
static PFN_vkGetInstanceProcAddr real_vkGetInstanceProcAddr = NULL;
static PFN_vkGetDeviceProcAddr real_vkGetDeviceProcAddr = NULL;

static FILE *g_log = NULL;
static int g_initialized = 0;

static const char *paths_to_try[] = {
    "/sdcard/xclipse/logs",
    "/storage/emulated/0/xclipse/logs",
    NULL
};

static void ensure_dir_recursive(const char *path) {
    char tmp[512];
    strncpy(tmp, path, sizeof(tmp));
    tmp[sizeof(tmp)-1] = 0;
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0775);
            *p = '/';
        }
    }
    mkdir(tmp, 0775);
}

static FILE* open_log_file(const char *filename) {
    // Try multiple locations for robustness
    for (int i=0; paths_to_try[i]; ++i) {
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", paths_to_try[i], filename);
        ensure_dir_recursive(paths_to_try[i]);
        FILE *f = fopen(full, "a");
        if (f) return f;
    }

    // Try Android/data fallback(s) with possible package names common for emulators
    const char *pkg_candidates[] = {
        "com.winlator",        // hypothetical
        "com.winalotor",      // some misspellings
        "dev.eden.eden_emulator",
        "com.yuzu.yuzu",      // other possibilities
        NULL
    };
    for (int i=0; pkg_candidates[i]; ++i) {
        char full[1024];
        snprintf(full, sizeof(full), "/storage/emulated/0/Android/data/%s/files/xclipse/logs", pkg_candidates[i]);
        ensure_dir_recursive(full);
        snprintf(full, sizeof(full), "%s/%s", full, filename);
        FILE *f = fopen(full, "a");
        if (f) return f;
    }

    // Last resort: /tmp
    FILE *f = fopen(filename, "a");
    return f;
}

static void log_printf(const char *fmt, ...) {
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);

    char buf[1024];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    int n = snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d] ",
                     tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                     tm.tm_hour, tm.tm_min, tm.tm_sec);

    vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
    fprintf(g_log, "%s\n", buf);
    fflush(g_log);
    va_end(ap);
}

static void open_global_log(void) {
    if (g_log) return;
    g_log = open_log_file("xclipse_runtime.log");
    if (!g_log) {
        // fallback to stderr
        g_log = stderr;
    }
}

static void load_real_vulkan(void) {
    if (real_vulkan) return;
    // Common system Vulkan library names
    const char *candidates[] = {
        "libvulkan.so",
        "libvulkan.so.1",
        NULL
    };
    for (int i=0; candidates[i]; ++i) {
        real_vulkan = dlopen(candidates[i], RTLD_NOW | RTLD_LOCAL);
        if (real_vulkan) break;
    }
    if (!real_vulkan) {
        log_printf("ERROR: cannot open system libvulkan");
        return;
    }
    real_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(real_vulkan, "vkGetInstanceProcAddr");
    real_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)dlsym(real_vulkan, "vkGetDeviceProcAddr");
    log_printf("Loaded real libvulkan: %p vkInst=%p vkDev=%p", real_vulkan, real_vkGetInstanceProcAddr, real_vkGetDeviceProcAddr);
}

__attribute__((constructor))
static void xeno_init(void) {
    open_global_log();
    log_printf("xeno_init() entry — initializing wrapper");

    load_real_vulkan();
    if (!real_vulkan) {
        log_printf("xeno_init: real_vulkan not loaded; wrapper will be passive");
    } else {
        log_printf("xeno_init: real_vulkan OK");
    }

    // perform feature dump to file (non-blocking best-effort)
    fdump_features("feature_dump.txt");
    log_printf("feature dump requested");

    g_initialized = 1;
}

__attribute__((destructor))
static void xeno_fini(void) {
    log_printf("xeno_fini() — shutting down wrapper");
    if (g_log && g_log != stderr) fclose(g_log);
    g_log = NULL;
    if (real_vulkan) dlclose(real_vulkan);
    real_vulkan = NULL;
}

// Exported symbols expected by Vulkan loader
void* vkGetInstanceProcAddr(void) {
    if (!g_initialized) open_global_log();
    if (!real_vkGetInstanceProcAddr) {
        load_real_vulkan();
    }
    if (!real_vkGetInstanceProcAddr) {
        log_printf("vkGetInstanceProcAddr: real function missing, returning NULL");
        return NULL;
    }
    // forward directly
    return real_vkGetInstanceProcAddr(NULL);
}

void* vkGetDeviceProcAddr(void) {
    if (!g_initialized) open_global_log();
    if (!real_vkGetDeviceProcAddr) {
        load_real_vulkan();
    }
    if (!real_vkGetDeviceProcAddr) {
        log_printf("vkGetDeviceProcAddr: real function missing, returning NULL");
        return NULL;
    }
    return real_vkGetDeviceProcAddr(NULL);
}
