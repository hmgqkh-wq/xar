#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

static FILE* log_file = NULL;

__attribute__((constructor))
void xeno_init() {
    log_file = fopen("xclipse_wrapper_log.txt", "w");

    if (!log_file) return;

    fprintf(log_file, "=== Xclipse 940 Wrapper Initialized ===\n");
    fprintf(log_file, "Manifest-based enhancements active.\n");
    fprintf(log_file, "Collecting Vulkan device data...\n\n");
    fflush(log_file);
}

__attribute__((destructor))
void xeno_shutdown() {
    if (log_file) {
        fprintf(log_file, "=== Wrapper shutdown ===\n");
        fclose(log_file);
    }
}
