//
//  SortedSurfaceRotatedTexture.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 27/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include "SortedSurfaceTexture.h"

struct SortedSurfaceRotatedTexture : SortedSurfaceTexture
{
	std::vector<uint32_t> indicesPerSurface;
};
