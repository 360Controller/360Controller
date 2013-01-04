/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    Wireless360Controller.h - declaration of the wireless controller driver class
    
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
#ifndef __WIRELESS360CONTROLLER_H__
#define __WIRELESS360CONTROLLER_H__

#include "../WirelessGamingReceiver/WirelessHIDDevice.h"

class Wireless360Controller : public WirelessHIDDevice
{
    OSDeclareDefaultStructors(Wireless360Controller);
public:
    bool init(OSDictionary *propTable = NULL);

    void SetRumbleMotors(unsigned char large, unsigned char small);
    
    IOReturn setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options);
    IOReturn newReportDescriptor(IOMemoryDescriptor ** descriptor ) const;

    IOReturn setProperties(OSObject *properties);

    virtual OSString* newManufacturerString() const;
    virtual OSNumber* newPrimaryUsageNumber() const;
    virtual OSNumber* newPrimaryUsagePageNumber() const;
    virtual OSNumber* newProductIDNumber() const;
    virtual OSString* newProductString() const;
    virtual OSString* newTransportString() const;
    virtual OSNumber* newVendorIDNumber() const;
protected:
    void readSettings(void);
    void receivedHIDupdate(unsigned char *data, int length);

    // Settings
    bool invertLeftX,invertLeftY;
    bool invertRightX,invertRightY;
    short deadzoneLeft,deadzoneRight;
    bool relativeLeft,relativeRight;
private:
    void fiddleReport(unsigned char *data, int length);
};

#endif // __WIRELESS360CONTROLLER_H__
