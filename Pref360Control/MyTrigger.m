//
//  MyTrigger.m
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import "MyTrigger.h"
#import "Pref360StyleKit.h"

@implementation MyTrigger


- (void)setName:(NSString *)name {
    _name = name;
    [self setNeedsDisplay:YES];
}

- (void)setVal:(int)value {
    _val = value;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    if (_name == NULL)
        _name = @"";
    [Pref360StyleKit drawTriggerMetterWithIntensity:(_val / 255.0) triggerTitle:_name];
}

@end
