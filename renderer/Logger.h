#pragma once

struct Logger
{
	virtual void Verbose(const char* message, ...) = 0;

	virtual void Info(const char* message, ...) = 0;

	virtual void Warn(const char* message, ...) = 0;

	virtual void Error(const char* message, ...) = 0;

	virtual ~Logger() = default;
};
