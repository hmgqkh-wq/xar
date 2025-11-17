/*
 * vulkan_wrapper.c
 *
 * Minimal wrapper for Eden / Winlator usage.
 * Exports xeno_init() which Eden calls when loading driver.
 * On init it writes a feature dump JSON into Eden-accessible folders.
 *
 * Exported symbol: xeno_init
 *
 * Build into: libvulkan.xeno_wrapper.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xeno_wrapper.h"

/* forward to feature dump writer */
int write_feature_to_eden_locations(void);

static void write_runtime_log(const char *message)
{
    /* Attempt to write a short runtime log next to the feature dumps */
    const char *candidates[] = {
        "/storage/emulated/0/Android/data/dev.eden.eden_emulator/files/gpu_drivers/xclipse_runtime.log",
        "/sdcard/Android/data/dev.eden.eden_emulator/files/gpu_drivers/xclipse_runtime.log",
        "/storage/emulated/0/Android/data/dev.eden.eden_emulator/files/xclipse_runtime.log",
        "/sdcard/Android/data/dev.eden.eden_emulator/files/xclipse_runtime.log",
        "xclipse_runtime.log",
        NULL
    };
    FILE *f = NULL;
    const char **p = candidates;
    for (; *p; ++p) {
        f = fopen(*p, "a");
        if (f) {
            fprintf(f, "%s\n", message);
            fclose(f);
            return;
        }
    }
    /* last fallback: output to stdout (visible in some logs) */
    fprintf(stdout, "xclipse_wrapper: %s\n", message);
}

/* Entry point expected by Eden/Winlator */
void xeno_init(void)
{
    write_runtime_log("xeno_init() called: starting feature dump generation");
    if (write_feature_to_eden_locations() == 0) {
        write_runtime_log("feature dump successfully written to eden locations");
    } else {
        write_runtime_log("failed to write feature dump to eden locations; wrote to cwd if possible");
    }
}

/* Provide a weak symbol so that if the loader expects others, we don't fail linking.
   This keeps the .so small and safe. */
__attribute__((weak)) void xeno_init_autostart(void) { }
