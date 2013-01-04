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

#define NO_ITEMS            @"No devices found"

// Passes a C callback back to the Objective C class
static void CallbackFunction(void *target,IOReturn result,void *refCon,void *sender)
{
    if(target!=NULL) [((Pref360ControlPref*)target) eventQueueFired:sender withResult:result];
}

// Handle callback for when our device is connected or disconnected. Both events are
// actually handled identically.
static void callbackHandleDevice(void *param,io_iterator_t iterator)
{
    io_service_t object=0;
    BOOL update;
    
    update=FALSE;
    while((object=IOIteratorNext(iterator))!=0) {
        IOObjectRelease(object);
        update=TRUE;
    }
    if(update) [(Pref360ControlPref*)param handleDeviceChange];
}

@implementation Pref360ControlPref

// Set the pattern on the LEDs
- (void)updateLED:(int)ledIndex
{
    FFEFFESCAPE escape;
    unsigned char c;
    
    if(ffDevice==0) return;
    c=ledIndex;
    escape.dwSize=sizeof(escape);
    escape.dwCommand=0x02;
    escape.cbInBuffer=sizeof(c);
    escape.lpvInBuffer=&c;
    escape.cbOutBuffer=0;
    escape.lpvOutBuffer=NULL;
    FFDeviceEscape(ffDevice,&escape);
}

// Enables and disables the rumble motor "override"
- (void)setMotorOverride:(BOOL)enable
{
    FFEFFESCAPE escape;
    char c;
    
    if(ffDevice==0) return;
    // If true, the motors will no longer obey any Force Feedback Framework
    // effects, and the motors may be controlled directly. False and the
    // motors will perform effects but can not be directly controlled.
    c=enable?0x01:0x00;
    escape.dwSize=sizeof(escape);
    escape.dwCommand=0x00;
    escape.cbInBuffer=sizeof(c);
    escape.lpvInBuffer=&c;
    escape.cbOutBuffer=0;
    escape.lpvOutBuffer=NULL;
    FFDeviceEscape(ffDevice,&escape);
}

// If the direct rumble control is enabled, this will set the motors
// to the desired speed.
- (void)testMotorsLarge:(unsigned char)large small:(unsigned char)small
{
    FFEFFESCAPE escape;
    char c[2];
    
    if(ffDevice==0) return;
    c[0]=large;
    c[1]=small;
    escape.dwSize=sizeof(escape);
    escape.dwCommand=0x01;
    escape.cbInBuffer=sizeof(c);
    escape.lpvInBuffer=c;
    escape.cbOutBuffer=0;
    escape.lpvOutBuffer=NULL;
    FFDeviceEscape(ffDevice,&escape);
}

// Update axis GUI component
- (void)axisChanged:(int)index newValue:(int)value
{
    switch(index) {
        case 0:
            [leftStick setPositionX:value];
            break;
        case 1:
            [leftStick setPositionY:value];
            break;
        case 2:
            [rightStick setPositionX:value];
            break;
        case 3:
            [rightStick setPositionY:value];
            break;
        case 4:
            [leftTrigger setDoubleValue:value];
            largeMotor=value;
//            [self testMotorsLarge:largeMotor small:smallMotor];
            break;
        case 5:
            [rightTrigger setDoubleValue:value];
            smallMotor=value;
//            [self testMotorsLarge:largeMotor small:smallMotor];
            break;
        default:
            break;
    }
}

// Update button GUI component
- (void)buttonChanged:(int)index newValue:(int)value
{
    BOOL b;
    
    b=value!=0;
    switch(index) {
        case 0:
            [rightButtons setA:b];
            break;
        case 1:
            [rightButtons setB:b];
            break;
        case 2:
            [rightButtons setX:b];
            break;
        case 3:
            [rightButtons setY:b];
            break;
        case 4:
            [leftShoulder setPressed:b];
            break;
        case 5:
            [rightShoulder setPressed:b];
            break;
        case 6:
            [leftStick setPressed:b];
            break;
        case 7:
            [rightStick setPressed:b];
            break;
        case 8:
            [centreButtons setStart:b];
            break;
        case 9:
            [centreButtons setBack:b];
            break;
        case 10:
            [centreButtons setSpecific:b];
            break;
        case 11:
            [digiStick setUp:b];
            break;
        case 12:
            [digiStick setDown:b];
            break;
        case 13:
            [digiStick setLeft:b];
            break;
        case 14:
            [digiStick setRight:b];
            break;
        default:
            break;
    }
}

// Handle message from I/O Kit indicating something happened on the device
- (void)eventQueueFired:(void*)sender withResult:(IOReturn)result
{
    AbsoluteTime zeroTime={0,0};
    IOHIDEventStruct event;
    BOOL found;
    int i;
    
    if(sender!=hidQueue) return;
    while(result==kIOReturnSuccess) {
        result=(*hidQueue)->getNextEvent(hidQueue,&event,zeroTime,0);
        if(result!=kIOReturnSuccess) continue;
        // Check axis
        for(i=0,found=FALSE;(i<6)&&(!found);i++) {
            if(event.elementCookie==axis[i]) {
                [self axisChanged:i newValue:event.value];
                found=TRUE;
            }
        }
        if(found) continue;
        // Check buttons
        for(i=0,found=FALSE;(i<15)&&(!found);i++) {
            if(event.elementCookie==buttons[i]) {
                [self buttonChanged:i newValue:event.value];
                found=TRUE;
            }
        }
        if(found) continue;
        // Cookie wasn't for us?
    }
}

// Enable input components
- (void)inputEnable:(BOOL)enable
{
    [leftStickDeadzone setEnabled:enable];
    [leftStickInvertX setEnabled:enable];
    [leftStickInvertY setEnabled:enable];
    [leftLinked setEnabled:enable];
    [rightStickDeadzone setEnabled:enable];
    [rightStickInvertX setEnabled:enable];
    [rightStickInvertY setEnabled:enable];
    [rightLinked setEnabled:enable];
}

// Reset GUI components
- (void)resetDisplay
{
    NSBundle *bundle;
    
    [leftStick setPositionX:0 y:0];
    [leftStick setPressed:FALSE];
    [leftStick setDeadzone:0];
    [digiStick setUp:FALSE];
    [digiStick setDown:FALSE];
    [digiStick setLeft:FALSE];
    [digiStick setRight:FALSE];
    [centreButtons setBack:FALSE];
    [centreButtons setSpecific:FALSE];
    [centreButtons setStart:FALSE];
    [rightStick setPositionX:0 y:0];
    [rightStick setPressed:FALSE];
    [rightStick setDeadzone:0];
    [rightButtons setA:FALSE];
    [rightButtons setB:FALSE];
    [rightButtons setX:FALSE];
    [rightButtons setY:FALSE];
    [leftShoulder setPressed:FALSE];
    [leftTrigger setDoubleValue:0];
    [rightShoulder setPressed:FALSE];
    [rightTrigger setDoubleValue:0];
    // Reset inputs
    [leftStickDeadzone setIntValue:0];
    [leftStickInvertX setState:NSOffState];
    [leftStickInvertY setState:NSOffState];
    [rightStickDeadzone setIntValue:0];
    [rightStickInvertX setState:NSOffState];
    [rightStickInvertY setState:NSOffState];
    // Disable inputs
    [self inputEnable:NO];
    [powerOff setHidden:YES];
    // Hide battery icon
    bundle = [NSBundle bundleForClass:[self class]];
    [batteryLevel setImage:[[[NSImage alloc] initByReferencingFile:[bundle pathForResource:@"battNone" ofType:@"tif"]] autorelease]];
}

// Stop using the HID device
- (void)stopDevice
{
    if(registryEntry==0) return;
//    [self testMotorsLarge:0 small:0];
//    [self setMotorOverride:FALSE];
//    [self updateLED:0x00];
    if(hidQueue!=NULL) {
        CFRunLoopSourceRef eventSource;
        
        (*hidQueue)->stop(hidQueue);
        eventSource=(*hidQueue)->getAsyncEventSource(hidQueue);
        if((eventSource!=NULL)&&CFRunLoopContainsSource(CFRunLoopGetCurrent(),eventSource,kCFRunLoopCommonModes))
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(),eventSource,kCFRunLoopCommonModes);
        (*hidQueue)->Release(hidQueue);
        hidQueue=NULL;
    }
    if(device!=NULL) {
        (*device)->close(device);
        device=NULL;
    }
    registryEntry=0;
}

// Start using a HID device
- (void)startDevice
{
    int i,j;
    CFArrayRef elements;
    CFDictionaryRef element;
    CFTypeRef object;
    long number;
    IOHIDElementCookie cookie;
    long usage,usagePage;
    CFRunLoopSourceRef eventSource;
    IOReturn ret;
    
    [self resetDisplay];
    i=(int)[deviceList indexOfSelectedItem];
    if(([deviceArray count]==0)||(i==-1)) {
        NSLog(@"No devices found? :( device count==%i, i==%i",(int)[deviceArray count],i);
        return;
    }
    {
        DeviceItem *item=[deviceArray objectAtIndex:i];
        
        device=[item hidDevice];
        ffDevice=[item ffDevice];
        registryEntry=[item rawDevice];
    }
    if((*device)->copyMatchingElements(device,NULL,&elements)!=kIOReturnSuccess) {
        NSLog(@"Can't get elements list");
        // Make note of failure?
        return;
    }
    for(i=0;i<CFArrayGetCount(elements);i++) {
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
    ret=(*hidQueue)->createAsyncEventSource(hidQueue,&eventSource);
    if(ret!=kIOReturnSuccess) {
        NSLog(@"Unable to create async event source");
        // Error?
        return;
    }
    // Set callback
    ret=(*hidQueue)->setEventCallout(hidQueue,CallbackFunction,self,NULL);
    if(ret!=kIOReturnSuccess) {
        NSLog(@"Unable to set event callback");
        // Error?
        return;
    }
    // Add to runloop
    CFRunLoopAddSource(CFRunLoopGetCurrent(),eventSource,kCFRunLoopCommonModes);
    // Add some elements
    for(i=0;i<6;i++)
        (*hidQueue)->addElement(hidQueue,axis[i],0);
    for(i=0;i<15;i++)
        (*hidQueue)->addElement(hidQueue,buttons[i],0);
    // Start
    ret=(*hidQueue)->start(hidQueue);
    if(ret!=kIOReturnSuccess) {
        NSLog(@"Unable to start queue - 0x%.8x",ret);
        // Error?
        return;
    }
    // Read existing properties
    {
//        CFDictionaryRef dict=(CFDictionaryRef)IORegistryEntryCreateCFProperty(registryEntry,CFSTR("DeviceData"),NULL,0);
        CFDictionaryRef dict = (CFDictionaryRef)[GetController(GetSerialNumber(registryEntry)) retain];
        if(dict!=0) {
            CFBooleanRef boolValue;
            CFNumberRef intValue;
            
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertLeftX"),(void*)&boolValue)) {
                [leftStickInvertX setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertLeftX");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertLeftY"),(void*)&boolValue)) {
                [leftStickInvertY setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertLeftY");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("RelativeLeft"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [leftLinked setState:enable?NSOnState:NSOffState];
                [leftStick setLinked:enable];
            } else NSLog(@"No value for RelativeLeft");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadzoneLeft"),(void*)&intValue)) {
                UInt16 i;
                
                CFNumberGetValue(intValue,kCFNumberShortType,&i);
                [leftStickDeadzone setIntValue:i];
                [leftStick setDeadzone:i];
            } else NSLog(@"No value for DeadzoneLeft");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertRightX"),(void*)&boolValue)) {
                [rightStickInvertX setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertRightX");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertRightY"),(void*)&boolValue)) {
                [rightStickInvertY setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertRightY");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("RelativeRight"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [rightLinked setState:enable?NSOnState:NSOffState];
                [rightStick setLinked:enable];
            } else NSLog(@"No value for RelativeRight");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadzoneRight"),(void*)&intValue)) {
                UInt16 i;
                
                CFNumberGetValue(intValue,kCFNumberShortType,&i);
                [rightStickDeadzone setIntValue:i];
                [rightStick setDeadzone:i];
            } else NSLog(@"No value for DeadzoneRight");
            CFRelease(dict);
        } else NSLog(@"No settings found");
    }
    // Enable GUI components
    [self inputEnable:YES];
    // Set LED and manual motor control
//    [self updateLED:0x0a];
//    [self setMotorOverride:TRUE];
//    [self testMotorsLarge:0 small:0];
    largeMotor=0;
    smallMotor=0;
    // Battery level?
    {
        NSBundle *bundle = [NSBundle bundleForClass:[self class]];
        NSString *path;
        CFTypeRef prop;
        
        path = nil;
        if (IOObjectConformsTo(registryEntry, "WirelessHIDDevice"))
        {
            prop = IORegistryEntryCreateCFProperty(registryEntry, CFSTR("BatteryLevel"), NULL, 0);
            if (prop != nil)
            {
                unsigned char level;
                
                if (CFNumberGetValue(prop, kCFNumberCharType, &level))
                    path = [bundle pathForResource:[NSString stringWithFormat:@"batt%i", level / 64] ofType:@"tif"];
                CFRelease(prop);
            }
            [powerOff setHidden:NO];
        }
        if (path == nil)
            path = [bundle pathForResource:@"battNone" ofType:@"tif"];
        [batteryLevel setImage:[[[NSImage alloc] initByReferencingFile:path] autorelease]];
    }
}

// Clear out the device lists
- (void)deleteDeviceList
{
    [deviceList removeAllItems];
    [deviceArray removeAllObjects];
}

// Update the device list from the I/O Kit
- (void)updateDeviceList
{
    CFMutableDictionaryRef hidDictionary;
    IOReturn ioReturn;
    io_iterator_t iterator;
    io_object_t hidDevice, parent;
    int count;
    DeviceItem *item;
    
    // Scrub old items
    [self stopDevice];
    [self deleteDeviceList];
    // Add new items
    hidDictionary=IOServiceMatching(kIOHIDDeviceKey);
    ioReturn=IOServiceGetMatchingServices(masterPort,hidDictionary,&iterator);
    if((ioReturn!=kIOReturnSuccess)||(iterator==0)) {
        [deviceList addItemWithTitle:NO_ITEMS];
        return;
    }
    count=0;
    while((hidDevice=IOIteratorNext(iterator))) {
		parent = 0;
		IORegistryEntryGetParentEntry(hidDevice, kIOServicePlane, &parent);
        BOOL deviceWired = IOObjectConformsTo(parent, "Xbox360Peripheral") && IOObjectConformsTo(hidDevice, "Xbox360ControllerClass");
        BOOL deviceWireless = IOObjectConformsTo(hidDevice, "WirelessHIDDevice");
        if ((!deviceWired) && (!deviceWireless))
        {
            IOObjectRelease(hidDevice);
            continue;
        }
        item=[DeviceItem allocateDeviceItemForDevice:hidDevice];
        if(item==NULL) continue;
        // Add to item
        NSString *name;
        name = [item name];
        if (name == nil)
            name = @"Generic Controller";
        [deviceList addItemWithTitle:[NSString stringWithFormat:@"%i: %@ (%@)", ++count, name, deviceWireless ? @"Wireless" : @"Wired"]];
        [deviceArray addObject:item];
    }
    IOObjectRelease(iterator);
    if(count==0) [deviceList addItemWithTitle:NO_ITEMS];
    [self startDevice];
}

// Start up
- (void)mainViewDidLoad
{
    io_object_t object;
    
    // Get master port, for accessing I/O Kit
    IOMasterPort(MACH_PORT_NULL,&masterPort);
    // Set up notification of USB device addition/removal
    notifyPort=IONotificationPortCreate(masterPort);
    notifySource=IONotificationPortGetRunLoopSource(notifyPort);
    CFRunLoopAddSource(CFRunLoopGetCurrent(),notifySource,kCFRunLoopCommonModes);
    // Prepare other fields
    deviceArray=[[NSMutableArray arrayWithCapacity:1] retain];
    device=NULL;
    hidQueue=NULL;
    // Activate callbacks
        // Wired
    IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching(kIOUSBDeviceClassName), callbackHandleDevice, self, &onIteratorWired);
    callbackHandleDevice(self, onIteratorWired);
    IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching(kIOUSBDeviceClassName), callbackHandleDevice, self, &offIteratorWired);
    while((object = IOIteratorNext(offIteratorWired)) != 0)
        IOObjectRelease(object);
        // Wireless
    IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching("WirelessHIDDevice"), callbackHandleDevice, self, &onIteratorWireless);
    callbackHandleDevice(self, onIteratorWireless);
    IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching("WirelessHIDDevice"), callbackHandleDevice, self, &offIteratorWireless);
    while((object = IOIteratorNext(offIteratorWireless)) != 0)
        IOObjectRelease(object);
}

// Shut down
- (void)dealloc
{
    int i;
    DeviceItem *item;
    FFEFFESCAPE escape;
    unsigned char c;

    // Remove notification source
    IOObjectRelease(onIteratorWired);
    IOObjectRelease(onIteratorWireless);
    IOObjectRelease(offIteratorWired);
    IOObjectRelease(offIteratorWireless);
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(),notifySource,kCFRunLoopCommonModes);
    CFRunLoopSourceInvalidate(notifySource);
    IONotificationPortDestroy(notifyPort);
    // Release device and info
    [self stopDevice];
    for (i = 0; i < [deviceArray count]; i++)
    {
        item = [deviceArray objectAtIndex:i];
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
    [deviceArray release];
    // Close master port
    mach_port_deallocate(mach_task_self(),masterPort);
    // Done
    [super dealloc];
}

- (mach_port_t)masterPort
{
    return masterPort;
}

// Handle selection from drop down menu
- (void)selectDevice:(id)sender
{
    [self startDevice];
}

// Handle changing a setting
- (void)changeSetting:(id)sender
{
    CFDictionaryRef dict;
    CFStringRef keys[8];
    CFTypeRef values[8];
    UInt16 i;
    
    // Set keys and values
    keys[0]=CFSTR("InvertLeftX");
    values[0]=([leftStickInvertX state]==NSOnState)?kCFBooleanTrue:kCFBooleanFalse;
    keys[1]=CFSTR("InvertLeftY");
    values[1]=([leftStickInvertY state]==NSOnState)?kCFBooleanTrue:kCFBooleanFalse;
    keys[2]=CFSTR("InvertRightX");
    values[2]=([rightStickInvertX state]==NSOnState)?kCFBooleanTrue:kCFBooleanFalse;
    keys[3]=CFSTR("InvertRightY");
    values[3]=([rightStickInvertY state]==NSOnState)?kCFBooleanTrue:kCFBooleanFalse;
    keys[4]=CFSTR("DeadzoneLeft");
    i=[leftStickDeadzone doubleValue];
    values[4]=CFNumberCreate(NULL,kCFNumberShortType,&i);
    keys[5]=CFSTR("DeadzoneRight");
    i=[rightStickDeadzone doubleValue];
    values[5]=CFNumberCreate(NULL,kCFNumberShortType,&i);
    keys[6]=CFSTR("RelativeLeft");
    values[6]=([leftLinked state]==NSOnState)?kCFBooleanTrue:kCFBooleanFalse;
    keys[7]=CFSTR("RelativeRight");
    values[7]=([rightLinked state]==NSOnState)?kCFBooleanTrue:kCFBooleanFalse;
    // Create dictionary
    dict=CFDictionaryCreate(NULL,(const void**)keys,(const void**)values,sizeof(keys)/sizeof(keys[0]),&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    // Set property
    IORegistryEntrySetCFProperties(registryEntry, dict);
    SetController(GetSerialNumber(registryEntry), (NSDictionary*)dict);
    // Update UI
    [leftStick setLinked:([leftLinked state]==NSOnState)];
    [leftStick setDeadzone:[leftStickDeadzone doubleValue]];
    [rightStick setLinked:([rightLinked state]==NSOnState)];
    [rightStick setDeadzone:[rightStickDeadzone doubleValue]];
}

// Handle I/O Kit device add/remove
- (void)handleDeviceChange
{
    // Ideally, this function would make a note of the controller's Location ID, then
    // reselect it when the list is updated, if it's still in the list.
    [self updateDeviceList];
}

- (IBAction)showDeviceList:(id)sender
{
    [deviceLister showWithOwner:self];
}

- (IBAction)powerOff:(id)sender
{
    FFEFFESCAPE escape;
    
    if(ffDevice==0) return;
    escape.dwSize=sizeof(escape);
    escape.dwCommand=0x03;
    escape.cbInBuffer=0;
    escape.lpvInBuffer=NULL;
    escape.cbOutBuffer=0;
    escape.lpvOutBuffer=NULL;
    FFDeviceEscape(ffDevice,&escape);
}

@end
