//
//  MyTrigger.m
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import "MyTrigger.h"
#import "Pref360StyleKit.h"

@implementation MyTrigger

@synthesize name;
@synthesize val;

- (void)setName:(NSString *)aName {
    name = aName;
    [self setNeedsDisplay:YES];
}

- (void)setVal:(int)value {
    val = value;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    if (name == NULL) 
        name = @"";
    [Pref360StyleKit drawTriggerMetterWithIntensity:(val / 255.0) triggerTitle:name];
}

@end
