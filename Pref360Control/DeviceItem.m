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

static NSString* GetDeviceName(io_service_t device)
{
    CFMutableDictionaryRef serviceProperties;
    NSDictionary *properties;
    NSString *deviceName = nil;
    
    if (IORegistryEntryCreateCFProperties(device, &serviceProperties, kCFAllocatorDefault, kNilOptions) != KERN_SUCCESS)
        return nil;
    properties = CFBridgingRelease(serviceProperties);
    deviceName = properties[@kIOHIDProductKey];
    if (deviceName == nil)
        deviceName = properties[@"USB Product Name"];
    return deviceName;
}

@interface DeviceItem ()
@property (strong, readwrite) NSString *name;
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
            return nil;
        }
        ret = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID122), (LPVOID)&interface);
        (*plugInInterface)->Release(plugInInterface);
        if (ret != kIOReturnSuccess) {
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
        return item;
    
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
}

@end
