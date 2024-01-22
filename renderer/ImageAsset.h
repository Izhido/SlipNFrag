#pragma once

#include "stb_image.h"
#include "FileLoader.h"

struct ImageAsset
{
	stbi_uc* image;
	int width;
	int height;
	int components;

	void Open(const char* name, FileLoader* loader);
	void Close() const;
};
