/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006 Colin Munro
    
    _60Controller.h - definition of the driver main class
    
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
*/#ifndef __XBOX360CONTROLLER_H__
#define __XBOX360CONTROLLER_H__

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>

class Xbox360ControllerClass : public IOHIDDevice
{
    OSDeclareDefaultStructors(Xbox360ControllerClass)

private:
    void ReleaseAll(void);
    bool QueueRead(void);
    bool QueueWrite(const void *bytes,UInt32 length);

    OSString* getDeviceString(UInt8 index,const char *def=NULL) const;
    
    static void ReadCompleteInternal(void *target,void *parameter,IOReturn status,UInt32 bufferSizeRemaining);
    static void WriteCompleteInternal(void *target,void *parameter,IOReturn status,UInt32 bufferSizeRemaining);
    
    void fiddleReport(IOBufferMemoryDescriptor *buffer);
    
    void readSettings(void);

protected:
    IOUSBDevice *device;
    IOUSBInterface *interface;
    IOUSBPipe *inPipe,*outPipe;
    IOBufferMemoryDescriptor *inBuffer;
    
    // Settings
    bool invertLeftX,invertLeftY;
    bool invertRightX,invertRightY;
    short deadzoneLeft,deadzoneRight;
    bool relativeLeft,relativeRight;

public:
    // this is from the IORegistryEntry - no provider yet
    virtual bool init(OSDictionary *propTable);
    virtual void free(void);

    // IOKit methods. These methods are defines in <IOKit/IOService.h>
    virtual IOService* probe(IOService *provider, SInt32 *score );

    virtual IOReturn setProperties(OSObject *properties);

    virtual IOReturn message(UInt32 type, IOService *provider, void *argument);
    
    virtual void ReadComplete(void *parameter,IOReturn status,UInt32 bufferSizeRemaining);
    virtual void WriteComplete(void *parameter,IOReturn status,UInt32 bufferSizeRemaining);
    
    // IOHidDevice methods
    virtual IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const;
    
    virtual IOReturn setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options=0);
    virtual IOReturn getReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options);

    virtual OSString* newManufacturerString() const;
    virtual OSNumber* newPrimaryUsageNumber() const;
    virtual OSNumber* newPrimaryUsagePageNumber() const;
    virtual OSNumber* newProductIDNumber() const;
    virtual OSString* newProductString() const;
    virtual OSString* newSerialNumberString() const;
    virtual OSString* newTransportString() const;
    virtual OSNumber* newVendorIDNumber() const;
protected:
    virtual bool handleStart(IOService *provider);
    virtual void handleStop(IOService *provider);
};

#endif /* __XBOX360CONTROLLER_H__ */
