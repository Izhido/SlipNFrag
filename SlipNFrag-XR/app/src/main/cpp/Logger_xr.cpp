#include "Logger_xr.h"
#include <android/log.h>

const char* Logger_xr::tag = "slipnfrag_native";

void Logger_xr::Verbose(const char* message, ...)
{
	va_list vl;
	va_start(vl, message);
	__android_log_vprint(ANDROID_LOG_VERBOSE, tag, message, vl);
	va_end(vl);
}

void Logger_xr::Info(const char* message, ...)
{
	va_list vl;
	va_start(vl, message);
	__android_log_vprint(ANDROID_LOG_INFO, tag, message, vl);
	va_end(vl);
}

void Logger_xr::Warn(const char* message, ...)
{
	va_list vl;
	va_start(vl, message);
	__android_log_vprint(ANDROID_LOG_WARN, tag, message, vl);
	va_end(vl);
}

void Logger_xr::Error(const char* message, ...)
{
	va_list vl;
	va_start(vl, message);
	__android_log_vprint(ANDROID_LOG_ERROR, tag, message, vl);
	va_end(vl);
}
