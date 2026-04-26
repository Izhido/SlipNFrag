//
//  AppDelegate.m
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 5/29/19.
//  Copyright © 2019 Heriberto Delgado. All rights reserved.
//

#import "AppDelegate.h"
#import "Slip___Frag-Swift.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)sender
{
    return YES;
}

- (IBAction)preferences:(NSMenuItem *)sender
{
	[[SettingsManager shared] openSettings];
}

@end
