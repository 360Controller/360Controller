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

struct SerialData
{
    uint16_t unknown;
    char serialNumber[14];
} __attribute__((packed));

struct Buttons
{
    uint32_t unknown : 2;
    uint32_t start : 1;
    uint32_t back : 1;
    uint32_t a : 1;
    uint32_t b : 1;
    uint32_t x : 1;
    uint32_t y : 1;
    uint32_t dpadUp : 1;
    uint32_t dpadDown : 1;
    uint32_t dpadLeft : 1;
    uint32_t dpadRight : 1;
    uint32_t bumperLeft : 1;
    uint32_t bumperRight : 1;
    uint32_t stickLeft : 1;
    uint32_t stickRight : 1;
} __attribute__((packed));

struct InputData
{
    Buttons buttons;
    uint16_t triggerLeft;
    uint16_t triggerRight;
    int16_t stickLeftX;
    int16_t stickLeftY;
    int16_t stickRightX;
    int16_t stickRightY;
} __attribute__((packed));

struct XBOX360_IN_REPORT;
class WirelessOneDongle;

class WirelessOneController : public IOHIDDevice
{
    OSDeclareDefaultStructors(WirelessOneController);

public:
    uint8_t macAddress[6];
    
    void handleSerialNumber(uint8_t data[]);
    void handleInput(uint8_t data[]);
    void handleGuideButton(uint8_t data[]);

protected:
    bool handleStart(IOService *provider) override;
    
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
    char serialNumber[15];
    
    void sendReport(XBOX360_IN_REPORT report);
};
