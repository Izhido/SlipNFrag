#include "Logger_oxr.h"
#include <android/log.h>

const char* Logger_oxr::tag = "slipnfrag_native";

void Logger_oxr::Verbose(const char* message, ...)
{
	va_list vl;
	va_start(vl, message);
	__android_log_vprint(ANDROID_LOG_VERBOSE, tag, message, vl);
	va_end(vl);
}

void Logger_oxr::Info(const char* message, ...)
{
	va_list vl;
	va_start(vl, message);
	__android_log_vprint(ANDROID_LOG_INFO, tag, message, vl);
	va_end(vl);
}

void Logger_oxr::Warn(const char* message, ...)
{
	va_list vl;
	va_start(vl, message);
	__android_log_vprint(ANDROID_LOG_WARN, tag, message, vl);
	va_end(vl);
}

void Logger_oxr::Error(const char* message, ...)
{
	va_list vl;
	va_start(vl, message);
	__android_log_vprint(ANDROID_LOG_ERROR, tag, message, vl);
	va_end(vl);
}
