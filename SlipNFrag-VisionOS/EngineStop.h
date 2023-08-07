//
//  EngineStop.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 6/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface EngineStop : NSObject

@property (atomic, assign) BOOL stopEngine;

@property (atomic, strong) NSString* stopEngineMessage;

@end
