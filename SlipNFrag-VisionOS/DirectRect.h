//
//  DirectRect.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 6/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <vector>
#include <mutex>

struct DirectRect
{
	int x;
	int y;
	int width;
	int height;
	int vid_width;
	int vid_height;
	unsigned char* data;

	static std::vector<DirectRect> directRects;
};
