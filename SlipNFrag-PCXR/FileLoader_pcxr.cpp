#include "FileLoader_pcxr.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <string>
#include <stdexcept>

bool FileLoader_pcxr::Exists(const char* filename)
{
	std::string toSearch = std::string("\"") + filename + "\"";
    auto res = FindResource(NULL, toSearch.c_str(), RT_RCDATA);
    return (res != NULL);
}

void FileLoader_pcxr::Load(const char* filename, std::vector<unsigned char>& data)
{
    std::string toSearch = std::string("\"") + filename + "\"";
    auto res = FindResource(NULL, toSearch.c_str(), RT_RCDATA);
    if (res == NULL)
    {
        throw std::runtime_error("Resource " + toSearch + " could not be found.");
	}   
    auto hData = LoadResource(nullptr, res);
    if (hData == NULL)
    {
        throw std::runtime_error("Resource " + toSearch + " could not be loaded.");
    }
    auto size = SizeofResource(nullptr, res);
    auto ptr = (unsigned char*)LockResource(hData);
    data.resize(size);
    std::copy(ptr, ptr + size, data.data());
}
