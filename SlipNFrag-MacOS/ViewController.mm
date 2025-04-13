//
//  ViewController.mm
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 5/29/19.
//  Copyright © 2019 Heriberto Delgado. All rights reserved.
//

#import "ViewController.h"
#import <MetalKit/MetalKit.h>
#import <GameController/GameController.h>
#import "Engine.h"
#include "scantokey.h"
#include "sys_macos.h"
#include "vid_macos.h"
#include "in_macos.h"
#include "Locks.h"
#include "DirectRect.h"
#include "AppDelegate.h"
#include "Input.h"

extern m_state_t m_state;

@interface ViewController () <MTKViewDelegate>

@end

@implementation ViewController
{
    NSButton* playButton;
    NSButton* preferencesButton;
	NSThread* engineThread;
    id<MTLCommandQueue> commandQueue;
    MTKView* mtkView;
    id<MTLRenderPipelineState> pipelineState;
    id<MTLTexture> screen;
    id<MTLTexture> console;
    id<MTLTexture> palette;
    id<MTLSamplerState> screenSamplerState;
    id<MTLSamplerState> consoleSamplerState;
	MTLRegion screenRegion;
	MTLRegion consoleRegion;
	MTLRegion paletteRegion;
    id<MTLBuffer> screenPlane;
    BOOL firstFrame;
    NSEventModifierFlags previousModifierFlags;
    NSTrackingArea* trackingArea;
    NSCursor* blankCursor;
	BOOL stopDrawing;
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    playButton = [[NSButton alloc] initWithFrame:NSZeroRect];
    playButton.image = [NSImage imageNamed:@"play"];
    playButton.alternateImage = [NSImage imageNamed:@"play-pressed"];
    playButton.bordered = NO;
    playButton.translatesAutoresizingMaskIntoConstraints = NO;
	[playButton setButtonType:NSButtonTypeMomentaryChange];
    [playButton setTarget:self];
    [playButton setAction:@selector(play:)];
    [self.view addSubview:playButton];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:playButton attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeCenterX multiplier:1 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:playButton attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeCenterY multiplier:1 constant:0]];
    [playButton addConstraint:[NSLayoutConstraint constraintWithItem:playButton attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:nil attribute:NSLayoutAttributeNotAnAttribute multiplier:1 constant:150]];
    [playButton addConstraint:[NSLayoutConstraint constraintWithItem:playButton attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:nil attribute:NSLayoutAttributeNotAnAttribute multiplier:1 constant:150]];
    preferencesButton = [[NSButton alloc] initWithFrame:NSZeroRect];
    preferencesButton.image = [NSImage imageNamed:NSImageNameAdvanced];
    preferencesButton.bordered = NO;
    preferencesButton.translatesAutoresizingMaskIntoConstraints = NO;
    preferencesButton.imageScaling = NSImageScaleProportionallyUpOrDown;
	[preferencesButton setButtonType:NSButtonTypeMomentaryChange];
    [preferencesButton setTarget:self];
    [preferencesButton setAction:@selector(preferences:)];
    [self.view addSubview:preferencesButton];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:preferencesButton attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeRight multiplier:1 constant:-10]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:preferencesButton attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeBottom multiplier:1 constant:-10]];
    [preferencesButton addConstraint:[NSLayoutConstraint constraintWithItem:preferencesButton attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:nil attribute:NSLayoutAttributeNotAnAttribute multiplier:1 constant:24]];
    [preferencesButton addConstraint:[NSLayoutConstraint constraintWithItem:preferencesButton attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:nil attribute:NSLayoutAttributeNotAnAttribute multiplier:1 constant:24]];
}

-(void)viewWillAppear
{
    [super viewWillAppear];
    trackingArea = [[NSTrackingArea alloc] initWithRect:mtkView.frame options:NSTrackingCursorUpdate | NSTrackingMouseMoved | NSTrackingActiveAlways owner:self userInfo:nil];
    [mtkView addTrackingArea:trackingArea];
    [NSNotificationCenter.defaultCenter addObserverForName:NSViewFrameDidChangeNotification object:mtkView queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note)
    {
        [self->mtkView removeTrackingArea:self->trackingArea];
        self->trackingArea = [[NSTrackingArea alloc] initWithRect:self->mtkView.frame options:NSTrackingCursorUpdate | NSTrackingMouseMoved | NSTrackingActiveAlways owner:self userInfo:nil];
        [self->mtkView addTrackingArea:self->trackingArea];
    }];
}

-(void)viewWillDisappear
{
    [super viewWillDisappear];
    [mtkView removeTrackingArea:trackingArea];
}

-(void)cursorUpdate:(NSEvent *)event
{
    [blankCursor set];
}

-(BOOL)performMouseMove:(NSEvent *)event
{
	if (mouseinitialized && key_dest == key_game)
	{
		CGPoint point = [self->mtkView convertPoint:event.locationInWindow fromView:nil];
		if ([self->mtkView mouse:point inRect:self->mtkView.bounds])
		{
			mx += event.deltaX;
			my += event.deltaY;
			NSScreen* screen = self.view.window.screen;
			CGDirectDisplayID displayId = [[screen.deviceDescription objectForKey:@"NSScreenNumber"] intValue];
			NSRect screenFrame = screen.frame;
			NSRect frame = self->mtkView.frame;
			NSRect centerFrame = NSMakeRect(NSMidX(frame), NSMidY(frame), 1, 1);
			NSRect centerFrameInWindow = [self->mtkView convertRect:centerFrame toView:nil];
			NSRect centerFrameInScreen = [self.view.window convertRectToScreen:centerFrameInWindow];
			CGPoint point = CGPointMake(centerFrameInScreen.origin.x, screenFrame.size.height - centerFrameInScreen.origin.y);
			CGDisplayMoveCursorToPoint(displayId, point);
			return YES;
		}
	}
	return NO;
}

-(void)mouseMoved:(NSEvent *)event
{
    if (![self performMouseMove:event])
    {
        [super mouseMoved:event];
    }
}

-(void)mouseDragged:(NSEvent *)event
{
	if (![self performMouseMove:event])
	{
		[super mouseDragged:event];
	}
}

-(void)rightMouseDragged:(NSEvent *)event
{
	if (![self performMouseMove:event])
	{
		[super rightMouseDragged:event];
	}
}

-(void)otherMouseDragged:(NSEvent *)event
{
	if (![self performMouseMove:event])
	{
		[super otherMouseDragged:event];
	}
}

-(void)mouseDown:(NSEvent *)event
{
    if (event.type == NSEventTypeLeftMouseDown)
    {
		Input::AddKeyInput(K_MOUSE1, true);
    }
    else if (event.type == NSEventTypeRightMouseDown)
    {
		Input::AddKeyInput(K_MOUSE3, true);
    }
    else if (event.type == NSEventTypeOtherMouseDown)
    {
		Input::AddKeyInput(K_MOUSE2, true);
    }
    else
    {
        [super mouseDown:event];
    }
}

-(void)mouseUp:(NSEvent *)event
{
    if (event.type == NSEventTypeLeftMouseUp)
    {
		Input::AddKeyInput(K_MOUSE1, false);
    }
    else if (event.type == NSEventTypeRightMouseUp)
    {
		Input::AddKeyInput(K_MOUSE3, false);
    }
    else if (event.type == NSEventTypeOtherMouseUp)
    {
		Input::AddKeyInput(K_MOUSE2, false);
    }
    else
    {
        [super mouseUp:event];
    }
}

-(void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size
{
}

-(void)buildScreenTexturesWithWidth:(CGFloat)width andHeight:(CGFloat)height
{
	vid_width = (int)width;
	vid_height = (int)height;
	[self calculateConsoleDimensions];
	
	VID_Resize();
	
	MTLTextureDescriptor* screenDescriptor = [MTLTextureDescriptor new];
	screenDescriptor.pixelFormat = MTLPixelFormatR8Uint;
	screenDescriptor.width = vid_width;
	screenDescriptor.height = vid_height;
	screenDescriptor.storageMode = MTLStorageModeManaged;
	screen = [mtkView.device newTextureWithDescriptor:screenDescriptor];
	
	screenRegion = MTLRegionMake2D(0, 0, vid_width, vid_height);
	
	MTLTextureDescriptor* consoleDescriptor = [MTLTextureDescriptor new];
	consoleDescriptor.pixelFormat = MTLPixelFormatR8Uint;
	consoleDescriptor.width = con_width;
	consoleDescriptor.height = con_height;
	screenDescriptor.storageMode = MTLStorageModeManaged;
	console = [mtkView.device newTextureWithDescriptor:consoleDescriptor];
	
	consoleRegion = MTLRegionMake2D(0, 0, con_width, con_height);
}

-(void)drawInMTKView:(MTKView *)view
{
	if (sys_quitcalled)
	{
		[NSApplication.sharedApplication.mainWindow close];
	}
	
	if ([self displaySysErrorIfNeeded])
	{
		return;
	}
	
	auto width = mtkView.bounds.size.width;
	auto height = mtkView.bounds.size.height;

	{
		std::lock_guard<std::mutex> lock(Locks::RenderMutex);
		
		if (firstFrame)
		{
			[self buildScreenTexturesWithWidth:width andHeight:height];
			firstFrame = NO;
		}
		
		[screen replaceRegion:screenRegion mipmapLevel:0 withBytes:vid_buffer.data() bytesPerRow:vid_width];
		[console replaceRegion:consoleRegion mipmapLevel:0 withBytes:con_buffer.data() bytesPerRow:con_width];
		[palette replaceRegion:paletteRegion mipmapLevel:0 withBytes:d_8to24table bytesPerRow:0];
	}
		
	{
		std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
		
		for (auto& directRect : DirectRect::directRects)
		{
			auto x = directRect.x * (consoleRegion.size.width - directRect.width) / (directRect.vid_width - directRect.width);
			auto y = directRect.y * (consoleRegion.size.height - directRect.width) / (directRect.vid_height - directRect.height);

			MTLRegion directRegion = MTLRegionMake2D(x, y, directRect.width, directRect.height);
			[console replaceRegion:directRegion mipmapLevel:0 withBytes:directRect.data bytesPerRow:directRect.width];
		}
	}

	id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
	MTLRenderPassDescriptor* renderPassDescriptor = mtkView.currentRenderPassDescriptor;
	id<CAMetalDrawable> currentDrawable = mtkView.currentDrawable;
	if (renderPassDescriptor != nil && currentDrawable != nil)
	{
		renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
		renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
		id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
		[commandEncoder setRenderPipelineState:pipelineState];
		[commandEncoder setVertexBuffer:screenPlane offset:0 atIndex:0];
		[commandEncoder setFragmentTexture:screen atIndex:0];
		[commandEncoder setFragmentTexture:console atIndex:1];
		[commandEncoder setFragmentTexture:palette atIndex:2];
		[commandEncoder setFragmentSamplerState:screenSamplerState atIndex:0];
		[commandEncoder setFragmentSamplerState:consoleSamplerState atIndex:1];
		[commandEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
		[commandEncoder endEncoding];
		[commandBuffer presentDrawable:currentDrawable];
	}
	[commandBuffer commit];

	if (vid_width != (int)width || vid_height != (int)height)
	{
		std::lock_guard<std::mutex> lock(Locks::RenderMutex);
		
		[self buildScreenTexturesWithWidth:width andHeight:height];
	}
}

-(void)play:(NSButton*)sender
{
    [playButton removeFromSuperview];

	vid_width = (int)self.view.frame.size.width;
	vid_height = (int)self.view.frame.size.height;
	[self calculateConsoleDimensions];

	auto arguments = [NSMutableArray<NSString*> new];
	[arguments addObject:@"SlipNFrag-MacOS"];
	NSData* basedir = [NSUserDefaults.standardUserDefaults objectForKey:@"basedir_bookmark"];
	if (basedir != nil)
	{
		NSURL* url = [NSURL URLByResolvingBookmarkData:basedir options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:nil error:nil];
		[url startAccessingSecurityScopedResource];
		[arguments addObject:[NSString stringWithCString:"-basedir" encoding:[NSString defaultCStringEncoding]]];
		[arguments addObject:url.path];
	}
	if ([NSUserDefaults.standardUserDefaults boolForKey:@"hipnotic_radio"])
	{
		[arguments addObject:[NSString stringWithCString:"-hipnotic" encoding:[NSString defaultCStringEncoding]]];
	}
	else if ([NSUserDefaults.standardUserDefaults boolForKey:@"rogue_radio"])
	{
		[arguments addObject:[NSString stringWithCString:"-rogue" encoding:[NSString defaultCStringEncoding]]];
	}
	else if ([NSUserDefaults.standardUserDefaults boolForKey:@"game_radio"])
	{
		auto game = [NSUserDefaults.standardUserDefaults stringForKey:@"game_text"];
		if (game != nil && ![game isEqualToString:@""])
		{
			[arguments addObject:[NSString stringWithCString:"-game" encoding:[NSString defaultCStringEncoding]]];
			[arguments addObject:game];
		}
	}
	auto commandLine = [NSUserDefaults.standardUserDefaults stringForKey:@"command_line_text"];
	if (commandLine != nil && ![commandLine isEqualToString:@""])
	{
		NSArray<NSString*>* components = [commandLine componentsSeparatedByString:@" "];
		[arguments addObjectsFromArray:components];
	}

	engineThread = [[NSThread alloc] initWithBlock:^{
		auto engine = [Engine new];
		[engine StartEngine:arguments];
	}];
	engineThread.name = @"Engine Thread";
	[engineThread start];

	while (!host_initialized && !Locks::StopEngine)
	{
		[NSThread sleepForTimeInterval:0];
	}

	if ([self displaySysErrorIfNeeded])
	{
		return;
	}

	id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    mtkView = [[MTKView alloc] initWithFrame:CGRectZero device:device];
    mtkView.delegate = self;
    mtkView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:mtkView];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:mtkView attribute:NSLayoutAttributeLeft relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeLeft multiplier:1 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:mtkView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeTop multiplier:1 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:mtkView attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeRight multiplier:1 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:mtkView attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeBottom multiplier:1 constant:0]];
    commandQueue = [device newCommandQueue];
    id<MTLLibrary> library = [device newDefaultLibrary];
    id<MTLFunction> vertexProgram = [library newFunctionWithName:@"vertexMain"];
    id<MTLFunction> fragmentProgram = [library newFunctionWithName:@"fragmentMain"];
    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor new];
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[0].offset = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[1].offset = 16;
    vertexDescriptor.attributes[1].bufferIndex = 0;
    vertexDescriptor.layouts[0].stride = 24;
    MTLRenderPipelineDescriptor* pipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
    pipelineStateDescriptor.vertexFunction = vertexProgram;
    pipelineStateDescriptor.fragmentFunction = fragmentProgram;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat;
    pipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineStateDescriptor.sampleCount = mtkView.sampleCount;
    NSError* error = nil;
    pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (error != nil)
    {
        NSLog(@"%@", error);
		[NSApplication.sharedApplication.mainWindow close];
    }
    MTLTextureDescriptor* paletteDescriptor = [MTLTextureDescriptor new];
    paletteDescriptor.textureType = MTLTextureType1D;
    paletteDescriptor.width = 256;
    paletteDescriptor.storageMode = MTLStorageModeManaged;
    palette = [device newTextureWithDescriptor:paletteDescriptor];
    MTLSamplerDescriptor* screenSamplerDescriptor = [MTLSamplerDescriptor new];
    screenSamplerState = [device newSamplerStateWithDescriptor:screenSamplerDescriptor];
    MTLSamplerDescriptor* consoleSamplerDescriptor = [MTLSamplerDescriptor new];
    consoleSamplerState = [device newSamplerStateWithDescriptor:consoleSamplerDescriptor];
	paletteRegion = MTLRegionMake1D(0, 256);
	float screenPlaneVertices[] = { -1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, -1, -1, 0, 1, 0, 1, 1, -1, 0, 1, 1, 1 };
    screenPlane = [device newBufferWithBytes:screenPlaneVertices length:sizeof(screenPlaneVertices) options:0];
	
    previousModifierFlags = 0;
    blankCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"blank"] hotSpot:NSZeroPoint];
    firstFrame = YES;
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent * _Nullable(NSEvent* _Nonnull event)
     {
         if (!self.view.window.isKeyWindow)
         {
             return event;
         }
         unsigned short code = event.keyCode;
         int mapped = scantokey[code];
         Input::AddKeyInput(mapped, true);
         return nil;
     }];
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:^NSEvent * _Nullable(NSEvent * _Nonnull event)
     {
         if (!self.view.window.isKeyWindow)
         {
             return event;
         }
         unsigned short code = event.keyCode;
         int mapped = scantokey[code];
         Input::AddKeyInput(mapped, false);
         return nil;
     }];
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskFlagsChanged handler:^NSEvent * _Nullable(NSEvent * _Nonnull event)
     {
         if (!self.view.window.isKeyWindow)
         {
             return event;
         }
         if ((event.modifierFlags & NSEventModifierFlagOption) != 0 && (self->previousModifierFlags & NSEventModifierFlagOption) == 0)
         {
			 Input::AddKeyInput(K_ALT, true);
         }
         else if ((event.modifierFlags & NSEventModifierFlagOption) == 0 && (self->previousModifierFlags & NSEventModifierFlagOption) != 0)
         {
			 Input::AddKeyInput(K_ALT, false);
         }
         if ((event.modifierFlags & NSEventModifierFlagControl) != 0 && (self->previousModifierFlags & NSEventModifierFlagControl) == 0)
         {
			 Input::AddKeyInput(K_CTRL, true);
         }
         else if ((event.modifierFlags & NSEventModifierFlagControl) == 0 && (self->previousModifierFlags & NSEventModifierFlagControl) != 0)
         {
			 Input::AddKeyInput(K_CTRL, false);
         }
         if ((event.modifierFlags & NSEventModifierFlagShift) != 0 && (self->previousModifierFlags & NSEventModifierFlagShift) == 0)
         {
			 Input::AddKeyInput(K_SHIFT, true);
         }
         else if ((event.modifierFlags & NSEventModifierFlagShift) == 0 && (self->previousModifierFlags & NSEventModifierFlagShift) != 0)
         {
			 Input::AddKeyInput(K_SHIFT, false);
         }
         self->previousModifierFlags = event.modifierFlags;
         return nil;
     }];
}

-(void)preferences:(NSButton*)sender
{
    AppDelegate* appDelegate = NSApplication.sharedApplication.delegate;
    [appDelegate preferences:nil];
}

-(void)calculateConsoleDimensions
{
    auto factor = (double)vid_width / 320;
    double new_conwidth = 320;
    auto new_conheight = ceil((double)vid_height / factor);
    if (new_conheight < 200)
    {
        factor = (double)vid_height / 200;
        new_conheight = 200;
        new_conwidth = (double)(((int)ceil((double)vid_width / factor) + 3) & ~3);
    }
    con_width = (int)new_conwidth;
    con_height = (int)new_conheight;
}

-(BOOL)displaySysErrorIfNeeded
{
    if (!stopDrawing && !sys_errormessage.empty())
    {
        if (sys_nogamedata)
        {
            NSAlert* alert = [NSAlert new];
            [alert setMessageText:@"Game data not found"];
            [alert setInformativeText:@"Go to Preferences and set Game directory (-basedir) to your copy of the game files."];
            [alert addButtonWithTitle:@"Go to Preferences"];
            [alert addButtonWithTitle:@"Where to buy the game"];
            [alert addButtonWithTitle:@"Where to get shareware episode"];
            [alert setAlertStyle:NSAlertStyleCritical];
            [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode)
            {
                [NSApplication.sharedApplication.mainWindow close];
                if (returnCode == NSAlertFirstButtonReturn)
                {
                    AppDelegate* appDelegate = NSApplication.sharedApplication.delegate;
                    [appDelegate preferences:nil];
                }
                else if (returnCode == NSAlertSecondButtonReturn)
                {
                    [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"https://www.google.com/search?q=buy+quake+1+game"]];
                }
                else if (returnCode == NSAlertThirdButtonReturn)
                {
                    [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"https://www.google.com/search?q=download+quake+1+shareware+episode"]];
                }
            }];
        }
        else
        {
            NSAlert* alert = [NSAlert new];
            [alert setMessageText:@"Sys_Error"];
            [alert setInformativeText:[NSString stringWithFormat:@"%s\n\nThe application will now close.", sys_errormessage.c_str()]];
            [alert addButtonWithTitle:@"Ok"];
            [alert setAlertStyle:NSAlertStyleCritical];
            [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode)
            {
                [NSApplication.sharedApplication.mainWindow close];
            }];
        }
		stopDrawing = true;
        return YES;
    }
    return NO;
}

@end
