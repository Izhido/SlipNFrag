#pragma once

#include "FileLoader.h"
#include <string>

struct FileLoader_pcxr : public FileLoader
{
	std::string Parse(const char* filename);

	bool Exists(const char* filename);

	void Load(const char* filename, std::vector<unsigned char>& data);
};
