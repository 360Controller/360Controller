/*
 *  ChatPad.h
 *  360Controller
 *
 *  Created by Colin on 18/11/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <IOKit/hid/IOHIDDevice.h>

class ChatPadKeyboardClass : public IOHIDDevice
{
	OSDeclareDefaultStructors(ChatPadKeyboardClass)
	
private:

public:
    virtual bool start(IOService *provider);

    // IOHidDevice methods
    virtual IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const;
    
    virtual IOReturn setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options=0);
    virtual IOReturn getReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options);

	virtual IOReturn handleReport(IOMemoryDescriptor *report, IOHIDReportType reportType = kIOHIDReportTypeInput, IOOptionBits options = 0);
	
    virtual OSString* newManufacturerString() const;
    virtual OSNumber* newPrimaryUsageNumber() const;
    virtual OSNumber* newPrimaryUsagePageNumber() const;
    virtual OSNumber* newProductIDNumber() const;
    virtual OSString* newProductString() const;
    virtual OSString* newSerialNumberString() const;
    virtual OSString* newTransportString() const;
    virtual OSNumber* newVendorIDNumber() const;
	
    virtual OSNumber* newLocationIDNumber() const;
};
