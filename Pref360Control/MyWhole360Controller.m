//
//  MyWhole360Controller.m
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import "MyWhole360Controller.h"
#import "Pref360StyleKit.h"

@implementation MyWhole360Controller


- (void)setAPressed:(BOOL)pressed {
    _aPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setBPressed:(BOOL)pressed {
    _bPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setXPressed:(BOOL)pressed {
    _xPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setYPressed:(BOOL)pressed {
    _yPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setLeftPressed:(BOOL)pressed {
    _leftPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setUpPressed:(BOOL)pressed {
    _upPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setRightPressed:(BOOL)pressed {
    _rightPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setDownPressed:(BOOL)pressed {
    _downPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setBackPressed:(BOOL)pressed {
    _backPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setStartPressed:(BOOL)pressed {
    _startPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setHomePressed:(BOOL)pressed {
    _homePressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setLbPressed:(BOOL)pressed {
    _lbPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setRbPressed:(BOOL)pressed {
    _rbPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setLeftStickPressed:(BOOL)pressed {
    _leftStickPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setRightStickPressed:(BOOL)pressed {
    _rightStickPressed = pressed;
    [self setNeedsDisplay:YES];
}

- (void)setLeftStickPosition:(CGPoint)position {
    _leftStickPosition = position;
    [self setNeedsDisplay:YES];
}

- (void)setRightStickPosition:(CGPoint)position {
    _rightStickPosition = position;
    [self setNeedsDisplay:YES];
}

- (void)setLeftStickDeadzone:(CGFloat)deadzone {
    _leftStickDeadzone = deadzone / 32768.0;
    [self setNeedsDisplay:YES];
}

- (void)setRightStickDeadzone:(CGFloat)deadzone {
    _rightStickDeadzone = deadzone / 32768.0;
    [self setNeedsDisplay:YES];
}

- (void)setLeftNormalized:(BOOL)normalized {
    _leftNormalized = normalized;
    [self setNeedsDisplay:YES];
}

- (void)setRightNormalized:(BOOL)normalized {
    _rightNormalized = normalized;
    [self setNeedsDisplay:YES];
}

- (void)setLeftStickXPos:(int)xPos {
    CGPoint p = _leftStickPosition;
    p.x = xPos / 32768.0;
    [self setLeftStickPosition:p];
}

- (void)setLeftStickYPos:(int)yPos {
    CGPoint p = _leftStickPosition;
    p.y = yPos / 32768.0;
    [self setLeftStickPosition:p];
}

- (void)setRightStickXPos:(int)xPos {
    CGPoint p = _rightStickPosition;
    p.x = xPos / 32768.0;
    [self setRightStickPosition:p];
}

- (void)setRightStickYPos:(int)yPos {
    CGPoint p = _rightStickPosition;
    p.y = yPos / 32768.0;
    [self setRightStickPosition:p];
}

- (void)reset {
    [self setAPressed:NO];
    [self setBPressed:NO];
    [self setXPressed:NO];
    [self setYPressed:NO];
    [self setLeftPressed:NO];
    [self setUpPressed:NO];
    [self setRightPressed:NO];
    [self setDownPressed:NO];
    [self setBackPressed:NO];
    [self setStartPressed:NO];
    [self setHomePressed:NO];
    [self setLbPressed:NO];
    [self setRbPressed:NO];
    [self setLeftStickPressed:NO];
    [self setRightStickPressed:NO];
    [self setLeftStickPosition:NSZeroPoint];
    [self setRightStickPosition:NSZeroPoint];
    [self setLeftStickDeadzone:0];
    [self setRightStickDeadzone:0];
    [self setLeftNormalized:NO];
    [self setRightNormalized:NO];
}

- (void)awakeFromNib {
    _leftStickPosition = NSZeroPoint;
    _rightStickPosition = NSZeroPoint;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    [Pref360StyleKit drawX360ControllerWithControllerNumber:0 aPressed:_aPressed bPressed:_bPressed xPressed:_xPressed yPressed:_yPressed leftPressed:_leftPressed upPressed:_upPressed rightPressed:_rightPressed downPressed:_downPressed backPressed:_backPressed startPressed:_startPressed lbPressed:_lbPressed rbPressed:_rbPressed homePressed:_homePressed leftStickPressed:_leftStickPressed rightStickPressed:_rightStickPressed leftStick:_leftStickPosition rightStick:_rightStickPosition leftStickDeadzone:_leftStickDeadzone rightStickDeadzone:_rightStickDeadzone isLeftNormalized:_leftNormalized isRightNormalized:_rightNormalized];
}

@end
