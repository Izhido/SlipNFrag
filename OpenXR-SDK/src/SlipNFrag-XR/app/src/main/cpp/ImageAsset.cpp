#include "ImageAsset.h"
#include <android_native_app_glue.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void ImageAsset::Open(const char* name, struct android_app* app)
{
	auto file = AAssetManager_open(app->activity->assetManager, name, AASSET_MODE_BUFFER);
	auto length = AAsset_getLength(file);
	std::vector<stbi_uc> source(length);
	AAsset_read(file, source.data(), length);
	image = stbi_load_from_memory(source.data(), length, &width, &height, &components, 4);
	AAsset_close(file);
}

void ImageAsset::Close() const
{
	stbi_image_free(image);
}
