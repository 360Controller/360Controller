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
#ifdef __i386__
{
@private
    // Window components
    MyCentreButtons *centreButtons;
    NSPopUpButton *deviceList;
    MyDigitalStick *digiStick;
    MyShoulderButton *leftShoulder;
    MyAnalogStick *leftStick;
    NSButton *leftLinked;
    NSSlider *leftStickDeadzone;
    NSButton *leftStickInvertX;
    NSButton *leftStickInvertY;
    NSLevelIndicator *leftTrigger;
    MyMainButtons *rightButtons;
    MyShoulderButton *rightShoulder;
    MyAnalogStick *rightStick;
    NSButton *rightLinked;
    NSSlider *rightStickDeadzone;
    NSButton *rightStickInvertX;
    NSButton *rightStickInvertY;
    NSLevelIndicator *rightTrigger;
    NSImageView *batteryLevel;
    DeviceLister *deviceLister;
    NSButton *powerOff;
    // Internal info
    mach_port_t masterPort;
    NSMutableArray *deviceArray;
    IOHIDElementCookie axis[6],buttons[15];
    
    IOHIDDeviceInterface122 **device;
    IOHIDQueueInterface **hidQueue;
    FFDeviceObjectReference ffDevice;
    io_registry_entry_t registryEntry;
    
    int largeMotor, smallMotor;
    
    IONotificationPortRef notifyPort;
    CFRunLoopSourceRef notifySource;
    io_iterator_t onIteratorWired, offIteratorWired;
    io_iterator_t onIteratorWireless, offIteratorWireless;
    
    FFEFFECT *effect;
    FFCUSTOMFORCE *customforce;
    FFEffectObjectReference effectRef;
}
#endif
// Window components
@property (arcweak) IBOutlet MyCentreButtons *centreButtons;
@property (arcweak) IBOutlet NSPopUpButton *deviceList;
@property (arcweak) IBOutlet MyDigitalStick *digiStick;
@property (arcweak) IBOutlet MyShoulderButton *leftShoulder;
@property (arcweak) IBOutlet MyAnalogStick *leftStick;
@property (arcweak) IBOutlet NSButton *leftLinked;
@property (arcweak) IBOutlet NSSlider *leftStickDeadzone;
@property (arcweak) IBOutlet NSButton *leftStickInvertX;
@property (arcweak) IBOutlet NSButton *leftStickInvertY;
@property (arcweak) IBOutlet NSLevelIndicator *leftTrigger;
@property (arcweak) IBOutlet MyMainButtons *rightButtons;
@property (arcweak) IBOutlet MyShoulderButton *rightShoulder;
@property (arcweak) IBOutlet MyAnalogStick *rightStick;
@property (arcweak) IBOutlet NSButton *rightLinked;
@property (arcweak) IBOutlet NSSlider *rightStickDeadzone;
@property (arcweak) IBOutlet NSButton *rightStickInvertX;
@property (arcweak) IBOutlet NSButton *rightStickInvertY;
@property (arcweak) IBOutlet NSLevelIndicator *rightTrigger;
@property (arcweak) IBOutlet NSImageView *batteryLevel;
@property (arcweak) IBOutlet DeviceLister *deviceLister;
@property (arcweak) IBOutlet NSButton *powerOff;

// Internal info
@property (readonly) mach_port_t masterPort;

- (void)eventQueueFired:(void*)sender withResult:(IOReturn)result;

- (void)handleDeviceChange;

- (IBAction)showDeviceList:(id)sender;
- (IBAction)powerOff:(id)sender;
- (IBAction)selectDevice:(id)sender;
- (IBAction)changeSetting:(id)sender;

@end
