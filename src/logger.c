#include "../include/logger.h"
#include <time.h>
#include <stdarg.h>

void ARCANE_GET_TIMESTAMP(char* format, char* buffer, int buffer_size) {
    time_t unix = time(NULL);
    struct tm* time_info = localtime(&unix);
    strftime(buffer, buffer_size, format, time_info);
}

void __ARCANE_LOG(FILE* source, char* log_label, char* timestamp, char* format, va_list va_args) {
    if (source == NULL) {
        // use syslog
        // or reporteventa for win
        return;
    }

    fprintf_s(source, "[%s] [%s] ", log_label, timestamp);
    vfprintf_s(source, format, va_args);
}

void ARCANE_GENERIC_LOG(FILE* stream_source, arc_bool use_syslog, char* log_file, char* log_label, char* format, va_list va_args) {
    char timestamp[256];
    ARCANE_GET_TIMESTAMP("%Y-%m-%d %H:%M:%S", timestamp, sizeof(timestamp));

    __ARCANE_LOG(stream_source, log_label, timestamp, format, va_args);

    if (use_syslog == arc_true)
        __ARCANE_LOG(NULL, NULL, timestamp, format, va_args);

    if (log_file != NULL) {
        FILE* f = fopen(log_file, "a");
        if (f != NULL) {
            __ARCANE_LOG(f, log_label, timestamp, format, va_args);
            fclose(f);
        }
    }
}

void ARCANE_LOGI(char* log_file, arc_bool use_syslog, char* format, va_list va_args) {
    return ARCANE_GENERIC_LOG(stdout, use_syslog, log_file, "INFO", format, va_args);
}

void ARCANE_LOGD(char* log_file, arc_bool use_syslog, char* format, va_list va_args) {
    return ARCANE_GENERIC_LOG(stdout, use_syslog, log_file, "DEBUG", format, va_args);
}

void ARCANE_LOGW(char* log_file, arc_bool use_syslog, char* format, va_list va_args) {
    return ARCANE_GENERIC_LOG(stdout, use_syslog, log_file, "WARNING", format, va_args);
}

void ARCANE_LOGC(char* log_file, arc_bool use_syslog, char* format, va_list va_args) {
    return ARCANE_GENERIC_LOG(stderr, use_syslog, log_file, "CRITICAL", format, va_args);
}

void ARCANE_LOGE(char* log_file, arc_bool use_syslog, char* format, va_list va_args) {
    return ARCANE_GENERIC_LOG(stderr, use_syslog, log_file, "ERROR", format, va_args);
}

void ARCANE_LOG(char* log_file, enum ARCANE_LOG_TYPE log_type, arc_bool use_syslog, char* format, ...) {
    va_list va;
    va_start(va, format);
    
    switch (log_type) {
        case ARC_INFO: {
            ARCANE_LOGI(log_file, use_syslog, format, va);
            break; 
        }
        case ARC_DEBUG: {
            ARCANE_LOGD(log_file, use_syslog, format, va);
            break; 
        }
        case ARC_WARNING: { 
            ARCANE_LOGW(log_file, use_syslog, format, va); 
            break; 
        }
        case ARC_CRITICAL: { 
            ARCANE_LOGC(log_file, use_syslog, format, va); 
            break; 
        }
        case ARC_ERROR: {
            ARCANE_LOGW(log_file, use_syslog, format, va); 
            break; 
        }
    }

    va_end(va);
}

