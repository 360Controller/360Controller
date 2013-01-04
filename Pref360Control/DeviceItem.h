/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    DeviceItem.h - declaration of wrapper class for device handles
    
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
#import <Cocoa/Cocoa.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <ForceFeedback/ForceFeedback.h>

@interface DeviceItem : NSObject {
    IOHIDDeviceInterface122 **interface;
    FFDeviceObjectReference forceFeedback;
    io_service_t deviceHandle;
    NSString *deviceName;
}

+ allocateDeviceItemForDevice:(io_service_t)device;

- (NSString*)name;
- (IOHIDDeviceInterface122**)hidDevice;
- (FFDeviceObjectReference)ffDevice;
- (io_service_t)rawDevice;

@end
