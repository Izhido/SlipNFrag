//
//  Engine.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 23/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "EngineStop.h"

@interface Engine : NSObject

-(void)startEngine:(NSArray<NSString*>*)args size:(CGSize)size engineStop:(EngineStop*)engineStop;

+(void)setAppScreenMode;

-(void)loopEngine:(EngineStop*)engineStop;

@end
