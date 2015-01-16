/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    MyDigitalStick.m - implementation of digital stick view
    
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
#import "MyDigitalStick.h"

#define INSET_AMOUNT 10

@implementation MyDigitalStick
{
@private
    NSBezierPath *up, *down, *left, *right;
}
@synthesize up = bUp;
@synthesize down = bDown;
@synthesize left = bLeft;
@synthesize right = bRight;

- (void)setUp:(BOOL)anup
{
    bUp = anup;
    self.needsDisplay = YES;
}

- (void)setDown:(BOOL)adown
{
    bDown = adown;
    self.needsDisplay = YES;
}

- (void)setLeft:(BOOL)aleft
{
    bLeft = aleft;
    self.needsDisplay = YES;
}

- (void)setRight:(BOOL)aright
{
    bRight = aright;
    self.needsDisplay = YES;
}

+ (NSBezierPath*)makeTriangle:(int)start inRectangle:(NSRect)rect;
{
    // Create path
    NSBezierPath *path = [NSBezierPath bezierPath];
    NSPoint centre, point;
    const int mult[][2]={
        {0,0},
        {1,0},
        {1,1},
        {0,1},
        {0,0}
    };
    
    // Find central part
    centre.x = rect.origin.x + (rect.size.width / 2);
    centre.y = rect.origin.y + (rect.size.height / 2);
    // Make triangle
    [path moveToPoint:centre];
    point.x = rect.origin.x + (rect.size.width * mult[start][0]);
    point.y = rect.origin.y + (rect.size.height * mult[start][1]);
    [path lineToPoint:point];
    point.x = rect.origin.x + (rect.size.width * mult[start + 1][0]);
    point.y = rect.origin.y + (rect.size.height * mult[start + 1][1]);
    [path lineToPoint:point];
    [path closePath];
    // Done
    return path;
}

- (id)initWithFrame:(NSRect)frameRect
{
    if ((self = [super initWithFrame:frameRect]) != nil) {
        NSRect rect = [self bounds], triangle;
        
        triangle.origin.x = INSET_AMOUNT;
        triangle.origin.y = INSET_AMOUNT;
        triangle.size.width =- INSET_AMOUNT * 2;
        triangle.size.height =- INSET_AMOUNT * 2;
        triangle.size.width = rect.size.width / 3;
        triangle.size.height = rect.size.height / 3;
        triangle.origin.y = rect.origin.y + (triangle.size.height * 2);
        triangle.origin.x = rect.origin.x + triangle.size.width;
        up = [MyDigitalStick makeTriangle:0 inRectangle:triangle];
        triangle.origin.y = rect.origin.y;
        down = [MyDigitalStick makeTriangle:2 inRectangle:triangle];
        triangle.origin.y = rect.origin.y + triangle.size.height;
        triangle.origin.x = rect.origin.x;
        left = [MyDigitalStick makeTriangle:1 inRectangle:triangle];
        triangle.origin.x = rect.origin.x + (triangle.size.width * 2);
        right = [MyDigitalStick makeTriangle:3 inRectangle:triangle];
    }
    return self;
}

- (void)drawRect:(NSRect)rect
{
    NSRect area = [self bounds];
    
    NSDrawLightBezel(area, area);
    [[NSColor blackColor] set];
    if (bUp) [up fill];
    if (bDown) [down fill];
    if (bLeft) [left fill];
    if (bRight) [right fill];
}

@end
