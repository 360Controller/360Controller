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
    const IOUSBEndpointDescriptor *ed;
    
    ed=pipe->GetEndpointDescriptor();
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
	if (device->DeviceRequest(&controlReq, 100, 100, NULL) == kIOReturnSuccess)
        return true;
    IOLog("start - failed to %s chatpad setting\n", sendOut ? "write" : "read");
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
    OSDictionary *dataDictionary;
    OSBoolean *value;
    OSNumber *number;
    
    dataDictionary=OSDynamicCast(OSDictionary,getProperty(kDriverSettingKey));
    if(dataDictionary==NULL) return;
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("InvertLeftX"));
    if(value!=NULL) invertLeftX=value->getValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("InvertLeftY"));
    if(value!=NULL) invertLeftY=value->getValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("InvertRightX"));
    if(value!=NULL) invertRightX=value->getValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("InvertRightY"));
    if(value!=NULL) invertRightY=value->getValue();
    number=OSDynamicCast(OSNumber,dataDictionary->getObject("DeadzoneLeft"));
    if(number!=NULL) deadzoneLeft=number->unsigned32BitValue();
    number=OSDynamicCast(OSNumber,dataDictionary->getObject("DeadzoneRight"));
    if(number!=NULL) deadzoneRight=number->unsigned32BitValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("RelativeLeft"));
    if(value!=NULL) relativeLeft=value->getValue();
    value=OSDynamicCast(OSBoolean,dataDictionary->getObject("RelativeRight"));
    if(value!=NULL) relativeRight=value->getValue();
    /*
    IOLog("Xbox360Peripheral preferences loaded:\n  invertLeft X: %s, Y: %s\n   invertRight X: %s, Y:%s\n  deadzone Left: %d, Right: %d\n\n",
            invertLeftX?"True":"False",invertLeftY?"True":"False",
            invertRightX?"True":"False",invertRightY?"True":"False",
            deadzoneLeft,deadzoneRight);
    */
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
    invertLeftX=invertLeftY=FALSE;
    invertRightX=invertRightY=FALSE;
    deadzoneLeft=deadzoneRight=0;
    relativeLeft=relativeRight=FALSE;
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
                IOLog("Unknown device release %.4x", release);
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
        IOLog("start - unable to find the interface\n");
        goto fail;
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

// Handle message sent to the driver
IOReturn Xbox360Peripheral::message(UInt32 type,IOService *provider,void *argument)
{
    switch(type) {
        case kIOMessageServiceIsTerminated:
        case kIOMessageServiceIsRequestingClose:
            if(device->isOpen(this)) ReleaseAll();
            return kIOReturnSuccess;
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
        OSDictionary *dictionary = OSDictionary::withObjects(objects, keys, sizeof(keys) / sizeof(keys[0]), 0);
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
