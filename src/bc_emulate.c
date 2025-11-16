#include <stdio.h>

int ensure_fallback_decoder_ready(const char* format) {
    printf("fallback activated for %s\n", format);
    return 0;
}
