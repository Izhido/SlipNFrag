#pragma once

#include <vector>
#include <mutex>

struct DirectRect
{
	int x;
	int y;
	int width;
	int height;
	int vid_rowbytes;
	int vid_height;
	unsigned char* data;

	static std::vector<DirectRect> directRects;
};
