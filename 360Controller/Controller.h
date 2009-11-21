/*
 *  Controller.h
 *  360Controller
 *
 *  Created by Colin on 20/11/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <IOKit/hid/IOHIDDevice.h>

class ControllerClass : public IOHIDDevice
{
	OSDeclareDefaultStructors(ControllerClass)

private:
    OSString* getDeviceString(UInt8 index,const char *def=NULL) const;

public:
    virtual IOReturn setProperties(OSObject *properties);

    virtual IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const;
    
    virtual IOReturn setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options=0);
    virtual IOReturn getReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options);
	
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
