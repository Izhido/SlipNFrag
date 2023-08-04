//
//  DirectRect.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 2/8/23.
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
	unsigned char* data;

	static std::vector<DirectRect> directRects;
};
