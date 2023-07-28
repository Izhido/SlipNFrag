//
//  PerDrawable.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 27/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <vector>
#include "Texture.h"
#include "PerView.h"

struct PerDrawable
{
	cp_drawable_t drawable;
	
	Texture palette;
	Texture screen;
	Texture console;
	
	std::vector<PerView> views;
};
