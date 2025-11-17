/*
 * feature_dump.c
 * Minimal feature-dump generator that writes the JSON manifest (Vulkan 1.3) to the
 * emulator-accessible locations so Eden can pick it up.
 *
 * Writes to:
 *  - /storage/emulated/0/Android/data/dev.eden.eden_emulator/files/gpu_drivers/
 *  - /sdcard/Android/data/dev.eden.eden_emulator/files/gpu_drivers/
 *
 * If those don't exist, falls back to current directory where possible.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static int mkdir_p(const char *path)
{
    char tmp[1024];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (len == 0) return -1;
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) && errno != EEXIST) return -1;
    return 0;
}

/* Minimal JSON dump contents (Vulkan 1.3 tailored) */
static const char *dump_json =
"{\n"
"  \"name\": \"xclipse-940\",\n"
"  \"target\": \"vulkan1.3\",\n"
"  \"supported\": {\n"
"    \"spirv_embedded\": true,\n"
"    \"mesh_shading\": true,\n"
"    \"variable_rate_shading\": true,\n"
"    \"descriptor_indexing\": true,\n"
"    \"buffer_device_address\": true,\n"
"    \"timeline_semaphores\": true,\n"
"    \"dynamic_rendering\": true,\n"
"    \"robust_buffer_access\": true\n"
"  }\n"
"}\n";

static int write_file(const char *path, const char *data)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(data, 1, strlen(data), f);
    fclose(f);
    return 0;
}

int produce_feature_dump(const char *outdir_base)
{
    char path[1024];
    int rc = 0;

    if (!outdir_base || strlen(outdir_base) == 0) return -1;

    if (mkdir_p(outdir_base) != 0) {
        /* non-fatal; attempt anyway */
    }

    snprintf(path, sizeof(path), "%s/xclipse_940_feature_dump.json", outdir_base);
    rc = write_file(path, dump_json);
    return rc;
}

/* utility tries multiple likely locations for Eden */
int write_feature_to_eden_locations(void)
{
    const char *candidates[] = {
        "/storage/emulated/0/Android/data/dev.eden.eden_emulator/files/gpu_drivers",
        "/sdcard/Android/data/dev.eden.eden_emulator/files/gpu_drivers",
        "/storage/emulated/0/Android/data/dev.eden.eden_emulator/files",
        "/sdcard/Android/data/dev.eden.eden_emulator/files",
        "/storage/emulated/0/Android/data/dev.eden.eden_emulator",
        "/sdcard/Android/data/dev.eden.eden_emulator",
        NULL
    };
    const char **p = candidates;
    int ok = 0;
    for (; *p; ++p) {
        if (produce_feature_dump(*p) == 0) ok = 1;
    }
    if (!ok) {
        /* fallback to current directory */
        if (produce_feature_dump("./") == 0) ok = 1;
    }
    return ok ? 0 : -1;
}
