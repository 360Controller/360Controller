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
    bool pretend360;

private:
    OSString* getDeviceString(UInt8 index, const char* def = NULL) const;

public:
    virtual bool start(IOService* provider) override;

    virtual IOReturn setProperties(OSObject* properties) override;

    virtual IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;

    virtual IOReturn setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options = 0) override;
    virtual IOReturn getReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) override;
    virtual IOReturn handleReport(IOMemoryDescriptor* report, IOHIDReportType reportType = kIOHIDReportTypeInput, IOOptionBits options = 0) override;

    virtual OSString* newManufacturerString() const override;
    virtual OSNumber* newPrimaryUsageNumber() const override;
    virtual OSNumber* newPrimaryUsagePageNumber() const override;
    virtual OSNumber* newProductIDNumber() const override;
    virtual OSString* newProductString() const override;
    virtual OSString* newSerialNumberString() const override;
    virtual OSString* newTransportString() const override;
    virtual OSNumber* newVendorIDNumber() const override;

    virtual OSNumber* newLocationIDNumber() const override;

    virtual void remapButtons(void* buffer);
    virtual void remapAxes(void* buffer);
};


class Xbox360Pretend360Class : public Xbox360ControllerClass
{
    OSDeclareDefaultStructors(Xbox360Pretend360Class)
    
public:
    virtual OSString* newProductString() const override;
    virtual OSNumber* newProductIDNumber() const override;
    virtual OSNumber* newVendorIDNumber() const override;
};


class XboxOriginalControllerClass : public Xbox360ControllerClass
{
    OSDeclareDefaultStructors(XboxOriginalControllerClass)

private:
    UInt8 lastData[32];
    UInt32 repeatCount;

public:
    virtual IOReturn setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options = 0) override;
    virtual IOReturn handleReport(IOMemoryDescriptor* report, IOHIDReportType reportType = kIOHIDReportTypeInput, IOOptionBits options = 0) override;

    virtual OSString* newManufacturerString() const override;
    virtual OSNumber* newProductIDNumber() const override;
    virtual OSNumber* newVendorIDNumber() const override;
    virtual OSString* newProductString() const override;
};


class XboxOneControllerClass : public Xbox360ControllerClass
{
    OSDeclareDefaultStructors(XboxOneControllerClass)

#define XboxOne_Prepare(x,t)      {memset(&x,0,sizeof(x));x.header.command=t;x.header.size=sizeof(x-4);}

protected:
    UInt8 lastData[20];
    bool isXboxOneGuideButtonPressed;
    void reorderButtons(UInt16* buttons, UInt8 mapping[]);
    UInt16 convertButtonPacket(UInt16 buttons);

public:
    virtual IOReturn setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options = 0) override;
    virtual IOReturn handleReport(IOMemoryDescriptor* report, IOHIDReportType reportType = kIOHIDReportTypeInput, IOOptionBits options = 0) override;

    virtual void convertFromXboxOne(void* buffer, UInt8 packetSize);
    virtual OSString* newProductString() const override;
};


class XboxOnePretend360Class : public XboxOneControllerClass
{
    OSDeclareDefaultStructors(XboxOnePretend360Class)

public:
    virtual OSString* newProductString() const override;
    virtual OSNumber* newProductIDNumber() const override;
    virtual OSNumber* newVendorIDNumber() const override;
};
