#ifndef __WIRELESSHIDDEVICE_H__
#define __WIRELESSHIDDEVICE_H__

#include <IOKit/hid/IOHIDDevice.h>

class WirelessDevice;

class WirelessHIDDevice : public IOHIDDevice
{
    OSDeclareDefaultStructors(WirelessHIDDevice);
public:
    void SetLEDs(int mode);
    unsigned char GetBatteryLevel(void);
    
    IOReturn setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options);
protected:
    bool handleStart(IOService *provider);
    void handleStop(IOService *provider);
    virtual void receivedData(void);
    virtual void receivedMessage(IOMemoryDescriptor *data);
    virtual void receivedUpdate(unsigned char type, unsigned char *data);
    virtual void receivedHIDupdate(unsigned char *data, int length);
private:
    static void _receivedData(void *target, WirelessDevice *sender, void *parameter);
    
    unsigned char battery;
};

#endif // __WIRELESSHIDDEVICE_H__
