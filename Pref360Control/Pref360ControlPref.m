/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro

    Pref360ControlPref.m - main source of the pref pane

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
#include <mach/mach.h>
#include <IOKit/usb/IOUSBLib.h>
#include <libkern/OSKextLib.h>
#include <IOKit/kext/KextManager.h>
#import "Pref360ControlPref.h"
#import "DeviceItem.h"
#import "ControlPrefs.h"
#import "DeviceLister.h"
#import "MyWhole360Controller.h"
#import "MyWhole360ControllerMapper.h"
#import "MyTrigger.h"
#import "MyDeadZoneViewer.h"
#import "MyBatteryMonitor.h"
#import "MyAnalogStick.h"
#import "../360Daemon/360DaemonSocketProtocol.h"

#define NO_ITEMS @"No devices found"

@interface NSLayoutConstraint (Description)

@end

@implementation NSLayoutConstraint (Description)

-(NSString *)description {
    return [NSString stringWithFormat:@"id: %@, constant: %f", self.identifier, self.constant];
}

@end

// Passes a C callback back to the Objective C class
static void CallbackFunction(void *target,IOReturn result,void *refCon,void *sender)
{
    if (target) [((__bridge Pref360ControlPref*)target) eventQueueFired:sender withResult:result];
}

// Handle callback for when our device is connected or disconnected. Both events are
// actually handled identically.
static void callbackHandleDevice(void *param,io_iterator_t iterator)
{
    io_service_t object = 0;
    BOOL update = NO;

    while ((object = IOIteratorNext(iterator))) {
        IOObjectRelease(object);
        update = YES;
    }

    if (update) [(__bridge Pref360ControlPref*)param handleDeviceChange];
}

@interface Pref360ControlPref ()
@property (strong) NSMutableArray *deviceArray;
@end

@implementation Pref360ControlPref
{
@private
    // Internal info
    IOHIDElementCookie axis[6],buttons[15];

    IOHIDDeviceInterface122 **device;
    IOHIDQueueInterface **hidQueue;
    FFDeviceObjectReference ffDevice;
    io_registry_entry_t registryEntry;
    io_string_t devicePath;

    int largeMotor, smallMotor;

    IONotificationPortRef notifyPort;
    CFRunLoopSourceRef notifySource;
    io_iterator_t onIteratorWired, offIteratorWired;
    io_iterator_t onIteratorWireless, offIteratorWireless;

    FFEFFECT *effect;
    FFCUSTOMFORCE *customforce;
    FFEffectObjectReference effectRef;
}


-(void)awakeFromNib {
//    [_aboutPopover setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameAqua]];
//    [_aboutPopover setAppearance:NSPopoverAppearanceHUD];
    [_rumbleOptions removeAllItems];
    [_rumbleOptions addItemsWithTitles:@[@"Default", @"None"]];
}

// Set the pattern on the LEDs
- (void)updateLED:(int)ledIndex
{
    FFEFFESCAPE escape = {0};
    unsigned char c = ledIndex;

    if (ffDevice == 0) return;
    escape.dwSize = sizeof(escape);
    escape.dwCommand = 0x02;
    escape.cbInBuffer = sizeof(c);
    escape.lpvInBuffer = &c;
    FFDeviceEscape(ffDevice, &escape);
}

// This will initialize the ff effect.
- (void)testMotorsInit
{
    if (ffDevice == 0) return;

    FFCAPABILITIES capabs;
    FFDeviceGetForceFeedbackCapabilities(ffDevice, &capabs);

    if(capabs.numFfAxes != 2) return;

    effect = calloc(1, sizeof(FFEFFECT));
    customforce = calloc(1, sizeof(FFCUSTOMFORCE));
    LONG *c = calloc(2, sizeof(LONG));
    DWORD *a = calloc(2, sizeof(DWORD));
    LONG *d = calloc(2, sizeof(LONG));

    c[0] = 0;
    c[1] = 0;
    a[0] = capabs.ffAxes[0];
    a[1] = capabs.ffAxes[1];
    d[0] = 0;
    d[1] = 0;

    customforce->cChannels = 2;
    customforce->cSamples = 2;
    customforce->rglForceData = c;
    customforce->dwSamplePeriod = 100*1000;

    effect->cAxes = capabs.numFfAxes;
    effect->rglDirection = d;
    effect->rgdwAxes = a;
    effect->dwSamplePeriod = 0;
    effect->dwGain = 10000;
    effect->dwFlags = FFEFF_OBJECTOFFSETS | FFEFF_SPHERICAL;
    effect->dwSize = sizeof(FFEFFECT);
    effect->dwDuration = FF_INFINITE;
    effect->dwSamplePeriod = 100*1000;
    effect->cbTypeSpecificParams = sizeof(FFCUSTOMFORCE);
    effect->lpvTypeSpecificParams = customforce;
    effect->lpEnvelope = NULL;
    FFDeviceCreateEffect(ffDevice, kFFEffectType_CustomForce_ID, effect, &effectRef);
}

- (void)testMotorsCleanUp
{
    if (effectRef == NULL) return;
    FFDeviceReleaseEffect(ffDevice, effectRef);
    free(customforce->rglForceData);
    free(effect->rgdwAxes);
    free(effect->rglDirection);
    free(customforce);
    free(effect);
}
- (void)testMotorsLarge:(unsigned char)large small:(unsigned char)small
{
    if (effectRef == NULL) return;
    customforce->rglForceData[0] = (large * 10000) / 255;
    customforce->rglForceData[1] = (small * 10000) / 255;
    FFEffectSetParameters(effectRef, effect, FFEP_TYPESPECIFICPARAMS);
    FFEffectStart(effectRef, 1, 0);
}

// Update axis GUI component
- (void)axisChanged:(int)index newValue:(int)value
{
    NSInteger tabIndex = [_tabView indexOfTabViewItem:[_tabView selectedTabViewItem]];

    switch(index) {
        case 0:
            [_wholeController setLeftStickXPos:value];
            [_leftStickAnalog setPositionX:value];
            break;

        case 1:
            [_wholeController setLeftStickYPos:value];
            [_leftStickAnalog setPositionY:value];
            break;

        case 2:
            [_wholeController setRightStickXPos:value];
            [_rightStickAnalog setPositionX:value];
            break;

        case 3:
            [_wholeController setRightStickYPos:value];
            [_rightStickAnalog setPositionY:value];
            break;

        case 4:
            if (tabIndex == 0) { // Controller Test
                [_leftTrigger setVal:value];
                largeMotor=value;
                [self testMotorsLarge:largeMotor small:smallMotor];
            }
            break;

        case 5:
            if (tabIndex == 0) {
                [_rightTrigger setVal:value];
                smallMotor=value;
                [self testMotorsLarge:largeMotor small:smallMotor];
            }
            break;

        default:
            break;
    }
}

// Update button GUI component
- (void)buttonChanged:(int)index newValue:(int)value
{
    BOOL b = (value != 0);
    NSInteger tabIndex = [_tabView indexOfTabViewItem:[_tabView selectedTabViewItem]];

    if (tabIndex == 0) { // Controller Test
        switch (index) {
            case 0:
                [_wholeController setAPressed:b];
                break;

            case 1:
                [_wholeController setBPressed:b];
                break;

            case 2:
                [_wholeController setXPressed:b];
                break;

            case 3:
                [_wholeController setYPressed:b];
                break;

            case 4:
                [_wholeController setLbPressed:b];
                break;

            case 5:
                [_wholeController setRbPressed:b];
                break;

            case 6:
                [_wholeController setLeftStickPressed:b];
                break;

            case 7:
                [_wholeController setRightStickPressed:b];
                break;

            case 8:
                [_wholeController setStartPressed:b];
                break;

            case 9:
                [_wholeController setBackPressed:b];
                break;

            case 10:
                [_wholeController setHomePressed:b];
                break;

            case 11:
                [_wholeController setUpPressed:b];
                break;

            case 12:
                [_wholeController setDownPressed:b];
                break;

            case 13:
                [_wholeController setLeftPressed:b];
                break;

            case 14:
                [_wholeController setRightPressed:b];
                break;

            default:
                break;
        }
    }
    else if (tabIndex == 1) { // Tweaks
        if ([_wholeControllerMapper isMapping]) {
            if (b == YES) {
                [_wholeControllerMapper buttonPressedAtIndex:index];
            }
        }
    }
}

// Handle message from I/O Kit indicating something happened on the device
- (void)eventQueueFired:(void*)sender withResult:(IOReturn)result
{
    AbsoluteTime zeroTime = {0,0};
    IOHIDEventStruct event;
    BOOL found = NO;
    int i;

    if (sender != hidQueue) return;
    while (result == kIOReturnSuccess) {
        result = (*hidQueue)->getNextEvent(hidQueue,&event,zeroTime,0);
        if (result != kIOReturnSuccess) continue;
        // Check axis
        for (i = 0, found = NO; (i < 6) && !found; i++) {
            if (event.elementCookie == axis[i]) {
                [self axisChanged:i newValue:event.value];
                found = YES;
            }
        }
        if (found) continue;
        // Check buttons
        for (i = 0, found = NO; (i < 15) && !found; i++) {
            if(event.elementCookie==buttons[i]) {
                [self buttonChanged:i newValue:event.value];
                found = YES;
            }
        }
        if(found) continue;
        // Cookie wasn't for us?
    }
}

// Enable input components
- (void)inputEnable:(BOOL)enable
{
    [_leftStickDeadzone setEnabled:enable];
    [_leftStickDeadzoneAlt setEnabled:enable];
    [_leftStickInvertX setEnabled:enable];
    [_leftStickInvertXAlt setEnabled:enable];
    [_leftStickInvertY setEnabled:enable];
    [_leftStickInvertYAlt setEnabled:enable];
    [_leftLinked setEnabled:enable];
    [_leftLinkedAlt setEnabled:enable];
    [_leftStickNormalize setEnabled:enable];
    [_rightStickDeadzone setEnabled:enable];
    [_rightStickDeadzoneAlt setEnabled:enable];
    [_rightStickInvertX setEnabled:enable];
    [_rightStickInvertXAlt setEnabled:enable];
    [_rightStickInvertY setEnabled:enable];
    [_rightStickInvertYAlt setEnabled:enable];
    [_rightLinked setEnabled:enable];
    [_rightLinkedAlt setEnabled:enable];
    [_rightStickNormalize setEnabled:enable];
    [_rumbleOptions setEnabled:enable];
    [_swapSticks setEnabled:enable];
    [_pretend360Button setEnabled:enable];
}

// Reset GUI components
- (void)resetDisplay
{
    [_leftTrigger setVal:0];
    [_rightTrigger setVal:0];
    [_wholeController reset];
    [_leftDeadZone setVal:0];
    [_leftStickAnalog setDeadzone:0];
    [_leftDeadZone setLinked:NO];
    [_leftStickAnalog setLinked:NO];
    [_rightDeadZone setVal:0];
    [_rightStickAnalog setDeadzone:0];
    [_rightDeadZone setLinked:NO];
    [_rightStickAnalog setLinked:NO];
    // Reset inputs
    [_leftStickDeadzone setIntValue:0];
    [_leftStickDeadzoneAlt setIntValue:0];
    [_leftStickInvertX setState:NSControlStateValueOff];
    [_leftStickInvertXAlt setState:NSControlStateValueOff];
    [_leftStickInvertY setState:NSControlStateValueOff];
    [_leftStickInvertYAlt setState:NSControlStateValueOff];
    [_rightStickDeadzone setIntValue:0];
    [_rightStickDeadzoneAlt setIntValue:0];
    [_rightStickInvertX setState:NSControlStateValueOff];
    [_rightStickInvertXAlt setState:NSControlStateValueOff];
    [_rightStickInvertY setState:NSControlStateValueOff];
    [_rightStickInvertYAlt setState:NSControlStateValueOff];
    [_swapSticks setState:NSControlStateValueOff];
    // Disable inputs
    [self inputEnable:NO];
    [_powerOff setHidden:YES];
    // Hide battery status
    [_batteryStatus setHidden:YES];
}

// Stop using the HID device
- (void)stopDevice
{
    if(registryEntry==0) return;
    [self testMotorsLarge:0 small:0];
    [self testMotorsCleanUp];
    if (hidQueue) {
        CFRunLoopSourceRef eventSource;

        (*hidQueue)->stop(hidQueue);
        eventSource=(*hidQueue)->getAsyncEventSource(hidQueue);
        if((eventSource!=NULL)&&CFRunLoopContainsSource(CFRunLoopGetCurrent(),eventSource,kCFRunLoopCommonModes))
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(),eventSource,kCFRunLoopCommonModes);
        (*hidQueue)->Release(hidQueue);
        hidQueue = NULL;
    }
    if (device) {
        (*device)->close(device);
        device = NULL;
    }
    registryEntry = 0;
}

// Start using a HID device
- (void)startDevice
{
    int i = (int)[_deviceList indexOfSelectedItem];
    int j;
    CFArrayRef CFelements;
    IOHIDElementCookie cookie;
    long usage,usagePage;
    CFRunLoopSourceRef eventSource;
    IOReturn ret;

    [self resetDisplay];
    if(([_deviceArray count]==0)||(i==-1)) {
        NSLog(@"No devices found? :( device count==%i, i==%i",(int)[_deviceArray count], i);
        return;
    }
    {
        DeviceItem *item = _deviceArray[i];

        device = [item hidDevice];
        ffDevice = [item ffDevice];
        registryEntry = [item rawDevice];
        controllerType = [item controllerType];
        IORegistryEntryGetPath(registryEntry, "IOService", devicePath);
    }

    if((*device)->copyMatchingElements(device,NULL,&CFelements)!=kIOReturnSuccess) {
        NSLog(@"Can't get elements list");
        // Make note of failure?
        return;
    }

    NSArray *elements = CFBridgingRelease(CFelements);

    for (NSDictionary *element in elements) {
        id object;
        // Get cookie
        object = element[@kIOHIDElementCookieKey];
        if (object == nil || ![object isKindOfClass:[NSNumber class]]) {
            continue;
        }
        cookie = (IOHIDElementCookie)[(NSNumber*)object unsignedIntValue];
        // Get usage
        object = element[@kIOHIDElementUsageKey];
        if (object == nil || ![object isKindOfClass:[NSNumber class]]) {
            continue;
        }
        usage = [(NSNumber*)object longValue];
        // Get usage page
        object = element[@kIOHIDElementUsagePageKey];
        if (object == nil || ![object isKindOfClass:[NSNumber class]]) {
            continue;
        }
        usagePage = [(NSNumber*)object longValue];
        // Match up items
        switch(usagePage) {
            case 0x01:  // Generic Desktop
                j=0;
                switch(usage) {
                    case 0x35:  // Right trigger
                        j++;
                    case 0x32:  // Left trigger
                        j++;
                    case 0x34:  // Right stick Y
                        j++;
                    case 0x33:  // Right stick X
                        j++;
                    case 0x31:  // Left stick Y
                        j++;
                    case 0x30:  // Left stick X
                        axis[j]=cookie;
                        break;
                    default:
                        break;
                }
                break;
            case 0x09:  // Button
                if((usage>=1)&&(usage<=15)) {
                    // Button 1-11
                    buttons[usage-1]=cookie;
                }
                break;
            default:
                break;
        }
    }
    // Start queue
    if((*device)->open(device,0)!=kIOReturnSuccess) {
        NSLog(@"Can't open device");
        // Make note of failure?
        return;
    }
    hidQueue=(*device)->allocQueue(device);
    if(hidQueue==NULL) {
        NSLog(@"Unable to allocate queue");
        // Error?
        return;
    }
    ret=(*hidQueue)->create(hidQueue,0,32);
    if(ret!=kIOReturnSuccess) {
        NSLog(@"Unable to create the queue");
        // Error?
        return;
    }
    // Create event source
    ret=(*hidQueue)->createAsyncEventSource(hidQueue, &eventSource);
    if(ret!=kIOReturnSuccess) {
        NSLog(@"Unable to create async event source");
        // Error?
        return;
    }
    // Set callback
    ret=(*hidQueue)->setEventCallout(hidQueue, CallbackFunction, (__bridge void *)(self), NULL);
    if(ret!=kIOReturnSuccess) {
        NSLog(@"Unable to set event callback");
        // Error?
        return;
    }
    // Add to runloop
    CFRunLoopAddSource(CFRunLoopGetCurrent(), eventSource, kCFRunLoopCommonModes);
    // Add some elements
    for (i = 0; i < 6; i++)
        (*hidQueue)->addElement(hidQueue, axis[i], 0);
    for (i = 0; i < 15; i++)
        (*hidQueue)->addElement(hidQueue, buttons[i], 0);
    // Start
    ret = (*hidQueue)->start(hidQueue);
    if (ret != kIOReturnSuccess) {
        NSLog(@"Unable to start queue - 0x%.8x", ret);
        // Error?
        return;
    }
    // Read existing properties
    {
        // CFDictionaryRef dict=(CFDictionaryRef)IORegistryEntryCreateCFProperty(registryEntry,CFSTR("DeviceData"),NULL,0);
        CFDictionaryRef dict = (CFDictionaryRef)CFBridgingRetain(GetController(GetSerialNumber(registryEntry)));
        if(dict) {
            CFBooleanRef boolValue;
            CFNumberRef intValue;

            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertLeftX"),(void*)&boolValue)) {
                [_leftStickInvertX setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
                [_leftStickInvertXAlt setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
            } else NSLog(@"No value for InvertLeftX\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertLeftY"),(void*)&boolValue)) {
                [_leftStickInvertY setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
                [_leftStickInvertYAlt setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
            } else NSLog(@"No value for InvertLeftY\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("RelativeLeft"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [_leftLinked setState:enable?NSControlStateValueOn:NSControlStateValueOff];
                [_leftLinkedAlt setState:enable?NSControlStateValueOn:NSControlStateValueOff];
                [_leftDeadZone setLinked:enable];
                [_leftStickAnalog setLinked:enable];
            } else NSLog(@"No value for RelativeLeft\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadOffLeft"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [_leftStickNormalize setState:enable];
                [_wholeController setLeftNormalized:enable];
                [_leftStickAnalog setNormalized:enable];
            } else NSLog(@"No value for DeadOffLeft\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadzoneLeft"),(void*)&intValue)) {
                UInt16 i;

                CFNumberGetValue(intValue,kCFNumberShortType,&i);
                [_leftStickDeadzone setIntValue:i];
                [_leftStickDeadzoneAlt setIntValue:i];
                [_leftDeadZone setVal:i];
                [_wholeController setLeftStickDeadzone:i];
                [_leftStickAnalog setDeadzone:i];
            } else NSLog(@"No value for DeadzoneLeft\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertRightX"),(void*)&boolValue)) {
                [_rightStickInvertX setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
                [_rightStickInvertXAlt setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
            } else NSLog(@"No value for InvertRightX\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertRightY"),(void*)&boolValue)) {
                [_rightStickInvertY setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
                [_rightStickInvertYAlt setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
            } else NSLog(@"No value for InvertRightY\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("RelativeRight"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [_rightLinked setState:enable?NSControlStateValueOn:NSControlStateValueOff];
                [_rightLinkedAlt setState:enable?NSControlStateValueOn:NSControlStateValueOff];
                [_rightDeadZone setLinked:enable];
                [_wholeController setRightStickDeadzone:i];
                [_rightStickAnalog setLinked:enable];
            } else NSLog(@"No value for RelativeRight\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadOffRight"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [_rightStickNormalize setState:enable];
                [_wholeController setRightNormalized:enable];
                [_rightStickAnalog setNormalized:enable];
            } else NSLog(@"No value for DeadOffRight\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadzoneRight"),(void*)&intValue)) {
                UInt16 i;

                CFNumberGetValue(intValue,kCFNumberShortType,&i);
                [_rightStickDeadzone setIntValue:i];
                [_rightStickDeadzoneAlt setIntValue:i];
                [_rightDeadZone setVal:i];
                [_rightStickAnalog setDeadzone:i];
            } else NSLog(@"No value for DeadzoneRight\n");

            if(CFDictionaryGetValueIfPresent(dict,CFSTR("ControllerType"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                ControllerType controllerTypePrefs = (ControllerType)[num integerValue];
                if (controllerTypePrefs != controllerType)
                    NSLog(@"ControllerType from prefs was %d, expected %d", (int)controllerTypePrefs, (int)controllerType);
            } else NSLog(@"No value for ControllerType\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("RumbleType"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [_rumbleOptions selectItemAtIndex:[num integerValue]];
//                [_rumbleOptions setState:[num integerValue]];
            } else NSLog(@"No value for RumbleType\n");

            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingUp"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][0] = [num intValue];
            } else NSLog(@"No value for BindingUp\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingDown"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][1] = [num intValue];
            } else NSLog(@"No value for BindingDown\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingLeft"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][2] = [num intValue];
            } else NSLog(@"No value for BindingLeft\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingRight"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][3] = [num intValue];
            } else NSLog(@"No value for BindingRight\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingStart"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][4] = [num intValue];
            } else NSLog(@"No value for BindingStart\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingBack"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][5] = [num intValue];
            } else NSLog(@"No value for BindingBack\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingLSC"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][6] = [num intValue];
            } else NSLog(@"No value for BindingLSC\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingRSC"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][7] = [num intValue];
            } else NSLog(@"No value for BindingRSC\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingLB"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][8] = [num intValue];
            } else NSLog(@"No value for BindingLB\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingRB"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][9] = [num intValue];
            } else NSLog(@"No value for BindingRB\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingGuide"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][10] = [num intValue];
            } else NSLog(@"No value for BindingGuide\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingA"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][11] = [num intValue];
            } else NSLog(@"No value for BindingA\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingB"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][12] = [num intValue];
            } else NSLog(@"No value for BindingB\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingX"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][13] = [num intValue];
            } else NSLog(@"No value for BindingX\n");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("BindingY"),(void*)&intValue)) {
                NSNumber *num = (__bridge NSNumber *)intValue;
                [MyWhole360ControllerMapper mapping][14] = [num intValue];
            } else NSLog(@"No value for BindingY\n");

            if(CFDictionaryGetValueIfPresent(dict,CFSTR("SwapSticks"),(void*)&boolValue)) {
                [_swapSticks setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
            } else NSLog(@"No value for SwapSticks\n");

            if(CFDictionaryGetValueIfPresent(dict,CFSTR("Pretend360"),(void*)&boolValue)) {
                [_pretend360Button setState:CFBooleanGetValue(boolValue)?NSControlStateValueOn:NSControlStateValueOff];
            } else NSLog(@"No value for Pretend360");
        } else NSLog(@"No settings found\n");
    }
    
    // Set force feedback options
    if (controllerType == XboxOneController || controllerType == XboxOnePretend360Controller)
    {
        [_rumbleOptions removeAllItems];
        [_rumbleOptions addItemsWithTitles:@[@"Default", @"None", @"Triggers Only", @"Both"]];
    }
    else
    {
        [_rumbleOptions removeAllItems];
        [_rumbleOptions addItemsWithTitles:@[@"Default", @"None"]];
    }
    
    // Enable GUI components
    [self inputEnable:YES];
    // Set device capabilities
    // Set FF motor control
    [self testMotorsInit];
    [self testMotorsLarge:0 small:0];
    largeMotor = 0;
    smallMotor = 0;
    // Battery level?
    {
        int batteryLevel = -1;
        int batteryPercentage = -1;
        CFTypeRef prop;

        if (IOObjectConformsTo(registryEntry, "WirelessHIDDevice") ||
            IOObjectConformsTo(registryEntry, "WirelessOneController")
        ) {
            prop = IORegistryEntryCreateCFProperty(registryEntry, CFSTR("BatteryLevel"), NULL, 0);
            if (prop != nil) {
                unsigned char level;

                if (CFNumberGetValue(prop, kCFNumberCharType, &level)) {
                    batteryLevel = level / 64;
                    batteryPercentage = level * 100 / 255.0f + 0.5f;
                }
                CFRelease(prop);
            }
            [_powerOff setHidden:NO];
        }
        if ( batteryLevel >= 0) {
            [_batteryStatus setBars:batteryLevel];
            [_batteryStatus setPercentage:batteryPercentage];
            [_batteryStatus setHidden:NO];
        } else {
            [_batteryStatus setHidden:YES];
        }
    }

    [_mappingTable reloadData];
}

// Clear out the device lists
- (void)deleteDeviceList
{
    [_deviceList removeAllItems];
    [_deviceListBinding removeAllItems];
    [_deviceListAdvanced removeAllItems];
    [_deviceArray removeAllObjects];
}

// Update the device list from the I/O Kit
- (void)updateDeviceList
{
    CFMutableDictionaryRef hidDictionary;
    IOReturn ioReturn;
    io_iterator_t iterator;
    io_object_t hidDevice;
    int count = 0;
    int targetIndex = 0;

    // Scrub old items
    [self stopDevice];
    [self deleteDeviceList];
    // Add new items
    hidDictionary=IOServiceMatching(kIOHIDDeviceKey);
    ioReturn=IOServiceGetMatchingServices(_masterPort,hidDictionary,&iterator);
    if((ioReturn != kIOReturnSuccess) || (iterator == 0)) {
        [_deviceList addItemWithTitle:NO_ITEMS];
        [_deviceListBinding addItemWithTitle:NO_ITEMS];
        [_deviceListAdvanced addItemWithTitle:NO_ITEMS];
        return;
    }

    while ((hidDevice = IOIteratorNext(iterator))) {
		io_object_t parent = 0;
		IORegistryEntryGetParentEntry(hidDevice, kIOServicePlane, &parent);
        BOOL deviceWired = IOObjectConformsTo(parent, "Xbox360Peripheral") && IOObjectConformsTo(hidDevice, "Xbox360ControllerClass");
        BOOL deviceWireless = IOObjectConformsTo(hidDevice, "WirelessHIDDevice") || IOObjectConformsTo(hidDevice, "WirelessOneController");
        if ((!deviceWired) && (!deviceWireless))
        {
            IOObjectRelease(hidDevice);
            continue;
        }
        DeviceItem *item = [DeviceItem allocateDeviceItemForDevice:hidDevice];
        if (item == nil) continue;
        // Match to changed device if you can
        io_string_t newDevicePath;
        IORegistryEntryGetPath(hidDevice, "IOService", newDevicePath);
        NSString *oldPath = [NSString stringWithCString:devicePath encoding:NSASCIIStringEncoding];
        NSString *newPath = [NSString stringWithCString:newDevicePath encoding:NSASCIIStringEncoding];
        if ([[newPath stringByDeletingLastPathComponent] isEqualToString:[oldPath stringByDeletingLastPathComponent]])
        {
            targetIndex = count;
        }
        // Add to item
        NSString *name = item.displayName;
        [_deviceList addItemWithTitle:[NSString stringWithFormat:@"%i: %@ (%@)", ++count, name, deviceWireless ? @"Wireless" : @"Wired"]];
        [_deviceListBinding addItemWithTitle:[NSString stringWithFormat:@"%i: %@ (%@)", count, name, deviceWireless ? @"Wireless" : @"Wired"]];
        [_deviceListAdvanced addItemWithTitle:[NSString stringWithFormat:@"%i: %@ (%@)", count, name, deviceWireless ? @"Wireless" : @"Wired"]];
        [_deviceArray addObject:item];
    }
    IOObjectRelease(iterator);
    if (count==0) {
        [_deviceList addItemWithTitle:NO_ITEMS];
        [_deviceListBinding addItemWithTitle:NO_ITEMS];
        [_deviceListAdvanced addItemWithTitle:NO_ITEMS];
    } else if (targetIndex > 0) {
        [_deviceList selectItemAtIndex:targetIndex];
        [_deviceListBinding selectItemAtIndex:targetIndex];
        [_deviceListAdvanced selectItemAtIndex:targetIndex];
    }
    [self startDevice];
}

// Start up
- (void)didSelect
{
    io_object_t object;

    // Get master port, for accessing I/O Kit
    IOMasterPort(MACH_PORT_NULL,&_masterPort);
    // Set up notification of USB device addition/removal
    notifyPort=IONotificationPortCreate(_masterPort);
    notifySource=IONotificationPortGetRunLoopSource(notifyPort);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), notifySource, kCFRunLoopCommonModes);
    // Prepare other fields
    _deviceArray = [[NSMutableArray alloc] initWithCapacity:1];
    device=NULL;
    hidQueue=NULL;
    // Activate callbacks
        // Wired
    IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching(kIOUSBDeviceClassName), callbackHandleDevice, (__bridge void *)(self), &onIteratorWired);
    callbackHandleDevice((__bridge void *)(self), onIteratorWired);
    IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching(kIOUSBDeviceClassName), callbackHandleDevice, (__bridge void *)(self), &offIteratorWired);
    while((object = IOIteratorNext(offIteratorWired)) != 0)
        IOObjectRelease(object);
        // Wireless
    IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching("WirelessHIDDevice"), callbackHandleDevice, (__bridge void *)(self), &onIteratorWireless);
    IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching("WirelessOneController"), callbackHandleDevice, (__bridge void *)(self), &onIteratorWireless);
    callbackHandleDevice((__bridge void *)(self), onIteratorWireless);
    IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching("WirelessHIDDevice"), callbackHandleDevice, (__bridge void *)(self), &offIteratorWireless);
    IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching("WirelessOneController"), callbackHandleDevice, (__bridge void *)(self), &offIteratorWireless);
    while((object = IOIteratorNext(offIteratorWireless)) != 0)
        IOObjectRelease(object);

    // Enable the "enable driver" checkbox if the kext is loaded in the memory
    [self updateEnableDriverCheckboxState];

    // Initialize the socket to the daemon
    [self startClient];
}

// Shut down
- (void)didUnselect
{
    unsigned char c;

    // Remove notification source
    IOObjectRelease(onIteratorWired);
    IOObjectRelease(onIteratorWireless);
    IOObjectRelease(offIteratorWired);
    IOObjectRelease(offIteratorWireless);
    CFRunLoopSourceInvalidate(notifySource);
    IONotificationPortDestroy(notifyPort);
    // Release device and info
    [self stopDevice];
    for (DeviceItem *item in _deviceArray)
    {
        FFEFFESCAPE escape = {0};
        NSInteger i = [_deviceArray indexOfObject:item];
        if ([item ffDevice] == 0)
            continue;
        c = 0x06 + (i % 0x04);
        escape.dwSize = sizeof(escape);
        escape.dwCommand = 0x02;
        escape.cbInBuffer = sizeof(c);
        escape.lpvInBuffer = &c;
        escape.cbOutBuffer = 0;
        escape.lpvOutBuffer = NULL;
        FFDeviceEscape([item ffDevice], &escape);
    }
    [self deleteDeviceList];
    // Close master port
    mach_port_deallocate(mach_task_self(), _masterPort);
    // Close down the daemon socket
    [self stopClient];
    // Done
}

// Handle selection from drop down menu
- (IBAction)selectDevice:(id)sender
{
    NSInteger tabIndex = [_tabView indexOfTabViewItem:[_tabView selectedTabViewItem]];
    if (tabIndex == 0) { // Controller Test
        [_deviceListBinding selectItemAtIndex:[_deviceList indexOfSelectedItem]];
        [_deviceListAdvanced selectItemAtIndex:[_deviceList indexOfSelectedItem]];
    }
    else if (tabIndex == 1) { // Binding Tab
        [_deviceList selectItemAtIndex:[_deviceListBinding indexOfSelectedItem]];
        [_deviceListAdvanced selectItemAtIndex:[_deviceListBinding indexOfSelectedItem]];
    }
    else if (tabIndex == 2) { // Deadzones Tab
        [_deviceList selectItemAtIndex:[_deviceListAdvanced indexOfSelectedItem]];
        [_deviceListBinding selectItemAtIndex:[_deviceListAdvanced indexOfSelectedItem]];
    }

    [self startDevice];
}

// Handle changing a setting
- (IBAction)changeSetting:(id)sender
{
    // Send normalize to joysticks
    [_wholeController setLeftNormalized:(BOOL)[_leftStickNormalize state]];
    [_leftStickAnalog setNormalized:(BOOL)[_leftStickNormalize state]];
    [_wholeController setRightNormalized:(BOOL)[_rightStickNormalize state]];
    [_rightStickAnalog setNormalized:(BOOL)[_rightStickNormalize state]];

    // Sync settings between tabs
    NSInteger tabIndex = [_tabView indexOfTabViewItem:[_tabView selectedTabViewItem]];

    if (tabIndex == 0) { // Controller Test
        [_leftLinkedAlt setState:[_leftLinked state]];
        [_leftStickDeadzoneAlt setDoubleValue:[_leftStickDeadzone doubleValue]];
        [_leftStickInvertXAlt setState:[_leftStickInvertX state]];
        [_leftStickInvertYAlt setState:[_leftStickInvertY state]];
        [_rightLinkedAlt setState:[_rightLinked state]];
        [_rightStickDeadzoneAlt setDoubleValue:[_rightStickDeadzone doubleValue]];
        [_rightStickInvertXAlt setState:[_rightStickInvertX state]];
        [_rightStickInvertYAlt setState:[_rightStickInvertY state]];
    }
    else if (tabIndex == 2) { // Advanced Tab
        [_leftLinked setState:[_leftLinkedAlt state]];
        [_leftStickDeadzone setDoubleValue:[_leftStickDeadzoneAlt doubleValue]];
        [_leftStickInvertX setState:[_leftStickInvertXAlt state]];
        [_leftStickInvertY setState:[_leftStickInvertYAlt state]];
        [_rightLinked setState:[_rightLinkedAlt state]];
        [_rightStickDeadzone setDoubleValue:[_rightStickDeadzoneAlt doubleValue]];
        [_rightStickInvertX setState:[_rightStickInvertXAlt state]];
        [_rightStickInvertY setState:[_rightStickInvertYAlt state]];
    }

    BOOL pretend360 = ([_pretend360Button state] == NSControlStateValueOn);
    if (controllerType == XboxOneController || controllerType == XboxOnePretend360Controller)
    {
        if (pretend360)
            controllerType = XboxOnePretend360Controller;
        else
            controllerType = XboxOneController;
    }

    // Create dictionary
    NSDictionary *dict = @{@"InvertLeftX": @((BOOL)([_leftStickInvertX state]==NSControlStateValueOn)),
                           @"InvertLeftY": @((BOOL)([_leftStickInvertY state]==NSControlStateValueOn)),
                           @"InvertRightX": @((BOOL)([_rightStickInvertX state]==NSControlStateValueOn)),
                           @"InvertRightY": @((BOOL)([_rightStickInvertY state]==NSControlStateValueOn)),
                           @"DeadzoneLeft": @((UInt16)[_leftStickDeadzone doubleValue]),
                           @"DeadzoneRight": @((UInt16)[_rightStickDeadzone doubleValue]),
                           @"RelativeLeft": @((BOOL)([_leftLinked state]==NSControlStateValueOn)),
                           @"RelativeRight": @((BOOL)([_rightLinked state]==NSControlStateValueOn)),
                           @"DeadOffLeft": @((BOOL)([_leftStickNormalize state]==NSControlStateValueOn)),
                           @"DeadOffRight": @((BOOL)([_rightStickNormalize state]==NSControlStateValueOn)),
                           @"ControllerType": @((UInt8)(controllerType)),
                           @"RumbleType": @((UInt8)([_rumbleOptions indexOfSelectedItem])),
                           @"BindingUp": @((UInt8)([MyWhole360ControllerMapper mapping][0])),
                           @"BindingDown": @((UInt8)([MyWhole360ControllerMapper mapping][1])),
                           @"BindingLeft": @((UInt8)([MyWhole360ControllerMapper mapping][2])),
                           @"BindingRight": @((UInt8)([MyWhole360ControllerMapper mapping][3])),
                           @"BindingStart": @((UInt8)([MyWhole360ControllerMapper mapping][4])),
                           @"BindingBack": @((UInt8)([MyWhole360ControllerMapper mapping][5])),
                           @"BindingLSC": @((UInt8)([MyWhole360ControllerMapper mapping][6])),
                           @"BindingRSC": @((UInt8)([MyWhole360ControllerMapper mapping][7])),
                           @"BindingLB": @((UInt8)([MyWhole360ControllerMapper mapping][8])),
                           @"BindingRB": @((UInt8)([MyWhole360ControllerMapper mapping][9])),
                           @"BindingGuide": @((UInt8)([MyWhole360ControllerMapper mapping][10])),
                           @"BindingA": @((UInt8)([MyWhole360ControllerMapper mapping][11])),
                           @"BindingB": @((UInt8)([MyWhole360ControllerMapper mapping][12])),
                           @"BindingX": @((UInt8)([MyWhole360ControllerMapper mapping][13])),
                           @"BindingY": @((UInt8)([MyWhole360ControllerMapper mapping][14])),
                           @"SwapSticks": @((BOOL)([_swapSticks state]==NSControlStateValueOn)),
                           @"Pretend360": @((BOOL)pretend360)};

    // Set property
    IORegistryEntrySetCFProperties(registryEntry, (__bridge CFTypeRef)(dict));
    SetController(GetSerialNumber(registryEntry), dict);
    // Update UI
    [_leftDeadZone setLinked:[_leftLinked state] == NSControlStateValueOn];
    [_leftStickAnalog setLinked:[_leftLinked state] == NSControlStateValueOn];
    [_leftDeadZone setVal:[_leftStickDeadzone doubleValue]];
    [_wholeController setLeftStickDeadzone:[_leftStickDeadzone doubleValue]];
    [_leftStickAnalog setDeadzone:[_leftStickDeadzone doubleValue]];
    [_rightDeadZone setLinked:[_rightLinked state] == NSControlStateValueOn];
    [_rightStickAnalog setLinked:[_rightLinked state] == NSControlStateValueOn];
    [_rightDeadZone setVal:[_rightStickDeadzone doubleValue]];
    [_wholeController setRightStickDeadzone:[_rightStickDeadzone doubleValue]];
    [_rightStickAnalog setDeadzone:[_rightStickDeadzone doubleValue]];
}

// Run an AppleScript from String and returns YES on successful execution
- (BOOL)runInlineAppleScript:(NSString *)scriptString
{
    NSDictionary* errorDict;
    NSAppleEventDescriptor* returnDescriptor = NULL;

    NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource:scriptString];

    returnDescriptor = [scriptObject executeAndReturnError: &errorDict];
    scriptObject = nil;

    if (returnDescriptor != NULL)
    {
        // successful execution
        if (kAENullEvent != [returnDescriptor descriptorType])
        {
            return YES;
            /* Uncomment this to handle the returned values */
//            // script returned an AppleScript result
//            if (cAEList == [returnDescriptor descriptorType])
//            {
//                // result is a list of other descriptors
//            }
//            else
//            {
//                // coerce the result to the appropriate ObjC type
//            }
        }
    }
    else
    {
        // no script result, handle error
        id val = [errorDict objectForKey:@"NSAppleScriptErrorRange"];
        if (!val) {
            NSLog(@"APPLESCRIPT ERROR:\n%@", errorDict);
        } else {
            NSRange r = [val rangeValue];
            NSMutableString *errorPoint = [NSMutableString stringWithString:scriptString];
            [errorPoint insertString:@"<---***" atIndex:r.location+r.length]; // end
            [errorPoint insertString:@"***ERROR_HERE--->" atIndex:r.location]; // start
            NSLog(@"APPLESCRIPT ERROR:\n%@"
                  @"\nERROR LOCATION:\n%@",
                  errorDict, errorPoint);
        }
    }
    return NO;
}

- (void)stream:(NSStream *)stream handleEvent:(NSStreamEvent)eventCode
{
    switch(eventCode)
    {
        case NSStreamEventOpenCompleted:
        {
            // Streams are open
        } break;

        case NSStreamEventHasSpaceAvailable:
        {
            // Queue writes here
        } break;

        case NSStreamEventHasBytesAvailable:
        {
            if (stream == self.readStream)
            {
                OSReturn kextManagerResult = kOSReturnSuccess; // Zero out the data
                UInt length = 0;

                length = (UInt)[self.readStream read:(UInt8*)&kextManagerResult maxLength:sizeof(OSReturn)];

                if (length >= sizeof(OSReturn))
                {
                    [self handleDaemonEnableResponse:kextManagerResult];
                }
            }
        } break;

        case NSStreamEventErrorOccurred:
        {
            // Error occurred
        } break;

        default:
        {
            // Other cases
        } break;
    }
}

- (void)handleDaemonEnableResponse:(OSReturn)returnCode
{
    if (self.enableDriverCheckBox.state == NSControlStateValueOn)
    {
        if (returnCode != kOSReturnSuccess)
        {
            [self.enableDriverCheckBox setState:NSControlStateValueOff];
        }

        if (returnCode == kOSKextReturnNotPrivileged)
        {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Cannot Load Driver"];
            [alert setInformativeText:@"In order to load the driver, you need administrator privileges."];
            [alert setAlertStyle:NSAlertStyleWarning];
            [alert runModal];
        }
        else if (returnCode == kOSKextReturnSystemPolicy)
        {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"Open Security Preferences"];
            [alert addButtonWithTitle:@"Cancel"];
            [alert setMessageText:@"Cannot Load Driver"];
            [alert setInformativeText:@"In order to use the driver, you must allow the developer, \"Drew Mills\", in your system's security preferences."];
            [alert setAlertStyle:NSAlertStyleWarning];
            [alert beginSheetModalForWindow:self.mainView.window completionHandler:^(NSModalResponse modalResponse) {
                if (modalResponse == NSAlertFirstButtonReturn) {
                    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?General"]];
                }
            }];
        }
        else if (returnCode == kOSKextReturnNotFound)
        {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Cannot Find Driver"];
            [alert setInformativeText:@"Please make sure that the driver is installed."];
            [alert setAlertStyle:NSAlertStyleWarning];
            [alert runModal];
        }
        // Catch-all for all other failures.
        else if (returnCode != kOSReturnSuccess)
        {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"Cancel"];
            [alert setMessageText:@"Cannot Load Driver"];
            [alert setInformativeText:[NSString stringWithFormat:@"The driver has failed to load due to an unknown error, code: %d. Please report this error to the driver maintainer.", returnCode]];
            [alert setAlertStyle:NSAlertStyleWarning];
            [alert runModal];
        }
    }
    else if (self.enableDriverCheckBox.state == NSControlStateValueOff)
    {
        [self powerOff:nil];
        [self stopDevice];

        if (returnCode == kOSKextReturnNotPrivileged)
        {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Cannot Unload Driver"];
            [alert setInformativeText:@"In order to unload the driver, you need administrator privileges."];
            [alert setAlertStyle:NSAlertStyleWarning];
            [alert runModal];
        }
        else if (returnCode == kOSKextReturnSystemPolicy)
        {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"Open Security Preferences"];
            [alert addButtonWithTitle:@"Cancel"];
            [alert setMessageText:@"Cannot Unload Driver"];
            [alert setInformativeText:@"In order to use the driver, you must allow the developer, \"Drew Mills\", in your system's security preferences."];
            [alert setAlertStyle:NSAlertStyleWarning];
            [alert beginSheetModalForWindow:self.mainView.window completionHandler:^(NSModalResponse modalResponse) {
                if (modalResponse == NSAlertSecondButtonReturn) {
                    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?General"]];
                }
            }];
        }
        else if (returnCode == kOSKextReturnInUse)
        {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Cannot Unload Driver"];
            [alert setInformativeText:@"The kext cannot be unloaded because it is retained by macOS. You will need to reboot your computer before you can unload the kext."];
            [alert setAlertStyle:NSAlertStyleWarning];
            [alert runModal];
        }
        else if (returnCode == kOSKextReturnNotFound)
        {
            // Kext already unloaded. Just ignore that anything happened.
        }
        // Catch-all for all other failures.
        else if (returnCode != kIOReturnSuccess)
        {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Cannot Unload Driver"];
            [alert setInformativeText:[NSString stringWithFormat:@"The driver has failed to unload due to an unknown error, code: %d. Please report this error to the driver maintainer.", returnCode]];
            [alert setAlertStyle:NSAlertStyleWarning];
            [alert runModal];
        }
    }

    [self updateDeviceList];
}

// Enable/disable the driver
// FIXME: currently only works after the controller is connected and loaded once.
// FIXME: will not uncheck the "Enabled" box if the prefpane is started with the driver disabled
- (IBAction)toggleDriverEnabled:(NSButton *)sender
{
    // Give the client another chance to connect
    if (self.writeStream == nil)
    {
        [self startClient];
    }

    if (self.writeStream != nil)
    {
        DaemonProtocolCommand command = UnloadWired;
        if (sender.state == NSControlStateValueOn)
        {
            command = LoadWired;
        }
        else if (sender.state == NSControlStateValueOff)
        {
            command = UnloadWired;
        }
        [self.writeStream write:(UInt8*)&command maxLength:1];
    }
    else
    {
        if (sender.state == NSControlStateValueOn)
        {
            [self.enableDriverCheckBox setState:NSControlStateValueOn];
        }
        else if (sender.state == NSControlStateValueOff)
        {
            [self.enableDriverCheckBox setState:NSControlStateValueOff];
        }

        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"Cancel"];
        [alert setMessageText:@"Unable to Connect to Daemon"];
        [alert setInformativeText:@"The driver currently cannot be enabled or disabled, because the preference pane can't get a connection to the daemon to perform the requested operation."];
        [alert setAlertStyle:NSAlertStyleWarning];
        [alert runModal];
    }
}

- (BOOL)isDriverLoaded
{
    CFStringRef wiredDriverIdentifier = DOM_WIRED_DRIVER;
    CFArrayRef miceDivers = CFArrayCreate(kCFAllocatorDefault, (void*)&wiredDriverIdentifier, 1, &kCFTypeArrayCallBacks);
    CFDictionaryRef loadedKexts = KextManagerCopyLoadedKextInfo(miceDivers, NULL);
    NSDictionary *dict = CFBridgingRelease(loadedKexts);

    return ([dict count] > 0);
}

- (void)updateEnableDriverCheckboxState
{
    if ([self isDriverLoaded])
    {
        [self.enableDriverCheckBox setState:NSControlStateValueOn];
    }
    else
    {
        [self.enableDriverCheckBox setState:NSControlStateValueOff];
    }
}

// Asks the user to uninstall the package, If YES, runs inline AppleScript to do that procedure.
- (IBAction)willPerformUninstallation:(id)sender
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"Uninstall"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setMessageText:@"Uninstall 360Controller?"];
    [alert setInformativeText:@"This operation cannot be undone."];
    [alert setAlertStyle:NSAlertStyleWarning];

    if ([alert runModal] != NSAlertFirstButtonReturn)
    {
        return;
    }

    NSString *uninstallFilePath = [[self bundle] pathForResource:@"upgrade" ofType:@"sh"];

    NSString *script = [NSString stringWithFormat:@"do shell script \"%@\" with administrator privileges", uninstallFilePath];

    if (script != nil && [self runInlineAppleScript:script]) {
        NSLog(@"...done!");

        NSAlert* doneAlert = [[NSAlert alloc] init];
        [doneAlert setMessageText:@"Success!"];
        [doneAlert setInformativeText:@"The driver was uninstalled successfully!\n Note that you may need to restart your Mac to be able to install it again properly."];
        [doneAlert runModal];

        // close the Preference Panel, as it needs to clean stuff
        [[NSApplication sharedApplication] terminate:nil];
    } else {
        NSLog(@"...error!");
        NSAlert* errorAlert = [[NSAlert alloc] init];
        [errorAlert setMessageText:@"Error!"];
        [errorAlert setInformativeText:@"Error Uninstalling the Driver!"];
        [errorAlert runModal];
    }
}

// Handle I/O Kit device add/remove
- (void)handleDeviceChange
{
    // Ideally, this function would make a note of the controller's Location ID, then
    // reselect it when the list is updated, if it's still in the list.
    [self updateDeviceList];
}

- (IBAction)powerOff:(id)sender
{
    FFEFFESCAPE escape = {0};

    if (ffDevice == 0) return;
    escape.dwSize=sizeof(escape);
    escape.dwCommand=0x03;
    FFDeviceEscape(ffDevice, &escape);
}

- (IBAction)startRemappingPressed:(id)sender {
    if (![_wholeControllerMapper isMapping])
        [_wholeControllerMapper runMapperWithButton:_remappingButton andOwner:self];
    else
        [_wholeControllerMapper cancelMappingWithButton:_remappingButton andOwner:self];
}

- (IBAction)resetRemappingPressed:(id)sender {
    [_remappingButton setState:NSControlStateValueOff];
    [_wholeControllerMapper resetWithOwner:self];
}

- (IBAction)pretend360Checked:(id)sender {
    [self changeSetting:NULL];
    [self performSelector:@selector(updateDeviceList) withObject:nil afterDelay:0.5];
}

- (void)startClient
{
    CFReadStreamRef readStream = nil;
    CFWriteStreamRef writeStream = nil;
    CFNetServiceRef netServiceRef = CFNetServiceCreate(kCFAllocatorDefault, CFSTR("local."), CFSTR("_360Daemon._tcp."), CFSTR("360Daemon"), 0);
    if (netServiceRef != nil)
    {
        CFStreamCreatePairWithSocketToNetService(kCFAllocatorDefault, netServiceRef, &readStream, &writeStream);
        CFRelease(netServiceRef);

        if (readStream == nil)
        {
            return;
        }
        if (writeStream == nil)
        {
            return;
        }

        self.readStream = CFBridgingRelease(readStream);
        self.writeStream = CFBridgingRelease(writeStream);

        [self.readStream setDelegate:self];
        [self.writeStream setDelegate:self];

        [self.readStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [self.writeStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];

        [self.readStream open];
        [self.writeStream open];
    }
}

- (void)stopClient
{
    if (self.writeStream != nil)
    {
        [self.writeStream setDelegate:nil];
        [self.writeStream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [self.writeStream close];
    }

    if (self.readStream != nil)
    {
        [self.readStream setDelegate:nil];
        [self.readStream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [self.readStream close];
    }
}

- (void)tabView:(NSTabView *)tabView willSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    NSInteger tabIndex = [tabView indexOfTabViewItem:tabViewItem];
    if (tabIndex == 0) // Controller Test
    {

    }
    else if (tabIndex == 1) // Binding Tab
    {

    }
    else if (tabIndex == 2) // Advanced Tab
    {
        [self updateEnableDriverCheckboxState];
    }
    else if (tabIndex == 3) // About Tab
    {

    }
}

@end
