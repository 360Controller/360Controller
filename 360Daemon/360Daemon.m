/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro
 
 360Daemon.m - main functionality of the support daemon
 
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
#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#include <IOKit/usb/IOUSBLib.h>
#import <ForceFeedback/ForceFeedback.h>
#include "ControlPrefs.h"

#define CHECK_SHOWAGAIN     @"Do not show this message again"

#define INSTALL_PATH        @"/Library/Application Support/MICE/360Daemon"
#define RESOURCE_PATH       INSTALL_PATH @"/Resources"

mach_port_t masterPort;
IONotificationPortRef notifyPort;
CFRunLoopSourceRef notifySource;
io_iterator_t onIteratorWired;
io_iterator_t onIteratorWireless;
io_iterator_t onIteratorOther;
io_iterator_t offIteratorWired;
io_iterator_t offIteratorWireless;
BOOL foundWirelessReceiver;
NSString *leds[4];

CFUserNotificationRef activeAlert = nil;
CFRunLoopSourceRef activeAlertSource;
int activeAlertIndex;

enum {
    kaPlugNCharge = 0,
};

NSString *alertStrings[] = {
    @"You have attached a Microsoft Play & Charge cable for your XBox 360 Wireless Controller. While this cable will allow you to charge your wireless controller, you will require the Microsoft Wireless Gaming Receiver for Windows to use your wireless controller in Mac OS X!",
};

static void releaseAlert(void)
{
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), activeAlertSource, kCFRunLoopCommonModes);
    CFRelease(activeAlertSource);
    CFRelease(activeAlert);
    activeAlertSource = nil;
    activeAlert = nil;
}

static void callbackAlert(CFUserNotificationRef userNotification, CFOptionFlags responseFlags)
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    if (responseFlags & CFUserNotificationCheckBoxChecked(0))
        SetAlertDisabled(activeAlertIndex);
    releaseAlert();
    [pool release];
}

static void ShowAlert(int index)
{
    SInt32 error;
    NSArray *checkBoxes = [NSArray arrayWithObjects:CHECK_SHOWAGAIN, nil];
    NSArray *dictKeys = [NSArray arrayWithObjects:
        (NSString*)kCFUserNotificationAlertHeaderKey,
        (NSString*)kCFUserNotificationAlertMessageKey,
        (NSString*)kCFUserNotificationCheckBoxTitlesKey,
        (NSString*)kCFUserNotificationIconURLKey,
        nil];
    NSArray *dictValues = [NSArray arrayWithObjects:
        @"XBox 360 Controller Driver",
        alertStrings[index],
        checkBoxes,
        [NSURL fileURLWithPath:RESOURCE_PATH @"/Alert.tif"],
        nil];
    NSDictionary *dictionary = [NSDictionary dictionaryWithObjects:dictValues forKeys:dictKeys];
    
    if (AlertDisabled(index))
        return;

    if (activeAlert != nil)
    {
        CFUserNotificationCancel(activeAlert);
        releaseAlert();
    }

    activeAlertIndex = index;
    activeAlert = CFUserNotificationCreate(kCFAllocatorDefault, 0, kCFUserNotificationPlainAlertLevel, &error, (CFDictionaryRef)dictionary);
    activeAlertSource = CFUserNotificationCreateRunLoopSource(kCFAllocatorDefault, activeAlert, callbackAlert, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), activeAlertSource, kCFRunLoopCommonModes);
}

static void ConfigureDevice(io_service_t object)
{
    IOCFPlugInInterface **iodev;
    IOUSBDeviceInterface **dev;
    IOReturn err;
    SInt32 score;
    
    if ((!IOCreatePlugInInterfaceForService(object, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &iodev, &score))&&iodev)
    {
        err = (*iodev)->QueryInterface(iodev, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID)&dev);
        (*iodev)->Release(iodev);
        if ((!err) && dev)
        {
            if ((*dev)->USBDeviceOpen(dev) == 0)
            {
                IOUSBConfigurationDescriptorPtr confDesc;
                
                if ((*dev)->GetConfigurationDescriptorPtr(dev, 0, &confDesc) == 0)
                {
                    (*dev)->SetConfiguration(dev, confDesc->bConfigurationValue);
                    // Open interface? Hopefully not necessary
                }
                (*dev)->USBDeviceClose(dev);
            }
            (*dev)->Release(dev);
        }
    }
}

// Supported device - connecting - set settings?
static void callbackConnected(void *param,io_iterator_t iterator)
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    io_service_t object = 0;
    
    while ((object = IOIteratorNext(iterator)) != 0)
    {
		/*
		CFStringRef bob = IOObjectCopyClass(object);
		NSLog(@"Found %p: %@", object, bob);
		CFRelease(bob);
		 */
        if (IOObjectConformsTo(object, "WirelessHIDDevice") || IOObjectConformsTo(object, "Xbox360ControllerClass"))
        {
            FFDeviceObjectReference forceFeedback;
            NSString *serialNumber;
            
            serialNumber = GetSerialNumber(object);
            // Supported device - load settings
            ConfigController(object, GetController(serialNumber));
            // Set LEDs
            forceFeedback = 0;
            if (FFCreateDevice(object, &forceFeedback) != FF_OK)
                forceFeedback = 0;
            if (forceFeedback != 0)
            {
                FFEFFESCAPE escape;
                unsigned char c;
                int i;
    
                c = 0x0a;
                if (serialNumber != nil)
                {
                    for (i = 0; i < 4; i++)
                    {
                        if ((leds[i] == nil) || ([leds[i] caseInsensitiveCompare:serialNumber] == NSOrderedSame))
                        {
                            c = 0x06 + i;
                            if (leds[i] == nil)
                                leds[i] = [serialNumber retain];
//                            NSLog(@"Added controller with LED %i", i);
                            break;
                        }
                    }
                }
                escape.dwSize = sizeof(escape);
                escape.dwCommand = 0x02;
                escape.cbInBuffer = sizeof(c);
                escape.lpvInBuffer = &c;
                escape.cbOutBuffer = 0;
                escape.lpvOutBuffer = NULL;
                FFDeviceEscape(forceFeedback, &escape);
                FFReleaseDevice(forceFeedback);
            }
        }
        else
        {
            CFTypeRef vendorID = IORegistryEntrySearchCFProperty(object,kIOServicePlane,CFSTR("idVendor"),kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);
            CFTypeRef productID = IORegistryEntrySearchCFProperty(object,kIOServicePlane,CFSTR("idProduct"),kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);
            if ((vendorID != NULL) && (productID != NULL))
            {
                UInt32 idVendor = [((NSNumber*)vendorID) unsignedIntValue];
                UInt32 idProduct = [((NSNumber*)productID) unsignedIntValue];
                if (idVendor == 0x045e)
                {
                    // Microsoft
                    switch (idProduct)
                    {
                        case 0x028f:    // Plug'n'charge cable
                            if (!foundWirelessReceiver)
                                ShowAlert(kaPlugNCharge);
                            ConfigureDevice(object);
                            break;
                        case 0x0719:    // Microsoft Wireless Gaming Receiver
                        case 0x0291:    // Third party Wireless Gaming Receiver
                            foundWirelessReceiver = TRUE;
                            break;
                    }
                }
            }
            if (vendorID != NULL)
                CFRelease(vendorID);
            if (productID != NULL)
                CFRelease(productID);
        }
        IOObjectRelease(object);
    }
    [pool release];
}

// Supported device - disconnecting
static void callbackDisconnected(void *param, io_iterator_t iterator)
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    io_service_t object = 0;
    NSString *serial;
    int i;
    
    while ((object = IOIteratorNext(iterator)) != 0)
    {
		/*
		CFStringRef bob = IOObjectCopyClass(object);
		NSLog(@"Lost %p: %@", object, bob);
		CFRelease(bob);
		 */
        serial = GetSerialNumber(object);
        if (serial != nil)
        {
            for (i = 0; i < 4; i++)
            {
                if (leds[i] == nil)
                    continue;
                if ([leds[i] caseInsensitiveCompare:serial] == NSOrderedSame)
                {
                    [leds[i] release];
                    leds[i] = nil;
//                    NSLog(@"Removed controller with LED %i", i);
                }
            }
        }
        IOObjectRelease(object);
    }
    [pool release];
}

// Entry point
int main (int argc, const char * argv[])
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    foundWirelessReceiver = FALSE;
    memset(leds, 0, sizeof(leds));
    // Get master port, for accessing I/O Kit
    IOMasterPort(MACH_PORT_NULL,&masterPort);
    // Set up notification of USB device addition/removal
    notifyPort=IONotificationPortCreate(masterPort);
    notifySource=IONotificationPortGetRunLoopSource(notifyPort);
    CFRunLoopAddSource(CFRunLoopGetCurrent(),notifySource,kCFRunLoopCommonModes);
    // Start listening
        // USB devices
    IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching(kIOUSBDeviceClassName), callbackConnected, NULL, &onIteratorOther);
    callbackConnected(NULL, onIteratorOther);
        // Wired 360 devices
    IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching("Xbox360ControllerClass"), callbackConnected, NULL, &onIteratorWired);
    callbackConnected(NULL, onIteratorWired);
    IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching("Xbox360ControllerClass"), callbackDisconnected, NULL, &offIteratorWired);
    callbackDisconnected(NULL, offIteratorWired);
        // Wireless 360 devices
    IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching("WirelessHIDDevice"), callbackConnected, NULL, &onIteratorWireless);
    callbackConnected(NULL, onIteratorWireless);
    IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching("WirelessHIDDevice"), callbackDisconnected, NULL, &offIteratorWireless);
    callbackDisconnected(NULL, offIteratorWireless);
    // Run loop
    CFRunLoopRun();
    // Stop listening
    IOObjectRelease(onIteratorOther);
    IOObjectRelease(onIteratorWired);
    IOObjectRelease(offIteratorWired);
    IOObjectRelease(onIteratorWireless);
    IOObjectRelease(offIteratorWireless);
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), notifySource, kCFRunLoopCommonModes);
    CFRunLoopSourceInvalidate(notifySource);
    IONotificationPortDestroy(notifyPort);
    // End
    [pool release];
    return 0;
}
