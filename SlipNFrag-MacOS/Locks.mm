//
//  Locks.mm
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 31/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Locks.h"

@interface Locks ()

@end

@implementation Locks

-(id)init
{
	self = [super init];
	
	if (self!=nil)
	{
		self.inputLock = [NSObject new];
		self.renderLock = [NSObject new];
	}
	
	return self;
}

@end
