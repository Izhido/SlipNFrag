#pragma once

#include "Logger.h"

struct Logger_pcxr : public Logger
{
	static const char* tag;

	void Verbose(const char* message, ...) override;

	void Info(const char* message, ...) override;

	void Warn(const char* message, ...) override;

	void Error(const char* message, ...) override;
};
