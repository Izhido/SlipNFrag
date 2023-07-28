//
//  Engine.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 23/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <CompositorServices/CompositorServices.h>

@interface Engine : NSObject

@property(nonatomic, assign) BOOL stopGame;

@property (nonatomic, readonly) NSString* stopGameMessage;

-(void)StartGame:(NSArray<NSString*>*)args layerRenderer:(CP_OBJECT_cp_layer_renderer*)layerRenderer;

@end
