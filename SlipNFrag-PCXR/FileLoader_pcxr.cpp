#include "FileLoader_pcxr.h"

#include <fstream>

void FileLoader_pcxr::Load(const char* filename, std::vector<unsigned char>& data)
{
    const std::string assets = "slipnfrag_assets/";
    std::ifstream file(assets + filename, std::ios::binary);
    file.seekg(0, std::ios::end);
    auto length = file.tellg();
    file.seekg(0);
    data.resize(length);
    file.read((char*)data.data(), length);
    file.close();
}
