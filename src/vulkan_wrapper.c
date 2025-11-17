#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <vulkan/vulkan.h>

static void *real_vulkan = NULL;
static FILE *logfile = NULL;

static void init_log()
{
    if (logfile) return;

    // ALWAYS write logs here (Android-safe, Winlator-safe)
    logfile = fopen("/storage/emulated/0/xclipse_output/wrapper_log.txt", "a");

    // fallback for non-permission environments
    if (!logfile)
        logfile = fopen("/data/local/tmp/xclipse_wrapper_log.txt", "a");

    if (logfile)
    {
        fprintf(logfile, "=== Xclipse 940 Vulkan Wrapper Init ===\n");
        fflush(logfile);
    }
}

static void logf(const char *fmt, ...)
{
    init_log();
    if (!logfile) return;

    va_list args;
    va_start(args, fmt);
    vfprintf(logfile, fmt, args);
    fprintf(logfile, "\n");
    fflush(logfile);
    va_end(args);
}

static void load_real_vulkan()
{
    if (real_vulkan) return;

    init_log();

    const char *paths[] = {
        "libvulkan.so",
        "/system/lib64/libvulkan.so",
        "/system/lib64/hw/vulkan.exynos.so",
        "/apex/com.android.vulkan/lib64/libvulkan.so",
        "/vendor/lib64/hw/vulkan.exynos.so"
    };

    for (int i = 0; i < sizeof(paths)/sizeof(paths[0]); i++)
    {
        real_vulkan = dlopen(paths[i], RTLD_NOW | RTLD_LOCAL);
        if (real_vulkan)
        {
            logf("[Wrapper] Loaded real Vulkan: %s", paths[i]);
            return;
        }
    }

    logf("[Wrapper] ERROR: Failed to load any Vulkan implementation!");
}

/* ================================
   REQUIRED EXPORTED FUNCTION
   ================================ */

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance instance, const char* pName)
{
    load_real_vulkan();

    if (!real_vulkan)
    {
        logf("[Wrapper] FATAL: no real Vulkan backend!");
        return NULL;
    }

    static PFN_vkGetInstanceProcAddr real_vkGetInstanceProcAddr = NULL;

    if (!real_vkGetInstanceProcAddr)
    {
        real_vkGetInstanceProcAddr =
            (PFN_vkGetInstanceProcAddr)dlsym(real_vulkan, "vkGetInstanceProcAddr");

        if (!real_vkGetInstanceProcAddr)
        {
            logf("[Wrapper] ERROR: Failed to resolve real vkGetInstanceProcAddr");
            return NULL;
        }
    }

    // Log ALL Vulkan symbol requests
    logf("[vkGPA] Request: %s", pName);

    PFN_vkVoidFunction fn = real_vkGetInstanceProcAddr(instance, pName);
    if (!fn)
        logf("[vkGPA] WARNING: symbol %s not found in real driver", pName);

    return fn;
}

/* ================================
   EXPORT FOR LOADER
   ================================ */

__attribute__((visibility("default")))
PFN_vkVoidFunction vk_icdGetInstanceProcAddr(
        VkInstance instance,
        const char* pName)
{
    return vkGetInstanceProcAddr(instance, pName);
}

__attribute__((visibility("default")))
VkResult vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *pVersion)
{
    *pVersion = 5; // modern loader compatibility
    logf("[Wrapper] Loader ICD version -> 5");
    return VK_SUCCESS;
}
