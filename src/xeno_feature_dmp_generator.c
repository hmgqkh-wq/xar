#include <stdio.h>

int main() {
    printf("Feature dump generator run.\n");
    FILE *f = fopen("feature_dump.txt", "w");
    fprintf(f, "Xclipse generator test\n");
    fclose(f);
    return 0;
}
