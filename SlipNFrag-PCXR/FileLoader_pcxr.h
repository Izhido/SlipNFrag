#pragma once

#include "FileLoader.h"

struct FileLoader_pcxr : public FileLoader
{
    void Load(const char* filename, std::vector<unsigned char>& data);
};
