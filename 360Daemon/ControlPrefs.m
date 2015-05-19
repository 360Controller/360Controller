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
    CFPreferencesSetValue((__bridge CFStringRef)serial, (__bridge CFPropertyListRef)(data), DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
}

NSDictionary* GetController(NSString *serial)
{
    CFPropertyListRef value;
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    value = CFPreferencesCopyValue((__bridge CFStringRef)serial, DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
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
    // Setting the dictionary should work?
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject:devices];
    CFPreferencesSetValue((CFStringRef)D_KNOWNDEV, (__bridge CFPropertyListRef)(data), DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
}

NSDictionary* GetKnownDevices(void)
{
    CFPropertyListRef value;
    NSData *data;
    
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    value = CFPreferencesCopyValue((CFStringRef)D_KNOWNDEV, DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    data = CFBridgingRelease(value);
    if (data == nil)
        return nil;
    return [NSKeyedUnarchiver unarchiveObjectWithData:data];
}
