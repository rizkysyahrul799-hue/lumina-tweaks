#pragma once
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>

#define LOG_MAIN     "/data/adb/modules/lumina/lumina.log"
#define LOG_THERMAL  "/data/adb/modules/lumina/thermal.log"
#define LOG_PROFILER "/data/adb/modules/lumina/sysmon.log"
#define LOG_MAX_SIZE 1048576  // Limit 1MB per file log

typedef enum {
    LOG_TARGET_MAIN     = 0,
    LOG_TARGET_THERMAL  = 1,
    LOG_TARGET_PROFILER = 2,
} LogTarget;

static const char * const log_paths[] = {
    LOG_MAIN,
    LOG_THERMAL,
    LOG_PROFILER,
};

static inline void rotate_if_needed(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && st.st_size > LOG_MAX_SIZE) {
        char old_path[256];
        snprintf(old_path, sizeof(old_path), "%s.old", path);
        rename(path, old_path);
    }
}

static inline void lumina_log(LogTarget target, const char *level, const char *tag, const char *fmt, ...) {
    struct timespec ts;
    struct tm tm_info;
    char timebuf[32];
    char msgbuf[512];
    va_list args;

    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm_info); // Thread-safe
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_info);

    va_start(args, fmt);
    vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
    va_end(args);

    const char *path = log_paths[target];
    rotate_if_needed(path);

    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "%s.%03ld %s %s: %s\n",
                timebuf, ts.tv_nsec / 1000000,
                level, tag, msgbuf);
        fclose(f);
    }

    // Duplicate event ke Master Log (lumina.log)
    if (target != LOG_TARGET_MAIN) {
        rotate_if_needed(LOG_MAIN);
        FILE *mf = fopen(LOG_MAIN, "a");
        if (mf) {
            fprintf(mf, "%s.%03ld %s %s: %s\n",
                    timebuf, ts.tv_nsec / 1000000,
                    level, tag, msgbuf);
            fclose(mf);
        }
    }
}

// Macro Log Umum
#define LOGI(fmt, ...)  lumina_log(LOG_TARGET_MAIN,     "I", "Lumina",          fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)  lumina_log(LOG_TARGET_MAIN,     "W", "Lumina",          fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)  lumina_log(LOG_TARGET_MAIN,     "D", "Lumina",          fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)  lumina_log(LOG_TARGET_MAIN,     "E", "Lumina",          fmt, ##__VA_ARGS__)

// Macro Log Thermal
#define THERM_I(fmt, ...) lumina_log(LOG_TARGET_THERMAL,  "I", "Lumina_Thermal",  fmt, ##__VA_ARGS__)
#define THERM_D(fmt, ...) lumina_log(LOG_TARGET_THERMAL,  "D", "Lumina_Thermal",  fmt, ##__VA_ARGS__)
#define THERM_E(fmt, ...) lumina_log(LOG_TARGET_THERMAL,  "E", "Lumina_Thermal",  fmt, ##__VA_ARGS__)

// Macro Log Profiler / CPU / IO
#define PROF_I(fmt, ...)  lumina_log(LOG_TARGET_PROFILER, "I", "Lumina_Profiler", fmt, ##__VA_ARGS__)
#define PROF_D(fmt, ...)  lumina_log(LOG_TARGET_PROFILER, "D", "Lumina_Profiler", fmt, ##__VA_ARGS__)
#define PROF_E(fmt, ...)  lumina_log(LOG_TARGET_PROFILER, "E", "Lumina_Profiler", fmt, ##__VA_ARGS__)

// Header saat service/daemon baru jalan
static inline void log_session_start(const char *module_ver) {
    rotate_if_needed(LOG_MAIN);
    FILE *f = fopen(LOG_MAIN, "a");
    if (!f) return;
    fprintf(f,
        "\n################################################################\n"
        "                       Lumina Diagnostics                      \n"
        "################################################################\n\n"
        "  Module Version : %s\n\n"
        "--- START OF LUMINA SERVICE ---\n",
        module_ver
    );
    fclose(f);
}
