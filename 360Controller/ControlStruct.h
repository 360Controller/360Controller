/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    ControlStruct.h - Structures used by the device
    
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
#ifndef __CONTROLSTRUCT_H__
#define __CONTROLSTRUCT_H__

typedef UInt8 XBox360_Byte;
typedef UInt16 XBox360_Short;
typedef SInt16 XBox360_SShort;

#define Xbox360_Prepare(x,t)      {memset(&x,0,sizeof(x));x.header.command=t;x.header.size=sizeof(x);}

#define PACKED __attribute__((__packed__))

// Common structure format
typedef struct XBOX360_PACKET {
    XBox360_Byte command;
    XBox360_Byte size;
} PACKED XBOX360_PACKET;

// Analog stick format
typedef struct XBOX360_HAT {
    XBox360_SShort x,y;
} PACKED XBOX360_HAT;

// Structure describing the report had back from the controller
typedef struct XBOX360_IN_REPORT {
    XBOX360_PACKET header;
    XBox360_Short buttons;
    XBox360_Byte trigL,trigR;
    XBOX360_HAT left,right;
    XBox360_Byte reserved[6];
} PACKED XBOX360_IN_REPORT;

// Structure describing the command to change LED status
typedef struct XBOX360_OUT_LED {
    XBOX360_PACKET header;
    XBox360_Byte pattern;
} PACKED XBOX360_OUT_LED;

// Structure describing the command to change rumble motor status
typedef struct XBOX360_OUT_RUMBLE {
    XBOX360_PACKET header;
    XBox360_Byte reserved1;
    XBox360_Byte big,little;
    XBox360_Byte reserved[3];
} PACKED XBOX360_OUT_RUMBLE;

// Enumeration of command types
enum CommandTypes {
    // In
    inReport  = 0,
    // Out
    outRumble = 0,
    outLed    = 1
};

// Button bits
enum ButtonBits {
    btnHatRight      = 0x8000,
    btnHatLeft       = 0x4000,
    btnBack          = 0x2000,
    btnStart         = 0x1000,
    btnDigiRight     = 0x0800,
    btnDigiLeft      = 0x0400,
    btnDigiDown      = 0x0200,
    btnDigiUp        = 0x0100,
    btnY             = 0x0080,
    btnX             = 0x0040,
    btnB             = 0x0020,
    btnA             = 0x0010,
    btnReserved1     = 0x0008,  // Unused?
    btnXbox          = 0x0004,
    btnShoulderRight = 0x0002,
    btnShoulderLeft  = 0x0001
};

// LED values
enum LEDValues {
    ledOff          = 0x00,
    ledBlinkingAll  = 0x01,
    ledFlashOn1     = 0x02,
    ledFlashOn2     = 0x03,
    ledFlashOn3     = 0x04,
    ledFlashOn4     = 0x05,
    ledOn1          = 0x06,
    ledOn2          = 0x07,
    ledOn3          = 0x08,
    ledOn4          = 0x09,
    ledRotating     = 0x0a,
    ledBlinking     = 0x0b, // Blinking of previously enabled LED (e.g. from 0x01-0x09)
    ledBlinkingSlow = 0x0c, // As above
    ldAlternating   = 0x0d  // 1+4, 2+3, then back to previous after a short time
};

#endif // __CONTROLSTRUCT_H__
