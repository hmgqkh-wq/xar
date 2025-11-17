/*
 * simple logging wrapper for Eden/Winlator usage
 * provides xeno_init() entrypoint (no Vulkan calls)
 * writes logs to Eden's accessible folder so you can inspect runtime behavior.
 *
 * This is a conservative, robust stub. It avoids calling real Vulkan loader functions
 * so linking/calling errors (signature mismatches) do not occur.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #define PATH_SEP "\\"
#else
  #define PATH_SEP "/"
#endif

/* The path used by Eden emulator to store GPU driver logs.
   If your Eden package path differs, set EDEN_BASE via manifest or edit here accordingly.
   Observed Eden package id earlier: dev.eden.eden_emulator
*/
static const char *eden_base = "/storage/emulated/0/Android/data/dev.eden.eden_emulator/files";
static const char *driver_folder = "gpu_drivers" ;
static const char *driver_name = "xclipse-940";

static void ensure_dir_log_path(char *path_out, size_t len)
{
    /* Build the logs directory path */
    /* path_out must be large enough */
    snprintf(path_out, len, "%s%s%s%s%s%slogs", eden_base, PATH_SEP, driver_folder, PATH_SEP, driver_name, PATH_SEP);
}

/* create directories (best-effort). We cannot reliably mkdir -p on all Android devices
   from native C without using POSIX dir functions; do best-effort using system().
*/
static void try_mkdirs(const char *path)
{
    /* Use /system/bin/sh -c "mkdir -p 'path'" if available */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s' 2>/dev/null || true", path);
    system(cmd);
}

/* get simple timestamp */
static void timestamp(char *buf, size_t n)
{
    time_t t = time(NULL);
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    strftime(buf, n, "%Y-%m-%d %H:%M:%S", &tm);
}

/* The manifest data we want to dump into a file so Eden can show it */
static const char *manifest_dump_header = "=== Xclipse wrapper runtime manifest dump ===\n";

/* writes a small feature dump file and a wrapper.log file */
static void write_logs()
{
    char logs_dir[512];
    ensure_dir_log_path(logs_dir, sizeof(logs_dir));
    try_mkdirs(logs_dir);

    char logfile[768];
    char dumpfile[768];
    snprintf(logfile, sizeof(logfile), "%s%swrapper.log", logs_dir, PATH_SEP);
    snprintf(dumpfile, sizeof(dumpfile), "%s%sfeature_dump.txt", logs_dir, PATH_SEP);

    FILE *f = fopen(logfile, "a");
    if (f) {
        char ts[64];
        timestamp(ts, sizeof(ts));
        fprintf(f, "[%s] xeno_wrapper starting up\n", ts);
        fprintf(f, "[%s] manifest target: vulkan1.3\n", ts);
        fprintf(f, "[%s] driver filename: libxeno_wrapper.so\n", ts);
        fprintf(f, "[%s] path: %s\n", ts, logs_dir);
        fprintf(f, "[%s] Note: this wrapper intentionally does not call into Vulkan loader to avoid ABI mismatches\n", ts);
        fflush(f);
        fclose(f);
    }

    FILE *d = fopen(dumpfile, "w");
    if (d) {
        fprintf(d, "%s\n", manifest_dump_header);
        fprintf(d, "target: vulkan1.3\n");
        fprintf(d, "driver: libxeno_wrapper.so\n");
        fprintf(d, "features (selected):\n");
        fprintf(d, "  - descriptor_indexing\n");
        fprintf(d, "  - buffer_device_address\n");
        fprintf(d, "  - timeline_semaphores\n");
        fprintf(d, "  - dynamic_rendering\n");
        fprintf(d, "  - robust_buffer_access\n");
        fprintf(d, "extensions (sample):\n");
        fprintf(d, "  - VK_KHR_acceleration_structure\n");
        fprintf(d, "  - VK_KHR_ray_tracing_pipeline\n");
        fprintf(d, "  - VK_EXT_descriptor_indexing\n");
        fprintf(d, "  - VK_KHR_dynamic_rendering\n");
        fprintf(d, "\nNote: This is a feature dump produced by wrapper stub. For a full runtime dump, replace this generator with a real runtime that queries vkEnumerateInstanceExtensionProperties etc.\n");
        fclose(d);
    }
}

/* This is the entrypoint name you told me manifest uses: xeno_init */
int xeno_init(void)
{
    /* Write logs immediately (best effort) so Eden can collect them */
    write_logs();
    return 0; /* success */
}

/* Expose a small function to allow explicit log generation by the emulator, if it dlopens the lib */
int xeno_generate_feature_dump(void)
{
    write_logs();
    return 0;
}

/* Provide a destructor to append shutdown note */
__attribute__((destructor))
static void xeno_shutdown_note(void)
{
    char logs_dir[512];
    ensure_dir_log_path(logs_dir, sizeof(logs_dir));
    char logfile[768];
    snprintf(logfile, sizeof(logfile), "%s%swrapper.log", logs_dir, PATH_SEP);
    FILE *f = fopen(logfile, "a");
    if (f) {
        char ts[64];
        timestamp(ts, sizeof(ts));
        fprintf(f, "[%s] xeno_wrapper shutdown\n", ts);
        fclose(f);
    }
}
