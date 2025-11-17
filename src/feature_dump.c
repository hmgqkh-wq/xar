#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <android/log.h>

#define LOG_TAG "XCLIPSE_WRAPPER"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* Eden storage path (always accessible) */
static const char* LOG_PATH =
    "/storage/emulated/0/Android/data/dev.eden.eden_emulator/files/xclipse_log.txt";

static FILE* log_file = NULL;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

/* Open file safely */
static void init_log_file(void)
{
    pthread_mutex_lock(&log_lock);

    if (log_file == NULL)
    {
        log_file = fopen(LOG_PATH, "a");

        if (log_file == NULL)
        {
            LOGE("FAILED to open log file: %s", LOG_PATH);
        }
        else
        {
            LOGI("Log file initialized at: %s", LOG_PATH);
            fprintf(log_file, "===== XCLIPSE WRAPPER START =====\n");
            fflush(log_file);
        }
    }

    pthread_mutex_unlock(&log_lock);
}

/* Public init called from vulkan_wrapper */
void feature_dump_init(void)
{
    init_log_file();
}

/* Thread-safe log write */
void feature_dump_log(const char* text)
{
    if (!text)
        return;

    pthread_mutex_lock(&log_lock);

    if (!log_file)
        init_log_file();

    if (log_file)
    {
        fprintf(log_file, "%s\n", text);
        fflush(log_file);
    }

    pthread_mutex_unlock(&log_lock);

    LOGI("%s", text); /* Send to Android logcat too */
}

/* Log a Vulkan call by name */
void feature_dump_vk_call(const char* name)
{
    if (!name)
        return;

    char buf[256];
    snprintf(buf, sizeof(buf), "VK CALL: %s", name);

    feature_dump_log(buf);
}

/* Cleanup */
void feature_dump_close(void)
{
    pthread_mutex_lock(&log_lock);

    if (log_file)
    {
        fprintf(log_file, "===== XCLIPSE WRAPPER END =====\n");
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }

    pthread_mutex_unlock(&log_lock);
}
