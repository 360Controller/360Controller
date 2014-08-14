/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    WirelessGamingReceiver.h - declaration of the wireless receiver driver
    
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
#ifndef __WIRELESSGAMINGRECEIVER_H__
#define __WIRELESSGAMINGRECEIVER_H__

#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>

// This value is defined by the hardware and fixed
#define WIRELESS_CONNECTIONS        4

class WirelessDevice;

typedef struct WIRELESS_CONNECTION
{
    // Controller
    IOUSBInterface *controller;
    IOUSBPipe *controllerIn, *controllerOut;
    
    // Mystery
    IOUSBInterface *other;
    IOUSBPipe *otherIn, *otherOut;
    
    // Runtime data
    OSArray *inputArray;
    WirelessDevice *service;
    bool controllerStarted;
}
WIRELESS_CONNECTION;

class WirelessGamingReceiver : public IOService
{
    OSDeclareDefaultStructors(WirelessGamingReceiver);
public:
    bool start(IOService *provider);
    void stop(IOService *provider);

    IOReturn message(UInt32 type,IOService *provider,void *argument);

    // For WirelessDevice to use
    OSNumber* newLocationIDNumber() const;
    
private:
    friend class WirelessDevice;
    bool IsDataQueued(int index);
    IOMemoryDescriptor* ReadBuffer(int index);
    bool QueueWrite(int index, const void *bytes, UInt32 length);
    
private:
    IOUSBDevice *device;
    WIRELESS_CONNECTION connections[WIRELESS_CONNECTIONS];
    int connectionCount;
    
    void InstantiateService(int index);
    
    void ProcessMessage(int index, const unsigned char *data, int length);
    
    bool QueueRead(int index);
    void ReadComplete(void *parameter, IOReturn status, UInt32 bufferSizeRemaining);
    
    void WriteComplete(void *parameter, IOReturn status, UInt32 bufferSizeRemaining);
    
    void ReleaseAll(void);

    bool didTerminate(IOService *provider, IOOptionBits options, bool *defer);
    
    static void _ReadComplete(void *target, void *parameter, IOReturn status, UInt32 bufferSizeRemaining);
    static void _WriteComplete(void *target, void *parameter, IOReturn status, UInt32 bufferSizeRemaining);
};

#endif // __WIRELESSGAMINGRECEIVER_H__
