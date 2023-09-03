//
//  SortedTurbulentTexture.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 2/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <vector>

struct SortedTurbulentTexture
{
	float texturePosition[8];

	float textureSize[2];
	
	bool texturePositionSet;

	std::vector<uint32_t> entries;

	uint32_t texture;
	
	bool textureSet;
	
	uint32_t indices;
};
