#ifndef __WIRELESSDEVICE_H__
#define __WIRELESSDEVICE_H__

#include <IOKit/IOService.h>

class WirelessDevice;

typedef void (*WirelessDeviceWatcher)(void *target, WirelessDevice *sender, void *parameter);

class WirelessDevice : public IOService
{
    OSDeclareDefaultStructors(WirelessDevice);
public:
    bool init(OSDictionary *dictionary = 0);

    // Controller interface
    bool IsDataAvailable(void);
    IOMemoryDescriptor* NextPacket(void);
    
    void SendPacket(const void *data, size_t length);
    
    void RegisterWatcher(void *target, WirelessDeviceWatcher function, void *parameter);
private:
    friend class WirelessGamingReceiver;
    void SetIndex(int i);
    void NewData(void);
private:
    int index;
    // callback
    void *target, *parameter;
    WirelessDeviceWatcher function;
};

#endif // __WIRELESSDEVICE_H__
