//
//  MyBatteryMonitor.m
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import "MyBatteryMonitor.h"
#import "Pref360StyleKit.h"

@implementation MyBatteryMonitor

- (void)setBars:(int)value {
    _bars = value;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    [Pref360StyleKit drawBatteryMonitorWithBars:_bars];
}

@end
