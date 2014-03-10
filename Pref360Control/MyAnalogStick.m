/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    MyAnalogStick.m - implementation of analog stick view
    
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
#import "MyAnalogStick.h"

#define PRESSED_INSET       5
#define AREA_INSET          4

@implementation MyAnalogStick
@synthesize pressed;
@synthesize appearance;
@synthesize deadzone;
@synthesize positionX = x;
@synthesize positionY = y;
@synthesize linked;

- (id)initWithFrame:(NSRect)frameRect
{
    if (self = [super initWithFrame:frameRect]) {
        [self addObserver:self forKeyPath:@"deadzone" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld context:NULL];
        [self addObserver:self forKeyPath:@"positionX" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld context:NULL];
        [self addObserver:self forKeyPath:@"positionY" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld context:NULL];
        [self addObserver:self forKeyPath:@"pressed" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld context:NULL];
        [self addObserver:self forKeyPath:@"linked" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld context:NULL];
    }
    return self;
}

- (void)dealloc
{
    [self removeObserver:self forKeyPath:@"pressed"];
    [self removeObserver:self forKeyPath:@"positionX"];
    [self removeObserver:self forKeyPath:@"positiony"];
    [self removeObserver:self forKeyPath:@"deadzone"];
    [self removeObserver:self forKeyPath:@"linked"];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (object == self) {
        [self setNeedsDisplay:YES];
    }
}

- (void)drawRect:(NSRect)rect
{
    NSRect area = [self bounds], deadRect, posRect;
    
    // Compute positions
    // Deadzone
    deadRect.size.width=(deadzone*area.size.width)/32768;
    deadRect.size.height=(deadzone*area.size.height)/32768;
    deadRect.origin.x=area.origin.x+((area.size.width-deadRect.size.width)/2);
    deadRect.origin.y=area.origin.y+((area.size.height-deadRect.size.height)/2);
    // Position
    posRect.size.width=4;
    posRect.size.height=4;
    posRect.origin.x=area.origin.x+AREA_INSET+(((x+32768)*(area.size.width-(AREA_INSET*2)))/65536)-(posRect.size.width/2);
    posRect.origin.y=area.origin.y+area.size.height-AREA_INSET-(((y+32768)*(area.size.height-(AREA_INSET*2)))/65536)-(posRect.size.height/2);
    // Draw border
    NSDrawLightBezel(area,area);
    // Draw pressed state
    if(pressed) {
        NSRect pressArea;
        
        pressArea=area;
        pressArea.origin.x+=PRESSED_INSET;
        pressArea.origin.y+=PRESSED_INSET;
        pressArea.size.width-=PRESSED_INSET*2;
        pressArea.size.height-=PRESSED_INSET*2;
        [[NSColor blackColor] set];
        NSRectFill(pressArea);
    }
    // Draw deadzone
    if (deadzone != 0) {
        [[NSColor redColor] set];
        if (linked) {
            NSFrameRect(deadRect);
        } else {
            NSRect trueRect;
            
            trueRect=deadRect;
            trueRect.origin.x=area.origin.x;
            trueRect.size.width=area.size.width;
            NSFrameRect(trueRect);
            trueRect=deadRect;
            trueRect.origin.y=area.origin.y;
            trueRect.size.height=area.size.height;
            NSFrameRect(trueRect);
        }
    }
    // Draw position
    if (pressed)
        [[NSColor whiteColor] set];
    else
        [[NSColor blackColor] set];
    NSRectFill(posRect);
}

- (void)setPositionX:(int)xPos y:(int)yPos
{
    x = xPos;
    self.positionY = yPos;
}

@end
