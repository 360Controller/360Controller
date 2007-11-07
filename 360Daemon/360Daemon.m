#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#include <IOKit/usb/IOUSBLib.h>

#define CHECK_SHOWAGAIN     @"Do not show this message again"

mach_port_t masterPort;
IONotificationPortRef notifyPort;
CFRunLoopSourceRef notifySource;
io_iterator_t onIteratorWired;
io_iterator_t onIteratorWireless;
io_iterator_t onIteratorOther;

CFUserNotificationRef activeAlert = nil;
CFRunLoopSourceRef activeAlertSource;

enum {
    kaPlugNCharge = 0,
};

NSString *alertStrings[] = {
    @"You have attached a Microsoft plug'n'charge cable for your XBox 360 Wireless Controller. While this cable will allow you to charge your wireless controller, you will require the Microsoft Wireless Gaming Receiver for Windows to use your wireless controller in Mac OS X!",
};

void releaseAlert(void)
{
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), activeAlertSource, kCFRunLoopCommonModes);
    CFRelease(activeAlertSource);
    CFRelease(activeAlert);
    activeAlertSource = nil;
    activeAlert = nil;
}

void callbackAlert(CFUserNotificationRef userNotification, CFOptionFlags responseFlags)
{
    if (responseFlags & CFUserNotificationCheckBoxChecked(0))
    {
        NSLog(@"Checkbox checked!");
    }
    releaseAlert();
    NSLog(@"Alert closed by user");
}

void ShowAlert(int index)
{
    SInt32 error;
    NSArray *checkBoxes = [NSArray arrayWithObjects:CHECK_SHOWAGAIN, nil];
    NSArray *dictKeys = [NSArray arrayWithObjects:
        (NSString*)kCFUserNotificationAlertHeaderKey,
        (NSString*)kCFUserNotificationAlertMessageKey,
        (NSString*)kCFUserNotificationCheckBoxTitlesKey,
        nil];
    NSArray *dictValues = [NSArray arrayWithObjects:
        @"XBox 360 Controller Driver",
        alertStrings[index],
        checkBoxes,
        nil];
    NSDictionary *dictionary = [NSDictionary dictionaryWithObjects:dictValues forKeys:dictKeys];
    
    if (activeAlert != nil)
    {
        CFUserNotificationCancel(activeAlert);
        releaseAlert();
    }
        
    activeAlert = CFUserNotificationCreate(kCFAllocatorDefault, 0, kCFUserNotificationPlainAlertLevel, &error, (CFDictionaryRef)dictionary);
    NSLog(@"Error = %i", error);
    activeAlertSource = CFUserNotificationCreateRunLoopSource(kCFAllocatorDefault, activeAlert, callbackAlert, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), activeAlertSource, kCFRunLoopCommonModes);
}

// Supported device - connecting - set settings?
static void callbackConnected(void *param,io_iterator_t iterator)
{
    io_service_t object = 0;
    
    while ((object = IOIteratorNext(iterator)) != 0)
    {
        if (IOObjectConformsTo(object, "WirelessHIDDevice") || IOObjectConformsTo(object, "Xbox360ControllerClass"))
        {
            // Supported device - load settings
            if (IOObjectConformsTo(object, "Xbox360ControllerClass"))
                NSLog(@"Got wired controller!");
            else
                NSLog(@"Got wireless controller!");
        }
        else
        {
            CFTypeRef vendorID = IORegistryEntrySearchCFProperty(object,kIOServicePlane,CFSTR("idVendor"),kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);
            CFTypeRef productID = IORegistryEntrySearchCFProperty(object,kIOServicePlane,CFSTR("idProduct"),kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);
            if ((vendorID != NULL) && (productID != NULL))
            {
//                UInt32 idVendor = *((UInt32*)CFDataGetBytePtr(vendorID));
                UInt32 idVendor = [((NSNumber*)vendorID) unsignedIntValue];
//                UInt32 idProduct = *((UInt32*)CFDataGetBytePtr(productID));
                UInt32 idProduct = [((NSNumber*)productID) unsignedIntValue];
                if ((idVendor == 0x045e) && (idProduct == 0x028f))
                {
                    // Unsupported - plug'n'charge cable
                    ShowAlert(kaPlugNCharge);
                }
            }
            if (vendorID != NULL)
                CFRelease(vendorID);
            if (productID != NULL)
                CFRelease(productID);
        }
        IOObjectRelease(object);
    }
}

// Entry point
int main (int argc, const char * argv[]) {
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

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
        // Wireless 360 devices
    IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching("WirelessHIDDevice"), callbackConnected, NULL, &onIteratorWireless);
    callbackConnected(NULL, onIteratorWireless);
    // Run loop
    CFRunLoopRun();
    // Stop listening
    IOObjectRelease(onIteratorOther);
    IOObjectRelease(onIteratorWired);
    IOObjectRelease(onIteratorWireless);
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), notifySource, kCFRunLoopCommonModes);
    CFRunLoopSourceInvalidate(notifySource);
    IONotificationPortDestroy(notifyPort);
    // End
    [pool release];
    return 0;
}
