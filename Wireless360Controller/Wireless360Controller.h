#ifndef __WIRELESS360CONTROLLER_H__
#define __WIRELESS360CONTROLLER_H__

#include "../WirelessGamingReceiver/WirelessHIDDevice.h"

class Wireless360Controller : public WirelessHIDDevice
{
    OSDeclareDefaultStructors(Wireless360Controller);
public:
    bool init(OSDictionary *propTable = NULL);

    void SetRumbleMotors(unsigned char large, unsigned char small);
    
    IOReturn setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options);
    IOReturn newReportDescriptor(IOMemoryDescriptor ** descriptor ) const;

    IOReturn setProperties(OSObject *properties);

    virtual OSString* newManufacturerString() const;
    virtual OSNumber* newPrimaryUsageNumber() const;
    virtual OSNumber* newPrimaryUsagePageNumber() const;
    virtual OSNumber* newProductIDNumber() const;
    virtual OSString* newProductString() const;
    virtual OSString* newSerialNumberString() const;
    virtual OSString* newTransportString() const;
    virtual OSNumber* newVendorIDNumber() const;
protected:
    void readSettings(void);
    void receivedHIDupdate(unsigned char *data, int length);

    // Settings
    bool invertLeftX,invertLeftY;
    bool invertRightX,invertRightY;
    short deadzoneLeft,deadzoneRight;
    bool relativeLeft,relativeRight;
private:
    void fiddleReport(unsigned char *data, int length);
};

#endif // __WIRELESS360CONTROLLER_H__
