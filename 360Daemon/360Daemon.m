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
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/usb/IOUSBLib.h>
#include <ForceFeedback/ForceFeedback.h>
#import "ControlPrefs.h"
#import "DaemonLEDs.h"

#define CHECK_SHOWAGAIN     @"Do not show this message again"

#define INSTALL_PATH        @"/Library/Application Support/MICE/360Daemon"
#define RESOURCE_PATH       INSTALL_PATH @"/Resources"

static mach_port_t masterPort;
static IONotificationPortRef notifyPort;
static CFRunLoopSourceRef notifySource;
static io_iterator_t onIteratorWired;
static io_iterator_t onIteratorWireless;
static io_iterator_t onIteratorOther;
static io_iterator_t offIteratorWired;
static io_iterator_t offIteratorWireless;
static BOOL foundWirelessReceiver;
static DaemonLEDs *leds;

static CFUserNotificationRef activeAlert = nil;
static CFRunLoopSourceRef activeAlertSource;
static NSInteger activeAlertIndex;

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
    activeAlertSource = NULL;
    activeAlert = NULL;
}

static void callbackAlert(CFUserNotificationRef userNotification, CFOptionFlags responseFlags)
{
@autoreleasepool {
    if (responseFlags & CFUserNotificationCheckBoxChecked(0))
        SetAlertDisabled(activeAlertIndex);
    releaseAlert();
}
}

static void ShowAlert(NSInteger index)
{
    SInt32 error;
    NSArray *checkBoxes = @[NSLocalizedString(CHECK_SHOWAGAIN, nil)];
    NSDictionary *dictionary = @{(NSString*)kCFUserNotificationAlertHeaderKey: NSLocalizedString(@"XBox 360 Controller Driver", nil),
                                 (NSString*)kCFUserNotificationAlertMessageKey: NSLocalizedString(alertStrings[index], nil),
                                 (NSString*)kCFUserNotificationCheckBoxTitlesKey: checkBoxes,
                                 (NSString*)kCFUserNotificationIconURLKey: [[NSBundle mainBundle] URLForImageResource:@"Alert"]};
    
    if (AlertDisabled(index))
        return;

    if (activeAlert != nil)
    {
        CFUserNotificationCancel(activeAlert);
        releaseAlert();
    }

    activeAlertIndex = index;
    activeAlert = CFUserNotificationCreate(kCFAllocatorDefault, 0, kCFUserNotificationPlainAlertLevel, &error, (__bridge CFDictionaryRef)dictionary);
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
@autoreleasepool {
    io_service_t object = 0;
    
    while ((object = IOIteratorNext(iterator)) != 0)
    {
#if 0
        CFStringRef bob = IOObjectCopyClass(object);
        NSLog(@"Found %p: %@", object, bob);
        CFRelease(bob);
#endif
        if (IOObjectConformsTo(object, "WirelessHIDDevice") || IOObjectConformsTo(object, "Xbox360ControllerClass"))
        {
            FFDeviceObjectReference forceFeedback = 0;
            NSString *serialNumber = GetSerialNumber(object);
            
            // Supported device - load settings
            ConfigController(object, GetController(serialNumber));
            // Set LEDs
            if (FFCreateDevice(object, &forceFeedback) != FF_OK)
                forceFeedback = 0;
            if (forceFeedback != 0)
            {
                FFEFFESCAPE escape = {0};
                unsigned char c;
                int i;
    
                c = 0x0a;
                if (serialNumber != nil)
                {
                    for (i = 0; i < 4; i++)
                    {
                        if ([leds serialNumberAtLEDIsBlank:i] || ([[leds serialNumberAtLED:i] caseInsensitiveCompare:serialNumber] == NSOrderedSame))
                        {
                            c = 0x06 + i;
                            if ([leds serialNumberAtLEDIsBlank:i]) {
                                [leds setLED:i toSerialNumber:serialNumber];
                                // NSLog(@"Added controller with LED %i", i);
                            }
                            break;
                        }
                    }
                }
                escape.dwSize = sizeof(escape);
                escape.dwCommand = 0x02;
                escape.cbInBuffer = sizeof(c);
                escape.lpvInBuffer = &c;
                FFDeviceEscape(forceFeedback, &escape);
                FFReleaseDevice(forceFeedback);
            }
        }
        else
        {
            NSNumber *vendorID = CFBridgingRelease(IORegistryEntrySearchCFProperty(object,kIOServicePlane,CFSTR("idVendor"),kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents));
            NSNumber *productID = CFBridgingRelease(IORegistryEntrySearchCFProperty(object,kIOServicePlane,CFSTR("idProduct"),kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents));
            if ((vendorID != NULL) && (productID != NULL))
            {
                UInt32 idVendor = [vendorID unsignedIntValue];
                UInt32 idProduct = [productID unsignedIntValue];
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
                            foundWirelessReceiver = YES;
                            break;
                    }
                }
            }
        }
        IOObjectRelease(object);
    }
}
}

// Supported device - disconnecting
static void callbackDisconnected(void *param, io_iterator_t iterator)
{
@autoreleasepool {
    io_service_t object = 0;
    NSString *serial;
    int i;
    
    while ((object = IOIteratorNext(iterator)) != 0)
    {
#if 0
        CFStringRef bob = IOObjectCopyClass(object);
        NSLog(@"Lost %p: %@", object, bob);
        CFRelease(bob);
#endif
        serial = GetSerialNumber(object);
        if (serial != nil)
        {
            for (i = 0; i < 4; i++)
            {
                if ([leds serialNumberAtLEDIsBlank:i])
                    continue;
                if ([[leds serialNumberAtLED:i] caseInsensitiveCompare:serial] == NSOrderedSame)
                {
                    [leds clearSerialNumberAtLED:i];
                    // NSLog(@"Removed controller with LED %i", i);
                }
            }
        }
        IOObjectRelease(object);
    }
}
}

// Entry point
int main (int argc, const char * argv[])
{
@autoreleasepool {
    foundWirelessReceiver = NO;
    leds = [[DaemonLEDs alloc] init];
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
}
    return 0;
}
