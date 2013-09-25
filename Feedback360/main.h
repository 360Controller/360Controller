/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    main.h - defines the structure used for the plugin COM object
    
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
#include <ForceFeedback/IOForceFeedbackLib.h>
#include <IOKit/IOCFPlugin.h>
#include "emulator.h"
#include "devlink.h"

struct _Xbox360ForceFeedback;

typedef struct _Xbox360InterfaceMap {
    IUnknownVTbl *pseudoVTable;
    struct _Xbox360ForceFeedback *obj;
} Xbox360InterfaceMap;

typedef struct _Xbox360ForceFeedback {
    Xbox360InterfaceMap iIOCFPlugInInterface;
    Xbox360InterfaceMap iIOForceFeedbackDeviceInterface;
    CFUUIDRef factoryID;
    UInt32 refCount;
    ForceEmulator emulator;
    DeviceLink device;
    bool manual;
} Xbox360ForceFeedback;

#define FFThis(interface)                       (((interface)==NULL)?NULL:(((Xbox360InterfaceMap*)(interface))->obj))

#define FeedbackDriverVersionMajor              1
#define FeedbackDriverVersionMinor              0
#define FeedbackDriverVersionStage              developStage
#define FeedbackDriverVersionNonRelRev          0

// B8ED278F-EC8A-4E8E-B4CF-13E2A9D68E83
#define FeedbackDriverUuid CFUUIDGetConstantUUIDWithBytes(NULL, 	\
    0xB8, 0xED, 0x27, 0x8F, 0xEC, 0x8A, 0x4E, 0x8E, 					\
    0xB4, 0xCF, 0x13, 0xE2, 0xA9, 0xD6, 0x8E, 0x83)

// Factory function
void* Control360Factory(CFAllocatorRef allocator,CFUUIDRef uuid);
