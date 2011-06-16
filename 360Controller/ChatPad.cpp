/*
 *  ChatPad.cpp
 *  360Controller
 *
 *  Created by Colin on 18/11/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <IOKit/IOLib.h>
#include "ChatPad.h"
namespace HID_ChatPad {
#include "chatpadhid.h"
}
#include "chatpadkeys.h"
#include "_60Controller.h"

OSDefineMetaClassAndStructors(ChatPadKeyboardClass, IOHIDDevice)

IOReturn ChatPadKeyboardClass::newReportDescriptor(IOMemoryDescriptor **descriptor) const
{
    IOBufferMemoryDescriptor *buffer;
    
    buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(HID_ChatPad::ReportDescriptor));
    if (buffer == NULL)
		return kIOReturnNoResources;
    buffer->writeBytes(0, HID_ChatPad::ReportDescriptor, sizeof(HID_ChatPad::ReportDescriptor));
    *descriptor = buffer;
    return kIOReturnSuccess;
}

IOReturn ChatPadKeyboardClass::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options)
{
	// Maybe add LED support later?
    return kIOReturnUnsupported;
}

IOReturn ChatPadKeyboardClass::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options)
{
    return kIOReturnUnsupported;
}

IOReturn ChatPadKeyboardClass::handleReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options)
{
	IOBufferMemoryDescriptor *realReport = OSDynamicCast(IOBufferMemoryDescriptor, report);
	if (realReport != NULL)
	{
		unsigned char *data = (unsigned char*)realReport->getBytesNoCopy();
		if (data[0] == 0x00)
		{
			for (int i = 2; i < 5; i++)
			{
				data[i] = ChatPad2USB(data[i]);
			}
		}
	}
	return IOHIDDevice::handleReport(report, reportType, options);
}

OSNumber* ChatPadKeyboardClass::newPrimaryUsageNumber() const
{
    return OSNumber::withNumber(HID_ChatPad::ReportDescriptor[3], 8);
}

OSNumber* ChatPadKeyboardClass::newPrimaryUsagePageNumber() const
{
    return OSNumber::withNumber(HID_ChatPad::ReportDescriptor[1], 8);
}

OSString* ChatPadKeyboardClass::newProductString() const
{
    return OSString::withCString("ChatPad");
}

OSString* ChatPadKeyboardClass::newTransportString() const
{
    return OSString::withCString("Serial");
}

OSNumber* ChatPadKeyboardClass::newVendorIDNumber() const
{
	// Same as USB
	return OSNumber::withNumber(100, 32);
}

OSNumber* ChatPadKeyboardClass::newProductIDNumber() const
{
	// Same as USB, plus one
	return OSNumber::withNumber(100, 32);
}

/*
static IOHIDDevice* GetParent(IOService *current)
{
	IOService *parent = current->getProvider();
	if (parent == NULL)
		return NULL;
	return OSDynamicCast(IOHIDDevice, parent->getProvider());
}
 */
/*
static IOHIDDevice* GetParent(const IOService *current)
{
	return OSDynamicCast(IOHIDDevice, current->getProvider());
}
 */
static IOHIDDevice* GetParent(const IOService *current)
{
	Xbox360Peripheral *owner;
	
	owner = OSDynamicCast(Xbox360Peripheral, current->getProvider());
	if (owner == NULL)
		return NULL;
	return owner->getController(0);
}

bool ChatPadKeyboardClass::start(IOService *provider)
{
    if (!IOHIDDevice::start(provider))
        return false;
    return OSDynamicCast(Xbox360Peripheral, provider) != NULL;
}

OSString* ChatPadKeyboardClass::newManufacturerString() const
{
	IOHIDDevice *device = GetParent(this);
	if (device == NULL)
		return NULL;
	return device->newManufacturerString();
}

OSString* ChatPadKeyboardClass::newSerialNumberString() const
{
	IOHIDDevice *device = GetParent(this);
	if (device == NULL)
		return NULL;
	return device->newSerialNumberString();
}

OSNumber* ChatPadKeyboardClass::newLocationIDNumber() const
{
	IOHIDDevice *device = GetParent(this);
	if (device == NULL)
		return NULL;
	OSNumber *number = device->newLocationIDNumber();
	if (number == NULL)
		return NULL;
	UInt32 value = number->unsigned32BitValue();
	number->release();
	return OSNumber::withNumber(value + 1, 32);
}
