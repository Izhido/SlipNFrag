#include "ImageAsset.h"
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void ImageAsset::Open(const char* name, FileLoader* loader)
{
    std::vector<stbi_uc> source;
    loader->Load(name, source);
	image = stbi_load_from_memory(source.data(), (int)source.size(), &width, &height, &components, 4);
}

void ImageAsset::Close() const
{
	stbi_image_free(image);
}
