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
 * Xbox original controller. 
 * Convert reports to Xbox 360 controller format and fake product ids
 */

typedef struct {
    XBOX360_PACKET header;
    XBox360_Byte buttons;
    XBox360_Byte reserved1;
    XBox360_Byte a, b, x, y, black, white;
    XBox360_Byte trigL,trigR;
    XBox360_Short xL,yL;
    XBox360_Short xR,yR;
} PACKED XBOX_IN_REPORT;

typedef struct {
    XBOX360_PACKET header;
    XBox360_Byte reserved1;
    XBox360_Byte left;
    XBox360_Byte reserved2;
    XBox360_Byte right;
} PACKED XBOX_OUT_RUMBLE;


OSDefineMetaClassAndStructors(XboxOriginalControllerClass, Xbox360ControllerClass)

OSNumber* XboxOriginalControllerClass::newVendorIDNumber() const
{
    return OSNumber::withNumber(1118,16);
}

OSString* XboxOriginalControllerClass::newManufacturerString() const
{
    return OSString::withCString("Holtek");
}

OSNumber* XboxOriginalControllerClass::newProductIDNumber() const
{
    return OSNumber::withNumber(654,16);
}

OSString* XboxOriginalControllerClass::newProductString() const
{
    return OSString::withCString("Xbox 360 Wired Controller");
}

static void logData(UInt8 *data, int len) {
    for (int i = 0; i < len; i++) IOLog("%02x ", (int)data[i]);
    IOLog("\n");
}

// This converts XBox original controller report into XBox360 form
// See https://github.com/Grumbel/xboxdrv/blob/master/src/controller/xbox_controller.cpp
static void convertFromXBoxOriginal(UInt8 *data) {
    if (data[0] != 0x00 || data[1] != 0x14) {
        IOLog("Unknown report command %d, length %d\n", (int)data[0], (int)data[1]);
        return;
    }
    XBOX360_IN_REPORT report;
    Xbox360_Prepare (report, 0);
    XBOX_IN_REPORT *in = (XBOX_IN_REPORT*)data;
    XBox360_Short buttons = in->buttons;
    if (in->a) buttons |= 1 << 12; // a
    if (in->b) buttons |= 1 << 13; // b
    if (in->x) buttons |= 1 << 14; // x
    if (in->y) buttons |= 1 << 15; // y
    if (in->black) buttons |= 1 << 9; // black mapped to shoulder right
    if (in->white) buttons |= 1 << 8; // white mapped to shoulder left
    report.buttons = buttons;
    report.trigL = in->trigL;
    report.trigR = in->trigR;
    report.left.x = in->xL;
    report.left.y = in->yL;
    report.right.x = in->xR;
    report.right.y = in->yR;
    *((XBOX360_IN_REPORT *)data) = report;
}

IOReturn XboxOriginalControllerClass::handleReport(IOMemoryDescriptor * descriptor, IOHIDReportType reportType, IOOptionBits options) {
    //IOLog("%s\n", __FUNCTION__);
    UInt8 data[sizeof(XBOX360_IN_REPORT)];
    if (descriptor->getLength() >= sizeof(XBOX360_IN_REPORT)) {
        descriptor->readBytes(0, data, sizeof(XBOX360_IN_REPORT));
        const XBOX360_IN_REPORT *report=(const XBOX360_IN_REPORT*)data;
        if ((report->header.command==inReport) && (report->header.size==sizeof(XBOX360_IN_REPORT))) {
            convertFromXBoxOriginal(data);
            if (memcmp(data, lastData, sizeof(XBOX360_IN_REPORT)) == 0) {
                repeatCount ++;
                // drop triplicate reports
                if (repeatCount > 1) {
                    return kIOReturnSuccess;
                }
            } else {
                repeatCount = 0;
            }
            memcpy(lastData, data, sizeof(XBOX360_IN_REPORT));
            descriptor->writeBytes(0, data, sizeof(XBOX360_IN_REPORT));
            //if (data[2]&1) {
            //    IOLog("%s after %d ", __FUNCTION__, (int)reportType);
            //    logData(data, 20);
            //}
        } else {
            IOLog("%s %d \n", __FUNCTION__, (int)descriptor->getLength());
            IOLog("%s  %d \n", __FUNCTION__, (int)reportType);
            logData(data, (int)descriptor->getLength());
        }
        
    } else {
        descriptor->readBytes(0, data, descriptor->getLength());
        if (reportType != 0 && data[0] != 0) {
            // not a rumble report
            IOLog("%s %d \n", __FUNCTION__, (int)reportType);
            logData(data, (int)descriptor->getLength());
        }
    }
    
    IOReturn ret = Xbox360ControllerClass::handleReport(descriptor, reportType, options);
    //IOLog("%s END\n", __FUNCTION__);
    return ret;
}

IOReturn XboxOriginalControllerClass::setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    char data[2];
    
    report->readBytes(0, data, 2);
    switch(data[0]) {
        case 0x00:  // Set force feedback
            if((data[1]!=report->getLength()) || (data[1]!=0x04)) return kIOReturnUnsupported;
        {
            XBOX_OUT_RUMBLE rumble;
            Xbox360_Prepare(rumble,outRumble);
            report->readBytes(2,data,2);
            rumble.left=data[0]; // CHECKME != big, little
            rumble.right=data[1];
            GetOwner(this)->QueueWrite(&rumble,sizeof(rumble));
            // IOLog("Set rumble: big(%d) little(%d)\n", rumble.big, rumble.little);
        }
            return kIOReturnSuccess;
        case 0x01:  // Set LEDs
            if((data[1]!=report->getLength())||(data[1]!=0x03)) return kIOReturnUnsupported;
            // No leds
            return kIOReturnSuccess;
            
        default:
            IOLog("Unknown escape %d\n", data[0]);
            return kIOReturnUnsupported;
    }
}


