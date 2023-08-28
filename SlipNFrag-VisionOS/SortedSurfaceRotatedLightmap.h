//
//  SortedSurfaceRotatedLightmap.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 27/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include "SortedSurfaceRotatedTexture.h"

struct SortedSurfaceRotatedLightmap
{
	float texturePosition[16];

	bool texturePositionSet;

	uint32_t lightmap;
	
	bool lightmapSet;

	std::unordered_map<void*, SortedSurfaceRotatedTexture> textures;
};
