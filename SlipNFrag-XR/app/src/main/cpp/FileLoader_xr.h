#pragma once

#include "FileLoader.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>

struct FileLoader_xr : public FileLoader
{
    android_app* app;

    explicit FileLoader_xr(android_app* app);

	bool Exists(const char* filename) override;

    void Load(const char* filename, std::vector<unsigned char>& data) override;
};
