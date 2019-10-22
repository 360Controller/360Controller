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

#include <IOKit/IOService.h>

struct ControllerFrame
{
    uint8_t command;
    uint8_t message;
    uint8_t sequence;
    uint8_t length;
} __attribute__((packed));

struct RumbleData
{
    uint8_t unknown1;
    uint8_t type;
    uint8_t triggerLeft;
    uint8_t triggerRight;
    uint8_t left;
    uint8_t right;
    uint8_t time;
    uint16_t unknown2;
} __attribute__((packed));

struct LedModeData
{
    uint8_t unknown;
    uint8_t mode;
    uint8_t brightness;
} __attribute__((packed));

class WirelessOneMT76;

class WirelessOneDongle : public IOService
{
    OSDeclareDefaultStructors(WirelessOneDongle);
    
public:
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
    
    void handleConnect(uint8_t macAddress[]);
    void handleDisconnect(uint8_t macAddress[]);
    void handleData(uint8_t macAddress[], uint8_t data[]);
    
    bool rumble(uint8_t macAddress[], RumbleData data);
    bool powerOff(uint8_t macAddress[]);
    
    uint32_t generateLocationId();

private:
    WirelessOneMT76 *mt;
    uint32_t locationIdCounter;
    
    bool initController(uint8_t macAddress[], char serialNumber[]);
    
    template<typename T>
    void iterateControllers(T each);
    
    bool executeHandshake(uint8_t macAddress[]);
    bool requestSerialNumber(uint8_t macAddress[]);
    bool setLedMode(uint8_t macAddress[], LedModeData data);
    bool send(uint8_t macAddress[], ControllerFrame frame, uint8_t data[]);
};
