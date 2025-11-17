// src/vulkan_wrapper.c
// Corrected Vulkan wrapper entry points with proper function-pointer types.
// - Exports vkGetInstanceProcAddr and vkGetDeviceProcAddr with correct signatures
// - Loads real system libvulkan and forwards calls
// - Lightweight runtime logging and feature dump trigger (calls into feature_dump.h)

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

/*
  We avoid including <vulkan/vulkan.h> to keep the build simple across environments.
  Instead, define minimal types required for function pointer signatures:
*/
typedef void* PFN_vkVoidFunction;
typedef PFN_vkVoidFunction (*PFN_vkGetInstanceProcAddr_t)(void* /*VkInstance*/, const char* /*pName*/);
typedef PFN_vkVoidFunction (*PFN_vkGetDeviceProcAddr_t)(void* /*VkDevice*/, const char* /*pName*/);

static void *real_vulkan = NULL;
static PFN_vkGetInstanceProcAddr_t real_vkGetInstanceProcAddr = NULL;
static PFN_vkGetDeviceProcAddr_t real_vkGetDeviceProcAddr = NULL;

static FILE *g_log = NULL;
static int g_initialized = 0;

static const char *paths_to_try[] = {
    "/sdcard/xclipse/logs",
    "/storage/emulated/0/xclipse/logs",
    NULL
};

static void ensure_dir_recursive(const char *path) {
    if (!path || !*path) return;
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

static FILE* open_log_file(const char *filename) {
    for (int i = 0; paths_to_try[i]; ++i) {
        const char *base = paths_to_try[i];
        ensure_dir_recursive(base);
        char full[1024];
        int ok = snprintf(full, sizeof(full), "%s/%s", base, filename);
        if (ok <= 0 || ok >= (int)sizeof(full)) continue;
        FILE *f = fopen(full, "a");
        if (f) return f;
    }

    // Try common Android/data locations (best-effort)
    const char *pkg_candidates[] = {
        "com.winlator",
        "com.winalotor",
        "dev.eden.eden_emulator",
        "com.yuzu.yuzu",
        NULL
    };
    for (int i = 0; pkg_candidates[i]; ++i) {
        char dir[1024];
        int ok = snprintf(dir, sizeof(dir),
            "/storage/emulated/0/Android/data/%s/files/xclipse/logs",
            pkg_candidates[i]);
        if (ok <= 0 || ok >= (int)sizeof(dir)) continue;
        ensure_dir_recursive(dir);
        char full[1200];
        snprintf(full, sizeof(full), "%s/%s", dir, filename);
        FILE *f = fopen(full, "a");
        if (f) return f;
    }

    // Last resort: current directory
    return fopen(filename, "a");
}

static void log_printf(const char *fmt, ...) {
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);

    char buf[2048];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    int n = snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d] ",
                     tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                     tm.tm_hour, tm.tm_min, tm.tm_sec);
    vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] != '\n') {
        buf[len] = '\n';
        buf[len+1] = '\0';
    }
    fprintf(g_log, "%s", buf);
    fflush(g_log);
    va_end(ap);
}

static void open_global_log(void) {
    if (g_log) return;
    g_log = open_log_file("xclipse_runtime.log");
    if (!g_log) {
        g_log = stderr;
    }
}

/* Load the real system Vulkan library and resolve pointers */
static void load_real_vulkan(void) {
    if (real_vulkan) return;

    const char *candidates[] = {
        "libvulkan.so",
        "libvulkan.so.1",
        NULL
    };
    for (int i = 0; candidates[i]; ++i) {
        real_vulkan = dlopen(candidates[i], RTLD_NOW | RTLD_LOCAL);
        if (real_vulkan) break;
    }
    if (!real_vulkan) {
        log_printf("ERROR: cannot open system libvulkan: %s", dlerror());
        return;
    }

    // Resolve function pointers with correct types
    real_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr_t)dlsym(real_vulkan, "vkGetInstanceProcAddr");
    real_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr_t)dlsym(real_vulkan, "vkGetDeviceProcAddr");

    log_printf("Loaded real libvulkan: %p vkGetInstanceProcAddr=%p vkGetDeviceProcAddr=%p",
               real_vulkan, (void*)real_vkGetInstanceProcAddr, (void*)real_vkGetDeviceProcAddr);
}

__attribute__((constructor))
static void xeno_init(void) {
    open_global_log();
    log_printf("xeno_init() starting wrapper init");

    load_real_vulkan();
    if (!real_vulkan) {
        log_printf("xeno_init: real_vulkan not loaded; wrapper will be passive");
    } else {
        log_printf("xeno_init: real_vulkan loaded");
    }

    // Fire off a feature dump (best-effort, non-blocking)
    fdump_features("feature_dump_runtime.txt");
    log_printf("feature dump triggered");
    g_initialized = 1;
}

__attribute__((destructor))
static void xeno_fini(void) {
    log_printf("xeno_fini() shutting down");
    if (g_log && g_log != stderr) fclose(g_log);
    g_log = NULL;
    if (real_vulkan) {
        dlclose(real_vulkan);
        real_vulkan = NULL;
    }
}

/*
  Exported symbols expected by Vulkan loader:
    PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName)
    PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char* pName)

  We use void* for PFN_vkVoidFunction to avoid depending on Vulkan headers here.
*/

void* vkGetInstanceProcAddr(void* instance, const char* pName) {
    if (!g_initialized) open_global_log();
    if (!real_vkGetInstanceProcAddr) {
        load_real_vulkan();
    }
    if (!real_vkGetInstanceProcAddr) {
        log_printf("vkGetInstanceProcAddr: real function missing for %s", (pName ? pName : "(null)"));
        return NULL;
    }

    // Forward the call to the real loader
    PFN_vkVoidFunction f = real_vkGetInstanceProcAddr(instance, pName);
    // Optionally log a few critical function lookups
    if (pName && (strcmp(pName, "vkCreateInstance") == 0 || strcmp(pName, "vkEnumerateInstanceExtensionProperties") == 0)) {
        log_printf("vkGetInstanceProcAddr asked for %s -> %p", pName, (void*)f);
    }
    return (void*)f;
}

void* vkGetDeviceProcAddr(void* device, const char* pName) {
    if (!g_initialized) open_global_log();
    if (!real_vkGetDeviceProcAddr) {
        load_real_vulkan();
    }
    if (!real_vkGetDeviceProcAddr) {
        log_printf("vkGetDeviceProcAddr: real function missing for %s", (pName ? pName : "(null)"));
        return NULL;
    }

    PFN_vkVoidFunction f = real_vkGetDeviceProcAddr(device, pName);
    if (pName && (strcmp(pName, "vkCreateDevice") == 0 || strcmp(pName, "vkQueueSubmit") == 0)) {
        log_printf("vkGetDeviceProcAddr asked for %s -> %p", pName, (void*)f);
    }
    return (void*)f;
}
