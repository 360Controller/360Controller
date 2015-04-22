/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    Pref360ControlPref.h - definition for the pref pane class
    
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

#import <PreferencePanes/PreferencePanes.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <ForceFeedback/ForceFeedback.h>

@class MyWhole360Controller;
@class MyWhole360ControllerMapper;
@class MyTrigger;
@class MyBatteryMonitor;
@class MyDeadZoneViewer;
@class MyAnalogStick;
@class DeviceLister;

typedef NS_ENUM(NSUInteger, ControllerType) {
    Xbox360Controller = 0,
    XboxOriginalController = 1,
    XboxOneController = 2
} controllerType;

@interface Pref360ControlPref : NSPreferencePane
// Window components
@property (weak) IBOutlet NSPopUpButton *deviceList;
@property (weak) IBOutlet NSButton *leftLinked;
@property (weak) IBOutlet NSSlider *leftStickDeadzone;
@property (weak) IBOutlet NSButton *leftStickInvertX;
@property (weak) IBOutlet NSButton *leftStickInvertY;
@property (weak) IBOutlet NSButton *rightLinked;
@property (weak) IBOutlet NSSlider *rightStickDeadzone;
@property (weak) IBOutlet NSButton *rightStickInvertX;
@property (weak) IBOutlet NSButton *rightStickInvertY;
@property (weak) IBOutlet DeviceLister *deviceLister;
@property (weak) IBOutlet NSButton *powerOff;
@property (weak) IBOutlet MyWhole360Controller *wholeController;
@property (weak) IBOutlet MyTrigger *leftTrigger;
@property (weak) IBOutlet MyTrigger *rightTrigger;
@property (weak) IBOutlet MyBatteryMonitor *batteryStatus;
@property (weak) IBOutlet MyDeadZoneViewer *leftDeadZone;
@property (weak) IBOutlet MyDeadZoneViewer *rightDeadZone;
@property (strong) IBOutlet NSPopover *aboutPopover;
@property (weak) IBOutlet NSPopUpButton *rumbleOptions;

// Binding Tab
@property (weak) IBOutlet NSPopUpButton *deviceListBinding;
@property (weak) IBOutlet MyWhole360ControllerMapper *wholeControllerMapper;
@property (weak) IBOutlet NSTabView *tabView;
@property (weak) IBOutlet NSButton *remappingButton;
@property (weak) IBOutlet NSTableView *mappingTable;
@property (weak) IBOutlet NSButton *remappingResetButton;

// Advanced Tab
@property (weak) IBOutlet NSPopUpButton *deviceListAdvanced;
// Advanced Tab - Options
@property (weak) IBOutlet NSButton *enableDriverCheckBox;
@property (weak) IBOutlet NSButton *uninstallDriverButton;
// Advanced Tab - Deadzones
@property (weak) IBOutlet MyAnalogStick *leftStickAnalog;
@property (weak) IBOutlet MyAnalogStick *rightStickAnalog;
@property (weak) IBOutlet NSButton *leftLinkedAlt;
@property (weak) IBOutlet NSSlider *leftStickDeadzoneAlt;
@property (weak) IBOutlet NSButton *leftStickInvertXAlt;
@property (weak) IBOutlet NSButton *leftStickInvertYAlt;
@property (weak) IBOutlet NSButton *rightLinkedAlt;
@property (weak) IBOutlet NSSlider *rightStickDeadzoneAlt;
@property (weak) IBOutlet NSButton *rightStickInvertXAlt;
@property (weak) IBOutlet NSButton *rightStickInvertYAlt;
@property (weak) IBOutlet NSButton *normalizeDeadzoneLeft;
@property (weak) IBOutlet NSButton *normalizeDeadzoneRight;

// About Tab
/* put About Tab's @properties here */

// Internal info
@property (readonly) mach_port_t masterPort;

- (void)eventQueueFired:(void*)sender withResult:(IOReturn)result;

- (void)handleDeviceChange;

- (IBAction)powerOff:(id)sender;
- (IBAction)selectDevice:(id)sender;
- (IBAction)changeSetting:(id)sender;

- (IBAction)toggleDriverEnabled:(NSButton *)sender;
- (IBAction)willPerformUninstallation:(id)sender;

@end
