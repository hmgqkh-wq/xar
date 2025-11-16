/* src/xeno_wrapper.c
   Auto-detecting wrapper for Xclipse 940.
   Writes logs and a feature dump to the host emulator's external app storage.
   - Tries GameHub package: com.xiaoji.egggame
   - Tries Winlator paths
   - Tries Eden path
   - Falls back to /sdcard/xclipse_output/
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

static const char *candidate_bases[] = {
    /* GameHub (you told me) */
    "/sdcard/Android/media/com.xiaoji.egggame/files",
    "/sdcard/Android/data/com.xiaoji.egggame/files",
    /* Winlator typical media/data paths */
    "/sdcard/Android/media/com.winlator/files",
    "/sdcard/Android/data/com.winlator/files",
    /* Eden */
    "/sdcard/Android/media/dev.eden.eden_emulator/files",
    "/sdcard/Android/data/dev.eden.eden_emulator/files",
    /* Generic Winlator fallback */
    "/sdcard/Winlator",
    /* Public fallback */
    "/sdcard/xclipse_output",
    "/sdcard/xclipse"
};

static FILE *side_log_fp = NULL;
static char chosen_dir[1024] = {0};

/* ensure directory exists recursively */
static int mkdir_p(const char *path) {
    char tmp[1024];
    size_t len;
    int ret = 0;
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len-1] == '/') tmp[len-1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) && errno != EEXIST) ret = -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) && errno != EEXIST) ret = -1;
    return ret;
}

/* choose an output directory from candidates by trying to create it */
static const char* select_output_dir(void) {
    for (size_t i = 0; i < sizeof(candidate_bases)/sizeof(candidate_bases[0]); ++i) {
        const char *base = candidate_bases[i];
        if (!base) continue;
        if (mkdir_p(base) == 0) {
            /* create subfolder */
            snprintf(chosen_dir, sizeof(chosen_dir), "%s/xclipse_output", base);
            if (mkdir_p(chosen_dir) == 0) {
                return chosen_dir;
            }
        }
    }
    /* final fallback: try /data/local/tmp */
    const char *fallback = "/data/local/tmp/xclipse_output";
    mkdir_p(fallback);
    snprintf(chosen_dir, sizeof(chosen_dir), "%s", fallback);
    return chosen_dir;
}

static void xlogf(const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    /* stderr for immediate visibility */
    fprintf(stderr, "[XCLIPSE] %s\n", buf);

    if (!side_log_fp) {
        const char *d = chosen_dir[0] ? chosen_dir : select_output_dir();
        char path[1200];
        snprintf(path, sizeof(path), "%s/xeno_wrapper.log", d);
        side_log_fp = fopen(path, "a");
    }
    if (side_log_fp) {
        time_t t = time(NULL);
        struct tm tm;
        localtime_r(&t, &tm);
        char times[64]; strftime(times, sizeof(times), "%Y-%m-%d %H:%M:%S", &tm);
        fprintf(side_log_fp, "[%s] %s\n", times, buf);
        fflush(side_log_fp);
    }
}

/* write feature dump JSON */
static void write_feature_dump(void) {
    const char *d = chosen_dir[0] ? chosen_dir : select_output_dir();
    char path[1200];
    snprintf(path, sizeof(path), "%s/xeno_tune_report.json", d);
    FILE *f = fopen(path, "w");
    if (!f) {
        xlogf("failed to open tune report: %s", strerror(errno));
        return;
    }
    /* Minimal but useful device info; real runtime code would detect real hardware */
    fprintf(f, "{\n");
    fprintf(f, "  \"device\":\"Xclipse 940\",\n");
    fprintf(f, "  \"profile\":\"balanced-auto\",\n");
    fprintf(f, "  \"vulkan_target\":\"vulkan1.3\",\n");
    fprintf(f, "  \"rt_max_recursion\":2,\n");
    fprintf(f, "  \"meshlets_per_draw\":128,\n");
    fprintf(f, "  \"bc_hw\":true,\n");
    fprintf(f, "  \"notes\":\"placeholder detection — send this file to the tuner to refine settings\"\n");
    fprintf(f, "}\n");
    fflush(f);
    fclose(f);
    xlogf("feature dump written to %s", path);
}

/* ensure chosen_dir set on startup */
__attribute__((constructor))
void xeno_init(void) {
    const char *d = select_output_dir();
    xlogf("xeno_init called — chosen dir: %s", d);
    write_feature_dump();
    xlogf("xeno_init completed.");
}

/* close log on unloaded */
__attribute__((destructor))
void xeno_shutdown(void) {
    if (side_log_fp) {
        xlogf("xeno_shutdown called.");
        fclose(side_log_fp);
        side_log_fp = NULL;
    }
}
