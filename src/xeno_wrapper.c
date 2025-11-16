#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define LOG_FILE "/sdcard/xclipse/xeno_wrapper.log"
#define TUNE_FILE "/sdcard/xclipse/xeno_tune_report.json"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void write_log(const char *msg) {
    pthread_mutex_lock(&lock);
    FILE *f = fopen(LOG_FILE, "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
    pthread_mutex_unlock(&lock);
}

static void write_report() {
    FILE *f = fopen(TUNE_FILE, "w");
    if (!f) return;

    fprintf(f,
        "{\n"
        "  \"device\": \"Xclipse 940\",\n"
        "  \"profile\": \"balanced\",\n"
        "  \"rt_recursion\": 2,\n"
        "  \"meshlets\": 128,\n"
        "  \"vrs\": \"2x2\",\n"
        "  \"ser\": true\n"
        "}\n"
    );
    fclose(f);
}

__attribute__((visibility("default")))
void xeno_init() {
    write_log("xeno_init() called");
    write_report();
}
