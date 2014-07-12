/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro
 
 DeviceLister.h - decleration of a class to display supported devices
 
 This file is part of Xbox360Controller.
 
 Xbox360Controller is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 Xbox360Controller is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Foobar; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#import <Cocoa/Cocoa.h>

@class Pref360ControlPref;

@interface DeviceLister : NSObject <NSTableViewDataSource>
#ifdef __i386__
{
@private
    Pref360ControlPref *owner;
    NSWindow *sheet;
    NSTableView *list;
    
    NSMutableDictionary *entries;
    NSMutableArray *connected, *enabled;
    
    BOOL changed;
}
#endif
@property (arcweak) IBOutlet NSWindow *sheet;
@property (arcweak) IBOutlet NSTableView *list;

- (void)showWithOwner:(Pref360ControlPref*)pane;
- (IBAction)done:(id)sender;

@end
