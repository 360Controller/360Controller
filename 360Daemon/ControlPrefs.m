/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro

 ControlPrefs.m - code to read and write shared preferences

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
#import "ControlPrefs.h"
#import <libkern/OSKextLib.h>
#import <IOKit/kext/KextManager.h>

void SetAlertDisabled(NSInteger index)
{
    NSString *prop = [NSString stringWithFormat:@"%@%li", D_SHOWONCE, (long)index];

    CFPreferencesSetValue((__bridge CFStringRef)prop, kCFBooleanTrue, DOM_DAEMON, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    CFPreferencesSynchronize(DOM_DAEMON, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
}

BOOL AlertDisabled(NSInteger index)
{
    NSString *prop = [NSString stringWithFormat:@"%@%li", D_SHOWONCE, (long)index];
    BOOL result = NO;
    CFPropertyListRef value = CFPreferencesCopyValue((__bridge CFStringRef)prop, DOM_DAEMON, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);

    if (value != NULL)
    {
        result = [CFBridgingRelease(value) boolValue];
    }
    return result;
}

void SetController(NSString *serial, NSDictionary *data)
{
    CFPreferencesSetValue((__bridge CFStringRef)serial, (__bridge CFPropertyListRef)(data), DOM_CONTROLLERS, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
}

NSDictionary* GetController(NSString *serial)
{
    CFPropertyListRef value;
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    value = CFPreferencesCopyValue((__bridge CFStringRef)serial, DOM_CONTROLLERS, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    return CFBridgingRelease(value);
}

NSString* GetSerialNumber(io_service_t device)
{
    CFTypeRef value = IORegistryEntrySearchCFProperty(device, kIOServicePlane, CFSTR("USB Serial Number"), kCFAllocatorDefault, kIORegistryIterateRecursively);

    if (value == NULL)
        value = IORegistryEntrySearchCFProperty(device, kIOServicePlane, CFSTR("SerialNumber"), kCFAllocatorDefault, kIORegistryIterateRecursively);
    return CFBridgingRelease(value);
}

void ConfigController(io_service_t device, NSDictionary *config)
{
    IORegistryEntrySetCFProperties(device, (__bridge CFTypeRef)(config));
}

void SetKnownDevices(NSDictionary *devices)
{
    NSError* error = nil;
    NSData* data = nil;
    // Setting the dictionary should work?
    if (@available(macOS 10.13, *))
    {
        data = [NSKeyedArchiver archivedDataWithRootObject:devices requiringSecureCoding:true error:&error];
    }
    else
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        data = [NSKeyedArchiver archivedDataWithRootObject:devices];
#pragma GCC diagnostic pop
    }

    if (data != nil)
    {
        CFPreferencesSetValue((CFStringRef)D_KNOWNDEV, (__bridge CFPropertyListRef)(data), DOM_CONTROLLERS, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
        CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    }
    else if (error != nil)
    {
        NSLog(@"360Controller - Failed to set known devices with error: %@\n", error);
    }
}

NSDictionary* GetKnownDevices(void)
{
    CFPropertyListRef value;
    NSData *data;
    NSError* error = nil;
    NSDictionary* result;

    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    value = CFPreferencesCopyValue((CFStringRef)D_KNOWNDEV, DOM_CONTROLLERS, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    data = CFBridgingRelease(value);
    if (data == nil)
        return nil;

    if (@available(macOS 10.13, *))
    {
        result = [NSKeyedUnarchiver unarchivedObjectOfClass:[NSDictionary class] fromData:data error:&error];
    }
    else
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        result = [NSKeyedUnarchiver unarchiveObjectWithData:data];
#pragma GCC diagnostic pop
    }

    if (result == nil)
    {
        NSLog(@"360Controller - Failed to get known devices with error: %@\n", error);
    }

    return result;
}

OSReturn LoadDriver(CFStringRef kextIndentifier)
{
    return KextManagerLoadKextWithIdentifier(kextIndentifier, NULL);
}

OSReturn LoadDriverWired(void)
{
    return LoadDriver(DOM_WIRED_DRIVER);
}

OSReturn LoadDriverWireless(void)
{
    // TODO(Drew): Test and see if this correctly loads the wireless driver or if it needs a link to the wireless controller kext
    return LoadDriver(DOM_WIRELESS_DRIVER);
}

OSReturn UnloadDriver(CFStringRef kextIdentifer)
{
    return KextManagerUnloadKextWithIdentifier(kextIdentifer);
}

OSReturn UnloadDriverWired(void)
{
    return UnloadDriver(DOM_WIRED_DRIVER);
}

OSReturn UnloadDriverWireless(void)
{
    return UnloadDriver(DOM_WIRELESS_DRIVER);
}
