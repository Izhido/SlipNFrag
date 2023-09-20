//
//  SortedSpriteTexture.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 18/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <vector>

struct SortedSpriteTexture
{
	std::vector<uint32_t> entries;

	uint32_t texture;
	
	bool set;
	
	uint32_t indices;
};
