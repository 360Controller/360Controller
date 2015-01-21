//
//  MyWhole360Controller.m
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import "MyWhole360Controller.h"
#import "Pref360StyleKit.h"

@implementation MyWhole360Controller

@synthesize aPressed, bPressed, xPressed, yPressed;
@synthesize leftPressed, upPressed, rightPressed, downPressed;
@synthesize backPressed, startPressed, homePressed;
@synthesize lbPressed, rbPressed, leftStickPressed, rightStickPressed;
@synthesize leftStickPosition, rightStickPosition;
@synthesize leftStickXPos, leftStickYPos, rightStickXPos, rightStickYPos;

- (void)setAPressed:(BOOL)pressed {
    aPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setBPressed:(BOOL)pressed {
    bPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setXPressed:(BOOL)pressed {
    xPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setYPressed:(BOOL)pressed {
    yPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setLeftPressed:(BOOL)pressed {
    leftPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setUpPressed:(BOOL)pressed {
    upPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setRightPressed:(BOOL)pressed {
    rightPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setDownPressed:(BOOL)pressed {
    downPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setBackPressed:(BOOL)pressed {
    backPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setStartPressed:(BOOL)pressed {
    startPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setHomePressed:(BOOL)pressed {
    homePressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setLbPressed:(BOOL)pressed {
    lbPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setRbPressed:(BOOL)pressed {
    rbPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setLeftStickPressed:(BOOL)pressed {
    leftStickPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setRightStickPressed:(BOOL)pressed {
    rightStickPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setLeftStickPosition:(CGPoint)position {
    leftStickPosition = position;
    [self setNeedsDisplay:YES];
}

- (void)setRightStickPosition:(CGPoint)position {
    rightStickPosition = position;
    [self setNeedsDisplay:YES];
}

- (void)setLeftStickXPos:(int)xPos {
    CGPoint p = leftStickPosition;
    p.x = xPos / 32768.0;
    [self setLeftStickPosition:p];
}

- (void)setLeftStickYPos:(int)yPos {
    CGPoint p = leftStickPosition;
    p.y = yPos / 32768.0;
    [self setLeftStickPosition:p];
}

- (void)setRightStickXPos:(int)xPos {
    CGPoint p = rightStickPosition;
    p.x = xPos / 32768.0;
    [self setRightStickPosition:p];
}

- (void)setRightStickYPos:(int)yPos {
    CGPoint p = rightStickPosition;
    p.y = yPos / 32768.0;
    [self setRightStickPosition:p];
}

- (void)awakeFromNib {
    leftStickPosition = NSZeroPoint;
    rightStickPosition = NSZeroPoint;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    [Pref360StyleKit drawX360ControllerWithControllerNumber:0 aPressed:aPressed bPressed:bPressed xPressed:xPressed yPressed:yPressed leftPressed:leftPressed upPressed:upPressed rightPressed:rightPressed downPressed:downPressed backPressed:backPressed startPressed:startPressed lbPressed:lbPressed rbPressed:rbPressed homePressed:homePressed leftStickPressed:leftStickPressed rightStickPressed:rightStickPressed leftStick:leftStickPosition rightStick:rightStickPosition];
}

@end
