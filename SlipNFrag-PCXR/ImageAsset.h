#pragma once

#include "stb_image.h"

struct ImageAsset
{
	stbi_uc* image;
	int width;
	int height;
	int components;

	void Open(const char* name);
	void Close() const;
};
