//
//  Pipelines.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 25/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <Metal/Metal.h>
#import "EngineStop.h"

@interface Pipelines : NSObject

@property (nonatomic, strong) id<MTLRenderPipelineState> planar;

@property (nonatomic, strong) id<MTLRenderPipelineState> console;

@property (nonatomic, strong) id<MTLRenderPipelineState> sky;

@property (nonatomic, strong) id<MTLRenderPipelineState> sprite;

@property (nonatomic, strong) id<MTLRenderPipelineState> surface;

@property (nonatomic, strong) id<MTLRenderPipelineState> surfaceRotated;

@property (nonatomic, strong) id<MTLRenderPipelineState> turbulentLit;

@property (nonatomic, strong) id<MTLRenderPipelineState> turbulent;

@property (nonatomic, strong) id<MTLRenderPipelineState> alias;

@property (nonatomic, strong) id<MTLRenderPipelineState> viewmodel;

@property (nonatomic, strong) id<MTLRenderPipelineState> particle;

-(bool)create:(id<MTLDevice>)device colorPixelFormat:(MTLPixelFormat)colorPixelFormat depthPixelFormat:(MTLPixelFormat)depthPixelFormat engineStop:(EngineStop*)engineStop;

@end
