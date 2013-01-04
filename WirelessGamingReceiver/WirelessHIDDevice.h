/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    WirelessHIDDevice.h - declaration of generic wireless HID device
    
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
#ifndef __WIRELESSHIDDEVICE_H__
#define __WIRELESSHIDDEVICE_H__

#include <IOKit/hid/IOHIDDevice.h>

class WirelessDevice;

class WirelessHIDDevice : public IOHIDDevice
{
    OSDeclareDefaultStructors(WirelessHIDDevice);
public:
    void SetLEDs(int mode);
    void PowerOff(void);
    unsigned char GetBatteryLevel(void);
    
    IOReturn setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options);

    OSNumber* newLocationIDNumber() const;
    OSString* newSerialNumberString() const;
protected:
    bool handleStart(IOService *provider);
    void handleStop(IOService *provider);
    virtual void receivedData(void);
    virtual void receivedMessage(IOMemoryDescriptor *data);
    virtual void receivedUpdate(unsigned char type, unsigned char *data);
    virtual void receivedHIDupdate(unsigned char *data, int length);
private:
    static void _receivedData(void *target, WirelessDevice *sender, void *parameter);
    static void ChatPadTimerActionWrapper(OSObject *owner, IOTimerEventSource *sender);
    
	IOTimerEventSource *serialTimer;
    int serialTimerCount;
    
    unsigned char battery;
    char serialString[10];
};

#endif // __WIRELESSHIDDEVICE_H__
