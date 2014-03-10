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

#import "MyCentreButtons.h"
#import "MyDigitalStick.h"
#import "MyAnalogStick.h"
#import "MyMainButtons.h"
#import "MyShoulderButton.h"

@class DeviceLister;

@interface Pref360ControlPref : NSPreferencePane 
{
    // Internal info
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
    
    FFEFFECT *effect;
    FFCUSTOMFORCE *customforce;
    FFEffectObjectReference effectRef;
}
// Window components
@property (weak) IBOutlet MyCentreButtons *centreButtons;
@property (weak) IBOutlet NSPopUpButton *deviceList;
@property (weak) IBOutlet MyDigitalStick *digiStick;
@property (weak) IBOutlet MyShoulderButton *leftShoulder;
@property (weak) IBOutlet MyAnalogStick *leftStick;
@property (weak) IBOutlet NSButton *leftLinked;
@property (weak) IBOutlet NSSlider *leftStickDeadzone;
@property (weak) IBOutlet NSButton *leftStickInvertX;
@property (weak) IBOutlet NSButton *leftStickInvertY;
@property (weak) IBOutlet NSLevelIndicator *leftTrigger;
@property (weak) IBOutlet MyMainButtons *rightButtons;
@property (weak) IBOutlet MyShoulderButton *rightShoulder;
@property (weak) IBOutlet MyAnalogStick *rightStick;
@property (weak) IBOutlet NSButton *rightLinked;
@property (weak) IBOutlet NSSlider *rightStickDeadzone;
@property (weak) IBOutlet NSButton *rightStickInvertX;
@property (weak) IBOutlet NSButton *rightStickInvertY;
@property (weak) IBOutlet NSLevelIndicator *rightTrigger;
@property (weak) IBOutlet NSImageView *batteryLevel;
@property (weak) IBOutlet DeviceLister *deviceLister;
@property (weak) IBOutlet NSButton *powerOff;

// Internal info
@property (readonly) mach_port_t masterPort;

- (void)eventQueueFired:(void*)sender withResult:(IOReturn)result;

- (void)handleDeviceChange;

- (IBAction)showDeviceList:(id)sender;
- (IBAction)powerOff:(id)sender;
- (IBAction)selectDevice:(id)sender;
- (IBAction)changeSetting:(id)sender;

@end
