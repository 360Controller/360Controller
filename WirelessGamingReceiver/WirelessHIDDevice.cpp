/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    WirelessHIDDevice.cpp - generic wireless 360 device driver with HID support
    
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
#include <IOKit/IOTimerEventSource.h>
#include "WirelessHIDDevice.h"
#include "WirelessDevice.h"
#include "devices.h"

#define POWEROFF_TIMEOUT (15 * 60)

OSDefineMetaClassAndAbstractStructors(WirelessHIDDevice, IOHIDDevice)
#define super IOHIDDevice

// Some sort of message to send
const char weirdStart[] = {0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void WirelessHIDDevice::ChatPadTimerActionWrapper(OSObject *owner, IOTimerEventSource *sender)
{
	WirelessHIDDevice *device = OSDynamicCast(WirelessHIDDevice, owner);
    
    // Automatic shutoff
    device->serialTimerCount++;
    if (device->serialTimerCount > POWEROFF_TIMEOUT)
        device->PowerOff();
    // Reset
    sender->setTimeoutMS(1000);
}

// Sets the LED with the same format as the wired controller
void WirelessHIDDevice::SetLEDs(int mode)
{
    unsigned char buf[] = {0x00, 0x00, 0x08, (unsigned char)(0x40 + (mode % 0x0e)), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    WirelessDevice *device = OSDynamicCast(WirelessDevice, getProvider());

    if (device != NULL)
    {
        device->SendPacket(buf, sizeof(buf));
        device->SendPacket(weirdStart, sizeof(weirdStart));
    }
}

// Returns the battery level
unsigned char WirelessHIDDevice::GetBatteryLevel(void)
{
    return battery;
}

void WirelessHIDDevice::PowerOff(void)
{
    static const unsigned char buf[] = {0x00, 0x00, 0x08, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    WirelessDevice *device = OSDynamicCast(WirelessDevice, getProvider());
    
    if (device != NULL)
    {
        device->SendPacket(buf, sizeof(buf));
        // device->SendPacket(weirdStart, sizeof(weirdStart));
    }
}

// Called from userspace to do something, like set the LEDs
IOReturn WirelessHIDDevice::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options)
{
    char data[2];

    if (report->readBytes(0, data, 2) < 2)
        return kIOReturnUnsupported;
    
    switch (data[0]) {
        case 0x01:  // LED
            if ((data[1] != report->getLength()) || (data[1] != 0x03))
                return kIOReturnUnsupported;
            report->readBytes(2, data, 1);
            SetLEDs(data[0]);
            return kIOReturnSuccess;
        case 0x02:  // Power
            PowerOff();
            return kIOReturnSuccess;
        default:
            return super::setReport(report, reportType, options);
    }
}

// Start up the driver
bool WirelessHIDDevice::handleStart(IOService *provider)
{
    WirelessDevice *device;
    IOWorkLoop *workloop;
    
    if (!super::handleStart(provider))
        goto fail;

    device = OSDynamicCast(WirelessDevice, provider);
    if (device == NULL)
        goto fail;
    
    serialTimerCount = 0;
    
	serialTimer = IOTimerEventSource::timerEventSource(this, ChatPadTimerActionWrapper);
	if (serialTimer == NULL)
	{
		IOLog("start - failed to create timer for chatpad\n");
		goto fail;
	}
    workloop = getWorkLoop();
	if ((workloop == NULL) || (workloop->addEventSource(serialTimer) != kIOReturnSuccess))
	{
		IOLog("start - failed to connect timer for chatpad\n");
		goto fail;
	}
    
    device->RegisterWatcher(this, _receivedData, NULL);
    
    device->SendPacket(weirdStart, sizeof(weirdStart));

    serialTimer->setTimeoutMS(1000);

    return true;
    
fail:
    return false;
}

// Shut down the driver
void WirelessHIDDevice::handleStop(IOService *provider)
{
    WirelessDevice *device = OSDynamicCast(WirelessDevice, provider);

    if (device != NULL)
        device->RegisterWatcher(NULL, NULL, NULL);

    if (serialTimer != NULL) {
        serialTimer->cancelTimeout();
        IOWorkLoop *workloop = getWorkLoop();
        if (workloop != NULL)
            workloop->removeEventSource(serialTimer);
        serialTimer->release();
        serialTimer = NULL;
    }
    
    super::handleStop(provider);
}

// Handle new data from the device
void WirelessHIDDevice::receivedData(void)
{
    IOMemoryDescriptor *data;
    WirelessDevice *device = OSDynamicCast(WirelessDevice, getProvider());
    if (device == NULL)
        return;
    
    while ((data = device->NextPacket()) != NULL)
    {
        receivedMessage(data);
        data->release();
    }
}

const char *HexData = "0123456789ABCDEF";

// Process new data
void WirelessHIDDevice::receivedMessage(IOMemoryDescriptor *data)
{
    unsigned char buf[29];
    
    if (data->getLength() != 29)
        return;
        
    data->readBytes(0, buf, 29);
    
    switch (buf[1])
    {
        case 0x0f:  // Initial info
            if (buf[16] == 0x13)
                receivedUpdate(0x13, buf + 17);
            serialString[0] = HexData[(buf[0x0A] & 0xF0) >> 4];
            serialString[1] = HexData[buf[0x0A] & 0x0F];
            serialString[2] = HexData[(buf[0x0B] & 0xF0) >> 4];
            serialString[3] = HexData[buf[0x0B] & 0x0F];
            serialString[4] = HexData[(buf[0x0C] & 0xF0) >> 4];
            serialString[5] = HexData[buf[0x0C] & 0x0F];
            serialString[6] = HexData[(buf[0x0D] & 0xF0) >> 4];
            serialString[7] = HexData[buf[0x0D] & 0x0F];
            serialString[8] = '\0';
            IOLog("Got serial number: %s", serialString);
            break;
            
        case 0x01:  // HID info update
            if (buf[3] == 0xf0)
                receivedHIDupdate(buf + 4, buf[5]);
            break;
            
        case 0x00:  // Info update
            receivedUpdate(buf[3], buf + 4);
            break;
            
        default:
            break;
    }
}

// Received an update of a specific value
void WirelessHIDDevice::receivedUpdate(unsigned char type, unsigned char *data)
{
    switch (type)
    {
        case 0x13:  // Battery level
            battery = data[0];
            {
                OSObject *prop = OSNumber::withNumber(battery, 8);
                if (prop != NULL)
                {
                    setProperty(kIOWirelessBatteryLevel, prop);
                    prop->release();
                }
            }
            break;
            
        default:
            break;
    }
}

// Received a normal HID update from the device
void WirelessHIDDevice::receivedHIDupdate(unsigned char *data, int length)
{
    IOReturn err;
    IOMemoryDescriptor *report;
    
    serialTimerCount = 0;
    report = IOMemoryDescriptor::withAddress(data, length, kIODirectionNone);
    err = handleReport(report);
    report->release();
    if (err != kIOReturnSuccess)
        IOLog("handleReport return: 0x%.8x\n", err);
}

// Wrapper for notification of receiving data
void WirelessHIDDevice::_receivedData(void *target, WirelessDevice *sender, void *parameter)
{
    ((WirelessHIDDevice*)target)->receivedData();
}

// Get a location ID for this device, as some games require it
OSNumber* WirelessHIDDevice::newLocationIDNumber() const
{
    WirelessDevice *device;

    device = OSDynamicCast(WirelessDevice, getProvider());
    if (device == NULL)
        return NULL;
    return device->newLocationIDNumber();
}

// Get the serial number of the device
OSString* WirelessHIDDevice::newSerialNumberString() const
{
    return OSString::withCString(serialString);
}
