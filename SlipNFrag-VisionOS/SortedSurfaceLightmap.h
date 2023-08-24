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
	float vecs[2][4];

	float size[2];

	bool set;
	
	std::unordered_map<void*, SortedSurfaceTexture> textures;
};
