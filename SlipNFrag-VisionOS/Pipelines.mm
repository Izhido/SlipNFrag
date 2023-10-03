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

	vertexProgram = [library newFunctionWithName:@"skyVertexMain"];
	fragmentProgram = [library newFunctionWithName:@"skyFragmentMain"];

	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.sky = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Sky rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	vertexProgram = [library newFunctionWithName:@"spriteVertexMain"];
	fragmentProgram = [library newFunctionWithName:@"spriteFragmentMain"];

	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.sprite = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Sprite rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	vertexDescriptor = [MTLVertexDescriptor new];
	vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
	vertexDescriptor.attributes[0].offset = 0;
	vertexDescriptor.attributes[0].bufferIndex = 0;
	vertexDescriptor.layouts[0].stride = 16;

	pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;

	vertexProgram = [library newFunctionWithName:@"skyboxVertexMain"];
	fragmentProgram = [library newFunctionWithName:@"skyboxFragmentMain"];

	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.skybox = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Skybox rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	vertexDescriptor = [MTLVertexDescriptor new];
	vertexDescriptor.attributes[0].format = MTLVertexFormatFloat3;
	vertexDescriptor.attributes[0].offset = 0;
	vertexDescriptor.attributes[0].bufferIndex = 0;
	vertexDescriptor.layouts[0].stride = 12;

	pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;

	auto surfaceVertexProgram = [library newFunctionWithName:@"surfaceVertexMain"];
	fragmentProgram = [library newFunctionWithName:@"surfaceFragmentMain"];

	pipelineStateDescriptor.vertexFunction = surfaceVertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.surface = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Surface rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	auto surfaceRotatedVertexProgram = [library newFunctionWithName:@"surfaceRotatedVertexMain"];

	pipelineStateDescriptor.vertexFunction = surfaceRotatedVertexProgram;

	self.surfaceRotated = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Rotated surface rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	auto fenceFragmentProgram = [library newFunctionWithName:@"fenceFragmentMain"];
	
	pipelineStateDescriptor.vertexFunction = surfaceVertexProgram;
	pipelineStateDescriptor.fragmentFunction = fenceFragmentProgram;

	self.fence = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Fence rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	pipelineStateDescriptor.vertexFunction = surfaceRotatedVertexProgram;

	self.fenceRotated = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Rotated fence rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	fragmentProgram = [library newFunctionWithName:@"turbulentLitFragmentMain"];

	pipelineStateDescriptor.vertexFunction = surfaceVertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.turbulentLit = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Turbulent lit rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	vertexProgram = [library newFunctionWithName:@"turbulentVertexMain"];
	fragmentProgram = [library newFunctionWithName:@"turbulentFragmentMain"];

	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.turbulent = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Turbulent surface rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	vertexDescriptor = [MTLVertexDescriptor new];
	vertexDescriptor.attributes[0].format = MTLVertexFormatUChar4;
	vertexDescriptor.attributes[0].offset = 0;
	vertexDescriptor.attributes[0].bufferIndex = 0;
	vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
	vertexDescriptor.attributes[1].offset = 4;
	vertexDescriptor.attributes[1].bufferIndex = 0;
	vertexDescriptor.attributes[2].format = MTLVertexFormatFloat;
	vertexDescriptor.attributes[2].offset = 12;
	vertexDescriptor.attributes[2].bufferIndex = 0;
	vertexDescriptor.layouts[0].stride = 16;

	pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;

	vertexProgram = [library newFunctionWithName:@"aliasVertexMain"];
	fragmentProgram = [library newFunctionWithName:@"aliasFragmentMain"];

	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.alias = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Alias model rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	vertexProgram = [library newFunctionWithName:@"viewmodelVertexMain"];
	fragmentProgram = [library newFunctionWithName:@"viewmodelFragmentMain"];

	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.viewmodel = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Viewmodel rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	vertexDescriptor = [MTLVertexDescriptor new];
	vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
	vertexDescriptor.attributes[0].offset = 0;
	vertexDescriptor.attributes[0].bufferIndex = 0;
	vertexDescriptor.attributes[1].format = MTLVertexFormatFloat;
	vertexDescriptor.attributes[1].offset = 16;
	vertexDescriptor.attributes[1].bufferIndex = 0;
	vertexDescriptor.layouts[0].stride = 20;

	pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;

	vertexProgram = [library newFunctionWithName:@"particleVertexMain"];
	fragmentProgram = [library newFunctionWithName:@"particleFragmentMain"];

	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;

	self.particle = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Particle rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return false;
	}

	return true;
}

@end
