// src/feature_dump.c
#include <stdio.h>
#include <vulkan/vulkan.h>
#include "xeno_wrapper.h"

// This simple function can be called by the wrapper at startup or when instance created.
// For brevity it just writes a small static summary file (already done in constructor).
// You can expand this to query via vkEnumerate... if you prefer.

void xeno_dump_features_simple(void)
{
    // Nothing complex here — main wrapper already writes features file at constructor.
    // Left as placeholder so future code can call it from inside wrappers.
}
