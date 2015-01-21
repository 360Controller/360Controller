//
//  MyDeadZoneViewer.m
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import "MyDeadZoneViewer.h"
#import "Pref360StyleKit.h"

@implementation MyDeadZoneViewer

-(void)setVal:(double)value {
    _val = value;
    [self setNeedsDisplay:YES];
}

- (void)setLinked:(BOOL)linked {
    _linked = linked;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    [Pref360StyleKit drawDeadZoneViewerWithValue:_val / 32768 linked:_linked];
}

@end
