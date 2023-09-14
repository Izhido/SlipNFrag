//
//  SortedAliasTexture.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 9/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <vector>
#include "SortedAliasEntry.h"

struct SortedAliasTexture
{
	uint32_t texture;
	
	bool set;

	std::vector<SortedAliasEntry> entries;
};
