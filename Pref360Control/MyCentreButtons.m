/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    MyCentreButtons.m - implementation of central button view
    
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
*/#import "MyCentreButtons.h"

@implementation MyCentreButtons

- (id)initWithFrame:(NSRect)frameRect
{
	if ((self = [super initWithFrame:frameRect]) != nil) {
		back=start=appSpecific=FALSE;
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
    
    // Draw circle
    path=[NSBezierPath bezierPathWithOvalInRect:rect];
    colour=[NSColor blackColor];
    [colour set];
    if(down) {
        [path fill];
        colour=[NSColor whiteColor];
    } else [path stroke];
    // Draw text
    attributes=[NSDictionary dictionaryWithObject:colour forKey:NSForegroundColorAttributeName];
    size=[button sizeWithAttributes:attributes];
    point.x=rect.origin.x+((rect.size.width-size.width)/2);
    point.y=rect.origin.y+((rect.size.height-size.height)/2);
    [button drawAtPoint:point withAttributes:attributes];
}

- (void)drawRect:(NSRect)rect
{
    NSRect area,button;

    area=[self bounds];
    button.size.height=area.size.height/2;
    button.size.width=area.size.width/4;
    button.origin.x=area.origin.x;
    button.origin.y=area.origin.y+((area.size.height-button.size.height)/2);
    [self drawButton:@"Back" inRectangle:button pressed:back];
    button.origin.x=area.origin.x+area.size.width-button.size.width;
    [self drawButton:@"Start" inRectangle:button pressed:start];
    button.size.height=area.size.height-2;
    button.size.width=button.size.height;
    button.origin.x=area.origin.x+((area.size.width-button.size.width)/2);
    button.origin.y=area.origin.y+1;
    [self drawButton:@"" inRectangle:button pressed:appSpecific];
}

- (void)setBack:(BOOL)bBack
{
    back=bBack;
    [self setNeedsDisplay:TRUE];
}

- (void)setStart:(BOOL)bStart
{
    start=bStart;
    [self setNeedsDisplay:TRUE];
}

- (void)setSpecific:(BOOL)bSpecific
{
    appSpecific=bSpecific;
    [self setNeedsDisplay:TRUE];
}

@end
