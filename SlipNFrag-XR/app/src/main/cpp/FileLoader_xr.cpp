#include "FileLoader_xr.h"

FileLoader_xr::FileLoader_xr(android_app* app)
{
    this->app = app;
}

bool FileLoader_xr::Exists(const char *filename)
{
    auto file = AAssetManager_open(app->activity->assetManager, filename, AASSET_MODE_STREAMING);
	if (file == nullptr)
	{
		return false;
	}
    AAsset_close(file);
	return true;
}

void FileLoader_xr::Load(const char* filename, std::vector<unsigned char>& data)
{
    auto file = AAssetManager_open(app->activity->assetManager, filename, AASSET_MODE_BUFFER);
    size_t length = AAsset_getLength(file);
    data.resize(length);
    AAsset_read(file, data.data(), length);
    AAsset_close(file);
}
