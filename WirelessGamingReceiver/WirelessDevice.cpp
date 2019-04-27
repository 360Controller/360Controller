/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro

    WirelessDevice.cpp - generic Wireless 360 device driver

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
// TODO(Drew): Change devices over to IOUSBHostDevice
// TODO(Drew): Update the bundle libraries in the plist
#include "WirelessDevice.h"
#include "WirelessGamingReceiver.h"

OSDefineMetaClassAndStructors(WirelessDevice, IOService)
#define super IOService

// Initialise wireless device
bool WirelessDevice::init(OSDictionary* dictionary)
{
    if (!super::init(dictionary))
    {
        return false;
    }

    index = -1;
    function = nullptr;

    return true;
}

// Checks if there's any data for us
bool WirelessDevice::IsDataAvailable(void)
{
    if (index == -1)
    {
        return false;
    }

    WirelessGamingReceiver* receiver = OSDynamicCast(WirelessGamingReceiver, getProvider());
    if (receiver == nullptr)
    {
        return false;
    }

    return receiver->IsDataQueued(index);
}

// Gets the next item from our buffer
IOMemoryDescriptor* WirelessDevice::NextPacket(void)
{
    if (index == -1)
    {
        return nullptr;
    }

    WirelessGamingReceiver* receiver = OSDynamicCast(WirelessGamingReceiver, getProvider());
    if (receiver == nullptr)
    {
        return nullptr;
    }

    return receiver->ReadBuffer(index);
}

// Sends a buffer for this controller
void WirelessDevice::SendPacket(const void* data, size_t length)
{
    if (index == -1)
    {
        return;
    }

    WirelessGamingReceiver* receiver = OSDynamicCast(WirelessGamingReceiver, getProvider());
    if (receiver == nullptr)
    {
        return;
    }

    receiver->QueueWrite(index, data, (UInt32)length);
}

// Registers a callback function
void WirelessDevice::RegisterWatcher(void* target, WirelessDeviceWatcher function, void* parameter)
{
    this->target = target;
    this->parameter = parameter;
    this->function = function;

    if ((function != nullptr) && IsDataAvailable())
    {
        NewData();
    }
}

// For internal use, sets this instances index on the wireless gaming receiver
void WirelessDevice::SetIndex(int i)
{
    index = i;
}

// Called when new data arrives
void WirelessDevice::NewData(void)
{
    if (function != nullptr)
    {
        function(target, this, parameter);
    }
}

// Gets the location ID for this device
OSNumber* WirelessDevice::newLocationIDNumber() const
{
    OSNumber* owner = nullptr;
    UInt32 location = 0;

    if (index == -1)
    {
        return nullptr;
    }

    WirelessGamingReceiver* receiver = OSDynamicCast(WirelessGamingReceiver, getProvider());
    if (receiver == nullptr)
    {
        return nullptr;
    }

    owner = receiver->newLocationIDNumber();
    if (owner != nullptr)
    {
        location = owner->unsigned32BitValue() + 1 + index;
        owner->release();
    }

    return OSNumber::withNumber(location, 32);
}
