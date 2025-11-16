#include <stdio.h>

int main() {
    FILE* f = fopen("feature_dump.txt", "w");
    if (!f) return 1;

    fprintf(f, "=== Xclipse 940 Feature Dump (Placeholder) ===\n");
    fprintf(f, "This is the PC-side placeholder.\n");
    fprintf(f, "Real dumps will come from the wrapper on your Android device.\n");

    fclose(f);
    return 0;
}
