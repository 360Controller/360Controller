/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    MyMainButtons.m - implementation of A/B/X/Y button view
    
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
#import "MyMainButtons.h"

#define MINI_OFFSET         2

@implementation MyMainButtons

- (id)initWithFrame:(NSRect)frameRect
{
	if ((self = [super initWithFrame:frameRect]) != nil) {
        a=b=x=y=FALSE;
	}
	return self;
}

- (void)drawButton:(NSString*)button inRectangle:(NSRect)rect pressed:(BOOL)down
{
    NSBezierPath *path;
    NSSize size;
    NSDictionary *attributes;
    NSPoint point;
    NSColor *colour;
    NSRect bling;
    
    // Draw circle
    path=[NSBezierPath bezierPathWithOvalInRect:rect];
    if(down) {
        [path fill];
//        colour=[NSColor whiteColor];
    } else {
        [path stroke];
//        colour=[NSColor blackColor];
    }
    colour=[NSColor blackColor];
    bling=rect;
    bling.origin.x-=1;
    bling.origin.y-=1;
    bling.size.width+=2;
    bling.size.height+=2;
    path=[NSBezierPath bezierPathWithOvalInRect:bling];
    [colour set];
    [path stroke];
    // Draw text
    attributes=[NSDictionary dictionaryWithObject:colour forKey:NSForegroundColorAttributeName];
    size=[button sizeWithAttributes:attributes];
    point.x=rect.origin.x+((rect.size.width-size.width)/2);
    point.y=rect.origin.y+((rect.size.height-size.height)/2);
    [button drawAtPoint:point withAttributes:attributes];
}

- (void)drawRect:(NSRect)rect
{
    NSRect area,bit;
    
    area=[self bounds];
    bit.size.width=area.size.width/3;
    bit.size.height=area.size.height/3;
    bit.origin.x=area.origin.x+bit.size.width;
    bit.origin.y=area.origin.y+(bit.size.height*2)-MINI_OFFSET;
    [[NSColor yellowColor] set];
    [self drawButton:@"Y" inRectangle:bit pressed:y];
    bit.origin.y=area.origin.y+MINI_OFFSET;
    [[NSColor greenColor] set];
    [self drawButton:@"A" inRectangle:bit pressed:a];
    bit.origin.y=area.origin.y+bit.size.height;
    bit.origin.x=area.origin.x+MINI_OFFSET;
    [[NSColor blueColor] set];
    [self drawButton:@"X" inRectangle:bit pressed:x];
    bit.origin.x=area.origin.x+(bit.size.width*2)-MINI_OFFSET;
    [[NSColor redColor] set];
    [self drawButton:@"B" inRectangle:bit pressed:b];
}

- (void)setA:(BOOL)aVal
{
    a=aVal;
    [self setNeedsDisplay:TRUE];
}

- (void)setB:(BOOL)bVal
{
    b=bVal;
    [self setNeedsDisplay:TRUE];
}

- (void)setX:(BOOL)xVal
{
    x=xVal;
    [self setNeedsDisplay:TRUE];
}

- (void)setY:(BOOL)yVal
{
    y=yVal;
    [self setNeedsDisplay:TRUE];
}

@end
