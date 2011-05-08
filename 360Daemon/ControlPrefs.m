//
//  ControlPrefs.m
//  360Daemon
//
//  Created by Colin Munro on 8/11/07.
//  Copyright 2007 Colin Munro. All rights reserved.
//

#import "ControlPrefs.h"

void SetAlertDisabled(int index)
{
    NSString *prop;
    NSNumber *value;

    prop = [NSString stringWithFormat:@"%@%i", D_SHOWONCE, index];
    value = [NSNumber numberWithBool:TRUE];
    CFPreferencesSetValue((CFStringRef)prop, value, DOM_DAEMON, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    CFPreferencesSynchronize((CFStringRef)DOM_DAEMON, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
}

BOOL AlertDisabled(int index)
{
    NSString *prop;
    BOOL result;
    CFPropertyListRef value;
    
    result = FALSE;
    prop = [NSString stringWithFormat:@"%@%i", D_SHOWONCE, index];
    value = CFPreferencesCopyValue((CFStringRef)prop, DOM_DAEMON, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    if (value != NULL)
    {
        result = [((NSNumber*)value) boolValue];
        CFRelease(value);
    }
    return result;
}

void SetController(NSString *serial, NSDictionary *data)
{
    CFPreferencesSetValue((CFStringRef)serial, data, DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
}

NSDictionary* GetController(NSString *serial)
{
    CFPropertyListRef value;
    
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    value = CFPreferencesCopyValue((CFStringRef)serial, DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    return [((NSDictionary*)value) autorelease];
}

NSString* GetSerialNumber(io_service_t device)
{
    CFTypeRef value;
    
    value = IORegistryEntrySearchCFProperty(device, kIOServicePlane, CFSTR("USB Serial Number"), kCFAllocatorDefault, kIORegistryIterateRecursively);
    if (value == NULL)
        value = IORegistryEntrySearchCFProperty(device, kIOServicePlane, CFSTR("SerialNumber"), kCFAllocatorDefault, kIORegistryIterateRecursively);
    return [((NSString*)value) autorelease];
}

void ConfigController(io_service_t device, NSDictionary *config)
{
    IORegistryEntrySetCFProperties(device, config);
}

void SetKnownDevices(NSDictionary *devices)
{
    // Setting the dictionary should work?
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject:devices];
    CFPreferencesSetValue((CFStringRef)D_KNOWNDEV, data, DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
}

NSDictionary* GetKnownDevices(void)
{
    CFPropertyListRef value;
    NSData *data;
    
    CFPreferencesSynchronize(DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    value = CFPreferencesCopyValue((CFStringRef)D_KNOWNDEV, DOM_CONTROLLERS, kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
    data = [(NSData*)value autorelease];
    if (data == nil)
        return nil;
    return [NSKeyedUnarchiver unarchiveObjectWithData:data];
}
