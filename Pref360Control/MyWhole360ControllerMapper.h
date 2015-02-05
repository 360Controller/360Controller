//
//  MyWhole360ControllerMapper.h
//  360 Driver
//
//  Created by Drew Mills on 1/30/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MyWhole360Controller.h"
#import "Pref360ControlPref.h"

@interface MyWhole360ControllerMapper : MyWhole360Controller

@property BOOL isMapping;

- (void)runMapperWithButton:(NSButton *)button  andOwner:(Pref360ControlPref *)pref;
- (void)buttonPressedAtIndex:(int)index;
+ (UInt8 *)mapping;

@end