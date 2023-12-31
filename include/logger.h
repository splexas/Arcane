#ifndef LOGGER_H
#define LOGGER_H

#include "arc_types.h"
#include <stdio.h>

enum ARCANE_LOG_TYPE {
    ARC_INFO,
    ARC_DEBUG,
    ARC_WARNING,
    ARC_CRITICAL,
    ARC_ERROR
};

void ARCANE_GET_TIMESTAMP(char* format, char* buffer, int buffer_size);
void __ARCANE_LOG(FILE* source, char* log_label, char* timestamp, char* format, va_list va_args);
void ARCANE_GENERIC_LOG(FILE* stream_source, arc_bool use_syslog, char* log_file, char* log_label, char* format, va_list va_args);

void ARCANE_LOGI(char* log_file, arc_bool use_syslog, char* format, va_list va_args);
void ARCANE_LOGD(char* log_file, arc_bool use_syslog, char* format, va_list va_args);
void ARCANE_LOGW(char* log_file, arc_bool use_syslog, char* format, va_list va_args);
void ARCANE_LOGC(char* log_file, arc_bool use_syslog,  char* format, va_list va_args);
void ARCANE_LOGE(char* log_file, arc_bool use_syslog, char* format, va_list va_args);
void ARCANE_LOG(char* log_file, enum ARCANE_LOG_TYPE log_type, arc_bool use_syslog, char* format, ...);

#endif