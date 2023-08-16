//
//  Texture.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 27/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <Metal/Metal.h>

@interface Texture : NSObject

@property (nonatomic, strong) MTLTextureDescriptor* descriptor;

@property (nonatomic, strong) id<MTLTexture> texture;

@property (nonatomic, strong) MTLSamplerDescriptor* samplerDescriptor;

@property (nonatomic, strong) id<MTLSamplerState> samplerState;

@property (nonatomic, assign) MTLRegion region;

@end
