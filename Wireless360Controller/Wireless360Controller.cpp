/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    Wireless360Controller.cpp - main source of the standard wireless controller driver
    
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
#include <IOKit/IOLib.h>
#include "Wireless360Controller.h"
#include "../WirelessGamingReceiver/WirelessDevice.h"
#include "../360Controller/ControlStruct.h"
#include "../360Controller/xbox360hid.h"

#define kDriverSettingKey "DeviceData"

OSDefineMetaClassAndStructors(Wireless360Controller, WirelessHIDDevice)
#define super WirelessHIDDevice

static inline XBox360_SShort getAbsolute(XBox360_SShort value)
{
    XBox360_SShort reverse;
    
#ifdef __LITTLE_ENDIAN__
    reverse=value;
#elif __BIG_ENDIAN__
    reverse=((value&0xFF00)>>8)|((value&0x00FF)<<8);
#else
#error Unknown CPU byte order
#endif
    return (reverse<0)?~reverse:reverse;
}

bool Wireless360Controller::init(OSDictionary *propTable)
{
    bool res = super::init(propTable);
    
    // Default settings
    invertLeftX = invertLeftY = false;
    invertRightX = invertRightY = false;
    deadzoneLeft = deadzoneRight = 0;
    relativeLeft = relativeRight = false;
    readSettings();
    
    // Done
    return res;
}

// Read the settings from the registry
void Wireless360Controller::readSettings(void)
{
    OSBoolean *value;
    OSNumber *number;
    OSDictionary *dataDictionary = OSDynamicCast(OSDictionary, getProperty(kDriverSettingKey));
    
    if(dataDictionary==NULL) return;
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("InvertLeftX"));
    if(value!=NULL) invertLeftX=value->getValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("InvertLeftY"));
    if(value!=NULL) invertLeftY=value->getValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("InvertRightX"));
    if(value!=NULL) invertRightX=value->getValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("InvertRightY"));
    if(value!=NULL) invertRightY=value->getValue();
    number=OSDynamicCast(OSNumber,dataDictionary->getObject("DeadzoneLeft"));
    if(number!=NULL) deadzoneLeft=number->unsigned32BitValue();
    number=OSDynamicCast(OSNumber,dataDictionary->getObject("DeadzoneRight"));
    if(number!=NULL) deadzoneRight=number->unsigned32BitValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("RelativeLeft"));
    if(value!=NULL) relativeLeft=value->getValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("RelativeRight"));
    if(value!=NULL) relativeRight=value->getValue();
#if 0
    IOLog("Xbox360ControllerClass preferences loaded:\n  invertLeft X: %s, Y: %s\n   invertRight X: %s, Y:%s\n  deadzone Left: %d, Right: %d\n\n",
            invertLeftX?"True":"False",invertLeftY?"True":"False",
            invertRightX?"True":"False",invertRightY?"True":"False",
            deadzoneLeft,deadzoneRight);
#endif
}

// Adjusts the report for any settings specified by the user
void Wireless360Controller::fiddleReport(unsigned char *data, int length)
{
    XBOX360_IN_REPORT *report = (XBOX360_IN_REPORT*)data;
    
    if (invertLeftX)
        report->left.x = ~report->left.x;
    if (!invertLeftY)
        report->left.y = ~report->left.y;
    if (invertRightX)
        report->right.x = ~report->right.x;
    if (!invertRightY)
        report->right.y = ~report->right.y;
        
    if (deadzoneLeft != 0)
    {
        if (relativeLeft)
        {
            if ((getAbsolute(report->left.x) < deadzoneLeft) && (getAbsolute(report->left.y) < deadzoneLeft))
            {
                report->left.x = 0;
                report->left.y = 0;
            }
        }
        else
        {
            if (getAbsolute(report->left.x) < deadzoneLeft)
                report->left.x = 0;
            if (getAbsolute(report->left.y) < deadzoneLeft)
                report->left.y = 0;
        }
    }
    if (deadzoneRight != 0)
    {
        if (relativeRight)
        {
            if ((getAbsolute(report->right.x) < deadzoneRight) && (getAbsolute(report->right.y) < deadzoneRight))
            {
                report->right.x = 0;
                report->right.y = 0;
            }
        }
        else
        {
            if (getAbsolute(report->right.x) < deadzoneRight)
                report->right.x = 0;
            if (getAbsolute(report->right.y) < deadzoneRight)
                report->right.y = 0;
        }
    }
}

void Wireless360Controller::receivedHIDupdate(unsigned char *data, int length)
{
    fiddleReport(data, length);
    super::receivedHIDupdate(data, length);
}

void Wireless360Controller::SetRumbleMotors(unsigned char large, unsigned char small)
{
    unsigned char buf[] = {0x00, 0x01, 0x0f, 0xc0, 0x00, large, small, 0x00, 0x00, 0x00, 0x00, 0x00};
    WirelessDevice *device = OSDynamicCast(WirelessDevice, getProvider());
    
    if (device != NULL)
        device->SendPacket(buf, sizeof(buf));
}

IOReturn Wireless360Controller::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options)
{
    char data[2];
    
    // IOLog("setReport(%p, %d, %d)\n", report, reportType, options);
    if (report->readBytes(0, data, 2) < 2)
        return kIOReturnUnsupported;
        
    // Rumble
    if (data[0] == 0x00)
    {
        if ((data[1] != report->getLength()) || (data[1] != 0x04))
            return kIOReturnUnsupported;
        report->readBytes(2, data, 2);
        SetRumbleMotors(data[0], data[1]);
        return kIOReturnSuccess;
    }
    
    return super::setReport(report, reportType, options);
}

IOReturn Wireless360Controller::newReportDescriptor(IOMemoryDescriptor ** descriptor ) const
{
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(ReportDescriptor));
    
    if (buffer == NULL)
        return kIOReturnNoResources;
    buffer->writeBytes(0, ReportDescriptor, sizeof(ReportDescriptor));
    *descriptor = buffer;
    return kIOReturnSuccess;
}

// Called by the userspace IORegistryEntrySetCFProperties function
IOReturn Wireless360Controller::setProperties(OSObject *properties)
{
    OSDictionary *dictionary = OSDynamicCast(OSDictionary,properties);
    
    if(dictionary!=NULL) {
        setProperty(kDriverSettingKey,dictionary);
        readSettings();
        return kIOReturnSuccess;
    } else return kIOReturnBadArgument;
}

// Get info

OSString* Wireless360Controller::newManufacturerString() const
{
    return OSString::withCString("Microsoft");
}

OSNumber* Wireless360Controller::newPrimaryUsageNumber() const
{
    // Gamepad
    return OSNumber::withNumber(0x05, 8);
}

OSNumber* Wireless360Controller::newPrimaryUsagePageNumber() const
{
    // Generic Desktop
    return OSNumber::withNumber(0x01, 8);
}

OSNumber* Wireless360Controller::newProductIDNumber() const
{
    return OSNumber::withNumber((unsigned long long)0x28e, 16);
}

OSString* Wireless360Controller::newProductString() const
{
    return OSString::withCString("Wireless 360 Controller");
}

OSString* Wireless360Controller::newTransportString() const
{
    return OSString::withCString("Wireless");
}

OSNumber* Wireless360Controller::newVendorIDNumber() const
{
    return OSNumber::withNumber((unsigned long long)0x45e, 16);
}
