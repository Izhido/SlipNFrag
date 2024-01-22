#pragma once

#include <fstream>
#include <vector>

struct FileLoader
{
    void Load(const char* filename, std::vector<unsigned char>& data);
};