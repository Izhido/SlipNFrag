#include "FileLoader.h"

void FileLoader::Load(const char* filename, std::vector<unsigned char>& data)
{
    std::ifstream file(filename, std::ios::binary);
    file.seekg(0, std::ios::end);
    auto length = file.tellg();
    file.seekg(0);
    data.resize(length);
    file.read((char*)data.data(), length);
    file.close();
}
