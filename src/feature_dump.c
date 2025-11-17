#include <stdio.h>
#include <string.h>
#include "feature_dump.h"

/* Write a small JSON file with the features we report.
   This is intentionally small and static to avoid complex Vulkan queries
   inside CI/build-time. Runtime wrapper can update it if desired. */
void write_feature_dump(const char *out_path)
{
    FILE *f = fopen(out_path, "w");
    if (!f) return;
    fprintf(f, "{\n");
    fprintf(f, "  \"driver\": \"xeno_wrapper\",\n");
    fprintf(f, "  \"target\": \"vulkan1.3\",\n");
    fprintf(f, "  \"features\": {\n");
    fprintf(f, "    \"spirv_embedded\": true,\n");
    fprintf(f, "    \"pipeline_cache\": true,\n");
    fprintf(f, "    \"async_compile\": true,\n");
    fprintf(f, "    \"auto_tuning\": true,\n");
    fprintf(f, "    \"hud_overlay\": true\n");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");
    fclose(f);
}
