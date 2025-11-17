// src/feature_dump.h
#ifndef FEATURE_DUMP_H
#define FEATURE_DUMP_H

#ifdef __cplusplus
extern "C" {
#endif

// Produce a feature dump into <path>/<filename> under available storage locations.
// This routine is intentionally simple and does not call Vulkan functions that require loader setup.
// It writes environment info, available files, and a timestamp.
void fdump_features(const char *filename);

// Helper to write arbitrary text lines into the default log folder (/sdcard/xclipse/logs or fallback)
void fdump_append(const char *filename, const char *line);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_DUMP_H
