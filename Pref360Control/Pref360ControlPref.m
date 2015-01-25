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
#import "Pref360ControlPref.h"
#import "DeviceItem.h"
#import "ControlPrefs.h"
#import "DeviceLister.h"
#import "MyWhole360Controller.h"
#import "MyTrigger.h"
#import "MyDeadZoneViewer.h"
#import "MyBatteryMonitor.h"

#define NO_ITEMS @"No devices found"

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
    [_aboutPopover setAppearance:NSPopoverAppearanceHUD];
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
    switch(index) {
        case 0:
            [_wholeController setLeftStickXPos:value];
            break;
            
        case 1:
            [_wholeController setLeftStickYPos:value];
            break;
            
        case 2:
            [_wholeController setRightStickXPos:value];
            break;
            
        case 3:
            [_wholeController setRightStickYPos:value];
            break;
            
        case 4:
            [_leftTrigger setVal:value];
            largeMotor=value;
            [self testMotorsLarge:largeMotor small:smallMotor];
            break;
            
        case 5:
            [_rightTrigger setVal:value];
            smallMotor=value;
            [self testMotorsLarge:largeMotor small:smallMotor];
            break;
            
        default:
            break;
    }
}

// Update button GUI component
- (void)buttonChanged:(int)index newValue:(int)value
{
    BOOL b = (value != 0);
    
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
    [_leftStickInvertX setEnabled:enable];
    [_leftStickInvertY setEnabled:enable];
    [_leftLinked setEnabled:enable];
    [_rightStickDeadzone setEnabled:enable];
    [_rightStickInvertX setEnabled:enable];
    [_rightStickInvertY setEnabled:enable];
    [_rightLinked setEnabled:enable];
}

// Reset GUI components
- (void)resetDisplay
{
    [_leftTrigger setVal:0];
    [_rightTrigger setVal:0];
    [_wholeController reset];
    [_leftDeadZone setVal:0];
    [_leftDeadZone setLinked:NO];
    [_rightDeadZone setVal:0];
    [_rightDeadZone setLinked:NO];
    // Reset inputs
    [_leftStickDeadzone setIntValue:0];
    [_leftStickInvertX setState:NSOffState];
    [_leftStickInvertY setState:NSOffState];
    [_rightStickDeadzone setIntValue:0];
    [_rightStickInvertX setState:NSOffState];
    [_rightStickInvertY setState:NSOffState];
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
    CFArrayRef elements;
    CFDictionaryRef element;
    CFTypeRef object;
    long number;
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
    }
    
    if((*device)->copyMatchingElements(device,NULL,&elements)!=kIOReturnSuccess) {
        NSLog(@"Can't get elements list");
        // Make note of failure?
        return;
    }
    
    for (i = 0;i < CFArrayGetCount(elements); i++) {
        element=CFArrayGetValueAtIndex(elements,i);
        // Get cookie
        object=CFDictionaryGetValue(element,CFSTR(kIOHIDElementCookieKey));
        if((object==NULL)||(CFGetTypeID(object)!=CFNumberGetTypeID())) continue;
        if(!CFNumberGetValue((CFNumberRef)object,kCFNumberLongType,&number)) continue;
        cookie=(IOHIDElementCookie)number;
        // Get usage
        object=CFDictionaryGetValue(element,CFSTR(kIOHIDElementUsageKey));
        if((object==0)||(CFGetTypeID(object)!=CFNumberGetTypeID())) continue;
        if(!CFNumberGetValue((CFNumberRef)object,kCFNumberLongType,&number)) continue;
        usage=number;
        // Get usage page
        object=CFDictionaryGetValue(element,CFSTR(kIOHIDElementUsagePageKey));
        if((object==0)||(CFGetTypeID(object)!=CFNumberGetTypeID())) continue;
        if(!CFNumberGetValue((CFNumberRef)object,kCFNumberLongType,&number)) continue;
        usagePage=number;
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
                [_leftStickInvertX setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertLeftX");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertLeftY"),(void*)&boolValue)) {
                [_leftStickInvertY setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertLeftY");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("RelativeLeft"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [_leftLinked setState:enable?NSOnState:NSOffState];
                [_leftDeadZone setLinked:enable];
            } else NSLog(@"No value for RelativeLeft");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadzoneLeft"),(void*)&intValue)) {
                UInt16 i;
                
                CFNumberGetValue(intValue,kCFNumberShortType,&i);
                [_leftStickDeadzone setIntValue:i];
                [_leftDeadZone setVal:i];
            } else NSLog(@"No value for DeadzoneLeft");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertRightX"),(void*)&boolValue)) {
                [_rightStickInvertX setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertRightX");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertRightY"),(void*)&boolValue)) {
                [_rightStickInvertY setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertRightY");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("RelativeRight"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [_rightLinked setState:enable?NSOnState:NSOffState];
                [_rightDeadZone setLinked:enable];
            } else NSLog(@"No value for RelativeRight");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadzoneRight"),(void*)&intValue)) {
                UInt16 i;
                
                CFNumberGetValue(intValue,kCFNumberShortType,&i);
                [_rightStickDeadzone setIntValue:i];
                [_rightDeadZone setVal:i];
            } else NSLog(@"No value for DeadzoneRight");
            CFRelease(dict);
        } else NSLog(@"No settings found");
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
        CFTypeRef prop;
        
        if (IOObjectConformsTo(registryEntry, "WirelessHIDDevice")) {
            prop = IORegistryEntryCreateCFProperty(registryEntry, CFSTR("BatteryLevel"), NULL, 0);
            if (prop != nil) {
                unsigned char level;
                
                if (CFNumberGetValue(prop, kCFNumberCharType, &level))
                    batteryLevel = level / 64;
                CFRelease(prop);
            }
            [_powerOff setHidden:NO];
        }
        if ( batteryLevel >= 0) {
            [_batteryStatus setBars:batteryLevel];
            [_batteryStatus setHidden:NO];
        } else {
            [_batteryStatus setHidden:YES];
        }
    }
}

// Clear out the device lists
- (void)deleteDeviceList
{
    [_deviceList removeAllItems];
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
    
    // Scrub old items
    [self stopDevice];
    [self deleteDeviceList];
    // Add new items
    hidDictionary=IOServiceMatching(kIOHIDDeviceKey);
    ioReturn=IOServiceGetMatchingServices(_masterPort,hidDictionary,&iterator);
    if((ioReturn != kIOReturnSuccess) || (iterator == 0)) {
        [_deviceList addItemWithTitle:NO_ITEMS];
        return;
    }
    
    while ((hidDevice = IOIteratorNext(iterator))) {
		io_object_t parent = 0;
		IORegistryEntryGetParentEntry(hidDevice, kIOServicePlane, &parent);
        BOOL deviceWired = IOObjectConformsTo(parent, "Xbox360Peripheral") && IOObjectConformsTo(hidDevice, "Xbox360ControllerClass");
        BOOL deviceWireless = IOObjectConformsTo(hidDevice, "WirelessHIDDevice");
        if ((!deviceWired) && (!deviceWireless))
        {
            IOObjectRelease(hidDevice);
            continue;
        }
        DeviceItem *item = [DeviceItem allocateDeviceItemForDevice:hidDevice];
        if (item == nil) continue;
        // Add to item
        NSString *name = [item name];
        if (name == nil)
            name = @"Generic Controller";
        [_deviceList addItemWithTitle:[NSString stringWithFormat:@"%i: %@ (%@)", ++count, name, deviceWireless ? @"Wireless" : @"Wired"]];
        [_deviceArray addObject:item];
    }
    IOObjectRelease(iterator);
    if (count==0) [_deviceList addItemWithTitle:NO_ITEMS];
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
    callbackHandleDevice((__bridge void *)(self), onIteratorWireless);
    IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching("WirelessHIDDevice"), callbackHandleDevice, (__bridge void *)(self), &offIteratorWireless);
    while((object = IOIteratorNext(offIteratorWireless)) != 0)
        IOObjectRelease(object);
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
    // Done
}

// Handle selection from drop down menu
- (IBAction)selectDevice:(id)sender
{
    [self startDevice];
}

// Handle changing a setting
- (IBAction)changeSetting:(id)sender
{
    // Create dictionary
    NSDictionary *dict = @{@"InvertLeftX": @((BOOL)([_leftStickInvertX state]==NSOnState)),
                           @"InvertLeftY": @((BOOL)([_leftStickInvertY state]==NSOnState)),
                           @"InvertRightX": @((BOOL)([_rightStickInvertX state]==NSOnState)),
                           @"InvertRightY": @((BOOL)([_rightStickInvertY state]==NSOnState)),
                           @"DeadzoneLeft": @((UInt16)[_leftStickDeadzone doubleValue]),
                           @"DeadzoneRight": @((UInt16)[_rightStickDeadzone doubleValue]),
                           @"RelativeLeft": @((BOOL)([_leftLinked state]==NSOnState)),
                           @"RelativeRight": @((BOOL)([_rightLinked state]==NSOnState))};
    
    // Set property
    IORegistryEntrySetCFProperties(registryEntry, (__bridge CFTypeRef)(dict));
    SetController(GetSerialNumber(registryEntry), dict);
    // Update UI
    [_leftDeadZone setLinked:[_leftLinked state] == NSOnState];
    [_leftDeadZone setVal:[_leftStickDeadzone doubleValue]];
    [_rightDeadZone setLinked:[_rightLinked state] == NSOnState];
    [_rightDeadZone setVal:[_rightStickDeadzone doubleValue]];
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

- (IBAction)showAboutPopover:(id)sender {
    [_aboutPopover showRelativeToRect:[sender bounds] ofView:sender preferredEdge:NSMinXEdge];
}
@end
