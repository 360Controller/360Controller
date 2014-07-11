/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    WirelessDevice.h - declaration of the base wireless 360 device driver class
    
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
#ifndef __WIRELESSDEVICE_H__
#define __WIRELESSDEVICE_H__

#include <IOKit/IOService.h>

class WirelessDevice;

typedef void (*WirelessDeviceWatcher)(void *target, WirelessDevice *sender, void *parameter);

class WirelessDevice : public IOService
{
    OSDeclareDefaultStructors(WirelessDevice);
    
public:
    bool init(OSDictionary *dictionary = 0);

    // Controller interface
    bool IsDataAvailable(void);
    IOMemoryDescriptor* NextPacket(void);
    
    void SendPacket(const void *data, size_t length);
    
    void RegisterWatcher(void *target, WirelessDeviceWatcher function, void *parameter);

    OSNumber* newLocationIDNumber() const;
    
private:
    friend class WirelessGamingReceiver;
    void SetIndex(int i);
    void NewData(void);
    int index;
    // callback
    void *target, *parameter;
    WirelessDeviceWatcher function;
};

#endif // __WIRELESSDEVICE_H__
