// src/feature_dump_generator.c
// Small utility that can be built as part of the repo to exercise fdump_features at build-time or run-time.
// Not required by the wrapper; included for convenience.
#include "feature_dump.h"
#include <stdio.h>

int main(int argc, char **argv) {
    const char *filename = "feature_dump_runtime.txt";
    if (argc > 1) filename = argv[1];
    fdump_features(filename);
    printf("Feature dump written to %s (if filesystem allowed)\n", filename);
    return 0;
}
