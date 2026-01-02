#pragma once

#include "FileLoader.h"

struct FileLoader_pcxr : public FileLoader
{
	bool Exists(const char* filename);

	void Load(const char* filename, std::vector<unsigned char>& data);
};
