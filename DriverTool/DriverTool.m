/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro
 
 DriverTool.m - implementation of driver info tweaking tool
 
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

#define DRIVER_NAME @"360Controller.kext"

static NSDictionary *infoPlistAttributes = nil;

static inline NSString* GetDriverDirectory(void)
{
    NSArray *data = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSSystemDomainMask, YES);
    return [data[0] stringByAppendingPathComponent:@"Extensions"];
}

static NSString* GetDriverConfigPath(NSString *driver)
{
    NSString *root = GetDriverDirectory();
    NSArray *pathComp = [root pathComponents];
    pathComp = [pathComp arrayByAddingObjectsFromArray:@[driver, @"Contents", @"Info.plist"]];
    return [NSString pathWithComponents:pathComp];
}

static NSDictionary *ReadDriverConfig(NSString *driver)
{
    NSString *filename = GetDriverConfigPath(driver);
    NSError *error;
    NSData *data;
    
    infoPlistAttributes = [[NSFileManager defaultManager] attributesOfItemAtPath:filename error:&error];
    if (infoPlistAttributes == nil)
    {
        NSLog(@"Warning: Failed to read attributes of '%@': %@",
              filename, error);
    }
    data = [NSData dataWithContentsOfFile:filename];
    return [NSPropertyListSerialization propertyListFromData:data mutabilityOption:0 format:NULL errorDescription:NULL];
}

static void WriteDriverConfig(NSString *driver, id config)
{
    NSString *filename = GetDriverConfigPath(driver);
    NSError *error;
    NSData *data = [NSPropertyListSerialization dataWithPropertyList:config format:NSPropertyListXMLFormat_v1_0 options:0 error:&error];
    
    if (data == nil)
        NSLog(@"Error writing config for driver: %@", error);
    
    if (![data writeToFile:filename atomically:NO])
        NSLog(@"Failed to write file!");
    
    if (infoPlistAttributes != nil) {
        if (![[NSFileManager defaultManager] setAttributes:infoPlistAttributes ofItemAtPath:filename error:&error]) {
            NSLog(@"Error setting attributes on '%@': %@", filename, error);
        }
    }
}

static void ScrubDevices(NSMutableDictionary *devices)
{
    NSMutableArray *deviceKeys = [[NSMutableArray alloc] initWithCapacity:10];
    
    for (NSString *key in devices) {
        NSDictionary *device = devices[key];
        if ([(NSString*)device[@"IOClass"] compare:@"Xbox360Peripheral"] == NSOrderedSame)
            [deviceKeys addObject:key];
    }
    [devices removeObjectsForKeys:deviceKeys];
}

static id MakeMutableCopy(id object)
{
    return CFBridgingRelease(CFPropertyListCreateDeepCopy(kCFAllocatorDefault,
                                                          (CFTypeRef)object,
                                                          kCFPropertyListMutableContainers));
}

static void AddDevice(NSMutableDictionary *personalities, NSString *name, int vendor, int product)
{
    NSMutableDictionary *controller = [[NSMutableDictionary alloc] initWithCapacity:10];
    
    // Standard
    controller[@"CFBundleIdentifier"] = @"com.mice.driver.Xbox360Controller";
    controller[@"IOCFPlugInTypes"] = @{@"F4545CE5-BF5B-11D6-A4BB-0003933E3E3E": @"360Controller.kext/Contents/PlugIns/Feedback360.plugin"};
    controller[@"IOClass"] = @"Xbox360Peripheral";
    controller[@"IOProviderClass"] = @"IOUSBDevice";
    controller[@"IOKitDebug"] = @65535;
    
    // Device-specific
    controller[@"idVendor"] = @(vendor);
    controller[@"idProduct"] = @(product);
    
    // Add it to the tree
    personalities[name] = controller;
}

static void AddDevices(NSMutableDictionary *personalities, int argc, const char *argv[])
{
    int i, count = (argc - 2) / 3;
    for (i = 0; i < count; i++)
    {
        NSString *name = @(argv[(i * 3) + 2]);
        int vendor = atoi(argv[(i * 3) + 3]);
        int product = atoi(argv[(i * 3) + 4]);
        AddDevice(personalities, name, vendor, product);
    }
}

int main (int argc, const char * argv[]) {
@autoreleasepool {
    NSDictionary *config = ReadDriverConfig(DRIVER_NAME);
    if (argc == 1)
    {
        // Print out current types
        NSDictionary *types = config[@"IOKitPersonalities"];

        for (NSString *key in types) {
            NSDictionary *device = types[key];
            if ([(NSString*)device[@"IOClass"] compare:@"Xbox360Peripheral"] != NSOrderedSame)
                continue;
            fprintf(stdout, "%s,%i,%i\n",
                    [key UTF8String],
                    [device[@"idVendor"] intValue],
                    [device[@"idProduct"] intValue]);
        }
    } else if ((argc > 1) && (strcmp(argv[1], "edit") == 0) && (((argc - 2) % 3) == 0)) {
        NSMutableDictionary *saving;
        NSMutableDictionary *devices;
        
        saving = MakeMutableCopy(config);
        devices = saving[@"IOKitPersonalities"];
        ScrubDevices(devices);
        AddDevices(devices, argc, argv);
        WriteDriverConfig(DRIVER_NAME, saving);

        system("/usr/bin/touch /System/Library/Extensions");
    }
    else
        NSLog(@"Invalid number of parameters (%i)", argc);
    
    return 0;
}
}
