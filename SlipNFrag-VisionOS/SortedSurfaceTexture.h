//
//  SortedSurfaceTexture.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 22/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

struct SortedSurfaceTexture
{
	std::vector<uint32_t> entries;

	uint32_t texture;
	
	bool set;
	
	uint32_t indices;
};
