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

// Different frame types
// Command: controller doesn't respond
// Request: controller responds with data
// Request (ACK): controller responds with ack + data
#define TYPE_COMMAND 0x00
#define TYPE_REQUEST 0x02
#define TYPE_REQUEST_ACK 0x03

#define CMD_ACKNOWLEDGE 0x01
#define CMD_STATUS 0x03
#define CMD_POWER_MODE 0x05
#define CMD_GUIDE_BTN 0x07
#define CMD_RUMBLE 0x09
#define CMD_LED_MODE 0x0a
#define CMD_SERIAL_NUM 0x1e
#define CMD_INPUT 0x20

// Controller input can be paused temporarily
#define POWER_ON 0x00
#define POWER_SLEEP 0x01
#define POWER_OFF 0x04

#define LED_NORMAL 0x01

#define BATT_TYPE_ALKALINE 0x01
#define BATT_TYPE_NIMH 0x02

#define BATT_LEVEL_EMPTY 0x00
#define BATT_LEVEL_LOW 0x01
#define BATT_LEVEL_MED 0x02
#define BATT_LEVEL_HIGH 0x03

struct SerialData
{
    uint16_t unknown;
    char serialNumber[14];
} __attribute__((packed));

struct LedModeData
{
    uint8_t unknown;
    uint8_t mode;
    uint8_t brightness;
} __attribute__((packed));

struct StatusData
{
    uint32_t batteryLevel : 2;
    uint32_t batteryType : 2;
    uint32_t connectionInfo : 4;
    uint8_t unknown1;
    uint16_t unknown2;
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

struct GuideButtonData
{
    uint8_t pressed;
    uint8_t unknown;
} __attribute__((packed));

struct RumbleData
{
    uint8_t unknown1;
    uint8_t motors;
    uint8_t triggerLeft;
    uint8_t triggerRight;
    uint8_t left;
    uint8_t right;
    uint8_t time;
    uint16_t unknown2;
} __attribute__((packed));

struct ControllerFrame
{
    uint8_t command;
    uint32_t unknown : 4;
    uint32_t type : 4;
    uint8_t sequence;
    uint8_t length;
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
    
    bool setPowerMode(uint8_t macAddress[], uint8_t mode);
    bool rumble(uint8_t macAddress[], RumbleData data);
    
    uint32_t generateLocationId();

private:
    WirelessOneMT76 *mt;
    uint32_t locationIdCounter;
    
    bool initController(uint8_t macAddress[], char serialNumber[]);
    
    template<typename T>
    void iterateControllers(T each);
    
    bool acknowledgePacket(uint8_t macAddress[], ControllerFrame *frame);
    bool requestSerialNumber(uint8_t macAddress[]);
    bool setLedMode(uint8_t macAddress[], LedModeData data);
    bool send(uint8_t macAddress[], ControllerFrame frame, uint8_t data[]);
};
