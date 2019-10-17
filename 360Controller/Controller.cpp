/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro

 Controller.cpp - driver class for the 360 controller

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

#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>
#include "Controller.h"
namespace HID_360 {
#include "xbox360hid.h"
}
#include "_60Controller.h"

#pragma mark - Xbox360ControllerClass

OSDefineMetaClassAndStructors(Xbox360ControllerClass, IOHIDDevice)

static Xbox360Peripheral* GetOwner(IOService *us)
{
	IOService *prov = us->getProvider();

	if (prov == NULL)
		return NULL;
	return OSDynamicCast(Xbox360Peripheral, prov);
}

static IOUSBDevice* GetOwnerProvider(const IOService *us)
{
	IOService *prov = us->getProvider(), *provprov;

	if (prov == NULL)
		return NULL;
	provprov = prov->getProvider();
	if (provprov == NULL)
		return NULL;
	return OSDynamicCast(IOUSBDevice, provprov);
}

bool Xbox360ControllerClass::start(IOService *provider)
{
    if (OSDynamicCast(Xbox360Peripheral, provider) == NULL)
        return false;
    return IOHIDDevice::start(provider);
}

IOReturn Xbox360ControllerClass::setProperties(OSObject *properties)
{
	Xbox360Peripheral *owner = GetOwner(this);
	if (owner == NULL)
		return kIOReturnUnsupported;
	return owner->setProperties(properties);
}

// Returns the HID descriptor for this device
IOReturn Xbox360ControllerClass::newReportDescriptor(IOMemoryDescriptor **descriptor) const
{
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task,kIODirectionOut,sizeof(HID_360::ReportDescriptor));

    if (buffer == NULL) return kIOReturnNoResources;
    buffer->writeBytes(0,HID_360::ReportDescriptor,sizeof(HID_360::ReportDescriptor));
    *descriptor=buffer;
    return kIOReturnSuccess;
}

// Handles a message from the userspace IOHIDDeviceInterface122::setReport function
IOReturn Xbox360ControllerClass::setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    char data[2];

    report->readBytes(0, data, 2);
    if (GetOwner(this)->rumbleType == 1) // Don't Rumble
        return kIOReturnSuccess;
    switch(data[0]) {
        case 0x00:  // Set force feedback
            if((data[1]!=report->getLength()) || (data[1]!=0x04)) return kIOReturnUnsupported;
		{
			XBOX360_OUT_RUMBLE rumble;

			Xbox360_Prepare(rumble,outRumble);
			report->readBytes(2,data,2);
			rumble.big=data[0];
			rumble.little=data[1];
			GetOwner(this)->QueueWrite(&rumble,sizeof(rumble));
			// IOLog("Set rumble: big(%d) little(%d)\n", rumble.big, rumble.little);
		}
            return kIOReturnSuccess;
        case 0x01:  // Set LEDs
            if((data[1]!=report->getLength())||(data[1]!=0x03)) return kIOReturnUnsupported;
		{
			XBOX360_OUT_LED led;

			report->readBytes(2,data,1);
			Xbox360_Prepare(led,outLed);
			led.pattern=data[0];
			GetOwner(this)->QueueWrite(&led,sizeof(led));
			// IOLog("Set LED: %d\n", led.pattern);
		}
            return kIOReturnSuccess;
        default:
			IOLog("Unknown escape %d\n", data[0]);
            return kIOReturnUnsupported;
    }
}

// Get report
IOReturn Xbox360ControllerClass::getReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    // Doesn't do anything yet ;)
    return kIOReturnUnsupported;
}

IOReturn Xbox360ControllerClass::handleReport(IOMemoryDescriptor * descriptor, IOHIDReportType reportType, IOOptionBits options) {
    if (descriptor->getLength() >= sizeof(XBOX360_IN_REPORT)) {
        IOBufferMemoryDescriptor *desc = OSDynamicCast(IOBufferMemoryDescriptor, descriptor);
        if (desc != NULL) {
            XBOX360_IN_REPORT *report=(XBOX360_IN_REPORT*)desc->getBytesNoCopy();
            if ((report->header.command==inReport) && (report->header.size==sizeof(XBOX360_IN_REPORT))) {
                GetOwner(this)->fiddleReport(report->left, report->right);
                if (!(GetOwner(this)->noMapping))
                    remapButtons(report);
                if (GetOwner(this)->swapSticks)
                    remapAxes(report);
            }
        }
    }
    IOReturn ret = IOHIDDevice::handleReport(descriptor, reportType, options);
    return ret;
}


// Returns the string for the specified index from the USB device's string list, with an optional default
OSString* Xbox360ControllerClass::getDeviceString(UInt8 index,const char *def) const
{
    IOReturn err;
    char buf[1024];
    const char *string;

    err = GetOwnerProvider(this)->GetStringDescriptor(index, buf, sizeof(buf));
    if(err==kIOReturnSuccess) string=buf;
    else {
        if(def == NULL) string = "Unknown";
        else string = def;
    }
    return OSString::withCString(string);
}

OSString* Xbox360ControllerClass::newManufacturerString() const
{
    return getDeviceString(GetOwnerProvider(this)->GetManufacturerStringIndex());
}

OSNumber* Xbox360ControllerClass::newPrimaryUsageNumber() const
{
    return OSNumber::withNumber(HID_360::ReportDescriptor[3], 8);
}

OSNumber* Xbox360ControllerClass::newPrimaryUsagePageNumber() const
{
    return OSNumber::withNumber(HID_360::ReportDescriptor[1], 8);
}

OSNumber* Xbox360ControllerClass::newProductIDNumber() const
{
    return OSNumber::withNumber(GetOwnerProvider(this)->GetProductID(),16);
}

OSString* Xbox360ControllerClass::newProductString() const
{
    return OSString::withCString("Xbox 360 Wired Controller");
}

OSString* Xbox360ControllerClass::newSerialNumberString() const
{
    return getDeviceString(GetOwnerProvider(this)->GetSerialNumberStringIndex());
}

OSString* Xbox360ControllerClass::newTransportString() const
{
    return OSString::withCString("USB");
}

OSNumber* Xbox360ControllerClass::newVendorIDNumber() const
{
    return OSNumber::withNumber(GetOwnerProvider(this)->GetVendorID(),16);
}

OSNumber* Xbox360ControllerClass::newLocationIDNumber() const
{
	IOUSBDevice *device;
    OSNumber *number;
    UInt32 location = 0;

	device = GetOwnerProvider(this);
    if (device)
    {
        if ((number = OSDynamicCast(OSNumber, device->getProperty("locationID"))))
        {
            location = number->unsigned32BitValue();
        }
        else
        {
            // Make up an address
            if ((number = OSDynamicCast(OSNumber, device->getProperty("USB Address"))))
                location |= number->unsigned8BitValue() << 24;

            if ((number = OSDynamicCast(OSNumber, device->getProperty("idProduct"))))
                location |= number->unsigned8BitValue() << 16;
        }
    }

    return (location != 0) ? OSNumber::withNumber(location, 32) : 0;
}

void Xbox360ControllerClass::remapButtons(void *buffer)
{
    XBOX360_IN_REPORT *report360 = (XBOX360_IN_REPORT*)buffer;
    UInt16 new_buttons = 0;

    new_buttons |= ((report360->buttons & 1) == 1) << GetOwner(this)->mapping[0];
    new_buttons |= ((report360->buttons & 2) == 2) << GetOwner(this)->mapping[1];
    new_buttons |= ((report360->buttons & 4) == 4) << GetOwner(this)->mapping[2];
    new_buttons |= ((report360->buttons & 8) == 8) << GetOwner(this)->mapping[3];
    new_buttons |= ((report360->buttons & 16) == 16) << GetOwner(this)->mapping[4];
    new_buttons |= ((report360->buttons & 32) == 32) << GetOwner(this)->mapping[5];
    new_buttons |= ((report360->buttons & 64) == 64) << GetOwner(this)->mapping[6];
    new_buttons |= ((report360->buttons & 128) == 128) << GetOwner(this)->mapping[7];
    new_buttons |= ((report360->buttons & 256) == 256) << GetOwner(this)->mapping[8];
    new_buttons |= ((report360->buttons & 512) == 512) << GetOwner(this)->mapping[9];
    new_buttons |= ((report360->buttons & 1024) == 1024) << GetOwner(this)->mapping[10];
    new_buttons |= ((report360->buttons & 4096) == 4096) << GetOwner(this)->mapping[11];
    new_buttons |= ((report360->buttons & 8192) == 8192) << GetOwner(this)->mapping[12];
    new_buttons |= ((report360->buttons & 16384) == 16384) << GetOwner(this)->mapping[13];
    new_buttons |= ((report360->buttons & 32768) == 32768) << GetOwner(this)->mapping[14];

//    IOLog("BUTTON PACKET - %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", GetOwner(this)->mapping[0], GetOwner(this)->mapping[1], GetOwner(this)->mapping[2], GetOwner(this)->mapping[3], GetOwner(this)->mapping[4], GetOwner(this)->mapping[5], GetOwner(this)->mapping[6], GetOwner(this)->mapping[7], GetOwner(this)->mapping[8], GetOwner(this)->mapping[9], GetOwner(this)->mapping[10], GetOwner(this)->mapping[11], GetOwner(this)->mapping[12], GetOwner(this)->mapping[13], GetOwner(this)->mapping[14]);

    report360->buttons = new_buttons;
}

void Xbox360ControllerClass::remapAxes(void *buffer)
{
    XBOX360_IN_REPORT *report360 = (XBOX360_IN_REPORT*)buffer;

    XBOX360_HAT temp = report360->left;
    report360->left = report360->right;
    report360->right = temp;
}


#pragma mark - XboxOnePretend360Class

/*
 * Xbox 360 controller.
 * Fake PID and VID of Xbox 360 controller
 */

OSDefineMetaClassAndStructors(Xbox360Pretend360Class, Xbox360ControllerClass)

OSString* Xbox360Pretend360Class::newProductString() const
{
    return OSString::withCString("Xbox 360 Wired Controller");
}

OSNumber* Xbox360Pretend360Class::newProductIDNumber() const
{
    return OSNumber::withNumber(654,16);
}

OSNumber* Xbox360Pretend360Class::newVendorIDNumber() const
{
    return OSNumber::withNumber(1118,16);
}

#pragma mark - XboxOriginalControllerClass

/*
 * Xbox original controller.
 * Convert reports to Xbox 360 controller format and fake product ids
 */

typedef struct {
    XBOX360_PACKET header;
    Xbox360_Byte buttons;
    Xbox360_Byte reserved1;
    Xbox360_Byte a, b, x, y, black, white;
    Xbox360_Byte trigL,trigR;
    Xbox360_Short xL,yL;
    Xbox360_Short xR,yR;
} PACKED XBOX_IN_REPORT;

typedef struct {
    XBOX360_PACKET header;
    Xbox360_Byte reserved1;
    Xbox360_Byte left;
    Xbox360_Byte reserved2;
    Xbox360_Byte right;
} PACKED XBOX_OUT_RUMBLE;


OSDefineMetaClassAndStructors(XboxOriginalControllerClass, Xbox360ControllerClass)

OSNumber* XboxOriginalControllerClass::newVendorIDNumber() const
{
    return OSNumber::withNumber(1118,16);
}

OSString* XboxOriginalControllerClass::newManufacturerString() const
{
    return OSString::withCString("Holtek");
}

OSNumber* XboxOriginalControllerClass::newProductIDNumber() const
{
    return OSNumber::withNumber(654,16);
}

OSString* XboxOriginalControllerClass::newProductString() const
{
    return OSString::withCString("Xbox Original Wired Controller");
}

static void logData(UInt8 *data, int len) {
    for (int i = 0; i < len; i++) IOLog("%02x ", (int)data[i]);
    IOLog("\n");
}

// This converts Xbox original controller report into Xbox360 form
// See https://github.com/Grumbel/xboxdrv/blob/master/src/controller/xbox_controller.cpp
static void convertFromXboxOriginal(UInt8 *data) {
    if (data[0] != 0x00 || data[1] != 0x14) {
        IOLog("Unknown report command %d, length %d\n", (int)data[0], (int)data[1]);
        return;
    }
    XBOX360_IN_REPORT report;
    Xbox360_Prepare (report, 0);
    XBOX_IN_REPORT *in = (XBOX_IN_REPORT*)data;
    Xbox360_Short buttons = in->buttons;
    if (in->a) buttons |= 1 << 12; // a
    if (in->b) buttons |= 1 << 13; // b
    if (in->x) buttons |= 1 << 14; // x
    if (in->y) buttons |= 1 << 15; // y
    if (in->black) buttons |= 1 << 9; // black mapped to shoulder right
    if (in->white) buttons |= 1 << 8; // white mapped to shoulder left
    report.buttons = buttons;
    report.trigL = in->trigL;
    report.trigR = in->trigR;
    report.left.x = in->xL;
    report.left.y = in->yL;
    report.right.x = in->xR;
    report.right.y = in->yR;
    *((XBOX360_IN_REPORT *)data) = report;
}

IOReturn XboxOriginalControllerClass::handleReport(IOMemoryDescriptor * descriptor, IOHIDReportType reportType, IOOptionBits options) {
//    IOLog("%s\n", __FUNCTION__);
    UInt8 data[sizeof(XBOX360_IN_REPORT)];
    if (descriptor->getLength() >= sizeof(XBOX360_IN_REPORT)) {
        descriptor->readBytes(0, data, sizeof(XBOX360_IN_REPORT));
        const XBOX360_IN_REPORT *report=(const XBOX360_IN_REPORT*)data;
        if ((report->header.command==inReport) && (report->header.size==sizeof(XBOX360_IN_REPORT))) {
            convertFromXboxOriginal(data);
            if (memcmp(data, lastData, sizeof(XBOX360_IN_REPORT)) == 0) {
                repeatCount ++;
                // drop triplicate reports
                if (repeatCount > 1) {
                    return kIOReturnSuccess;
                }
            } else {
                repeatCount = 0;
            }
            memcpy(lastData, data, sizeof(XBOX360_IN_REPORT));
            descriptor->writeBytes(0, data, sizeof(XBOX360_IN_REPORT));
            //if (data[2]&1) {
            //    IOLog("%s after %d ", __FUNCTION__, (int)reportType);
            //    logData(data, 20);
            //}
        } else {
            IOLog("%s %d \n", __FUNCTION__, (int)descriptor->getLength());
            IOLog("%s  %d \n", __FUNCTION__, (int)reportType);
            logData(data, (int)descriptor->getLength());
        }

    } else {
        descriptor->readBytes(0, data, descriptor->getLength());
        if (reportType != 0 && data[0] != 0) {
            // not a rumble report
            IOLog("%s %d \n", __FUNCTION__, (int)reportType);
            logData(data, (int)descriptor->getLength());
        }
    }

    IOReturn ret = Xbox360ControllerClass::handleReport(descriptor, reportType, options);
    //IOLog("%s END\n", __FUNCTION__);
    return ret;
}

IOReturn XboxOriginalControllerClass::setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    char data[2];

    report->readBytes(0, data, 2);
    if (GetOwner(this)->rumbleType == 1) // Don't Rumble
        return kIOReturnSuccess;
    switch(data[0]) {
        case 0x00:  // Set force feedback
            if((data[1]!=report->getLength()) || (data[1]!=0x04)) return kIOReturnUnsupported;
        {
            XBOX_OUT_RUMBLE rumble;
            Xbox360_Prepare(rumble,outRumble);
            report->readBytes(2,data,2);
            rumble.left=data[0]; // CHECKME != big, little
            rumble.right=data[1];
            GetOwner(this)->QueueWrite(&rumble,sizeof(rumble));
            // IOLog("Set rumble: big(%d) little(%d)\n", rumble.big, rumble.little);
        }
            return kIOReturnSuccess;
        case 0x01:  // Set LEDs
            if((data[1]!=report->getLength())||(data[1]!=0x03)) return kIOReturnUnsupported;
            // No leds
            return kIOReturnSuccess;

        default:
            IOLog("Unknown escape %d\n", data[0]);
            return kIOReturnUnsupported;
    }
}


#pragma mark - XboxOneControllerClass

/*
 * Xbox One controller.
 * Does not pretend to be an Xbox 360 controller.
 */

typedef struct {
    UInt8 command;
    UInt8 reserved1;
    UInt8 counter;
    UInt8 size;
} PACKED XBOXONE_HEADER;

typedef struct {
    XBOXONE_HEADER header;
    UInt16 buttons;
    UInt16 trigL, trigR;
    XBOX360_HAT left, right;
} PACKED XBOXONE_IN_REPORT;

typedef struct {
    XBOXONE_HEADER header;
    UInt16 buttons;
    UInt16 trigL, trigR;
    XBOX360_HAT left, right;
    UInt8 unknown1[6];
    UInt8 triggersAsButtons; // 0x40 is RT. 0x80 is LT
    UInt8 unknown2[7];
} PACKED XBOXONE_IN_FIGHTSTICK_REPORT;

typedef struct {
    XBOXONE_HEADER header;
    UInt16 buttons;
    union {
        UInt16 steering; // leftX
        SInt16 leftX;
    };
    union {
        UInt16 accelerator;
        UInt16 trigR;
    };
    union {
        UInt16 brake;
        UInt16 trigL;
    };
    union {
        UInt8 clutch;
        UInt8 leftY;
    };
    UInt8 unknown3[8];
} PACKED XBOXONE_IN_WHEEL_REPORT;

typedef struct {
    XBOXONE_HEADER header;
    UInt16 buttons;
    UInt16 trigL, trigR;
    XBOX360_HAT left, right;
    UInt16 true_buttons;
    UInt16 true_trigL, true_trigR;
    XBOX360_HAT true_left, true_right;
    UInt8 paddle;
} PACKED XBOXONE_ELITE_IN_REPORT;

typedef struct {
    XBOXONE_HEADER header;
    UInt8 state;
    UInt8 dummy;
} PACKED XBOXONE_IN_GUIDE_REPORT;

typedef struct {
    XBOXONE_HEADER header;
    UInt8 data[4];
    UInt8 zero[5];
} PACKED XBOXONE_OUT_GUIDE_REPORT;

typedef struct {
    XBOXONE_HEADER header;
    UInt8 mode; // So far always 0x00
    UInt8 rumbleMask; // So far always 0x0F
    UInt8 trigL, trigR;
    UInt8 little, big;
    UInt8 length; // Length of time to rumble
    UInt8 period; // Period of time between pulses. DO NOT INCLUDE WHEN SUBSTRUCTURE IS 0x09
    UInt8 extra;
} PACKED XBOXONE_OUT_RUMBLE;

typedef struct {
    XBOXONE_HEADER header; // 0x0a 0x20 0x04 0x03
    UInt8 zero;
    UInt8 command;
    UInt8 brightness; // 0x00 - 0x20
} PACKED XBOXONE_OUT_LED;

typedef enum {
    XONE_SYNC           = 0x0001, // Bit 00
    XONE_MENU           = 0x0004, // Bit 02
    XONE_VIEW           = 0x0008, // Bit 03
    XONE_A              = 0x0010, // Bit 04
    XONE_B              = 0x0020, // Bit 05
    XONE_X              = 0x0040, // Bit 06
    XONE_Y              = 0x0080, // Bit 07
    XONE_DPAD_UP        = 0x0100, // Bit 08
    XONE_DPAD_DOWN      = 0x0200, // Bit 09
    XONE_DPAD_LEFT      = 0x0400, // Bit 10
    XONE_DPAD_RIGHT     = 0x0800, // Bit 11
    XONE_LEFT_SHOULDER  = 0x1000, // Bit 12
    XONE_RIGHT_SHOULDER = 0x2000, // Bit 13
    XONE_LEFT_THUMB     = 0x4000, // Bit 14
    XONE_RIGHT_THUMB    = 0x8000, // Bit 15
} GAMEPAD_XONE;

typedef enum {
    XONE_PADDLE_UPPER_LEFT      = 0x0001, // Bit 00
    XONE_PADDLE_UPPER_RIGHT     = 0x0002, // Bit 01
    XONE_PADDLE_LOWER_LEFT      = 0x0004, // Bit 02
    XONE_PADDLE_LOWER_RIGHT     = 0x0008, // Bit 03
    XONE_PADDLE_PRESET_NUM      = 0x0010, // Bit 04
} GAMEPAD_XONE_ELITE_PADDLE;

typedef enum {
    XONE_LED_OFF_1           = 0x00,
    XONE_LED_SOLID           = 0x01,
    XONE_LED_BLINK_FAST      = 0x02,
    XONE_LED_BLINK_SLOW      = 0x03,
    XONE_LED_BLINK_VERY_SLOW = 0x04,
    XONE_LED_SOLD_1          = 0x05,
    XONE_LED_SOLD_2          = 0x06,
    XONE_LED_SOLD_3          = 0x07,
    XONE_LED_PHASE_SLOW      = 0x08,
    XONE_LED_PHASE_FAST      = 0x09,
    XONE_LED_REBOOT_1        = 0x0a,
    XONE_LED_OFF             = 0x0b,
    XONE_LED_FLICKER         = 0x0c,
    XONE_LED_SOLID_4         = 0x0d,
    XONE_LED_SOLID_5         = 0x0e,
    XONE_LED_REBOOT_2        = 0x0f,
} LED_XONE;

OSDefineMetaClassAndStructors(XboxOneControllerClass, Xbox360ControllerClass)

OSString* XboxOneControllerClass::newProductString() const
{
    return OSString::withCString("Xbox One Wired Controller");
}

UInt16 XboxOneControllerClass::convertButtonPacket(UInt16 buttons)
{
    UInt16 new_buttons = 0;

    new_buttons |= ((buttons & 4) == 4) << 4;
    new_buttons |= ((buttons & 8) == 8) << 5;
    new_buttons |= ((buttons & 16) == 16) << 12;
    new_buttons |= ((buttons & 32) == 32) << 13;
    new_buttons |= ((buttons & 64) == 64) << 14;
    new_buttons |= ((buttons & 128) == 128) << 15;
    new_buttons |= ((buttons & 256) == 256) << 0;
    new_buttons |= ((buttons & 512) == 512) << 1;
    new_buttons |= ((buttons & 1024) == 1024) << 2;
    new_buttons |= ((buttons & 2048) == 2048) << 3;
    new_buttons |= ((buttons & 4096) == 4096) << 8;
    new_buttons |= ((buttons & 8192) == 8192) << 9;
    new_buttons |= ((buttons & 16384) == 16384) << 6;
    new_buttons |= ((buttons & 32768) == 32768) << 7;

    new_buttons |= (isXboxOneGuideButtonPressed) << 10;

    return new_buttons;
}

void XboxOneControllerClass::convertFromXboxOne(void *buffer, UInt8 packetSize)
{
    XBOXONE_ELITE_IN_REPORT *reportXone = (XBOXONE_ELITE_IN_REPORT*)buffer;
    XBOX360_IN_REPORT *report360 = (XBOX360_IN_REPORT*)buffer;
    UInt8 trigL = 0, trigR = 0;
    XBOX360_HAT left, right;

    report360->header.command = 0x00;
    report360->header.size = 0x14;

    if (packetSize == 0x1a) // Fight Stick
    {
        if ((0x80 & reportXone->true_trigR) == 0x80) { trigL = 255; }
        if ((0x40 & reportXone->true_trigR) == 0x40) { trigR = 255; }
        
        left = reportXone->left;
        right = reportXone->right;
    }
    else if (packetSize == 0x11) // Racing Wheel
    {
        XBOXONE_IN_WHEEL_REPORT *wheelReport=(XBOXONE_IN_WHEEL_REPORT*)buffer;

        trigR = (wheelReport->accelerator / 1023.0) * 255; // UInt16 -> UInt8
        trigL = (wheelReport->brake / 1023.0) * 255; // UInt16 -> UInt8
        left.x = wheelReport->steering - 32768; // UInt16 -> SInt16
        left.y = wheelReport->clutch * 128; // Clutch is 0-255. Upconvert to half signed 16 range. (0 - 32640)
        right = {};
    }
    else // Traditional Controllers
    {
        trigL = (reportXone->trigL / 1023.0) * 255;
        trigR = (reportXone->trigR / 1023.0) * 255;
        
        left = reportXone->left;
        right = reportXone->right;
    }

    report360->buttons = convertButtonPacket(reportXone->buttons);
    report360->trigL = trigL;
    report360->trigR = trigR;
    report360->left = left;
    report360->right = right;
}

IOReturn XboxOneControllerClass::handleReport(IOMemoryDescriptor * descriptor, IOHIDReportType reportType, IOOptionBits options)
{
    if (descriptor->getLength() >= sizeof(XBOXONE_IN_GUIDE_REPORT)) {
        IOBufferMemoryDescriptor *desc = OSDynamicCast(IOBufferMemoryDescriptor, descriptor);
        if (desc != NULL) {
            XBOXONE_ELITE_IN_REPORT *report=(XBOXONE_ELITE_IN_REPORT*)desc->getBytesNoCopy();
            if ((report->header.command==0x07) && (report->header.size==(sizeof(XBOXONE_IN_GUIDE_REPORT)-4)))
            {
                XBOXONE_IN_GUIDE_REPORT *guideReport=(XBOXONE_IN_GUIDE_REPORT*)report;
                
                if (guideReport->header.reserved1 == 0x30) // 2016 Controller
                {
                    XBOXONE_OUT_GUIDE_REPORT outReport = {};
                    outReport.header.command = 0x01;
                    outReport.header.reserved1 = 0x20;
                    outReport.header.counter = guideReport->header.counter;
                    outReport.header.size = 0x09;
                    outReport.data[0] = 0x00;
                    outReport.data[1] = 0x07;
                    outReport.data[2] = 0x20;
                    outReport.data[3] = 0x02;
                    
                    GetOwner(this)->QueueWrite(&outReport, 13);
                }
                
                isXboxOneGuideButtonPressed = (bool)guideReport->state;
                XBOX360_IN_REPORT *oldReport = (XBOX360_IN_REPORT*)lastData;
                oldReport->buttons ^= (-isXboxOneGuideButtonPressed ^ oldReport->buttons) & (1 << GetOwner(this)->mapping[10]);
                memcpy(report, lastData, sizeof(XBOX360_IN_REPORT));
            }
            else if (report->header.command==0x20)
            {
                convertFromXboxOne(report, report->header.size);
                XBOX360_IN_REPORT *report360=(XBOX360_IN_REPORT*)report;
                if (!(GetOwner(this)->noMapping))
                    remapButtons(report360);
                GetOwner(this)->fiddleReport(report360->left, report360->right);

                if (GetOwner(this)->swapSticks)
                    remapAxes(report360);

                memcpy(lastData, report360, sizeof(XBOX360_IN_REPORT));
            }
        }
    }
    IOReturn ret = IOHIDDevice::handleReport(descriptor, reportType, options);
    return ret;
}

IOReturn XboxOneControllerClass::setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    //    IOLog("Xbox One Controller - setReport\n");
    unsigned char data[4];
    report->readBytes(0, &data, 4);
//        IOLog("Attempting to send: %d %d %d %d\n",((unsigned char*)data)[0], ((unsigned char*)data)[1], ((unsigned char*)data)[2], ((unsigned char*)data)[3]);
    UInt8 rumbleType;
    switch(data[0])//(header.command)
    {
        case 0x00:  // Set force feedback
            XBOXONE_OUT_RUMBLE rumble;
            rumble.header.command = 0x09;
            rumble.header.reserved1 = 0x00;
            rumble.header.counter = (GetOwner(this)->outCounter)++;
            rumble.header.size = 0x09;
            rumble.mode = 0x00;
            rumble.rumbleMask = 0x0F;
            rumble.length = 0xFF;
            rumble.period = 0x00;
            rumble.extra = 0x00;
//            IOLog("Data: %d %d %d %d, outCounter: %d\n", data[0], data[1], data[2], data[3], rumble.reserved2);

            rumbleType = GetOwner(this)->rumbleType;
            if (rumbleType == 0) // Default
            {
                rumble.trigL = 0x00;
                rumble.trigR = 0x00;
                rumble.little = data[2];
                rumble.big = data[3];
            }
            else if (rumbleType == 1) // None
            {
                return kIOReturnSuccess;
            }
            else if (rumbleType == 2) // Trigger
            {
                rumble.trigL = data[2] / 2.0;
                rumble.trigR = data[3] / 2.0;
                rumble.little = 0x00;
                rumble.big = 0x00;
            }
            else if (rumbleType == 3) // Both
            {
                rumble.trigL = data[2] / 2.0;
                rumble.trigR = data[3] / 2.0;
                rumble.little = data[2];
                rumble.big = data[3];
            }

            GetOwner(this)->QueueWrite(&rumble,13);
            return kIOReturnSuccess;
        case 0x01: // Unsupported LED
            return kIOReturnSuccess;
        default:
            IOLog("Unknown escape %d\n", data[0]);
            return kIOReturnUnsupported;
    }
}


#pragma mark - XboxOnePretend360Class

/*
 * Xbox One controller.
 * Fake PID and VID of Xbox 360 controller
 */

OSDefineMetaClassAndStructors(XboxOnePretend360Class, XboxOneControllerClass)

OSString* XboxOnePretend360Class::newProductString() const
{
    return OSString::withCString("Xbox 360 Wired Controller");
}

OSNumber* XboxOnePretend360Class::newProductIDNumber() const
{
    return OSNumber::withNumber(654,16);
}

OSNumber* XboxOnePretend360Class::newVendorIDNumber() const
{
    return OSNumber::withNumber(1118,16);
}
