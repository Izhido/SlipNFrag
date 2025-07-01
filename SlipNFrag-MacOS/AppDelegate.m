//
//  AppDelegate.m
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 5/29/19.
//  Copyright © 2019 Heriberto Delgado. All rights reserved.
//

#import "AppDelegate.h"

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
    if (self.preferencesController != nil)
    {
        return;
    }
    self.preferencesController = [[NSStoryboard storyboardWithName:@"Main" bundle:NSBundle.mainBundle] instantiateControllerWithIdentifier:@"Preferences"];
    [self.preferencesController showWindow:NSApplication.sharedApplication.mainWindow];
}

@end
