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

#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <ForceFeedback/ForceFeedback.h>

#include "MyCentreButtons.h"
#include "MyDigitalStick.h"
#include "MyAnalogStick.h"
#include "MyMainButtons.h"
#include "MyShoulderButton.h"

@class DeviceLister;

@interface Pref360ControlPref : NSPreferencePane 
{
    // Window components
    IBOutlet MyCentreButtons *centreButtons;
    IBOutlet NSPopUpButton *deviceList;
    IBOutlet MyDigitalStick *digiStick;
    IBOutlet MyShoulderButton *leftShoulder;
    IBOutlet MyAnalogStick *leftStick;
    IBOutlet NSButton *leftLinked;
    IBOutlet NSSlider *leftStickDeadzone;
    IBOutlet NSButton *leftStickInvertX;
    IBOutlet NSButton *leftStickInvertY;
    IBOutlet NSProgressIndicator *leftTrigger;
    IBOutlet MyMainButtons *rightButtons;
    IBOutlet MyShoulderButton *rightShoulder;
    IBOutlet MyAnalogStick *rightStick;
    IBOutlet NSButton *rightLinked;
    IBOutlet NSSlider *rightStickDeadzone;
    IBOutlet NSButton *rightStickInvertX;
    IBOutlet NSButton *rightStickInvertY;
    IBOutlet NSProgressIndicator *rightTrigger;
    IBOutlet NSImageView *batteryLevel;
    IBOutlet DeviceLister *deviceLister;
    IBOutlet NSButton *powerOff;
    // Internal info
    mach_port_t masterPort;
    NSMutableArray *deviceArray;
    IOHIDElementCookie axis[6],buttons[15];
    
    IOHIDDeviceInterface122 **device;
    IOHIDQueueInterface **hidQueue;
    FFDeviceObjectReference ffDevice;
    io_registry_entry_t registryEntry;
    
    int largeMotor,smallMotor;
    
    IONotificationPortRef notifyPort;
    CFRunLoopSourceRef notifySource;
    io_iterator_t onIteratorWired, offIteratorWired;
    io_iterator_t onIteratorWireless, offIteratorWireless;
}

- (void)mainViewDidLoad;

- (void)eventQueueFired:(void*)sender withResult:(IOReturn)result;

- (void)handleDeviceChange;

- (IBAction)showDeviceList:(id)sender;
- (IBAction)powerOff:(id)sender;

- (mach_port_t)masterPort;

@end
