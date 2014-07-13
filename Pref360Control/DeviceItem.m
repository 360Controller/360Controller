/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    DeviceItem.m - implementation of device wrapper class
    
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
#import "DeviceItem.h"

#if !defined(__OBJC_GC__) && defined(__x86_64__)
#warning building without Garbage Collection support. The 64-bit Preference Pane won't be compatible with Lion.
#warning The 32-bit preference pane, however, will be. But the user will have to re-launch System Preferences.
#endif

static NSString* GetDeviceName(io_service_t device)
{
    CFMutableDictionaryRef serviceProperties;
    NSDictionary *properties;
    NSString *deviceName = nil;
    
    if (IORegistryEntryCreateCFProperties(device, &serviceProperties, kCFAllocatorDefault, kNilOptions) != KERN_SUCCESS)
        return nil;
    properties = CFBridgingRelease(serviceProperties);
    deviceName = [properties objectForKey:@kIOHIDProductKey];
    if (deviceName == nil)
        deviceName = [properties objectForKey:@"USB Product Name"];
    return deviceName;
}

@interface DeviceItem ()
@property (arcstrong, readwrite) NSString *name;
@property (readwrite) io_service_t rawDevice;
@property (readwrite) FFDeviceObjectReference ffDevice;
@property (readwrite) IOHIDDeviceInterface122 **hidDevice;
@end

@implementation DeviceItem
@synthesize name = deviceName;
@synthesize rawDevice = deviceHandle;
@synthesize ffDevice = forceFeedback;
@synthesize hidDevice = interface;

- (instancetype)initWithItemForDevice:(io_service_t)device
{
    if (self = [super init]) {
        IOReturn ret;
        IOCFPlugInInterface **plugInInterface;
        SInt32 score=0;
        
        ret = IOCreatePlugInInterfaceForService(device, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        if (ret != kIOReturnSuccess) {
            RELEASEOBJ(self);
            return nil;
        }
        ret = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID122), (LPVOID)&interface);
        (*plugInInterface)->Release(plugInInterface);
        if (ret != kIOReturnSuccess) {
            RELEASEOBJ(self);
            return nil;
        }
        forceFeedback = 0;
        FFCreateDevice(device, &forceFeedback);
        self.rawDevice = device;
        self.name = GetDeviceName(device);
    }
    return self;
}

+ (instancetype)allocateDeviceItemForDevice:(io_service_t)device
{
    DeviceItem *item = [[[self class] alloc] initWithItemForDevice:device];
    
    if (item)
        return AUTORELEASEOBJ(item);
    
    IOObjectRelease(device);
    return nil;
}

- (void)dealloc
{
    if (deviceHandle)
        IOObjectRelease(deviceHandle);
    if (interface)
        (*interface)->Release(interface);
    if (forceFeedback)
        FFReleaseDevice(forceFeedback);
#if !__has_feature(objc_arc)
    self.name = nil;
    
    [super dealloc];
#endif
}

#ifdef __OBJC_GC__
- (void)finalize
{
    if (deviceHandle)
        IOObjectRelease(deviceHandle);
    if (interface)
        (*interface)->Release(interface);
    if (forceFeedback)
        FFReleaseDevice(forceFeedback);
    
    [super finalize];
}
#endif

@end
