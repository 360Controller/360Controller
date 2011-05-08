//
//  DeviceLister.h
//  Pref360Control
//
//  Created by Colin Munro on 8/05/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class Pref360ControlPref;

@interface DeviceLister : NSObject {
    Pref360ControlPref *owner;
    IBOutlet NSWindow *sheet;
    IBOutlet NSTableView *list;
    
    NSMutableDictionary *entries;
    NSMutableArray *connected, *enabled;
    
    BOOL changed;
}

- (void)showWithOwner:(Pref360ControlPref*)pane;
- (IBAction)done:(id)sender;

@end
