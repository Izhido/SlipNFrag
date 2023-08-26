//
//  Pipelines.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 25/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Pipelines.h"

@interface Pipelines ()

@end

@implementation Pipelines

-(bool)create:(id<MTLDevice>)device colorPixelFormat:(MTLPixelFormat)colorPixelFormat depthPixelFormat:(MTLPixelFormat)depthPixelFormat engineStop:(EngineStop*)engineStop
{
	id<MTLLibrary> library = [device newDefaultLibrary];

	id<MTLFunction> vertexProgram = [library newFunctionWithName:@"planarVertexMain"];
	id<MTLFunction> fragmentProgram = [library newFunctionWithName:@"planarFragmentMain"];

	auto vertexDescriptor = [MTLVertexDescriptor new];
	vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
	vertexDescriptor.attributes[0].offset = 0;
	vertexDescriptor.attributes[0].bufferIndex = 0;
	vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
	vertexDescriptor.attributes[1].offset = 16;
	vertexDescriptor.attributes[1].bufferIndex = 0;
	vertexDescriptor.layouts[0].stride = 24;

	auto pipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
	pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;
	pipelineStateDescriptor.colorAttachments[0].pixelFormat = colorPixelFormat;
	pipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
	pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
	pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
	pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	pipelineStateDescriptor.depthAttachmentPixelFormat = depthPixelFormat;
	pipelineStateDescriptor.rasterSampleCount = 1;

	NSError* error = nil;
	self.planar = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Planar rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}
	
	fragmentProgram = [library newFunctionWithName:@"consoleFragmentMain"];

	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.console = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Console rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	vertexDescriptor = [MTLVertexDescriptor new];
	vertexDescriptor.attributes[0].format = MTLVertexFormatFloat3;
	vertexDescriptor.attributes[0].offset = 0;
	vertexDescriptor.attributes[0].bufferIndex = 0;
	vertexDescriptor.layouts[0].stride = 12;

	pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;

	vertexProgram = [library newFunctionWithName:@"surfaceVertexMain"];
	fragmentProgram = [library newFunctionWithName:@"surfaceFragmentMain"];

	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.surface = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Surface rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	return true;
}

@end
