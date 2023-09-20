//
//  PerDrawable.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 27/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <CompositorServices/CompositorServices.h>
#import "Lightmap.h"
#import "PerView.h"

@interface PerDrawable : NSObject

@property (nonatomic, assign) cp_drawable_t drawable;
	
@property (nonatomic, strong) Texture* palette;

@property (nonatomic, strong) Texture* colormap;

@property (nonatomic, strong) Texture* screen;

@property (nonatomic, strong) Texture* console;

@property (nonatomic, strong) id<MTLBuffer> indices;

@property (nonatomic, strong) id<MTLBuffer> texturedVertices;

@property (nonatomic, strong) id<MTLBuffer> surfaceVertices;

@property (nonatomic, strong) id<MTLBuffer> aliasVertices;

@property (nonatomic, strong) id<MTLBuffer> particleVertices;

@property (nonatomic, strong) NSMutableArray<Lightmap*>* lightmapCache;

@property (nonatomic, strong) NSMutableDictionary<NSNumber*, PerView*>* views;

@end
