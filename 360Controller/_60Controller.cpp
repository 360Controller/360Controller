/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    Bug fixes contributed by Cody "codeman38" Boisclair
    
    _60Controller.cpp - main source of the driver
    
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
#include <IOKit/IOLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOTimerEventSource.h>
#include "_60Controller.h"
#include "ControlStruct.h"
#include "ChatPad.h"
#include "Controller.h"

#define kDriverSettingKey       "DeviceData"

#define kIOSerialDeviceType   "Serial360Device"

OSDefineMetaClassAndStructors(Xbox360Peripheral, IOService)
#define super IOService

static void convertFromXBoxOriginal(UInt8 *data);

typedef struct {
    XBOX360_PACKET header;
    XBox360_Byte buttons;
    XBox360_Byte reserved1;
    XBox360_Byte a, b, x, y, black, white;
    XBox360_Byte trigL,trigR;
    XBox360_Short xL,yL;
    XBox360_Short xR,yR;
} PACKED XBOX_IN_REPORT;

typedef struct {
    XBOX360_PACKET header;
    XBox360_Byte reserved1;
    XBox360_Byte left;
    XBox360_Byte reserved2;
    XBox360_Byte right;
} PACKED XBOX_OUT_RUMBLE;

class LockRequired
{
private:
    IOLock *_lock;
public:
    LockRequired(IOLock *lock)
    {
        _lock = lock;
        IOLockLock(_lock);
    }
    ~LockRequired()
    {
        IOLockUnlock(_lock);
    }
};

// Find the maximum packet size of this pipe
static UInt32 GetMaxPacketSize(IOUSBPipe *pipe)
{
    const IOUSBEndpointDescriptor *ed = pipe->GetEndpointDescriptor();
    
    if(ed==NULL) return 0;
    else return ed->wMaxPacketSize;
}

void Xbox360Peripheral::SendSpecial(UInt16 value)
{
	IOUSBDevRequest controlReq;

	controlReq.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface);
	controlReq.bRequest = 0x00;
	controlReq.wValue = value;
	controlReq.wIndex = 0x0002;
	controlReq.wLength = 0;
	controlReq.pData = NULL;
	if (device->DeviceRequest(&controlReq, 100, 100, NULL) != kIOReturnSuccess)
		IOLog("Failed to send special message %.4x\n", value);
}

void Xbox360Peripheral::SendInit(UInt16 value, UInt16 index)
{
	IOUSBDevRequest controlReq;
	
	controlReq.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	controlReq.bRequest = 0xa9;
	controlReq.wValue = value;
	controlReq.wIndex = index;
	controlReq.wLength = 0;
	controlReq.pData = NULL;
	device->DeviceRequest(&controlReq, 100, 100, NULL);	// Will fail - but device should still act on it
}

bool Xbox360Peripheral::SendSwitch(bool sendOut)
{
	IOUSBDevRequest controlReq;

	controlReq.bmRequestType = USBmakebmRequestType(sendOut ? kUSBOut : kUSBIn, kUSBVendor, kUSBDevice);
	controlReq.bRequest = 0xa1;
	controlReq.wValue = 0x0000;
	controlReq.wIndex = 0xe416;
	controlReq.wLength = sizeof(chatpadInit);
	controlReq.pData = chatpadInit;
    IOReturn err = device->DeviceRequest(&controlReq, 100, 100, NULL);
    if (err == kIOReturnSuccess)
        return true;

    const char *errStr = stringFromReturn(err);
    IOLog("start - failed to %s chatpad setting (%s)\n", sendOut ? "write" : "read", errStr);
    return false;
}

void Xbox360Peripheral::SendToggle(void)
{
	SendSpecial(serialToggle ? 0x1F : 0x1E);
	serialToggle = !serialToggle;
}

void Xbox360Peripheral::ChatPadTimerActionWrapper(OSObject *owner, IOTimerEventSource *sender)
{
	Xbox360Peripheral *controller;

	controller = OSDynamicCast(Xbox360Peripheral, owner);
	controller->ChatPadTimerAction(sender);
}

void Xbox360Peripheral::ChatPadTimerAction(IOTimerEventSource *sender)
{
	int nextTime, serialGot;
	
	serialGot = 0;
	nextTime = 1000;
	switch (serialTimerState)
	{
		case tsToggle:
			SendToggle();
			if (serialActive)
			{
				if (!serialHeard)
				{
					serialActive = false;
					serialGot = 2;
				}
			}
			else
			{
				if (serialHeard)
				{
					serialTimerState = tsReset1;
					serialResetCount = 0;
					nextTime = 40;
				}
			}
			break;
			
		case tsMiniToggle:
			SendToggle();
			if (serialHeard)
			{
				serialTimerState = tsSet1;
				nextTime = 40;
			}
			else
			{
				serialResetCount++;
				if (serialResetCount > 3)
				{
					serialTimerState = tsToggle;
				}
				else
				{
					serialTimerState = tsReset1;
					nextTime = 40;
				}
			}
			break;
			
		case tsReset1:
			SendSpecial(0x1B);
			serialTimerState = tsReset2;
			nextTime = 35;
			break;
			
		case tsReset2:
			SendSpecial(0x1B);
			serialTimerState = tsMiniToggle;
			nextTime = 150;
			break;
			
		case tsSet1:
			SendSpecial(0x18);
			serialTimerState = tsSet2;
			nextTime = 10;
			break;
			
		case tsSet2:
			SendSpecial(0x10);
			serialTimerState = tsSet3;
			nextTime = 10;
			break;
			
		case tsSet3:
			SendSpecial(0x03);
			serialTimerState = tsToggle;
			nextTime = 940;
			serialActive = true;
			serialGot = 1;
			break;
	}
	sender->setTimeoutMS(nextTime);	// Todo: Make it take into account function execution time?
	serialHeard = false;
	// Make it happen after the timer's set, for minimum impact
	switch (serialGot)
	{
		case 1:
			SerialConnect();
			break;
			
		case 2:
			SerialDisconnect();
			break;
			
		default:
			break;
	}
}

// Read the settings from the registry
void Xbox360Peripheral::readSettings(void)
{
    OSBoolean *value = NULL;
    OSNumber *number = NULL;
    OSDictionary *dataDictionary = OSDynamicCast(OSDictionary, getProperty(kDriverSettingKey));
    
    if (dataDictionary == NULL) return;
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertLeftX"));
    if (value != NULL) invertLeftX = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertLeftY"));
    if (value != NULL) invertLeftY = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertRightX"));
    if (value != NULL) invertRightX = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertRightY"));
    if (value != NULL) invertRightY = value->getValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("DeadzoneLeft"));
    if (number != NULL) deadzoneLeft = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("DeadzoneRight"));
    if (number != NULL) deadzoneRight = number->unsigned32BitValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("RelativeLeft"));
    if (value != NULL) relativeLeft = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("RelativeRight"));
    if (value != NULL) relativeRight=value->getValue();
    
#if 0
    IOLog("Xbox360Peripheral preferences loaded:\n  invertLeft X: %s, Y: %s\n   invertRight X: %s, Y:%s\n  deadzone Left: %d, Right: %d\n\n",
            invertLeftX?"True":"False",invertLeftY?"True":"False",
            invertRightX?"True":"False",invertRightY?"True":"False",
            deadzoneLeft,deadzoneRight);
#endif
}

// Initialise the extension
bool Xbox360Peripheral::init(OSDictionary *propTable)
{
    bool res=super::init(propTable);
    mainLock = IOLockAlloc();
    device=NULL;
    interface=NULL;
    inPipe=NULL;
    outPipe=NULL;
    inBuffer=NULL;
	padHandler = NULL;
	serialIn = NULL;
	serialInPipe = NULL;
	serialInBuffer = NULL;
	serialTimer = NULL;
	serialHandler = NULL;
    // Default settings
    invertLeftX=invertLeftY=false;
    invertRightX=invertRightY=false;
    deadzoneLeft=deadzoneRight=0;
    relativeLeft=relativeRight=false;
    xboxOriginal=FALSE;
    // Done
    return res;
}

// Free the extension
void Xbox360Peripheral::free(void)
{
    IOLockFree(mainLock);
    super::free();
}

bool Xbox360Peripheral::start(IOService *provider)
{
    const IOUSBConfigurationDescriptor *cd;
    IOUSBFindInterfaceRequest intf;
    IOUSBFindEndpointRequest pipe;
    XBOX360_OUT_LED led;
    IOWorkLoop *workloop = NULL;
    
    if (!super::start(provider))
		return false;
    // Get device
    device=OSDynamicCast(IOUSBDevice,provider);
    if(device==NULL) {
        IOLog("start - invalid provider\n");
        goto fail;
    }
    // Check for configurations
    if(device->GetNumConfigurations()<1) {
        device=NULL;
        IOLog("start - device has no configurations!\n");
        goto fail;
    }
    // Set configuration
    cd=device->GetFullConfigurationDescriptor(0);
    if(cd==NULL) {
        device=NULL;
        IOLog("start - couldn't get configuration descriptor\n");
        goto fail;
    }
    // Open
    if(!device->open(this)) {
        device=NULL;
        IOLog("start - unable to open device\n");
        goto fail;
    }
    if(device->SetConfiguration(this,cd->bConfigurationValue,true)!=kIOReturnSuccess) {
        IOLog("start - unable to set configuration\n");
        goto fail;
    }
    // Get release
    {
        UInt16 release = device->GetDeviceRelease();
        switch (release) {
            default:
                IOLog("Unknown device release %.4x\n", release);
                // fall through
            case 0x0110:
                chatpadInit[0] = 0x01;
                chatpadInit[1] = 0x02;
                break;
            case 0x0114:
                chatpadInit[0] = 0x09;
                chatpadInit[1] = 0x00;
                break;
        }
    }
    // Find correct interface
    intf.bInterfaceClass=kIOUSBFindInterfaceDontCare;
    intf.bInterfaceSubClass=93;
    intf.bInterfaceProtocol=1;
    intf.bAlternateSetting=kIOUSBFindInterfaceDontCare;
    interface=device->FindNextInterface(NULL,&intf);
    if(interface==NULL) {
        IOLog("start - unable to find the interface, trying xbox orginal\n");
        // Find correct interface
        intf.bInterfaceClass=kIOUSBFindInterfaceDontCare;
        intf.bInterfaceSubClass=66;
        intf.bInterfaceProtocol=0;
        intf.bAlternateSetting=kIOUSBFindInterfaceDontCare;
        interface=device->FindNextInterface(NULL,&intf);
        if(interface==NULL) {
            IOLog("start - unable to find the interface\n");
            goto fail;
        }
        xboxOriginal = true;
    }
    interface->open(this);
    // Find pipes
    pipe.direction=kUSBIn;
    pipe.interval=0;
    pipe.type=kUSBInterrupt;
    pipe.maxPacketSize=0;
    inPipe=interface->FindNextPipe(NULL,&pipe);
    if(inPipe==NULL) {
        IOLog("start - unable to find in pipe\n");
        goto fail;
    }
    inPipe->retain();
    pipe.direction=kUSBOut;
    outPipe=interface->FindNextPipe(NULL,&pipe);
    if(outPipe==NULL) {
        IOLog("start - unable to find out pipe\n");
        goto fail;
    }
    outPipe->retain();
    // Get a buffer
    inBuffer=IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task,0,GetMaxPacketSize(inPipe));
    if(inBuffer==NULL) {
        IOLog("start - failed to allocate input buffer\n");
        goto fail;
    }
	// Find chatpad interface
	intf.bInterfaceClass = kIOUSBFindInterfaceDontCare;
	intf.bInterfaceSubClass = 93;
	intf.bInterfaceProtocol = 2;
	intf.bAlternateSetting = kIOUSBFindInterfaceDontCare;
	serialIn = device->FindNextInterface(NULL, &intf);
	if (serialIn == NULL) {
		IOLog("start - unable to find chatpad interface\n");
        goto nochat;
    }
	serialIn->open(this);
	// Find chatpad pipe
	pipe.direction = kUSBIn;
	pipe.interval = 0;
	pipe.type = kUSBInterrupt;
	pipe.maxPacketSize = 0;
	serialInPipe = serialIn->FindNextPipe(NULL, &pipe);
	if (serialInPipe == NULL)
	{
		IOLog("start - unable to find chatpad in pipe\n");
		goto fail;
	}
	serialInPipe->retain();
	// Get a buffer for the chatpad
	serialInBuffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, GetMaxPacketSize(serialInPipe));
	if (serialInBuffer == NULL)
	{
		IOLog("start - failed to allocate input buffer for chatpad\n");
		goto fail;
	}
	// Create timer for chatpad
	serialTimer = IOTimerEventSource::timerEventSource(this, ChatPadTimerActionWrapper);
	if (serialTimer == NULL)
	{
		IOLog("start - failed to create timer for chatpad\n");
		goto fail;
	}
    workloop = getWorkLoop();
	if ((workloop == NULL) || (workloop->addEventSource(serialTimer) != kIOReturnSuccess))
	{
		IOLog("start - failed to connect timer for chatpad\n");
		goto fail;
	}
	// Configure ChatPad
		// Send 'configuration'
	SendInit(0xa30c, 0x4423);
	SendInit(0x2344, 0x7f03);
	SendInit(0x5839, 0x6832);
		// Set 'switch'
    if ((!SendSwitch(false)) || (!SendSwitch(true)) || (!SendSwitch(false)))
        goto fail;
		// Begin toggle
	serialHeard = false;
	serialActive = false;
	serialToggle = false;
	serialResetCount = 0;
	serialTimerState = tsToggle;
	serialTimer->setTimeoutMS(1000);
    // Begin reading
    if (!QueueSerialRead())
        goto fail;
nochat:
    if (!QueueRead())
		goto fail;
    // Disable LED
    Xbox360_Prepare(led,outLed);
    led.pattern=ledOff;
    QueueWrite(&led,sizeof(led));
    // Done
	PadConnect();
	registerService();
    return true;
fail:
    ReleaseAll();
    return false;
}

// Set up an asynchronous read
bool Xbox360Peripheral::QueueRead(void)
{
    IOUSBCompletion complete;
    IOReturn err;

    if ((inPipe == NULL) || (inBuffer == NULL))
        return false;
    complete.target=this;
    complete.action=ReadCompleteInternal;
    complete.parameter=inBuffer;
    err=inPipe->Read(inBuffer,0,0,inBuffer->getLength(),&complete);
    if(err==kIOReturnSuccess) return true;
    else {
        IOLog("read - failed to start (0x%.8x)\n",err);
        return false;
    }
}

bool Xbox360Peripheral::QueueSerialRead(void)
{
    IOUSBCompletion complete;
    IOReturn err;
	
    if ((serialInPipe == NULL) || (serialInBuffer == NULL))
        return false;
    complete.target = this;
    complete.action = SerialReadCompleteInternal;
    complete.parameter = serialInBuffer;
    err = serialInPipe->Read(serialInBuffer, 0, 0, serialInBuffer->getLength(), &complete);
    if (err == kIOReturnSuccess)
	{
		return true;
	}
    else
	{
        IOLog("read - failed to start for chatpad (0x%.8x)\n",err);
        return false;
    }
}

// Set up an asynchronous write
bool Xbox360Peripheral::QueueWrite(const void *bytes,UInt32 length)
{
    IOBufferMemoryDescriptor *outBuffer;
    IOUSBCompletion complete;
    IOReturn err;
    
    outBuffer=IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task,0,length);
    if(outBuffer==NULL) {
        IOLog("send - unable to allocate buffer\n");
        return false;
    }
    outBuffer->writeBytes(0,bytes,length);
    complete.target=this;
    complete.action=WriteCompleteInternal;
    complete.parameter=outBuffer;
    err=outPipe->Write(outBuffer,0,0,length,&complete);
    if(err==kIOReturnSuccess) return true;
    else {
        IOLog("send - failed to start (0x%.8x)\n",err);
        return false;
    }
}

void Xbox360Peripheral::stop(IOService *provider)
{
    ReleaseAll();
    super::stop(provider);
}

// Releases all the objects used
void Xbox360Peripheral::ReleaseAll(void)
{
    LockRequired locker(mainLock);
    
	SerialDisconnect();
	PadDisconnect();
	if (serialTimer != NULL)
	{
		serialTimer->cancelTimeout();
		getWorkLoop()->removeEventSource(serialTimer);
		serialTimer->release();
		serialTimer = NULL;
	}
	if (serialInPipe != NULL)
	{
		serialInPipe->Abort();
		serialInPipe->release();
		serialInPipe = NULL;
	}
	if (serialInBuffer != NULL)
	{
		serialInBuffer->release();
		serialInBuffer = NULL;
	}
	if (serialIn != NULL)
	{
		serialIn->close(this);
		serialIn = NULL;
	}
    if(outPipe!=NULL) {
        outPipe->Abort();
        outPipe->release();
        outPipe=NULL;
    }
    if(inPipe!=NULL) {
        inPipe->Abort();
        inPipe->release();
        inPipe=NULL;
    }
    if(inBuffer!=NULL) {
        inBuffer->release();
        inBuffer=NULL;
    }
    if(interface!=NULL) {
        interface->close(this);
        interface=NULL;
    }
    if(device!=NULL) {
        device->close(this);
        device=NULL;
    }
}

// Handle termination
bool Xbox360Peripheral::didTerminate(IOService *provider, IOOptionBits options, bool *defer)
{
    // release all objects used and close the device
    ReleaseAll();
    return super::didTerminate(provider, options, defer);
}


// Handle message sent to the driver
IOReturn Xbox360Peripheral::message(UInt32 type,IOService *provider,void *argument)
{
    switch(type) {
        case kIOMessageServiceIsTerminated:
        case kIOMessageServiceIsRequestingClose:
        default:
            return super::message(type,provider,argument);
    }
}

// This returns the abs() value of a short, swapping it if necessary
static inline XBox360_SShort getAbsolute(XBox360_SShort value)
{
    XBox360_SShort reverse;
    
#ifdef __LITTLE_ENDIAN__
    reverse=value;
#elif __BIG_ENDIAN__
    reverse=((value&0xFF00)>>8)|((value&0x00FF)<<8);
#else
#error Unknown CPU byte order
#endif
    return (reverse<0)?~reverse:reverse;
}

// Adjusts the report for any settings speciified by the user
void Xbox360Peripheral::fiddleReport(IOBufferMemoryDescriptor *buffer)
{
    XBOX360_IN_REPORT *report=(XBOX360_IN_REPORT*)buffer->getBytesNoCopy();
    if (xboxOriginal) {
        convertFromXBoxOriginal((UInt8 *)report);
    }
    if(invertLeftX) report->left.x=~report->left.x;
    if(!invertLeftY) report->left.y=~report->left.y;
    if(invertRightX) report->right.x=~report->right.x;
    if(!invertRightY) report->right.y=~report->right.y;
    if(deadzoneLeft!=0) {
        if(relativeLeft) {
            if((getAbsolute(report->left.x)<deadzoneLeft)&&(getAbsolute(report->left.y)<deadzoneLeft)) {
                report->left.x=0;
                report->left.y=0;
            }
        } else {
            if(getAbsolute(report->left.x)<deadzoneLeft) report->left.x=0;
            if(getAbsolute(report->left.y)<deadzoneLeft) report->left.y=0;
        }
    }
    if(deadzoneRight!=0) {
        if(relativeRight) {
            if((getAbsolute(report->right.x)<deadzoneRight)&&(getAbsolute(report->right.y)<deadzoneRight)) {
                report->right.x=0;
                report->right.y=0;
            }
        } else {
            if(getAbsolute(report->right.x)<deadzoneRight) report->right.x=0;
            if(getAbsolute(report->right.y)<deadzoneRight) report->right.y=0;
        }
    }
}

// This converts XBox original controller report into XBox360 form
// See https://github.com/Grumbel/xboxdrv/blob/master/src/controller/xbox_controller.cpp
static void convertFromXBoxOriginal(UInt8 *data) {
    if (data[0] != 0x00 || data[1] != 0x14) {
        IOLog("Unknown report command %d, length %d\n", (int)data[0], (int)data[1]);
        return;
    }
    XBOX360_IN_REPORT report;
    XBOX_IN_REPORT *in = (XBOX_IN_REPORT*)data;
    report.header.command = 0;
    report.header.size = sizeof(XBOX360_IN_REPORT);
    XBox360_Short buttons = in->buttons;
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

IOReturn Xbox360Peripheral::setRumble(UInt8 big, UInt8 little)
{
    //IOLog("Set rumble: big(%d) little(%d)\n", big, little);
    if (xboxOriginal) {
        XBOX_OUT_RUMBLE rumble;
        Xbox360_Prepare(rumble,outRumble);
        rumble.left=big; // TODO check ? left, right != big, little
        rumble.right=little;
        QueueWrite(&rumble,sizeof(rumble));
    } else {
        XBOX360_OUT_RUMBLE rumble;
        Xbox360_Prepare(rumble,outRumble);
        rumble.big=big;
        rumble.little=little;
        QueueWrite(&rumble,sizeof(rumble));
    }
    return kIOReturnSuccess;
}

IOReturn Xbox360Peripheral::setLeds(UInt8 leds)
{
    //IOLog("Set LED: %d\n", leds);
    if (xboxOriginal) {
      // no leds
      return kIOReturnSuccess;
    } else {
        XBOX360_OUT_LED led;
        Xbox360_Prepare(led,outLed);
        led.pattern=leds;
        QueueWrite(&led,sizeof(led));
    }
    return kIOReturnSuccess;
}

static IOUSBDevice* GetOwnerProvider(const IOService *us)
{
    IOService *prov = us->getProvider();
    if (prov == NULL)
        return NULL;
    return OSDynamicCast(IOUSBDevice, prov);
}

// Returns the string for the specified index from the USB device's string list, with an optional default
OSString* Xbox360Peripheral::getDeviceString(UInt8 index,const char *def) const
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

OSString* Xbox360Peripheral::getManufacturerString()
{
    if (xboxOriginal) return OSString::withCString("Holtek");
    return getDeviceString(GetOwnerProvider(this)->GetManufacturerStringIndex());
}

OSNumber* Xbox360Peripheral::getProductIDNumber()
{
    if (xboxOriginal) return OSNumber::withNumber(654,16);
    return OSNumber::withNumber(GetOwnerProvider(this)->GetProductID(),16);
}

OSString* Xbox360Peripheral::getProductString()
{
    if (xboxOriginal) return OSString::withCString("Xbox 360 Wired Controller");
    OSString *retString = getDeviceString(GetOwnerProvider(this)->GetProductStringIndex());
    if (retString->isEqualTo("Controller")) {
        retString->release();
        return OSString::withCString("Xbox 360 Wired Controller");
    } else {
        return retString;
    }
}

OSString* Xbox360Peripheral::getSerialNumberString()
{
    return getDeviceString(GetOwnerProvider(this)->GetSerialNumberStringIndex());
}

OSString* Xbox360Peripheral::getTransportString()
{
    return OSString::withCString("USB");
}

OSNumber* Xbox360Peripheral::getVendorIDNumber()
{
    if (xboxOriginal) return OSNumber::withNumber(1118,16);
    return OSNumber::withNumber(GetOwnerProvider(this)->GetVendorID(),16);
}

OSNumber* Xbox360Peripheral::getLocationIDNumber()
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



// This forwards a completed read notification to a member function
void Xbox360Peripheral::ReadCompleteInternal(void *target,void *parameter,IOReturn status,UInt32 bufferSizeRemaining)
{
    if(target!=NULL)
        ((Xbox360Peripheral*)target)->ReadComplete(parameter,status,bufferSizeRemaining);
}

void Xbox360Peripheral::SerialReadCompleteInternal(void *target, void *parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
	if (target != NULL)
		((Xbox360Peripheral*)target)->SerialReadComplete(parameter, status, bufferSizeRemaining);
}

// This forwards a completed write notification to a member function
void Xbox360Peripheral::WriteCompleteInternal(void *target,void *parameter,IOReturn status,UInt32 bufferSizeRemaining)
{
    if(target!=NULL)
        ((Xbox360Peripheral*)target)->WriteComplete(parameter,status,bufferSizeRemaining);
}

// This handles a completed asynchronous read
void Xbox360Peripheral::ReadComplete(void *parameter,IOReturn status,UInt32 bufferSizeRemaining)
{
	if (padHandler != NULL) // avoid deadlock with release
	{
		LockRequired locker(mainLock);
		IOReturn err;
		bool reread=!isInactive();
		
		switch(status) {
			case kIOReturnOverrun:
				IOLog("read - kIOReturnOverrun, clearing stall\n");
				if (inPipe != NULL)
					inPipe->ClearStall();
				// Fall through
			case kIOReturnSuccess:
				if (inBuffer != NULL)
				{
					const XBOX360_IN_REPORT *report=(const XBOX360_IN_REPORT*)inBuffer->getBytesNoCopy();
					if((report->header.command==inReport)&&(report->header.size==sizeof(XBOX360_IN_REPORT))) {
						fiddleReport(inBuffer);
						err = padHandler->handleReport(inBuffer, kIOHIDReportTypeInput);
						if(err!=kIOReturnSuccess) {
							IOLog("read - failed to handle report: 0x%.8x\n",err);
						}
					}
				}
				break;
			case kIOReturnNotResponding:
				IOLog("read - kIOReturnNotResponding\n");
				reread=false;
				break;
			default:
				reread=false;
				break;
		}
		if(reread) QueueRead();
    }
}

void Xbox360Peripheral::SerialReadComplete(void *parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
	if (padHandler != NULL) // avoid deadlock with release
	{
		LockRequired locker(mainLock);
		bool reread = !isInactive();
	
		switch (status)
		{
			case kIOReturnOverrun:
				IOLog("read (serial) - kIOReturnOverrun, clearing stall\n");
				if (serialInPipe != NULL)
					serialInPipe->ClearStall();
				// Fall through
			case kIOReturnSuccess:
				serialHeard = true;
				if (serialInBuffer != NULL)
					SerialMessage(serialInBuffer, serialInBuffer->getCapacity() - bufferSizeRemaining);
				break;
				
			case kIOReturnNotResponding:
				IOLog("read (serial) - kIOReturnNotResponding\n");
				reread = false;
				break;
				
			default:
				reread = false;
				break;
		}
		if (reread)
			QueueSerialRead();
	}
}

// Handle a completed asynchronous write
void Xbox360Peripheral::WriteComplete(void *parameter,IOReturn status,UInt32 bufferSizeRemaining)
{
    IOMemoryDescriptor *memory=(IOMemoryDescriptor*)parameter;
    if(status!=kIOReturnSuccess) {
        IOLog("write - Error writing: 0x%.8x\n",status);
    }
    memory->release();
}


// Called by the userspace IORegistryEntrySetCFProperties function
IOReturn Xbox360Peripheral::setProperties(OSObject *properties)
{
    OSDictionary *dictionary;
    
    dictionary=OSDynamicCast(OSDictionary,properties);
    if(dictionary!=NULL) {
        setProperty(kDriverSettingKey,dictionary);
        readSettings();
        return kIOReturnSuccess;
    } else return kIOReturnBadArgument;
}

IOHIDDevice* Xbox360Peripheral::getController(int index)
{
	switch (index)
	{
		case 0:
			return padHandler;
		case 1:
			return serialHandler;
		default:
			return NULL;
	}
}

// Main controller support

void Xbox360Peripheral::PadConnect(void)
{
	PadDisconnect();
	padHandler = new Xbox360ControllerClass;
	if (padHandler != NULL)
	{
        const OSString *keys[] = {
            OSString::withCString(kIOSerialDeviceType),
			OSString::withCString("IOCFPlugInTypes"),
			OSString::withCString("IOKitDebug"),
        };
        const OSObject *objects[] = {
            OSNumber::withNumber((unsigned long long)1, 32),
			getProperty("IOCFPlugInTypes"),
            OSNumber::withNumber((unsigned long long)65535, 32),
        };
        OSDictionary *dictionary = OSDictionary::withObjects(objects, keys, sizeof(keys) / sizeof(keys[0]));
		if (padHandler->init(dictionary))
		{
			padHandler->attach(this);
			padHandler->start(this);
		}
		else
		{
			padHandler->release();
			padHandler = NULL;
		}
	}
}

void Xbox360Peripheral::PadDisconnect(void)
{
	if (padHandler != NULL)
	{
		padHandler->terminate(kIOServiceRequired | kIOServiceSynchronous);
		padHandler->release();
		padHandler = NULL;
	}
}

// Serial peripheral support

void Xbox360Peripheral::SerialConnect(void)
{
	SerialDisconnect();
	serialHandler = new ChatPadKeyboardClass;
    if (serialHandler != NULL)
    {
        const OSString *keys[] = {
            OSString::withCString(kIOSerialDeviceType),
        };
        const OSObject *objects[] = {
            OSNumber::withNumber((unsigned long long)0, 32),
        };
        OSDictionary *dictionary = OSDictionary::withObjects(objects, keys, sizeof(keys) / sizeof(keys[0]), 0);
        if (serialHandler->init(dictionary))
        {
            serialHandler->attach(this);
			serialHandler->start(this);
        }
        else
        {
            serialHandler->release();
            serialHandler = NULL;
        }
    }
}

void Xbox360Peripheral::SerialDisconnect(void)
{
	if (serialHandler != NULL)
	{
        // Hope it's okay to terminate twice...
		serialHandler->terminate(kIOServiceRequired | kIOServiceSynchronous);
		serialHandler->release();
		serialHandler = NULL;
	}
}

void Xbox360Peripheral::SerialMessage(IOBufferMemoryDescriptor *data, size_t length)
{
	if (serialHandler != NULL)
	{
		char *buffer = (char*)data->getBytesNoCopy();
		if ((length == 5) && (buffer[0] == 0x00))
			serialHandler->handleReport(data, kIOHIDReportTypeInput);
	}
}
