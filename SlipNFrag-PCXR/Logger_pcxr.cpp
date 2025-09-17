#include "Logger_pcxr.h"
#include <cstdio>
#include <cstdarg>

const char* Logger_pcxr::tag = "slipnfrag_native";

void Logger_pcxr::Verbose(const char* message, ...)
{
    printf("[LOG_VERBOSE] ");
    printf(tag);
    printf(": ");
    va_list vl;
    va_start(vl, message);
    vprintf(message, vl);
    va_end(vl);
    printf("\n");
}

void Logger_pcxr::Info(const char* message, ...)
{
    printf("[LOG_INFO] ");
    printf(tag);
    printf(": ");
    va_list vl;
    va_start(vl, message);
    vprintf(message, vl);
    va_end(vl);
    printf("\n");
}

void Logger_pcxr::Warn(const char* message, ...)
{
    printf("[LOG_WARN] ");
    printf(tag);
    printf(": ");
    va_list vl;
    va_start(vl, message);
    vprintf(message, vl);
    va_end(vl);
    printf("\n");
}

void Logger_pcxr::Error(const char* message, ...)
{
    printf("[LOG_ERROR] ");
    printf(tag);
    printf(": ");
    va_list vl;
    va_start(vl, message);
    vprintf(message, vl);
    va_end(vl);
    printf("\n");
}
