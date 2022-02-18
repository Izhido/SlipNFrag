#pragma once

#include <vector>
#include <mutex>

struct DirectRect
{
	int x;
	int y;
	int width;
	int height;
	unsigned char* data;

	static std::vector<DirectRect> directRects;
};
