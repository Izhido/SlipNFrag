//
//  PreferencesController.mm
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 6/29/19.
//  Copyright © 2019 Heriberto Delgado. All rights reserved.
//

#import "PreferencesController.h"
#import "AppDelegate.h"

@interface PreferencesController () <NSTextFieldDelegate>

@property (weak) IBOutlet NSTextField *basedir_label;
@property (weak) IBOutlet NSTextField *basedir_text;
@property (weak) IBOutlet NSButton *basedir_choose;
@property (weak) IBOutlet NSButton *standard_quake_radio;
@property (weak) IBOutlet NSButton *hipnotic_radio;
@property (weak) IBOutlet NSButton *rogue_radio;
@property (weak) IBOutlet NSButton *game_radio;
@property (weak) IBOutlet NSTextField *game_text;
@property (weak) IBOutlet NSTextField *command_line_text;

@end

@implementation PreferencesController

-(void)viewDidLoad
{
    [super viewDidLoad];
    self.game_text.delegate = self;
    self.command_line_text.delegate = self;
    
    NSData* bookmark = [NSUserDefaults.standardUserDefaults objectForKey:@"basedir_bookmark"];
    if (bookmark != nil)
    {
        NSURL* url = [NSURL URLByResolvingBookmarkData:bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:nil error:nil];
        self.basedir_text.stringValue = url.path;
    }

    [self loadBooleanUserDefault:@"standard_quake_radio" intoButton:self.standard_quake_radio];
    [self loadBooleanUserDefault:@"hipnotic_radio" intoButton:self.hipnotic_radio];
    [self loadBooleanUserDefault:@"rogue_radio" intoButton:self.rogue_radio];
    [self loadBooleanUserDefault:@"game_radio" intoButton:self.game_radio];
    [self loadStringUserDefault:@"game_text" intoTextField:self.game_text];
    [self loadStringUserDefault:@"command_line_text" intoTextField:self.command_line_text];
    if (self.standard_quake_radio.state == NSControlStateValueOff && self.hipnotic_radio.state == NSControlStateValueOff && self.rogue_radio.state == NSControlStateValueOff && self.game_radio.state == NSControlStateValueOff)
    {
        self.standard_quake_radio.state = NSControlStateValueOn;
    }
}

-(void)viewWillDisappear
{
    [super viewWillDisappear];
    AppDelegate* appDelegate = NSApplication.sharedApplication.delegate;
    appDelegate.preferencesController = nil;
}

-(void)controlTextDidChange:(NSNotification *)notification
{
    NSTextField* textField = notification.object;
    if (textField == self.game_text)
    {
        [self storeStringUserDefault:@"game_text" fromTextField:self.game_text];
    }
    else if (textField == self.command_line_text)
    {
        [self storeStringUserDefault:@"command_line_text" fromTextField:self.command_line_text];
    }
}

-(IBAction)chooseMissionPack:(NSButton *)sender
{
    [self storeBooleanUserDefault:@"standard_quake_radio" fromButton:sender ifMatchingButton:self.standard_quake_radio];
    [self storeBooleanUserDefault:@"hipnotic_radio" fromButton:sender ifMatchingButton:self.hipnotic_radio];
    [self storeBooleanUserDefault:@"rogue_radio" fromButton:sender ifMatchingButton:self.rogue_radio];
    [self storeBooleanUserDefault:@"game_radio" fromButton:sender ifMatchingButton:self.game_radio];
}

- (IBAction)chooseBaseDir:(NSButton *)sender
{
    NSOpenPanel* openPanel = [NSOpenPanel new];
    openPanel.canChooseFiles = NO;
    openPanel.canChooseDirectories = YES;
    if ([openPanel runModal] == NSModalResponseOK)
    {
        NSURL* url = openPanel.URL;
        self.basedir_text.stringValue = url.path;
        NSData* bookmark = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:nil];
        [NSUserDefaults.standardUserDefaults setObject:bookmark forKey:@"basedir_bookmark"];
        [url startAccessingSecurityScopedResource];
        BOOL found = [NSFileManager.defaultManager fileExistsAtPath:[url.path stringByAppendingPathComponent:@"ID1/PAK0.PAK"]];
        [url stopAccessingSecurityScopedResource];
        if (!found)
        {
            NSAlert* alert = [NSAlert new];
            [alert setMessageText:@"Core game data not found"];
            [alert setInformativeText:[NSString stringWithFormat:@"The folder %@ does not contain a folder ID1 with a file PAK0.PAK - the game might not start.\n\nEnsure that all required files are present before starting the game.", url.path]];
            [alert setAlertStyle:NSAlertStyleCritical];
            [alert beginSheetModalForWindow:self.view.window completionHandler:nil];
        }
    }
}

-(void)loadStringUserDefault:(NSString*)key intoTextField:(NSTextField*)textField
{
    NSString* value = [NSUserDefaults.standardUserDefaults stringForKey:key];
    if (value == nil)
    {
        value = @"";
    }
    textField.stringValue = value;
}

-(void)loadBooleanUserDefault:(NSString*)key intoButton:(NSButton*)button
{
    BOOL value = [NSUserDefaults.standardUserDefaults boolForKey:key];
    if (value)
    {
        button.state = NSControlStateValueOn;
    }
    else
    {
        button.state = NSControlStateValueOff;
    }
}

-(void)storeStringUserDefault:(NSString*)key fromTextField:(NSTextField*)textField
{
    NSString* value = textField.stringValue;
    if (value == nil || [value isEqualToString:@""])
    {
        [NSUserDefaults.standardUserDefaults removeObjectForKey:key];
    }
    else
    {
        [NSUserDefaults.standardUserDefaults setObject:value forKey:key];
    }
}

-(void)storeBooleanUserDefault:(NSString*)key fromButton:(NSButton*)button ifMatchingButton:(NSButton*)matchingButton
{
    if (button == matchingButton && button.state == NSControlStateValueOn)
    {
        [NSUserDefaults.standardUserDefaults setObject:@YES forKey:key];
    }
    else
    {
        [NSUserDefaults.standardUserDefaults removeObjectForKey:key];
    }
}

@end
