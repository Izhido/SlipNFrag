#include "FileLoader_pcxr.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <stdexcept>

std::string FileLoader_pcxr::Parse(const char* filename)
{
    std::string parsed;
    for (size_t i = 0; i < 1024; i++)
    {
        auto c = filename[i];
        if (c == 0)
        {
            break;
        }
        else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
        {
            parsed.push_back(c);
        }
        else if (c >= 'a' && c <= 'z')
        {
            parsed.push_back(std::toupper(c));
        }
        else
        {
            parsed.push_back('_');
        }
    }
    return parsed;
}

bool FileLoader_pcxr::Exists(const char* filename)
{
	std::string toSearch = Parse(filename);
    auto res = FindResource(NULL, toSearch.c_str(), RT_RCDATA);
    return (res != NULL);
}

void FileLoader_pcxr::Load(const char* filename, std::vector<unsigned char>& data)
{
    std::string toSearch = Parse(filename);
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
