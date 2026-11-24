#define _POSIX_C_SOURCE 200809L
#include "loglib.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

void log_now_ms(char* buf, size_t len) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buf, len, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

#else

void log_now_ms(char* buf, size_t len) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm_info;
    localtime_r(&ts.tv_sec, &tm_info);

    int ms = (int)(ts.tv_nsec / 1000000L);
    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec, ms);
}

#endif

void log_write(const char* path, proc_id_t pid, const char* msg) {
    char time_buf[32];
    log_now_ms(time_buf, sizeof(time_buf));

    FILE* f = fopen(path, "a");
    if (!f) {
        perror("fopen log");
        return;
    }

    fprintf(f, "[%s] [PID %llu] %s\n", time_buf, (unsigned long long)pid, msg);
    fclose(f);
}