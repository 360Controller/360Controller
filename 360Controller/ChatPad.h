/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro

 ChatPad.h - Driver class for the ChatPad accessory

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

class ChatPadKeyboardClass : public IOHIDDevice
{
	OSDeclareDefaultStructors(ChatPadKeyboardClass)

private:

public:
    virtual bool start(IOService* provider) override;

    // IOHidDevice methods
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
};
