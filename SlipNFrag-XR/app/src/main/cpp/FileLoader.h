#pragma once

#include <android_native_app_glue.h>
#include <vector>

struct FileLoader
{
    android_app* app;

    FileLoader(android_app* app);

    void Load(const char* filename, std::vector<unsigned char>& data);
};