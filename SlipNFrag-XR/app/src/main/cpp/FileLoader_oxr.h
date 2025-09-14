#pragma once

#include "FileLoader.h"
#include <android_native_app_glue.h>

struct FileLoader_oxr : public FileLoader
{
    android_app* app;

    explicit FileLoader_oxr(android_app* app);

    void Load(const char* filename, std::vector<unsigned char>& data) override;
};
