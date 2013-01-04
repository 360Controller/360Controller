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

#define INSET_AMOUNT        10

@implementation MyDigitalStick

- (NSBezierPath*)makeTriangle:(int)start inRectangle:(NSRect)rect;
{
    NSBezierPath *path;
    NSPoint centre,point;
    const int mult[][2]={
        {0,0},
        {1,0},
        {1,1},
        {0,1},
        {0,0}
    };
    
    // Find central part
    centre.x=rect.origin.x+(rect.size.width/2);
    centre.y=rect.origin.y+(rect.size.height/2);
    // Create path
    path=[NSBezierPath bezierPath];
    // Make triangle
    [path moveToPoint:centre];
    point.x=rect.origin.x+(rect.size.width*mult[start][0]);
    point.y=rect.origin.y+(rect.size.height*mult[start][1]);
    [path lineToPoint:point];
    point.x=rect.origin.x+(rect.size.width*mult[start+1][0]);
    point.y=rect.origin.y+(rect.size.height*mult[start+1][1]);
    [path lineToPoint:point];
    [path closePath];
    // Done
    return [path retain];
}

- (id)initWithFrame:(NSRect)frameRect
{
	if ((self = [super initWithFrame:frameRect]) != nil) {
        NSRect rect,triangle;
        
        bUp=bDown=bLeft=bRight=FALSE;
        rect=[self bounds];
        triangle.origin.x+=INSET_AMOUNT;
        triangle.origin.y+=INSET_AMOUNT;
        triangle.size.width-=INSET_AMOUNT*2;
        triangle.size.height-=INSET_AMOUNT*2;
        triangle.size.width=rect.size.width/3;
        triangle.size.height=rect.size.height/3;
        triangle.origin.y=rect.origin.y+(triangle.size.height*2);
        triangle.origin.x=rect.origin.x+triangle.size.width;
        up=[self makeTriangle:0 inRectangle:triangle];
        triangle.origin.y=rect.origin.y;
        down=[self makeTriangle:2 inRectangle:triangle];
        triangle.origin.y=rect.origin.y+triangle.size.height;
        triangle.origin.x=rect.origin.x;
        left=[self makeTriangle:1 inRectangle:triangle];
        triangle.origin.x=rect.origin.x+(triangle.size.width*2);
        right=[self makeTriangle:3 inRectangle:triangle];
	}
	return self;
}

- (void)dealloc
{
    [up release];
    [down release];
    [left release];
    [right release];
    [super dealloc];
}

- (void)drawRect:(NSRect)rect
{
    NSRect area;
    
    area=[self bounds];
    NSDrawLightBezel(area,area);
    [[NSColor blackColor] set];
    if(bUp) [up fill];
    if(bDown) [down fill];
    if(bLeft) [left fill];
    if(bRight) [right fill];
}

- (void)setUp:(BOOL)upState
{
    bUp=upState;
    [self setNeedsDisplay:TRUE];
}

- (void)setDown:(BOOL)downState
{
    bDown=downState;
    [self setNeedsDisplay:TRUE];
}

- (void)setLeft:(BOOL)leftState
{
    bLeft=leftState;
    [self setNeedsDisplay:TRUE];
}

- (void)setRight:(BOOL)rightState
{
    bRight=rightState;
    [self setNeedsDisplay:TRUE];
}

@end
