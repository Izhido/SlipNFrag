//
//  Engine.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 23/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "Locks.h"

@interface Engine : NSObject

-(void)StartEngine:(NSArray<NSString*>*)args locks:(Locks*)locks;

@end
