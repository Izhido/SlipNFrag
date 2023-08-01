//
//  Locks.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 31/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface Locks : NSObject

@property (atomic, assign) BOOL engineStarted;

@property (atomic, assign) BOOL stopEngine;

@property (nonatomic, strong) NSObject* inputLock;

@property (nonatomic, strong) NSObject* renderLock;

@end
