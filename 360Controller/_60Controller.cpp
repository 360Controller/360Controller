/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006 Colin Munro
    
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
#include "_60Controller.h"
#include "ControlStruct.h"
#include "xbox360hid.h"

#define kDriverSettingKey       "DeviceData"

OSDefineMetaClassAndStructors(Xbox360ControllerClass, IOHIDDevice)
#define super IOHIDDevice

// Find the maximum packet size of this pipe
static UInt32 GetMaxPacketSize(IOUSBPipe *pipe)
{
    const IOUSBEndpointDescriptor *ed;
    
    ed=pipe->GetEndpointDescriptor();
    if(ed==NULL) return 0;
    else return ed->wMaxPacketSize;
}

// Read the settings from the registry
void Xbox360ControllerClass::readSettings(void)
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
    IOLog("Xbox360ControllerClass preferences loaded:\n  invertLeft X: %s, Y: %s\n   invertRight X: %s, Y:%s\n  deadzone Left: %d, Right: %d\n\n",
            invertLeftX?"True":"False",invertLeftY?"True":"False",
            invertRightX?"True":"False",invertRightY?"True":"False",
            deadzoneLeft,deadzoneRight);
    */
}

// Initialise the extension
bool Xbox360ControllerClass::init(OSDictionary *propTable)
{
    bool res=super::init(propTable);
    device=NULL;
    interface=NULL;
    inPipe=NULL;
    outPipe=NULL;
    inBuffer=NULL;
    // Default settings
    invertLeftX=invertLeftY=FALSE;
    invertRightX=invertRightY=FALSE;
    deadzoneLeft=deadzoneRight=0;
    relativeLeft=relativeRight=FALSE;
    readSettings();
    // Done
    return res;
}

// Free the extension
void Xbox360ControllerClass::free(void)
{
    // Don't actually do anything yet
    super::free();
}

IOService* Xbox360ControllerClass::probe(IOService *provider, SInt32 *score )
{
    IOService *res=super::probe(provider,score);
    return res;
}

//bool Xbox360ControllerClass::start(IOService *provider)
bool Xbox360ControllerClass::handleStart(IOService *provider)
{
    const IOUSBConfigurationDescriptor *cd;
    IOUSBFindInterfaceRequest intf;
    IOUSBFindEndpointRequest pipe;
    XBOX360_OUT_LED led;
    
    if(!super::handleStart(provider)) return false;
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
    // Begin reading
    if(!QueueRead()) goto fail;
    // Disable LED
    Xbox360_Prepare(led,outLed);
    led.pattern=ledOff;
    QueueWrite(&led,sizeof(led));
    // Done
    readSettings();
    return true;
fail:
    ReleaseAll();
    return false;
}

// Set up an asynchronous read
bool Xbox360ControllerClass::QueueRead(void)
{
    IOUSBCompletion complete;
    IOReturn err;

    complete.target=this;
    complete.action=ReadCompleteInternal;
    complete.parameter=inBuffer;
    err=inPipe->Read(inBuffer,0,0,inBuffer->getLength(),&complete);
    if(err==kIOReturnSuccess) return true;
    else {
        IOLog("read - failed to start (0x%.8x\n",err);
        return false;
    }
}

// Set up an asynchronous write
bool Xbox360ControllerClass::QueueWrite(const void *bytes,UInt32 length)
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

//void Xbox360ControllerClass::stop(IOService *provider)
void Xbox360ControllerClass::handleStop(IOService *provider)
{
//    IOLog("Stopping\n");
    ReleaseAll();
    super::handleStop(provider);
}

// Releases all the objects used
void Xbox360ControllerClass::ReleaseAll(void)
{
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
IOReturn Xbox360ControllerClass::message(UInt32 type,IOService *provider,void *argument)
{
//    IOLog("Message\n");
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
void Xbox360ControllerClass::fiddleReport(IOBufferMemoryDescriptor *buffer)
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
void Xbox360ControllerClass::ReadCompleteInternal(void *target,void *parameter,IOReturn status,UInt32 bufferSizeRemaining)
{
    if(target!=NULL)
        ((Xbox360ControllerClass*)target)->ReadComplete(parameter,status,bufferSizeRemaining);
}

// This forwards a completed write notification to a member function
void Xbox360ControllerClass::WriteCompleteInternal(void *target,void *parameter,IOReturn status,UInt32 bufferSizeRemaining)
{
    if(target!=NULL)
        ((Xbox360ControllerClass*)target)->WriteComplete(parameter,status,bufferSizeRemaining);
}

// This handles a completed asynchronous read
void Xbox360ControllerClass::ReadComplete(void *parameter,IOReturn status,UInt32 bufferSizeRemaining)
{
    IOReturn err;
    bool reread=!isInactive();
    
    switch(status) {
        case kIOReturnOverrun:
            IOLog("read - kIOReturnOverrun, clearing stall\n");
            inPipe->ClearStall();
            // Fall through
        case kIOReturnSuccess:
            {
                const XBOX360_IN_REPORT *report=(const XBOX360_IN_REPORT*)inBuffer->getBytesNoCopy();
                if((report->header.command==inReport)&&(report->header.size==sizeof(XBOX360_IN_REPORT))) {
                    fiddleReport(inBuffer);
                    err=handleReport(inBuffer,kIOHIDReportTypeInput);
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

// Handle a completed asynchronous write
void Xbox360ControllerClass::WriteComplete(void *parameter,IOReturn status,UInt32 bufferSizeRemaining)
{
    IOMemoryDescriptor *memory=(IOMemoryDescriptor*)parameter;
    if(status!=kIOReturnSuccess) {
        IOLog("write - Error writing: 0x%.8x\n",status);
    }
    memory->release();
}

// Returns the HID descriptor for this device
IOReturn Xbox360ControllerClass::newReportDescriptor(IOMemoryDescriptor **descriptor) const
{
    IOBufferMemoryDescriptor *buffer;
    
    buffer=IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task,0,sizeof(ReportDescriptor));
    if(buffer==NULL) return kIOReturnNoResources;
    buffer->writeBytes(0,ReportDescriptor,sizeof(ReportDescriptor));
    *descriptor=buffer;
    return kIOReturnSuccess;
}

// Handles a message from the userspace IOHIDDeviceInterface122::setReport function
IOReturn Xbox360ControllerClass::setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    char data[2];
    
    report->readBytes(0,data,2);
    switch(data[0]) {
        case 0x00:  // Set force feedback
            if((data[1]!=report->getLength())||(data[1]!=0x04)) return kIOReturnUnsupported;
            {
                XBOX360_OUT_RUMBLE rumble;
                
                Xbox360_Prepare(rumble,outRumble);
                report->readBytes(2,data,2);
                rumble.big=data[0];
                rumble.little=data[1];
                QueueWrite(&rumble,sizeof(rumble));
            }
            return kIOReturnSuccess;
        case 0x01:  // Set LEDs
            if((data[1]!=report->getLength())||(data[1]!=0x03)) return kIOReturnUnsupported;
            {
                XBOX360_OUT_LED led;
                
                report->readBytes(2,data,1);
                Xbox360_Prepare(led,outLed);
                led.pattern=data[0];
                QueueWrite(&led,sizeof(led));
            }
            return kIOReturnSuccess;
        default:
            return kIOReturnUnsupported;
    }
}

// Get report
IOReturn Xbox360ControllerClass::getReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options)
{
    // Doesn't do anything yet ;)
    return kIOReturnUnsupported;
}

// Returns the string for the specified index from the USB device's string list, with an optional default
OSString* Xbox360ControllerClass::getDeviceString(UInt8 index,const char *def) const
{
    IOReturn err;
    char buf[1024];
    const char *string;
    
    err=device->GetStringDescriptor(index,buf,sizeof(buf));
    if(err==kIOReturnSuccess) string=buf;
    else {
        if(def==NULL) string="Unknown";
        else string=def;
    }
    return OSString::withCString(string);
}

OSString* Xbox360ControllerClass::newManufacturerString() const
{
    return getDeviceString(device->GetManufacturerStringIndex());
}

OSNumber* Xbox360ControllerClass::newPrimaryUsageNumber() const
{
    // Gamepad
    return OSNumber::withNumber(0x05,8);
}

OSNumber* Xbox360ControllerClass::newPrimaryUsagePageNumber() const
{
    // Generic Desktop
    return OSNumber::withNumber(0x01,8);
}

OSNumber* Xbox360ControllerClass::newProductIDNumber() const
{
    return OSNumber::withNumber(device->GetProductID(),16);
}

OSString* Xbox360ControllerClass::newProductString() const
{
    return getDeviceString(device->GetProductStringIndex());
}

OSString* Xbox360ControllerClass::newSerialNumberString() const
{
    return getDeviceString(device->GetSerialNumberStringIndex());
}

OSString* Xbox360ControllerClass::newTransportString() const
{
    return OSString::withCString("USB");
}

OSNumber* Xbox360ControllerClass::newVendorIDNumber() const
{
    return OSNumber::withNumber(device->GetVendorID(),16);
}

// Called by the userspace IORegistryEntrySetCFProperties function
IOReturn Xbox360ControllerClass::setProperties(OSObject *properties)
{
    OSDictionary *dictionary;
    
    dictionary=OSDynamicCast(OSDictionary,properties);
    if(dictionary!=NULL) {
        setProperty(kDriverSettingKey,dictionary);
        readSettings();
        return kIOReturnSuccess;
    } else return kIOReturnBadArgument;
}
