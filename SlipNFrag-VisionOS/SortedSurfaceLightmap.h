//
//  SortedSurfaceLightmap.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 22/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include "SortedSurfaceTexture.h"

struct SortedSurfaceLightmap
{
	float texturePosition[16];

	bool texturePositionSet;

	uint32_t lightmap;
	
	bool lightmapSet;

	std::unordered_map<void*, SortedSurfaceTexture> textures;
};
