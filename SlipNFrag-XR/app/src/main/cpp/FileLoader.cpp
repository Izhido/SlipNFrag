#include "FileLoader.h"

FileLoader::FileLoader(android_app* app)
{
    this->app = app;
}

void FileLoader::Load(const char* filename, std::vector<unsigned char>& data)
{
    auto file = AAssetManager_open(app->activity->assetManager, filename, AASSET_MODE_BUFFER);
    size_t length = AAsset_getLength(file);
    data.resize(length);
    AAsset_read(file, data.data(), length);
    AAsset_close(file);
}
