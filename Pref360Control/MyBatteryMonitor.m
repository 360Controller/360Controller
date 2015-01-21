//
//  MyBatteryMonitor.m
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import "MyBatteryMonitor.h"
#import "Pref360StyleKit.h"

@implementation MyBatteryMonitor

@synthesize bars;

- (void)setBars:(int)value {
    bars = value;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    [Pref360StyleKit drawBatteryMonitorWithBars:bars];
}

@end
