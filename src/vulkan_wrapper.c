/*
 * Minimal Vulkan wrapper stub for Eden.
 * Exposes logging and init hook; real Vulkan hooking is not implemented here.
 *
 * Builds to libvulkan.xeno_wrapper.so and provides xeno_init symbol.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "xeno_wrapper.h"
#include "feature_dump.h"
#include "feature_dump_generator.h"

/* Chosen log path on device (user requested): */
static const char *EDEN_LOG_DIR = "/sdcard/xclipse_logs";
static const char *EDEN_LOG_FILE = "/sdcard/xclipse_logs/eden_wrapper.log";
static const char *FEATURE_DUMP_FILE = "/sdcard/xclipse_logs/xeno_features.json";

/* Ensure log directory exists - try a naive approach using system() when available.
   In restricted runtime (sandbox) this call may fail silently; fallback is OK. */
static void ensure_log_dir(void)
{
#ifdef __ANDROID__
    /* attempt to create the directory if possible */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", EDEN_LOG_DIR);
    /* ignore return value */
    (void)system(cmd);
#else
    (void)EDEN_LOG_DIR;
#endif
}

/* Simple variadic logger */
static void eden_log(const char *fmt, ...)
{
    ensure_log_dir();
    FILE *f = fopen(EDEN_LOG_FILE, "a");
    if (!f) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    fprintf(f, "\n");
    va_end(ap);
    fclose(f);
}

/* Exported init hook */
void xeno_init(void)
{
    eden_log("[xeno] xeno_init called");
    eden_log("[xeno] writing feature dump to %s", FEATURE_DUMP_FILE);
    /* best-effort feature dump at first init */
    generate_feature_dump_to(FEATURE_DUMP_FILE);
    eden_log("[xeno] feature dump written");
}

/* Minimal exported symbol so loader finds something meaningful.
   Keep real Vulkan interception out of this simple example. */
__attribute__((visibility("default")))
void xeno_entry_point(void)
{
    eden_log("[xeno] xeno_entry_point invoked");
}

/* If you need additional exported helpers for Eden, add them below. */
