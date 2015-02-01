/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro
 
 Controller.h - Driver class for the 360 controller
 
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

#include <IOKit/hid/IOHIDDevice.h>

class Xbox360ControllerClass : public IOHIDDevice
{
	OSDeclareDefaultStructors(Xbox360ControllerClass)

private:
    OSString* getDeviceString(UInt8 index,const char *def=NULL) const;

public:
    virtual bool start(IOService *provider);

    virtual IOReturn setProperties(OSObject *properties);

    virtual IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const;
    
    virtual IOReturn setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options=0);
    virtual IOReturn getReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options);
    virtual IOReturn handleReport(
                                  IOMemoryDescriptor * report,
                                  IOHIDReportType      reportType = kIOHIDReportTypeInput,
                                  IOOptionBits         options    = 0 );
	
    virtual OSString* newManufacturerString() const;
    virtual OSNumber* newPrimaryUsageNumber() const;
    virtual OSNumber* newPrimaryUsagePageNumber() const;
    virtual OSNumber* newProductIDNumber() const;
    virtual OSString* newProductString() const;
    virtual OSString* newSerialNumberString() const;
    virtual OSString* newTransportString() const;
    virtual OSNumber* newVendorIDNumber() const;
	
    virtual OSNumber* newLocationIDNumber() const;
    
    virtual void remapButtons(void *buffer);
};


class XboxOriginalControllerClass : public Xbox360ControllerClass
{
    OSDeclareDefaultStructors(XboxOriginalControllerClass)
    
private:
    UInt8 lastData[32];
    UInt32 repeatCount;
    
public:
    virtual IOReturn setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options=0);
    virtual IOReturn handleReport(
                                  IOMemoryDescriptor * report,
                                  IOHIDReportType      reportType = kIOHIDReportTypeInput,
                                  IOOptionBits         options    = 0 );
    
    virtual OSString* newManufacturerString() const;
    virtual OSNumber* newProductIDNumber() const;
    virtual OSNumber* newVendorIDNumber() const;
    virtual OSString* newProductString() const;
};


class XboxOneControllerClass : public Xbox360ControllerClass
{
    OSDeclareDefaultStructors(XboxOneControllerClass)
    
#define XboxOne_Prepare(x,t)      {memset(&x,0,sizeof(x));x.header.command=t;x.header.size=sizeof(x-4);}
    
private:
    UInt8 lastData[20];
    bool isXboxOneGuideButtonPressed;
    
public:
    virtual IOReturn setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options=0);
    virtual IOReturn handleReport(
                                  IOMemoryDescriptor * report,
                                  IOHIDReportType      reportType = kIOHIDReportTypeInput,
                                  IOOptionBits         options    = 0 );
    
    virtual OSString* newManufacturerString() const;
    virtual OSNumber* newProductIDNumber() const;
    virtual OSNumber* newVendorIDNumber() const;
    virtual OSString* newProductString() const;
    virtual void convertFromXboxOne(void *buffer, void* override);
};
