/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro
 
 Controller.cpp - driver class for the 360 controller
 
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

#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>
#include "Controller.h"
#include "ControlStruct.h"
namespace HID_360 {
#include "xbox360hid.h"
}
#include "_60Controller.h"

OSDefineMetaClassAndStructors(Xbox360ControllerClass, IOHIDDevice)

static Xbox360Peripheral* GetOwner(IOService *us)
{
	IOService *prov = us->getProvider();
    
	if (prov == NULL)
		return NULL;
	return OSDynamicCast(Xbox360Peripheral, prov);
}

static IOUSBDevice* GetOwnerProvider(const IOService *us)
{
	IOService *prov = us->getProvider(), *provprov;
	
	if (prov == NULL)
		return NULL;
	provprov = prov->getProvider();
	if (provprov == NULL)
		return NULL;
	return OSDynamicCast(IOUSBDevice, provprov);
}

bool Xbox360ControllerClass::start(IOService *provider)
{
    if (OSDynamicCast(Xbox360Peripheral, provider) == NULL)
        return false;
    return IOHIDDevice::start(provider);
}

IOReturn Xbox360ControllerClass::setProperties(OSObject *properties)
{
	Xbox360Peripheral *owner = GetOwner(this);
	if (owner == NULL)
		return kIOReturnUnsupported;
	return owner->setProperties(properties);
}

// Returns the HID descriptor for this device
IOReturn Xbox360ControllerClass::newReportDescriptor(IOMemoryDescriptor **descriptor) const
{
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task,0,sizeof(HID_360::ReportDescriptor));
    
    if (buffer == NULL) return kIOReturnNoResources;
    buffer->writeBytes(0,HID_360::ReportDescriptor,sizeof(HID_360::ReportDescriptor));
    *descriptor=buffer;
    return kIOReturnSuccess;
}

// Handles a message from the userspace IOHIDDeviceInterface122::setReport function
IOReturn Xbox360ControllerClass::setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    char data[2];
    
    report->readBytes(0, data, 2);
    switch(data[0]) {
        case 0x00:  // Set force feedback
            if((data[1]!=report->getLength()) || (data[1]!=0x04)) return kIOReturnUnsupported;
		{
			XBOX360_OUT_RUMBLE rumble;
			
			Xbox360_Prepare(rumble,outRumble);
			report->readBytes(2,data,2);
			rumble.big=data[0];
			rumble.little=data[1];
			GetOwner(this)->QueueWrite(&rumble,sizeof(rumble));
			// IOLog("Set rumble: big(%d) little(%d)\n", rumble.big, rumble.little);
		}
            return kIOReturnSuccess;
        case 0x01:  // Set LEDs
            if((data[1]!=report->getLength())||(data[1]!=0x03)) return kIOReturnUnsupported;
		{
			XBOX360_OUT_LED led;
			
			report->readBytes(2,data,1);
			Xbox360_Prepare(led,outLed);
			led.pattern=data[0];
			GetOwner(this)->QueueWrite(&led,sizeof(led));
			// IOLog("Set LED: %d\n", led.pattern);
		}
            return kIOReturnSuccess;
        default:
			IOLog("Unknown escape %d\n", data[0]);
            return kIOReturnUnsupported;
    }
}

// Get report
IOReturn Xbox360ControllerClass::getReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    // Doesn't do anything yet ;)
    return kIOReturnUnsupported;
}

IOReturn Xbox360ControllerClass::handleReport(IOMemoryDescriptor * descriptor, IOHIDReportType reportType, IOOptionBits options) {
    if (descriptor->getLength() >= sizeof(XBOX360_IN_REPORT)) {
        IOBufferMemoryDescriptor *desc = OSDynamicCast(IOBufferMemoryDescriptor, descriptor);
        if (desc != NULL) {
            const XBOX360_IN_REPORT *report=(const XBOX360_IN_REPORT*)desc->getBytesNoCopy();
            if ((report->header.command==inReport) && (report->header.size==sizeof(XBOX360_IN_REPORT))) {
                GetOwner(this)->fiddleReport(desc);
            }
        }
    }
    IOReturn ret = IOHIDDevice::handleReport(descriptor, reportType, options);
    return ret;
}


// Returns the string for the specified index from the USB device's string list, with an optional default
OSString* Xbox360ControllerClass::getDeviceString(UInt8 index,const char *def) const
{
    IOReturn err;
    char buf[1024];
    const char *string;
    
    err = GetOwnerProvider(this)->GetStringDescriptor(index, buf, sizeof(buf));
    if(err==kIOReturnSuccess) string=buf;
    else {
        if(def == NULL) string = "Unknown";
        else string = def;
    }
    return OSString::withCString(string);
}

OSString* Xbox360ControllerClass::newManufacturerString() const
{
    return getDeviceString(GetOwnerProvider(this)->GetManufacturerStringIndex());
}

OSNumber* Xbox360ControllerClass::newPrimaryUsageNumber() const
{
    return OSNumber::withNumber(HID_360::ReportDescriptor[3], 8);
}

OSNumber* Xbox360ControllerClass::newPrimaryUsagePageNumber() const
{
    return OSNumber::withNumber(HID_360::ReportDescriptor[1], 8);
}

OSNumber* Xbox360ControllerClass::newProductIDNumber() const
{
    return OSNumber::withNumber(GetOwnerProvider(this)->GetProductID(),16);
}

OSString* Xbox360ControllerClass::newProductString() const
{
    OSString *retString = getDeviceString(GetOwnerProvider(this)->GetProductStringIndex());
    if (retString->isEqualTo("Controller")) {
        retString->release();
        return OSString::withCString("Xbox 360 Wired Controller");
    } else {
        return retString;
    }
}

OSString* Xbox360ControllerClass::newSerialNumberString() const
{
    return getDeviceString(GetOwnerProvider(this)->GetSerialNumberStringIndex());
}

OSString* Xbox360ControllerClass::newTransportString() const
{
    return OSString::withCString("USB");
}

OSNumber* Xbox360ControllerClass::newVendorIDNumber() const
{
    return OSNumber::withNumber(GetOwnerProvider(this)->GetVendorID(),16);
}

OSNumber* Xbox360ControllerClass::newLocationIDNumber() const
{
	IOUSBDevice *device;
    OSNumber *number;
    UInt32 location = 0;
    
	device = GetOwnerProvider(this);
    if (device)
    {
        if ((number = OSDynamicCast(OSNumber, device->getProperty("locationID"))))
        {
            location = number->unsigned32BitValue();
        }
        else
        {
            // Make up an address
            if ((number = OSDynamicCast(OSNumber, device->getProperty("USB Address"))))
                location |= number->unsigned8BitValue() << 24;
			
            if ((number = OSDynamicCast(OSNumber, device->getProperty("idProduct"))))
                location |= number->unsigned8BitValue() << 16;
        }
    }
    
    return (location != 0) ? OSNumber::withNumber(location, 32) : 0;
}

/*
 * Xbox One controller
 * Convert reports to Xbox 360 controller format and fake product ids
 */

typedef struct {
    UInt8 command;
    UInt8 reserved1;
    UInt8 counter;
    UInt8 size;
} PACKED XBOXONE_HEADER;

typedef struct {
    XBOXONE_HEADER header;
    UInt16 buttons;
    UInt16 trigL, trigR;
    XBOX360_HAT left, right;
} PACKED XBOXONE_IN_REPORT;

typedef struct {
    XBOXONE_HEADER header;
    UInt8 state;
    UInt8 dummy;
} PACKED XBOXONE_IN_GUIDE_REPORT;

typedef struct {
    XBOXONE_HEADER header;
    UInt8 reserved1;
    UInt8 rumbleStyle;
    UInt8 trigL, trigR;
    UInt8 little, big;
    UInt8 length; // Length of time to rumble
    UInt8 reserved[2];
} PACKED XBOXONE_OUT_RUMBLE;

OSDefineMetaClassAndStructors(XboxOneControllerClass, Xbox360ControllerClass)

OSNumber* XboxOneControllerClass::newVendorIDNumber() const
{
    return OSNumber::withNumber(1118,16);
}

OSString* XboxOneControllerClass::newManufacturerString() const
{
    return OSString::withCString("Microsoft");
}

OSNumber* XboxOneControllerClass::newProductIDNumber() const
{
    return OSNumber::withNumber(654,16);
}

OSString* XboxOneControllerClass::newProductString() const
{
    return OSString::withCString("Xbox One Wired Controller");
}

// This converts Xbox One controller report into Xbox360 form
void XboxOneControllerClass::convertFromXboxOne(void *buffer, void* override) {
    
//    if (data[0] != 0x00 || data[1] != 0x14) {
//        IOLog("Unknown report command %d, length %d\n", (int)data[0], (int)data[1]);
//        return;
//    }
    
    XBOX360_IN_REPORT *report360 = (XBOX360_IN_REPORT*)buffer;
    UInt8 trigL = 0, trigR = 0;
    UInt16 new_buttons = 0;
    XBOX360_HAT left, right;
    
    if (override == NULL) {
        XBOXONE_IN_REPORT *reportXone = (XBOXONE_IN_REPORT*)buffer;
        report360->header.command = reportXone->header.command - 0x20; // Change 0x20 into 0x00
        report360->header.size = reportXone->header.size + 0x06; // Change 0x0E into 0x14
        
        new_buttons |= ((reportXone->buttons & 4) == 4) << 4;
        new_buttons |= ((reportXone->buttons & 8) == 8) << 5;
        new_buttons |= ((reportXone->buttons & 16) == 16) << 12;
        new_buttons |= ((reportXone->buttons & 32) == 32) << 13;
        new_buttons |= ((reportXone->buttons & 64) == 64) << 14;
        new_buttons |= ((reportXone->buttons & 128) == 128) << 15;
        new_buttons |= ((reportXone->buttons & 256) == 256) << 0;
        new_buttons |= ((reportXone->buttons & 512) == 512) << 1;
        new_buttons |= ((reportXone->buttons & 1024) == 1024) << 2;
        new_buttons |= ((reportXone->buttons & 2048) == 2048) << 3;
        new_buttons |= ((reportXone->buttons & 4096) == 4096) << 8;
        new_buttons |= ((reportXone->buttons & 8192) == 8192) << 9;
        new_buttons |= ((reportXone->buttons & 16384) == 16384) << 6;
        new_buttons |= ((reportXone->buttons & 32768) == 32768) << 7;
        new_buttons |= (isXboxOneGuideButtonPressed) << 10;
        trigL = (reportXone->trigL / 1023.0) * 255;
        trigR = (reportXone->trigR / 1023.0) * 255;
        left = reportXone->left;
        right = reportXone->right;
        
        report360->buttons = new_buttons;
        report360->trigL = trigL;
        report360->trigR = trigR;
        report360->left = left;
        report360->right = right;
    } else {
        XBOX360_IN_REPORT *reportOverride = (XBOX360_IN_REPORT*)override;
        report360->header = reportOverride->header;
        report360->buttons = reportOverride->buttons;
        report360->buttons |= (isXboxOneGuideButtonPressed) << 10;
        report360->trigL = reportOverride->trigL;
        report360->trigR = reportOverride->trigR;
        report360->left = reportOverride->left;
        report360->right = reportOverride->right;
    }
}

IOReturn XboxOneControllerClass::handleReport(IOMemoryDescriptor * descriptor, IOHIDReportType reportType, IOOptionBits options) {
    UInt8 data[sizeof(XBOXONE_IN_REPORT)];
    descriptor->readBytes(0, data, sizeof(XBOXONE_IN_REPORT));
    const XBOXONE_IN_REPORT *report=(const XBOXONE_IN_REPORT*)data;
    if ((report->header.command==0x20) && report->header.size==(sizeof(XBOXONE_IN_REPORT)-4)) {
        convertFromXboxOne(data, NULL);
        memcpy(lastData, data, sizeof(XBOX360_IN_REPORT));
        descriptor->writeBytes(0, data, sizeof(XBOX360_IN_REPORT));
    }
    else if (report->header.command==0x07 && report->header.size==(sizeof(XBOXONE_IN_GUIDE_REPORT)-4))
    {
        const XBOXONE_IN_GUIDE_REPORT *guideReport=(const XBOXONE_IN_GUIDE_REPORT*)report;
        isXboxOneGuideButtonPressed = (bool)guideReport->state;
        convertFromXboxOne(data, lastData);
        descriptor->writeBytes(0, data, sizeof(XBOX360_IN_REPORT));
    }
    
    IOReturn ret = Xbox360ControllerClass::handleReport(descriptor, reportType, options);
    return ret;
}

IOReturn XboxOneControllerClass::setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    XBOXONE_HEADER header;
    report->readBytes(0, &header, 4);
    switch(header.command)
    {
        case 0x00:  // Set force feedback
            IOLog("Rumble from 0x00");
        case 0x09:  // Set force feedback
            if ((header.size != report->getLength()) || (header.size != 0x04))
                return kIOReturnUnsupported;
            else
            {
                XBOXONE_OUT_RUMBLE rumble;
                Xbox360_Prepare(rumble, 0x09)
                rumble.header.counter = 0x03;
                rumble.rumbleStyle = 0x0F;
                rumble.trigL = 0x00;
                rumble.trigR = 0x00;
                rumble.little = 0x1D;
                rumble.big = 0x1D;
                rumble.length = 0xFF;
                GetOwner(this)->QueueWrite(&rumble,sizeof(rumble));
            }
            return kIOReturnSuccess;
        default:
            IOLog("Unknown escape %d\n", header.command);
            return kIOReturnUnsupported;
    }
}
