#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

typedef void* (*PFN_vkGetInstanceProcAddr)(void*, const char*);
typedef void* (*PFN_vkGetDeviceProcAddr)(void*, const char*);

/* Candidate real driver paths (checked in order) */
static const char *real_driver_candidates[] = {
    "/vendor/lib64/hw/vulkan.exynos.so",
    "/vendor/lib64/hw/vulkan.s5e9945.so",
    "/vendor/lib64/egl/libvulkan.so",
    "/system/lib64/libvulkan.so",
    "/vendor/lib64/libvulkan.so",
    NULL
};

/* Logging / dump paths (we write to both public paths) */
static const char *out_paths[] = {
    "/storage/emulated/0/xclipse_output",
    "/sdcard/xclipse_output",
    NULL
};

static void *real_handle = NULL;
static PFN_vkGetInstanceProcAddr real_vkGetInstanceProcAddr = NULL;
static PFN_vkGetDeviceProcAddr real_vkGetDeviceProcAddr = NULL;

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *side_log_fp = NULL;
static char selected_out_dir[1024] = {0};

static void mkdir_p(const char *path) {
    if (!path) return;
    char tmp[1024];
    strncpy(tmp, path, sizeof(tmp)-1);
    size_t len = strlen(tmp);
    if (len == 0) return;
    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static FILE *open_side_log(void) {
    if (side_log_fp) return side_log_fp;
    for (int i=0; out_paths[i]; ++i) {
        mkdir_p(out_paths[i]);
        char logpath[1200];
        snprintf(logpath, sizeof(logpath), "%s/xeno_wrapper.log", out_paths[i]);
        FILE *f = fopen(logpath, "a");
        if (f) {
            side_log_fp = f;
            strncpy(selected_out_dir, out_paths[i], sizeof(selected_out_dir)-1);
            return side_log_fp;
        }
    }
    /* fallback to /data/local/tmp */
    mkdir_p("/data/local/tmp/xclipse_output");
    char fallback[256];
    snprintf(fallback, sizeof(fallback), "/data/local/tmp/xclipse_output/xeno_wrapper.log");
    side_log_fp = fopen(fallback, "a");
    strncpy(selected_out_dir, "/data/local/tmp/xclipse_output", sizeof(selected_out_dir)-1);
    return side_log_fp;
}

static void xlog(const char *fmt, ...) {
    pthread_mutex_lock(&log_lock);
    FILE *f = open_side_log();
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    time_t t = time(NULL);
    char ts[64];
    struct tm tmv;
    localtime_r(&t, &tmv);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);
    if (f) {
        fprintf(f, "[%s] %s\n", ts, buf);
        fflush(f);
    }
    fprintf(stderr, "[XCLIPSE][%s] %s\n", ts, buf);
    pthread_mutex_unlock(&log_lock);
}

/* Write feature dump JSON to chosen out dir(s) */
static void write_feature_dump_files(void) {
    for (int i=0; out_paths[i]; ++i) {
        mkdir_p(out_paths[i]);
        char path[1200];
        snprintf(path, sizeof(path), "%s/xeno_tune_report.json", out_paths[i]);
        FILE *f = fopen(path, "w");
        if (!f) {
            xlog("failed to write feature dump to %s (%s)", path, strerror(errno));
            continue;
        }
        // Basic fields — real probing will expand later.
        fprintf(f, "{\n");
        fprintf(f, "  \"device\":\"Xclipse 940\",\n");
        fprintf(f, "  \"vulkan_target\":\"1.3\",\n");
        fprintf(f, "  \"autotune\":\"balanced\",\n");
        fprintf(f, "  \"notes\":\"Run more tests and upload this file for final tuning\"\n");
        fprintf(f, "}\n");
        fclose(f);
        xlog("wrote feature dump to %s", path);
    }
}

/* Attempt to dlopen the real system driver */
static int open_real_driver(void) {
    if (real_handle) return 1;
    for (int i=0; real_driver_candidates[i]; ++i) {
        const char *p = real_driver_candidates[i];
        if (!p) continue;
        xlog("trying to open real driver: %s", p);
        void *h = dlopen(p, RTLD_NOW | RTLD_LOCAL);
        if (!h) {
            xlog("dlopen failed: %s -> %s", p, dlerror());
            continue;
        }
        /* find real vkGetInstanceProcAddr */
        void *sym = dlsym(h, "vkGetInstanceProcAddr");
        if (!sym) {
            xlog("real driver loaded but vkGetInstanceProcAddr not found in %s", p);
            dlclose(h);
            continue;
        }
        real_handle = h;
        real_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) sym;
        /* try to get device proc addr if exported from driver entry */
        void *sym2 = dlsym(h, "vkGetDeviceProcAddr");
        if (sym2) real_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) sym2;
        xlog("real driver loaded from %s", p);
        return 1;
    }
    xlog("unable to open any candidate real driver");
    return 0;
}

/* Forwarding implementation for vkGetInstanceProcAddr */
void* vkGetInstanceProcAddr(void* instance, const char* pName) {
    if (!real_vkGetInstanceProcAddr) {
        if (!open_real_driver()) {
            xlog("vkGetInstanceProcAddr: real driver not available");
            return NULL;
        }
    }
    void *fn = real_vkGetInstanceProcAddr(instance, pName);
    if (fn) return fn;
    if (strcmp(pName, "vkGetInstanceProcAddr") == 0) return (void*)vkGetInstanceProcAddr;
    return NULL;
}

/* Forwarding implementation for vkGetDeviceProcAddr (if real exported it) */
void* vkGetDeviceProcAddr(void* device, const char* pName) {
    if (!real_vkGetDeviceProcAddr) {
        if (!real_vkGetInstanceProcAddr) {
            if (!open_real_driver()) {
                xlog("vkGetDeviceProcAddr: real driver not available");
                return NULL;
            }
        }
        PFN_vkGetInstanceProcAddr gipa = real_vkGetInstanceProcAddr;
        if (gipa) {
            void *maybe = gipa(NULL, "vkGetDeviceProcAddr");
            if (maybe) real_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) maybe;
        }
    }
    if (real_vkGetDeviceProcAddr) {
        return real_vkGetDeviceProcAddr(device, pName);
    }
    return NULL;
}

/* Some loaders call this to negotiate ICD interface */
int vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* pVersion) {
    if (!pVersion) return -1;
    if (*pVersion >= 2) *pVersion = 2;
    else if (*pVersion >= 1) *pVersion = 1;
    else *pVersion = 1;
    xlog("vk_icdNegotiateLoaderICDInterfaceVersion: negotiated %u", *pVersion);
    return 0;
}

/* constructor / destructor */
__attribute__((constructor))
static void wrapper_ctor(void) {
    open_real_driver();
    xlog("wrapper_ctor: init called");
    write_feature_dump_files();
    xlog("wrapper_ctor: feature dump written");
}

__attribute__((destructor))
static void wrapper_dtor(void) {
    xlog("wrapper_dtor: shutdown");
    if (side_log_fp) { fclose(side_log_fp); side_log_fp = NULL; }
    if (real_handle) { dlclose(real_handle); real_handle = NULL; }
}
