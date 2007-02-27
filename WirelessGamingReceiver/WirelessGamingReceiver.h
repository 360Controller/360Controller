#ifndef __WIRELESSGAMINGRECEIVER_H__
#define __WIRELESSGAMINGRECEIVER_H__

#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>

#define WIRELESS_CONNECTIONS        4

class WirelessDevice;

typedef struct
{
    // Controller
    IOUSBInterface *controller;
    IOUSBPipe *controllerIn, *controllerOut;
    // Mystery
    IOUSBInterface *other;
    IOUSBPipe *otherIn, *otherOut;
    // Runtime data
    OSArray *inputArray;
    WirelessDevice *service;
}
WIRELESS_CONNECTION;

class WirelessGamingReceiver : public IOService
{
    OSDeclareDefaultStructors(WirelessGamingReceiver);
public:
    bool start(IOService *provider);
    void stop(IOService *provider);
private:
    friend class WirelessDevice;
    bool IsDataQueued(int index);
    IOMemoryDescriptor* ReadBuffer(int index);
    bool QueueWrite(int index, const void *bytes, UInt32 length);
private:
    IOUSBDevice *device;
    WIRELESS_CONNECTION connections[WIRELESS_CONNECTIONS];
    int connectionCount;
    
    void InstantiateService(int index);
    
    void ProcessMessage(int index, const unsigned char *data, int length);
    
    bool QueueRead(int index);
    void ReadComplete(void *parameter, IOReturn status, UInt32 bufferSizeRemaining);
    
    void WriteComplete(void *parameter, IOReturn status, UInt32 bufferSizeRemaining);
    
    void ReleaseAll(void);
    
    static void _ReadComplete(void *target, void *parameter, IOReturn status, UInt32 bufferSizeRemaining);
    static void _WriteComplete(void *target, void *parameter, IOReturn status, UInt32 bufferSizeRemaining);
};

#endif // __WIRELESSGAMINGRECEIVER_H__
