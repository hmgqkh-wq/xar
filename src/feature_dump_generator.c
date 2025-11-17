#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "feature_dump_generator.h"
#include "feature_dump.h"

/* Wrapper that ensures directory and calls the simple writer. */
static int ensure_parent_dir(const char *path)
{
    /* On Android the CI may create directories externally; keep minimal here.
       This function purposely does not implement mkdir -p logic to avoid
       needing extra system calls in cross-compile environment. */
    (void)path;
    return 0;
}

int generate_feature_dump_to(const char *path)
{
    if (!path) return -1;
    ensure_parent_dir(path);
    write_feature_dump(path);
    return 0;
}
