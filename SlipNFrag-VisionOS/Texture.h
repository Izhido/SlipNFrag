//
//  Texture.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 27/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

struct Texture
{
	MTLTextureDescriptor* descriptor;
	id<MTLTexture> texture;
	MTLSamplerDescriptor* samplerDescriptor;
	id<MTLSamplerState> samplerState;
	MTLRegion region;
};
