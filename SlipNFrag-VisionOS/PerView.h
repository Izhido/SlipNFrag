//
//  PerView.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 27/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import <Metal/Metal.h>

@interface PerView : NSObject

@property (nonatomic, strong) MTLRenderPassDescriptor* renderPassDescriptor;

@end
