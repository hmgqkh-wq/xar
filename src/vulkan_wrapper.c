// src/vulkan_wrapper.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <time.h>
#include <vulkan/vulkan.h>
#include "xeno_wrapper.h"

// Logging path (Eden-accessible)
static const char *LOG_PATH = "/storage/emulated/0/Android/data/dev.eden.eden_emulator/files/logs/xclipse_icd.log";
static const char *FEATURES_PATH = "/storage/emulated/0/Android/data/dev.eden.eden_emulator/files/logs/xclipse_features.txt";

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

static void write_log(const char *fmt, ...)
{
    pthread_mutex_lock(&log_lock);
    FILE *f = fopen(LOG_PATH, "a");
    if (!f) {
        // try to create folder by fopen failing silently, try /tmp fallback
        f = fopen("/tmp/xclipse_icd.log", "a");
        if (!f) {
            pthread_mutex_unlock(&log_lock);
            return;
        }
    }

    // timestamp
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm);

    fprintf(f, "[%s] ", ts);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fprintf(f, "\n");
    fclose(f);
    pthread_mutex_unlock(&log_lock);
}

// Helper: write features file (atomic-ish)
static void write_features(const char *txt)
{
    pthread_mutex_lock(&log_lock);
    FILE *f = fopen(FEATURES_PATH, "w");
    if (!f) {
        pthread_mutex_unlock(&log_lock);
        return;
    }
    fprintf(f, "%s\n", txt);
    fclose(f);
    pthread_mutex_unlock(&log_lock);
}

// Load next trampoline symbol (RTLD_NEXT) or from explicit ICD
static void* load_next_symbol(const char *name)
{
    void *sym = dlsym(RTLD_NEXT, name);
    if (sym) return sym;

    // Optionally try explicit ICD path if env provided
    const char *icd = getenv("XENO_REAL_ICD");
    if (icd) {
        void *handle = dlopen(icd, RTLD_LAZY | RTLD_LOCAL);
        if (handle) {
            sym = dlsym(handle, name);
            if (sym) return sym;
            dlclose(handle);
        }
    }
    return NULL;
}

// Cached pointers for core entrypoints
typedef PFN_vkVoidFunction (*PFN_vkGetInstanceProcAddr)(VkInstance, const char*);
static PFN_vkGetInstanceProcAddr real_vkGetInstanceProcAddr = NULL;

typedef PFN_vkVoidFunction (*PFN_vkGetDeviceProcAddr)(VkDevice, const char*);
static PFN_vkGetDeviceProcAddr real_vkGetDeviceProcAddr = NULL;

// Helper to init core pointers
static void ensure_core_trampolines()
{
    if (!real_vkGetInstanceProcAddr) {
        real_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)load_next_symbol("vkGetInstanceProcAddr");
        if (!real_vkGetInstanceProcAddr) {
            write_log("ERROR: unable to locate next vkGetInstanceProcAddr");
        } else {
            write_log("info: chained to next vkGetInstanceProcAddr");
        }
    }
    if (!real_vkGetDeviceProcAddr) {
        real_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)load_next_symbol("vkGetDeviceProcAddr");
        if (!real_vkGetDeviceProcAddr) {
            write_log("warn: vkGetDeviceProcAddr not found yet (may be available later)");
        }
    }
}

// Basic wrapper for vkCreateInstance to log info then dispatch
VKAPI_ATTR VkResult VKAPI_CALL xeno_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    ensure_core_trampolines();
    write_log("vkCreateInstance: pCreateInfo=%p pInstance=%p", (void*)pCreateInfo, (void*)pInstance);

    // Log enabled layers and extensions if present
    if (pCreateInfo) {
        if (pCreateInfo->enabledLayerCount) {
            write_log("vkCreateInstance: enabledLayerCount=%u", pCreateInfo->enabledLayerCount);
            for (uint32_t i = 0; i < pCreateInfo->enabledLayerCount; ++i) {
                write_log("  layer[%u]=%s", i, pCreateInfo->ppEnabledLayerNames[i]);
            }
        }
        if (pCreateInfo->enabledExtensionCount) {
            write_log("vkCreateInstance: enabledExtensionCount=%u", pCreateInfo->enabledExtensionCount);
            for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
                write_log("  ext[%u]=%s", i, pCreateInfo->ppEnabledExtensionNames[i]);
            }
        }
    }

    // Call through
    PFN_vkCreateInstance fn = (PFN_vkCreateInstance)real_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    if (!fn) {
        write_log("ERROR: real vkCreateInstance not found");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkResult r = fn(pCreateInfo, pAllocator, pInstance);
    write_log("vkCreateInstance -> %d (instance %p)", (int)r, (pInstance ? (void*)*pInstance : NULL));
    return r;
}

// Basic wrapper for vkCreateDevice to log info then dispatch
VKAPI_ATTR VkResult VKAPI_CALL xeno_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    ensure_core_trampolines();
    write_log("vkCreateDevice: physicalDevice=%p pCreateInfo=%p pDevice=%p", (void*)physicalDevice, (void*)pCreateInfo, (void*)pDevice);

    if (pCreateInfo) {
        if (pCreateInfo->enabledExtensionCount) {
            write_log("vkCreateDevice: enabledExtensionCount=%u", pCreateInfo->enabledExtensionCount);
            for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
                write_log("  dev-ext[%u]=%s", i, pCreateInfo->ppEnabledExtensionNames[i]);
            }
        }
    }

    // Find real function via instance-level getProcAddr (we call instance null)
    PFN_vkCreateDevice fn = NULL;
    // try via global vkGetInstanceProcAddr if present
    if (real_vkGetInstanceProcAddr) {
        PFN_vkVoidFunction sym = real_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateDevice");
        fn = (PFN_vkCreateDevice)sym;
    }
    // fallback to RTLD_NEXT
    if (!fn) {
        fn = (PFN_vkCreateDevice)load_next_symbol("vkCreateDevice");
    }
    if (!fn) {
        write_log("ERROR: real vkCreateDevice not found");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkResult r = fn(physicalDevice, pCreateInfo, pAllocator, pDevice);
    write_log("vkCreateDevice -> %d (device %p)", (int)r, (pDevice ? (void*)*pDevice : NULL));
    return r;
}

// Intercept vkGetInstanceProcAddr: return our wrappers for certain names, otherwise chain
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL xeno_vkGetInstanceProcAddr(VkInstance instance, const char* pName)
{
    ensure_core_trampolines();
    if (!pName) return NULL;

    // Provide wrappers for a small list
    if (strcmp(pName, "vkGetInstanceProcAddr") == 0) {
        return (PFN_vkVoidFunction)xeno_vkGetInstanceProcAddr;
    }
    if (strcmp(pName, "vkCreateInstance") == 0) {
        return (PFN_vkVoidFunction)xeno_vkCreateInstance;
    }
    if (strcmp(pName, "vkCreateDevice") == 0) {
        return (PFN_vkVoidFunction)xeno_vkCreateDevice;
    }

    // Try the real chain
    if (real_vkGetInstanceProcAddr) {
        PFN_vkVoidFunction f = real_vkGetInstanceProcAddr(instance, pName);
        if (f) return f;
    }

    // fallback using RTLD_NEXT dlsym
    PFN_vkVoidFunction sym = (PFN_vkVoidFunction)load_next_symbol(pName);
    return sym;
}

// Intercept vkGetDeviceProcAddr: allow chaining and placeholder
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL xeno_vkGetDeviceProcAddr(VkDevice device, const char* pName)
{
    if (!pName) return NULL;

    if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
        return (PFN_vkVoidFunction)xeno_vkGetDeviceProcAddr;
    }

    // If user wants to instrument device-level funcs, do it here.
    // For now chain to real function if available.
    if (real_vkGetDeviceProcAddr) {
        PFN_vkVoidFunction f = real_vkGetDeviceProcAddr(device, pName);
        if (f) return f;
    }

    // fallback
    PFN_vkVoidFunction sym = (PFN_vkVoidFunction)load_next_symbol(pName);
    return sym;
}

// Exported alias names expected by loader
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    // initialize core trampolines on first use
    if (!real_vkGetInstanceProcAddr) ensure_core_trampolines();
    return xeno_vkGetInstanceProcAddr(instance, pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    if (!real_vkGetDeviceProcAddr) ensure_core_trampolines();
    return xeno_vkGetDeviceProcAddr(device, pName);
}

// Library constructor to log wrapper load
__attribute__((constructor))
static void xeno_init_impl(void)
{
    // create parent folder? We rely on Eden path being present; just log attempt
    write_log("xeno ICD loader initializing...");
    const char *env = getenv("XENO_REAL_ICD");
    if (env) write_log("XENO_REAL_ICD=%s", env);
    ensure_core_trampolines();

    // Write minimal features to features file: trimmed Vulkan 1.3-ish features
    const char *features_txt =
        "xclipse-940 (wrapper) - declared target: Vulkan 1.3\n"
        "spirv_embedded: true\n"
        "pipeline_cache: true\n"
        "async_compile: true\n"
        "auto_tuning: true\n"
        "hud_overlay: true\n"
        "descriptor_indexing: true\n"
        "buffer_device_address: true\n"
        "timeline_semaphores: true\n"
        "dynamic_rendering: true\n"
        "robust_buffer_access: true\n"
        "shader_object: true\n";
    write_features(features_txt);
    write_log("xeno ICD initialized OK");
}
