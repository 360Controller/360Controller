/*
 * Copyright (C) 2019 Medusalix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "WirelessOneController.h"
#include "WirelessOneDongle.h"
#include "../360Controller/xbox360hid.h"
#include "../360Controller/ControlStruct.h"
#include "../360Controller/_60Controller.h"

#define LOG(format, ...) IOLog("WirelessOne (Controller): %s - " format "\n", __func__, ##__VA_ARGS__)

OSDefineMetaClassAndStructors(WirelessOneController, IOHIDDevice)

bool WirelessOneController::handleStart(IOService *provider)
{
    if (!IOHIDDevice::handleStart(provider))
    {
        LOG("super failed");
        
        return false;
    }
    
    dongle = OSDynamicCast(WirelessOneDongle, provider);
    
    if (!dongle)
    {
        LOG("invalid provider");
        
        return false;
    }
    
    LOG("serial number: %s", serialNumber);
    
    return true;
}

IOReturn WirelessOneController::setProperties(OSObject *properties)
{
    OSDictionary *dictionary = OSDynamicCast(OSDictionary, properties);
    
    if (!dictionary)
    {
        return kIOReturnBadArgument;
    }
    
    // Set 'Xbox One' controller type
    if (!dictionary->setObject(
        OSString::withCString("ControllerType"),
        OSNumber::withNumber(2, 8)
    )) {
        LOG("failed to set controller type");
        
        return kIOReturnError;
    }
    
    if (!setProperty("DeviceData", dictionary))
    {
        LOG("failed to set device data");
        
        return kIOReturnError;
    }
    
    OSNumber *number = OSDynamicCast(OSNumber, dictionary->getObject("RumbleType"));
    
    if (number)
    {
        rumbleType = number->unsigned8BitValue();
    }
    
    return kIOReturnSuccess;
}

IOReturn WirelessOneController::setReport(
    IOMemoryDescriptor *report,
    IOHIDReportType reportType,
    IOOptionBits options
) {
    uint8_t command = 0;
    
    if (!report->readBytes(0, &command, 1))
    {
        return kIOReturnUnsupported;
    }
    
    if (command == 0x00)
    {
        uint8_t data[2];
        
        if (report->readBytes(2, data, 2) < 2)
        {
            return kIOReturnUnsupported;
        }
        
        // Rumble is disabled
        if (rumbleType == 0)
        {
            return kIOReturnSuccess;
        }
        
        RumbleData rumble = {};
        
        // Type always stays the same on Windows
        // The first two bits control the intensity
        rumble.type = 0x0f;
        rumble.time = 0xff;
        
        // Main motors
        if (rumbleType == 0)
        {
            rumble.left = data[0];
            rumble.right = data[1];
        }
        
        // Trigger motors
        else if (rumbleType == 2)
        {
            rumble.triggerLeft = data[0] / 2;
            rumble.triggerRight = data[1] / 2;
        }
        
        // All motors
        else if (rumbleType == 3)
        {
            rumble.triggerLeft = data[0] / 2;
            rumble.triggerRight = data[1] / 2;
            rumble.left = data[0];
            rumble.right = data[1];
        }
        
        if (!dongle->rumble(macAddress, rumble))
        {
            return kIOReturnError;
        }
        
        return kIOReturnSuccess;
    }
    
    if (command == 0x02)
    {
        // LEDs are only supported on 360 controllers
        return kIOReturnUnsupported;
    }
    
    if (command == 0x03)
    {
        // Power off the controller remotely
        if (!dongle->powerOff(macAddress))
        {
            return kIOReturnError;
        }
        
        return kIOReturnSuccess;
    }
    
    return IOHIDDevice::setReport(report, reportType, options);
}

void WirelessOneController::handleInput(uint8_t data[])
{
    InputData *input = (InputData*)data;
    XBOX360_IN_REPORT report = {};

    report.buttons |= input->buttons.dpadUp << 0;
    report.buttons |= input->buttons.dpadDown << 1;
    report.buttons |= input->buttons.dpadLeft << 2;
    report.buttons |= input->buttons.dpadRight << 3;
    report.buttons |= input->buttons.start << 4;
    report.buttons |= input->buttons.back << 5;
    report.buttons |= input->buttons.stickLeft << 6;
    report.buttons |= input->buttons.stickRight << 7;
    report.buttons |= input->buttons.bumperLeft << 8;
    report.buttons |= input->buttons.bumperRight << 9;
    report.buttons |= input->buttons.a << 12;
    report.buttons |= input->buttons.b << 13;
    report.buttons |= input->buttons.x << 14;
    report.buttons |= input->buttons.y << 15;
    
    report.trigL = input->triggerLeft / 1023.0 * 255;
    report.trigR = input->triggerRight / 1023.0 * 255;
    report.left.x = input->stickLeftX;
    report.left.y = ~input->stickLeftY;
    report.right.x = input->stickRightX;
    report.right.y = ~input->stickRightY;
    
    sendReport(report);
}

void WirelessOneController::handleGuideButton(uint8_t data[])
{
    bool pressed = data[0];
    XBOX360_IN_REPORT report = {};
    
    report.buttons = pressed << 10;
    
    sendReport(report);
}

void WirelessOneController::sendReport(XBOX360_IN_REPORT report)
{
    report.header.command = inReport;
    report.header.size = sizeof(report);
    
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::withBytes(
        &report,
        sizeof(report),
        kIODirectionOut
    );

    if (!buffer)
    {
        LOG("failed to allocate buffer");
        
        return;
    }
    
    handleReport(buffer);
    
    buffer->release();
}

IOReturn WirelessOneController::newReportDescriptor(IOMemoryDescriptor **descriptor) const
{
    *descriptor = IOBufferMemoryDescriptor::withBytes(
        ReportDescriptor,
        sizeof(ReportDescriptor),
        kIODirectionOut
    );
    
    if (!*descriptor)
    {
        LOG("failed to allocate buffer");
        
        return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

OSString* WirelessOneController::newTransportString() const
{
    return OSString::withCString("Wireless");
}

OSString* WirelessOneController::newManufacturerString() const
{
    return OSString::withCString("Microsoft");
}

OSString* WirelessOneController::newProductString() const
{
    return OSString::withCString("Wireless One Controller");
}

OSNumber* WirelessOneController::newVendorIDNumber() const
{
    return OSNumber::withNumber(0x045e, 16);
}

OSNumber* WirelessOneController::newProductIDNumber() const
{
    return OSNumber::withNumber(0x02ea, 16);
}

OSNumber* WirelessOneController::newPrimaryUsageNumber() const
{
    return OSNumber::withNumber(kHIDUsage_GD_GamePad, 8);
}

OSNumber* WirelessOneController::newPrimaryUsagePageNumber() const
{
    return OSNumber::withNumber(kHIDPage_GenericDesktop, 8);
}

OSString* WirelessOneController::newSerialNumberString() const
{
    return OSString::withCString(serialNumber);
}

OSNumber* WirelessOneController::newLocationIDNumber() const
{
    return OSNumber::withNumber(dongle->generateLocationId(), 32);
}
