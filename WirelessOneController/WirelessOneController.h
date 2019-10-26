/*
 * Copyright (C) 2019 Medusalix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <IOKit/hid/IOHIDDevice.h>

struct XBOX360_IN_REPORT;

class WirelessOneDongle;
struct StatusData;
struct InputData;
struct GuideButtonData;

class WirelessOneController : public IOHIDDevice
{
    OSDeclareDefaultStructors(WirelessOneController);

public:
    uint8_t macAddress[6];
    char serialNumber[15];
    
    void handleStatus(StatusData *status);
    void handleInput(InputData *input);
    void handleGuideButton(GuideButtonData *button);

protected:
    bool handleStart(IOService *provider) override;
    IOReturn setProperties(OSObject *properties) override;
    IOReturn setReport(
        IOMemoryDescriptor *report,
        IOHIDReportType reportType,
        IOOptionBits options = 0
    ) override;
    
    IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const override;
    OSString* newTransportString() const override;
    OSString* newManufacturerString() const override;
    OSString* newProductString() const override;
    OSNumber* newVendorIDNumber() const override;
    OSNumber* newProductIDNumber() const override;
    OSNumber* newPrimaryUsageNumber() const override;
    OSNumber* newPrimaryUsagePageNumber() const override;
    OSString* newSerialNumberString() const override;
    OSNumber* newLocationIDNumber() const override;
    
private:
    WirelessOneDongle *dongle;
    
    uint8_t rumbleType;
    
    void sendReport(XBOX360_IN_REPORT report);
};
