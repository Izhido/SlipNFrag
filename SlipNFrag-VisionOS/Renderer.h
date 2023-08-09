//
//  Renderer.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 30/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <CompositorServices/CompositorServices.h>
#import "EngineStop.h"

@interface Renderer : NSObject

@property (nonatomic, assign) BOOL stopRenderer;

-(void)startRenderer:(CP_OBJECT_cp_layer_renderer*)layerRenderer engineStop:(EngineStop*)engineStop;

+(void)renderFrame:(CGContextRef)context size:(CGSize)size date:(NSDate*)date;

@end
