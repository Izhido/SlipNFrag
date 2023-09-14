//
//  SortedAliasEntry.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 9/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

struct SortedAliasEntry
{
	uint32_t entry;
	
	float transform[16];
	
	uint32_t indexBuffer;
	
	int32_t colormap;
	
	uint32_t indices;
	
	uint32_t vertices;
};
