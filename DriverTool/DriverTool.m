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

#define DRIVER_NAME     @"360Controller.kext"

static NSDictionary *infoPlistAttributes = nil;

static NSString* GetDriverDirectory(void)
{
    NSArray *data = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSSystemDomainMask, YES);
    return [[data objectAtIndex:0] stringByAppendingPathComponent:@"Extensions"];
}

static NSString* GetDriverConfigPath(NSString *driver)
{
    NSString *root = GetDriverDirectory();
    NSString *driverPath = [root stringByAppendingPathComponent:driver];
    NSString *contents = [driverPath stringByAppendingPathComponent:@"Contents"];
    return [contents stringByAppendingPathComponent:@"Info.plist"];
}

static id ReadDriverConfig(NSString *driver)
{
    NSString *filename;
    NSError *error = nil;
    NSData *data;
    NSDictionary *config;
    
    filename = GetDriverConfigPath(driver);
    infoPlistAttributes = [[[NSFileManager defaultManager] attributesOfItemAtPath:filename error:&error] retain];
    if (infoPlistAttributes == nil)
    {
        NSLog(@"Warning: Failed to read attributes of '%@': %@",
              filename, error);
    }
    data = [NSData dataWithContentsOfFile:filename];
    config = [NSPropertyListSerialization propertyListFromData:data mutabilityOption:0 format:NULL errorDescription:NULL];
    return config;
}

static void WriteDriverConfig(NSString *driver, id config)
{
    NSString *filename;
    NSString *errorString;
    NSData *data;
    
    filename = GetDriverConfigPath(driver);
    errorString = nil;
    data = [NSPropertyListSerialization dataFromPropertyList:config format:NSPropertyListXMLFormat_v1_0 errorDescription:&errorString];
    if (data == nil)
        NSLog(@"Error writing config for driver: %@", errorString);
    [errorString release];
    if (![data writeToFile:filename atomically:NO])
        NSLog(@"Failed to write file!");
    if (infoPlistAttributes != nil)
    {
        NSError *error = nil;
        if (![[NSFileManager defaultManager] setAttributes:infoPlistAttributes ofItemAtPath:filename error:&error])
        {
            NSLog(@"Error setting attributes on '%@': %@",
                  filename, error);
        }
    }
}

static void ScrubDevices(NSMutableDictionary *devices)
{
    NSMutableArray *deviceKeys;
    
    deviceKeys = [NSMutableArray arrayWithCapacity:10];
    for (NSString *key in [devices allKeys])
    {
        NSDictionary *device = [devices objectForKey:key];
        if ([(NSString*)[device objectForKey:@"IOClass"] compare:@"Xbox360Peripheral"] == NSOrderedSame)
            [deviceKeys addObject:key];
    }
    [devices removeObjectsForKeys:deviceKeys];
}

static id MakeMutableCopy(id object)
{
    return [(id<NSObject>)CFPropertyListCreateDeepCopy(
                    kCFAllocatorDefault,
                    (CFTypeRef)object,
                    kCFPropertyListMutableContainers) autorelease];
}

static void AddDevice(NSMutableDictionary *personalities, NSString *name, int vendor, int product)
{
    NSMutableDictionary *controller;
    
    controller = [NSMutableDictionary dictionaryWithCapacity:10];
    
    // Standard 
    [controller setObject:@"com.mice.driver.Xbox360Controller"
                   forKey:@"CFBundleIdentifier"];
    [controller setObject:[NSDictionary dictionaryWithObject:@"360Controller.kext/Contents/PlugIns/Feedback360.plugin"
                                                      forKey:@"F4545CE5-BF5B-11D6-A4BB-0003933E3E3E"]
                   forKey:@"IOCFPlugInTypes"];
    [controller setObject:@"Xbox360Peripheral"
                   forKey:@"IOClass"];
    [controller setObject:@"IOUSBDevice"
                   forKey:@"IOProviderClass"];
    [controller setObject:[NSNumber numberWithInt:65535]
                   forKey:@"IOKitDebug"];
    
    // Device-specific
    [controller setObject:[NSNumber numberWithInt:vendor]
                   forKey:@"idVendor"];
    [controller setObject:[NSNumber numberWithInt:product]
                   forKey:@"idProduct"];
    
    // Add it to the tree
    [personalities setObject:controller
                      forKey:name];
}

static void AddDevices(NSMutableDictionary *personalities, int argc, const char *argv[])
{
    int i, count;
    
    count = (argc - 2) / 3;
    for (i = 0; i < count; i++)
    {
        NSString *name = [NSString stringWithCString:argv[(i * 3) + 2] encoding:NSUTF8StringEncoding];
        int vendor = atoi(argv[(i * 3) + 3]);
        int product = atoi(argv[(i * 3) + 4]);
        AddDevice(personalities, name, vendor, product);
    }
}

int main (int argc, const char * argv[]) {
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    NSDictionary *config = ReadDriverConfig(DRIVER_NAME);
    if (argc == 1)
    {
        // Print out current types
        NSDictionary *types;
        NSArray *keys;
        
        types = [config objectForKey:@"IOKitPersonalities"];
        keys = [types allKeys];
        for (NSString *key in keys)
        {
            NSDictionary *device = [types objectForKey:key];
            if ([(NSString*)[device objectForKey:@"IOClass"] compare:@"Xbox360Peripheral"] != NSOrderedSame)
                continue;
            fprintf(stdout, "%s,%i,%i\n",
                    [key UTF8String],
                    [[device objectForKey:@"idVendor"] intValue],
                    [[device objectForKey:@"idProduct"] intValue]);
        }
    }
    else if ((argc > 1) && (strcmp(argv[1], "edit") == 0) && (((argc - 2) % 3) == 0))
    {
        NSMutableDictionary *saving;
        NSMutableDictionary *devices;
        
        saving = MakeMutableCopy(config);
        devices = [saving objectForKey:@"IOKitPersonalities"];
        ScrubDevices(devices);
        AddDevices(devices, argc, argv);
        WriteDriverConfig(DRIVER_NAME, saving);

        system("/usr/bin/touch /System/Library/Extensions");
    }
    else
        NSLog(@"Invalid number of parameters (%i)", argc);
    
    [pool drain];
    return 0;
}
