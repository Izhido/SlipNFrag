//
//  LightmapCopying.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 23/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

struct LightmapCopying
{
	uint32_t lightmap;
	uint32_t* data;
	uint32_t width;
	uint32_t height;
	uint32_t size;
	uint32_t bytesPerRow;
};
