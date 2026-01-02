#pragma once

#include <vector>

struct FileLoader
{
	virtual bool Exists(const char* filename) = 0;

	virtual void Load(const char* filename, std::vector<unsigned char>& data) = 0;

	virtual ~FileLoader() = default;
};
