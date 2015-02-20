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

#define PRESSED_INSET   5
#define AREA_INSET      4

@implementation MyAnalogStick
@synthesize pressed;
@synthesize deadzone;
@synthesize positionX = x;
@synthesize positionY = y;
@synthesize realPositionX = realX;
@synthesize realPositionY = realY;
@synthesize normalized = normalized;
@synthesize linked;

- (void)setPressed:(BOOL)apressed
{
    pressed = apressed;
    self.needsDisplay = YES;
}

- (void)setDeadzone:(int)adeadzone
{
    deadzone = adeadzone;
    self.needsDisplay = YES;
}

- (void)setPositionX:(int)positionX
{
    x = positionX;
    
    if (normalized)
    {
        const UInt16 max16 = 32767;
        float maxVal = max16 - deadzone;
        
        if (x > 0)
            realX = (abs(x) * maxVal / max16) + deadzone + 1;
        else if (x < 0)
            realX = -((abs(x) * maxVal / max16) + deadzone + 1);
        else // x == 0
            realX = 0;
    }
    else
    {
        realX = 0;
    }
    
    self.needsDisplay = YES;
}

- (void)setPositionY:(int)positionY
{
    y = positionY;
    
    if (normalized)
    {
        const UInt16 max16 = 32767;
        float maxVal = max16 - deadzone;
        
        if (y > 0)
            realY = (abs(y) * maxVal / max16) + deadzone + 1;
        else if (y < 0)
            realY = -((abs(y) * maxVal / max16) + deadzone + 1);
        else // y == 0
            realY = 0;
    }
    else
    {
        realY = 0;
    }
    
    self.needsDisplay = YES;
}

- (void)setNormalized:(BOOL)isNormalized
{
    normalized = isNormalized;
}

- (void)setLinked:(BOOL)alinked
{
    linked = alinked;
    self.needsDisplay = YES;
}

- (void)drawRect:(NSRect)rect
{
    NSRect area = [self bounds], deadRect, posRect, realPosRect;
    
    // Compute positions
    // Deadzone
    deadRect.size.width = (deadzone * area.size.width) / 32768;
    deadRect.size.height = (deadzone * area.size.height) / 32768;
    deadRect.origin.x = area.origin.x + ((area.size.width - deadRect.size.width) / 2);
    deadRect.origin.y = area.origin.y + ((area.size.height - deadRect.size.height) / 2);
    // Position
    posRect.size.width = 4;
    posRect.size.height = 4;
    posRect.origin.x = area.origin.x + AREA_INSET + (((x + 32768) * (area.size.width - (AREA_INSET * 2))) / 65536) - (posRect.size.width / 2);
    posRect.origin.y = area.origin.y + area.size.height - AREA_INSET - (((y + 32768) * (area.size.height - (AREA_INSET * 2))) / 65536) - (posRect.size.height / 2);
    // Real Position
    realPosRect.size.width = 4;
    realPosRect.size.height = 4;
    realPosRect.origin.x = area.origin.x + AREA_INSET + (((realX + 32768) * (area.size.width - (AREA_INSET * 2))) / 65536) - (posRect.size.width / 2);
    realPosRect.origin.y = area.origin.y + area.size.height - AREA_INSET - (((realY + 32768) * (area.size.height - (AREA_INSET * 2))) / 65536) - (posRect.size.height / 2);
    // Draw border
    NSDrawLightBezel(area,area);
    // Draw pressed state
    if(pressed) {
        NSRect pressArea;
        
        pressArea=area;
        pressArea.origin.x += PRESSED_INSET;
        pressArea.origin.y += PRESSED_INSET;
        pressArea.size.width -= PRESSED_INSET * 2;
        pressArea.size.height -= PRESSED_INSET * 2;
        [[NSColor blackColor] set];
        NSRectFill(pressArea);
    }
    // Draw deadzone
    if (deadzone != 0) {
        [[NSColor redColor] set];
        if (linked)  NSFrameRect(deadRect);
        else {
            NSRect trueRect;
            
            trueRect = deadRect;
            trueRect.origin.x = area.origin.x;
            trueRect.size.width = area.size.width;
            NSFrameRect(trueRect);
            trueRect = deadRect;
            trueRect.origin.y = area.origin.y;
            trueRect.size.height = area.size.height;
            NSFrameRect(trueRect);
        }
    }
    // Draw real position
    if (realX || realY)
    {
        if (pressed) [[NSColor colorWithDeviceWhite:.3 alpha:1] set];
        else [[NSColor colorWithDeviceWhite:.7 alpha:1] set];
        NSRectFill(realPosRect);
    }
    // Draw position
    if (pressed) [[NSColor whiteColor] set];
    else [[NSColor blackColor] set];
    NSRectFill(posRect);
}

- (void)setPositionX:(int)xPos y:(int)yPos
{
    // This does not trigger the key-value observer.
    x = xPos;
    // This does.
    // Done so the setNeedsDisplay: is only called once
    self.positionY = yPos;
}

@end
