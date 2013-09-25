/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro
 
 DeviceLister.m - implementation of a class to list supported devices
 
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
#import <IOKit/usb/IOUSBLib.h>
#import "DeviceLister.h"
#import "Pref360ControlPref.h"
#import "ControlPrefs.h"

#define TOOL_FILENAME       @"DriverTool"

// Get some sort of CF type for a field in the IORegistry
static id GetDeviceValue(io_service_t device, NSString *key)
{
    CFTypeRef value;
    
    value = IORegistryEntrySearchCFProperty(device, kIOServicePlane, (CFStringRef)key, kCFAllocatorDefault, kIORegistryIterateRecursively);
    return [((NSObject*)value) autorelease];
}

// Make sure a name is as nice as possible for eventually going into the XML for the driver
static NSString* SanitiseName(NSString *name)
{
    NSMutableString *output;
    int i;
    
    output = [NSMutableString stringWithCapacity:100];
    for (i = 0; i < [name length]; i++)
    {
        unichar c = [name characterAtIndex:i];
        if (c == ' ')
            c = '_';
        else if (!(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || (c == '_')))
            continue;
        [output appendFormat:@"%C", c];
    }
    return output;
}

// Get the Device interface for a given IO service
static IOUSBDeviceInterface** GetDeviceInterface(io_service_t device)
{
    IOCFPlugInInterface **iodev;
    IOUSBDeviceInterface **dev;
    IOReturn err;
    SInt32 score;
    
    if ((IOCreatePlugInInterfaceForService(device, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &iodev, &score) == kIOReturnSuccess) && (iodev != NULL))
    {
        err = (*iodev)->QueryInterface(iodev, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID)&dev);
        (*iodev)->Release(iodev);
        if (err == kIOReturnSuccess)
            return dev;
    }
    return NULL;
}

// Get the Interface interface for a given IO service
static IOUSBInterfaceInterface** GetInterfaceInterface(io_service_t interface)
{
    IOCFPlugInInterface **iodev;
    IOUSBInterfaceInterface **intf;
    IOReturn err;
    SInt32 score;
    
    if ((IOCreatePlugInInterfaceForService(interface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &iodev, &score) == kIOReturnSuccess) && (iodev != NULL))
    {
        err = (*iodev)->QueryInterface(iodev, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID)&intf);
        (*iodev)->Release(iodev);
        if (err == kIOReturnSuccess)
            return intf;
    }
    return NULL;
}

// List of interfaces we expect on a normal Microsoft controller
const struct {
    int numEndpoints;
    UInt8 classNum, subClassNum, protocolNum;
} ControllerInterfaces[] = {
//  Endpoints   Class   Subclass    Protocol
    {2,         255,    93,         1},
    {4,         255,    93,         3},
    {1,         255,    93,         2},
    {0,         255,    253,        19},
};

// Detect if an IO service object is a Microsoft controller by running through and checking some things
static BOOL IsXBox360Controller(io_service_t device)
{
    IOUSBDeviceInterface **interface;
    IOUSBFindInterfaceRequest iRq;
    io_iterator_t iterator;
    io_service_t devInterface;
    IOUSBInterfaceInterface **interfaceInterface;
    
    int interfaceCount;
    UInt8 interfaceNum, classNum, subClassNum, protocolNum, endpointCount;
    
    BOOL devValid;

    // Get the interface to the device
    interface = GetDeviceInterface(device);
    if (interface == NULL)
        return NO;
    (*interface)->GetDeviceClass(interface, &classNum);
    (*interface)->GetDeviceSubClass(interface, &subClassNum);
    (*interface)->GetDeviceProtocol(interface, &protocolNum);
    devValid = (classNum == 0xFF) && (subClassNum == 0xFF) && (protocolNum == 0xFF);
    
    // Search the interfaces
    iRq.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    iRq.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    iRq.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    iRq.bAlternateSetting = kIOUSBFindInterfaceDontCare;
    interfaceCount = 0;
    if ((*interface)->CreateInterfaceIterator(interface, &iRq, &iterator) == kIOReturnSuccess)
    {
        while ((devInterface = IOIteratorNext(iterator)) != 0)
        {
            interfaceInterface = GetInterfaceInterface(devInterface);
            if (interfaceInterface != NULL)
            {
                (*interfaceInterface)->GetInterfaceNumber(interfaceInterface, &interfaceNum);
                (*interfaceInterface)->GetInterfaceClass(interfaceInterface, &classNum);
                (*interfaceInterface)->GetInterfaceSubClass(interfaceInterface, &subClassNum);
                (*interfaceInterface)->GetInterfaceProtocol(interfaceInterface, &protocolNum);
                (*interfaceInterface)->GetNumEndpoints(interfaceInterface, &endpointCount);
                if (interfaceNum < (sizeof(ControllerInterfaces) / sizeof(ControllerInterfaces[0])))
                {
                    if (
                        (ControllerInterfaces[interfaceNum].classNum == classNum)
                    &&
                        (ControllerInterfaces[interfaceNum].subClassNum == subClassNum)
                    &&
                        (ControllerInterfaces[interfaceNum].protocolNum == protocolNum)
                    &&
                        (ControllerInterfaces[interfaceNum].numEndpoints == endpointCount)
                       )
                    {
                        // Found another interface in the right place
                        interfaceCount++;
                    }
                }
                (*interfaceInterface)->Release(interfaceInterface);
            }
            IOObjectRelease(devInterface);
        }
        IOObjectRelease(iterator);
    }
    
    // Done
    (*interface)->Release(interface);
    
    return devValid && (interfaceCount >= 3);   // Only 3 in case the security descriptor is missing?
}

@implementation DeviceLister

- init
{
    if (self = [super init])
    {
        entries = [[NSMutableDictionary dictionaryWithCapacity:10] retain];
        connected = [[NSMutableArray arrayWithCapacity:10] retain];
        enabled = [[NSMutableArray arrayWithCapacity:10] retain];
    }
    return self;
}

- (void)dealloc
{
    [entries release];
    [connected release];
    [enabled release];
    [super dealloc];
}

- (NSString*)toolPath
{
    // Find the path of our tool in our bundle - should it be in the driver's bundle?
    return [[[owner bundle] resourcePath] stringByAppendingPathComponent:TOOL_FILENAME];
}

- (OSStatus)writeToolWithAuthorisation:(AuthorizationRef)authorisationRef
{
    OSStatus result;
    NSString *toolPath;
    NSMutableArray *parameters;
    const char **argv;
    int i;
    
    toolPath = [self toolPath];
    
    // Build array of parameters
    parameters = [NSMutableArray arrayWithCapacity:10];
    [parameters addObject:@"edit"];
    
    for (NSNumber *key in enabled)
    {
        NSString *name = [entries objectForKey:key];
        NSUInteger keyValue = [key unsignedIntValue];
        UInt16 vendor = (keyValue >> 16) & 0xFFFF;
        UInt16 product = keyValue & 0xFFFF;
        [parameters addObject:name];
        [parameters addObject:[NSString stringWithFormat:@"%i", vendor]];
        [parameters addObject:[NSString stringWithFormat:@"%i", product]];
    }
    
    // Convert parameters to a C array
    argv = malloc(sizeof(char*) * ([parameters count] + 1));
    i = 0;
    for (NSString *item in parameters)
        argv[i++] = [item UTF8String];
    argv[i] = NULL;
    
    // Execute the command
    result = AuthorizationExecuteWithPrivileges(authorisationRef,
                                                [toolPath UTF8String],
                                                kAuthorizationFlagDefaults,
                                                (char**)argv,
                                                NULL);
    
    // Done
    free(argv);
    return result;
}

- (NSString*)readTool
{
    NSTask *task;
    NSPipe *pipe, *error;
    NSData *data;
    NSString *response;
    NSArray *lines;
    
    // Prepare to run the tool
    task = [[[NSTask alloc] init] autorelease];
    [task setLaunchPath:[self toolPath]];
    
    // Hook up the pipe to catch the output
    pipe = [NSPipe pipe];
    [task setStandardOutput:pipe];
    error = [NSPipe pipe];
    [task setStandardError:error];
    
    // Run the tool
    [task launch];
    [task waitUntilExit];
    
    // Check result
    if ([task terminationStatus] != 0)
    {
        data = [[error fileHandleForReading] readDataToEndOfFile];
        return [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
    }
    
    // Read the data back
    data = [[pipe fileHandleForReading] readDataToEndOfFile];
    response = [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
    
    // Parse the results
    lines = [response componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]];
    for (NSString *line in lines)
    {
        NSArray *values = [line componentsSeparatedByString:@","];
        if ([values count] != 3)
            continue;
        NSUInteger vendor = [[values objectAtIndex:1] intValue];
        NSUInteger product = [[values objectAtIndex:2] intValue];
        NSNumber *key = [NSNumber numberWithUnsignedInt:(int)((vendor << 16) | product)];
        [enabled addObject:key];
        if ([entries objectForKey:key] == nil)
            [entries setObject:SanitiseName([values objectAtIndex:0]) forKey:key];
    }
    
    return nil;
}

// Get the list of devices we've seen from the settings
- (NSString*)readKnownDevices
{
    NSDictionary *known;
    NSArray *keys;
    
    known = GetKnownDevices();
    keys = [known allKeys];
    for (NSNumber *key in keys)
    {
        if ([entries objectForKey:key] == nil)
            [entries setObject:[known objectForKey:key] forKey:key];
    }
    return nil;
}

// Find any matching devices currently plugged in
- (NSString*)readIOKit
{
    io_iterator_t iterator;
    io_service_t object;
    
    iterator = 0;
    IOServiceGetMatchingServices([owner masterPort], IOServiceMatching(kIOUSBDeviceClassName), &iterator);
    if (iterator != 0)
    {
        while ((object = IOIteratorNext(iterator)) != 0)
        {
            if (IsXBox360Controller(object))
            {
                NSNumber *vendorValue, *productValue;
                UInt16 vendor,product;
                
                vendorValue = GetDeviceValue(object, @"idVendor");
                vendor = [vendorValue intValue];
                
                productValue = GetDeviceValue(object, @"idProduct");
                product = [productValue intValue];
                
                if ((vendorValue != nil) && (productValue != nil))
                {
                    NSNumber *key;
                    
                    key = [NSNumber numberWithUnsignedInt:(vendor << 16) | product];
                    [connected addObject:key];
                    if ([entries objectForKey:key] == nil)
                    {
                        NSString *name = GetDeviceValue(object, @"USB Product Name");
                        if (name == nil)
                            name = [NSString stringWithFormat:@"Unknown_%.4x_%.4x", vendor, product];
                        else
                            name = SanitiseName(name);
                        [entries setObject:name forKey:key];
                    }
                }
            }
            IOObjectRelease(object);
        }
        IOObjectRelease(iterator);
    }
    return nil;
}

- (void)showFailure:(NSString*)message
{
    NSAlert *alert = [NSAlert alertWithMessageText:nil
                                     defaultButton:nil
                                   alternateButton:nil
                                       otherButton:nil
                         informativeTextWithFormat:@"%@", message];
    [alert runModal];
}

- (BOOL)loadDevices
{
    NSString *error;
    
    // Initialise
    error = nil;
    [entries removeAllObjects];
    [connected removeAllObjects];
    [enabled removeAllObjects];
    
    // These can be done in any order, depending on the behaviour desired
    if (error == nil)
        error = [self readKnownDevices];
    if (error == nil)
        error = [self readTool];
    if (error == nil)
        error = [self readIOKit];
    
    // Check for errors
    if (error != nil)
    {
        [self showFailure:error];
        return NO;
    }
    
    // Done
    SetKnownDevices(entries);
    [list reloadData];
    changed = NO;
    return YES;
}

// attempt to authenticate so we can edit the driver's list of supported devices as root
- (BOOL)trySave
{
    OSStatus status;
    AuthorizationRef authorisationRef;
    BOOL success = NO;
    
    status = AuthorizationCreate(NULL,
                                 kAuthorizationEmptyEnvironment,
                                 kAuthorizationFlagDefaults,
                                 &authorisationRef);
    if (status != errAuthorizationSuccess)
    {
        [self showFailure:NSLocalizedString(@"Unable to create authorisation request", @"")];
        return NO;
    }
    
    AuthorizationItem right = {kAuthorizationRightExecute, 0, NULL, 0};
    AuthorizationRights rights = {1, &right};
    status = AuthorizationCopyRights(authorisationRef,
                                     &rights,
                                     NULL,
                                     kAuthorizationFlagDefaults | kAuthorizationFlagInteractionAllowed | kAuthorizationFlagPreAuthorize | kAuthorizationFlagExtendRights,
                                     NULL);
    if (status != errAuthorizationSuccess)
    {
        [self showFailure:NSLocalizedString(@"Unable to acquire authorisation", @"")];
        goto fail;
    }
    
    status = [self writeToolWithAuthorisation:authorisationRef];
    if (status != errAuthorizationSuccess)
    {
        [self showFailure:NSLocalizedString(@"Failed to execute the driver tool", @"")];
        goto fail;
    }
    
    success = YES;
    
fail:
    AuthorizationFree(authorisationRef, kAuthorizationFlagDestroyRights);
    return success;
}

- (void)showWithOwner:(Pref360ControlPref*)pane
{
    owner = pane;
    if (![self loadDevices])
        return;
    [NSApp beginSheet:sheet
       modalForWindow:[NSApp mainWindow]
        modalDelegate:nil
       didEndSelector:nil
          contextInfo:NULL];
}

- (IBAction)done:(id)sender
{
    if (changed)
        [self trySave];
    [NSApp endSheet:sheet];
    [sheet close];
}

// NSTableView data source

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView
{
    return [entries count];
}

- (id)tableView:(NSTableView*)aTableView objectValueForTableColumn:(NSTableColumn*)aTableColumn row:(NSInteger)rowIndex
{
    NSString *identifier = [aTableColumn identifier];
    NSString *key = [[entries allKeys] objectAtIndex:rowIndex];
    if ([identifier compare:@"enable"] == NSOrderedSame)
    {
        return [NSNumber numberWithBool:[enabled containsObject:key]];
    }
    if ([identifier compare:@"name"] == NSOrderedSame)
    {
        NSColor *colour;
        
        if ([connected containsObject:key])
            colour = [NSColor blueColor];
        else
            colour = [NSColor blackColor];
        return [[[NSAttributedString alloc] initWithString:[entries objectForKey:key]
                                                attributes:[NSDictionary dictionaryWithObject:colour forKey:NSForegroundColorAttributeName]] autorelease];
    }
    return nil;
}

- (void)tableView:(NSTableView*)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn*)aTableColumn row:(NSInteger)rowIndex
{
    if ([(NSString*)[aTableColumn identifier] compare:@"enable"] == NSOrderedSame)
    {
        NSString *key = [[entries allKeys] objectAtIndex:rowIndex];
        BOOL contains = [enabled containsObject:key];
        if ([(NSNumber*)anObject boolValue])
        {
            if (!contains)
            {
                [enabled addObject:key];
                changed = YES;
            }
        }
        else
        {
            if (contains)
            {
                [enabled removeObject:key];
                changed = YES;
            }
        }

    }
}

@end
