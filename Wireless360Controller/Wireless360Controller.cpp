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

static inline Xbox360_SShort getAbsolute(Xbox360_SShort value)
{
    Xbox360_SShort reverse;

#ifdef __LITTLE_ENDIAN__
    reverse=value;
#elif __BIG_ENDIAN__
    reverse=((value&0xFF00)>>8)|((value&0x00FF)<<8);
#else
#error Unknown CPU byte order
#endif
    return (reverse<0)?~reverse:reverse;
}

bool Wireless360Controller::init(OSDictionary* propTable)
{
    bool res = super::init(propTable);
    kprintf("Wireless360Controller - init - %d\n", res);

    // Default settings
    invertLeftX = invertLeftY = false;
    invertRightX = invertRightY = false;
    deadzoneLeft = deadzoneRight = 0;
    relativeLeft = relativeRight = false;
    readSettings();
    // Bindings
    noMapping = true;
    for (int i = 0; i < 11; i++)
    {
        mapping[i] = i;
    }
    for (int i = 12; i < 16; i++)
    {
        mapping[i - 1] = i;
    }

    // Done
    return res;
}

// Read the settings from the registry
void Wireless360Controller::readSettings(void)
{
    OSBoolean* value = nullptr;
    OSNumber* number = nullptr;
    OSDictionary* dataDictionary = OSDynamicCast(OSDictionary, getProperty(kDriverSettingKey));

    if(dataDictionary == nullptr) return;
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertLeftX"));
    if (value != nullptr) invertLeftX = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertLeftY"));
    if (value != nullptr) invertLeftY = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertRightX"));
    if (value != nullptr) invertRightX = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertRightY"));
    if (value != nullptr) invertRightY = value->getValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("DeadzoneLeft"));
    if (number != nullptr) deadzoneLeft = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("DeadzoneRight"));
    if (number != nullptr) deadzoneRight = number->unsigned32BitValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("RelativeLeft"));
    if (value != nullptr) relativeLeft = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("RelativeRight"));
    if (value != nullptr) relativeRight=value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("DeadOffLeft"));
    if (value != nullptr) deadOffLeft = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("DeadOffRight"));
    if (value != nullptr) deadOffRight = value->getValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("RumbleType"));
    if (number != nullptr) rumbleType = number->unsigned8BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingUp"));
    if (number != nullptr) mapping[0] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingDown"));
    if (number != nullptr) mapping[1] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingLeft"));
    if (number != nullptr) mapping[2] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingRight"));
    if (number != nullptr) mapping[3] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingStart"));
    if (number != nullptr) mapping[4] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingBack"));
    if (number != nullptr) mapping[5] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingLSC"));
    if (number != nullptr) mapping[6] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingRSC"));
    if (number != nullptr) mapping[7] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingLB"));
    if (number != nullptr) mapping[8] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingRB"));
    if (number != nullptr) mapping[9] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingGuide"));
    if (number != nullptr) mapping[10] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingA"));
    if (number != nullptr) mapping[11] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingB"));
    if (number != nullptr) mapping[12] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingX"));
    if (number != nullptr) mapping[13] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingY"));
    if (number != nullptr) mapping[14] = number->unsigned32BitValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("SwapSticks"));
    if (value != nullptr) swapSticks = value->getValue();

    noMapping = true;
    UInt8 normalMapping[15] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15 };
    for (int i = 0; i < 15; i++)
    {
        if (normalMapping[i] != mapping[i])
        {
            noMapping = false;
            break;
        }
    }
#if 0
    IOLog("Xbox360ControllerClass preferences loaded:\n  invertLeft X: %s, Y: %s\n   invertRight X: %s, Y:%s\n  deadzone Left: %d, Right: %d\n\n",
            invertLeftX?"True":"False",invertLeftY?"True":"False",
            invertRightX?"True":"False",invertRightY?"True":"False",
            deadzoneLeft,deadzoneRight);
#endif
}

void Wireless360Controller::normalizeAxis(SInt16& axis, short deadzone)
{
    static const UInt16 max16 = 32767;
    const float current = getAbsolute(axis);
    const float maxVal = max16 - deadzone;
    
    if (current > deadzone)
    {
        if (axis < 0)
        {
            axis = max16 * (current - deadzone) / maxVal;
            axis = ~axis;
        }
        else
        {
            axis = max16 * (current - deadzone) / maxVal;
        }
    }
    else
    {
        axis =0;
    }
}

// Adjusts the report for any settings specified by the user
void Wireless360Controller::fiddleReport(unsigned char* data, int length)
{
    XBOX360_IN_REPORT* report = (XBOX360_IN_REPORT*)data;
    if (invertLeftX) report->left.x = ~(report->left.x);
    if (!invertLeftY) report->left.y = ~(report->left.y);
    if (invertRightX) report->right.x = ~(report->right.x);
    if (!invertRightY) report->right.y = ~(report->right.y);
    
    if (deadzoneLeft != 0)
    {
        if (relativeLeft)
        {
            if ((getAbsolute(report->left.x) < deadzoneLeft) && (getAbsolute(report->left.y) < deadzoneLeft))
            {
                report->left.x = 0;
                report->left.y = 0;
            }
            else if (deadOffLeft)
            {
                normalizeAxis(report->left.x, deadzoneLeft);
                normalizeAxis(report->left.y, deadzoneLeft);
            }
        }
        else // Linked checkbox has no check
        {
            if (getAbsolute(report->left.x) < deadzoneLeft)
            {
                report->left.x = 0;
            }
            else if (deadOffLeft)
            {
                normalizeAxis(report->left.x, deadzoneLeft);
            }
            
            if (getAbsolute(report->left.y) < deadzoneLeft)
            {
                report->left.y = 0;
            }
            else if (deadOffLeft)
            {
                normalizeAxis(report->left.y, deadzoneLeft);
            }
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
            else if (deadOffRight)
            {
                normalizeAxis(report->left.x, deadzoneRight);
                normalizeAxis(report->left.y, deadzoneRight);
            }
        }
        else
        {
            if (getAbsolute(report->right.x) < deadzoneRight)
            {
                report->right.x = 0;
            }
            else if (deadOffRight)
            {
                normalizeAxis(report->right.x, deadzoneRight);
            }
            
            if (getAbsolute(report->right.y) < deadzoneRight)
            {
                report->right.y = 0;
            }
            else if (deadOffRight)
            {
                normalizeAxis(report->right.y, deadzoneRight);
            }
        }
    }
}

void Wireless360Controller::remapButtons(void* buffer)
{
    XBOX360_IN_REPORT* report360 = (XBOX360_IN_REPORT*)buffer;
    UInt16 new_buttons = 0;

    new_buttons |= ((report360->buttons & 1) == 1) << mapping[0];
    new_buttons |= ((report360->buttons & 2) == 2) << mapping[1];
    new_buttons |= ((report360->buttons & 4) == 4) << mapping[2];
    new_buttons |= ((report360->buttons & 8) == 8) << mapping[3];
    new_buttons |= ((report360->buttons & 16) == 16) << mapping[4];
    new_buttons |= ((report360->buttons & 32) == 32) << mapping[5];
    new_buttons |= ((report360->buttons & 64) == 64) << mapping[6];
    new_buttons |= ((report360->buttons & 128) == 128) << mapping[7];
    new_buttons |= ((report360->buttons & 256) == 256) << mapping[8];
    new_buttons |= ((report360->buttons & 512) == 512) << mapping[9];
    new_buttons |= ((report360->buttons & 1024) == 1024) << mapping[10];
    new_buttons |= ((report360->buttons & 4096) == 4096) << mapping[11];
    new_buttons |= ((report360->buttons & 8192) == 8192) << mapping[12];
    new_buttons |= ((report360->buttons & 16384) == 16384) << mapping[13];
    new_buttons |= ((report360->buttons & 32768) == 32768) << mapping[14];

//    IOLog("BUTTON PACKET - %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", mapping[0], mapping[1], mapping[2], mapping[3], mapping[4], mapping[5], mapping[6], mapping[7], mapping[8], mapping[9], mapping[10], mapping[11], mapping[12], mapping[13], mapping[14]);

    report360->buttons = new_buttons;
}

void Wireless360Controller::remapAxes(void* buffer)
{
    XBOX360_IN_REPORT* report360 = (XBOX360_IN_REPORT*)buffer;

    XBOX360_HAT temp = report360->left;
    report360->left = report360->right;
    report360->right = temp;
}

void Wireless360Controller::receivedHIDupdate(unsigned char* data, int length)
{
    fiddleReport(data, length);
    if (!noMapping)
    {
        remapButtons(data);
    }
    if (swapSticks)
    {
        remapAxes(data);
    }
    super::receivedHIDupdate(data, length);
}

void Wireless360Controller::SetRumbleMotors(unsigned char large, unsigned char small)
{
    unsigned char buf[] = {0x00, 0x01, 0x0f, 0xc0, 0x00, large, small, 0x00, 0x00, 0x00, 0x00, 0x00};
    WirelessDevice* device = OSDynamicCast(WirelessDevice, getProvider());

    if (device != nullptr)
    {
        device->SendPacket(buf, sizeof(buf));
    }
}

IOReturn Wireless360Controller::setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options)
{
    char data[2] = {};

    // IOLog("setReport(%p, %d, %d)\n", report, reportType, options);
    if (report->readBytes(0, data, 2) < 2)
    {
        return kIOReturnUnsupported;
    }

    // Rumble
    if (data[0] == 0x00)
    {
        if ((data[1] != report->getLength()) || (data[1] != 0x04))
        {
            return kIOReturnUnsupported;
        }
        
        report->readBytes(2, data, 2);
        SetRumbleMotors(data[0], data[1]);
        
        return kIOReturnSuccess;
    }

    return super::setReport(report, reportType, options);
}

IOReturn Wireless360Controller::newReportDescriptor(IOMemoryDescriptor** descriptor) const
{
    IOBufferMemoryDescriptor* buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(ReportDescriptor));

    if (buffer == nullptr)
    {
        return kIOReturnNoResources;
    }
    
    buffer->writeBytes(0, ReportDescriptor, sizeof(ReportDescriptor));
    *descriptor = buffer;
    
    return kIOReturnSuccess;
}

// Called by the userspace IORegistryEntrySetCFProperties function
IOReturn Wireless360Controller::setProperties(OSObject* properties)
{
    OSDictionary* dictionary = OSDynamicCast(OSDictionary, properties);

    if (dictionary != nullptr)
    {
        setProperty(kDriverSettingKey, dictionary);
        readSettings();

        return kIOReturnSuccess;
    }
    else
    {
        return kIOReturnBadArgument;
    }
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
