#include <android/log.h>
#include <cstdarg>
#include <cstdio>

int __android_log_print(int prio, const char* tag, const char* fmt, ...)
{
    printf("[");
    switch (prio)
    {
        case ANDROID_LOG_UNKNOWN:
            printf("LOG_UNKNOWN");
            break;

        case ANDROID_LOG_DEFAULT:
            printf("LOG_DEFAULT");
            break;

        case ANDROID_LOG_VERBOSE:
            printf("LOG_VERBOSE");
            break;

        case ANDROID_LOG_DEBUG:
            printf("LOG_DEBUG");
            break;

        case ANDROID_LOG_INFO:
            printf("LOG_INFO");
            break;

        case ANDROID_LOG_WARN:
            printf("LOG_WARN");
            break;

        case ANDROID_LOG_ERROR:
            printf("LOG_ERROR");
            break;

        case ANDROID_LOG_FATAL:
            printf("LOG_FATAL");
            break;

        default:
            printf("LOG_SILENT");
            break;
    }
    printf("] ");
    printf(tag);
    printf(": ");
    va_list argptr;
    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    va_end(argptr);
    printf("\n");
    return 0;
}
