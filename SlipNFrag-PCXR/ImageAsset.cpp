#include "ImageAsset.h"
#include <vector>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void ImageAsset::Open(const char* name)
{
    std::ifstream file(name, std::ios::binary);
    file.seekg(0, std::ios::end);
    auto length = file.tellg();
    file.seekg(0);
	std::vector<stbi_uc> source(length);
    file.read((char*)source.data(), length);
	image = stbi_load_from_memory(source.data(), length, &width, &height, &components, 4);
}

void ImageAsset::Close() const
{
	stbi_image_free(image);
}
