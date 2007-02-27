#include <IOKit/IOLib.h>
#include "WirelessHIDDevice.h"
#include "WirelessDevice.h"
#include "devices.h"

OSDefineMetaClassAndAbstractStructors(WirelessHIDDevice, IOHIDDevice)
#define super IOHIDDevice

const char weirdStart[] = {0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void WirelessHIDDevice::SetLEDs(int mode)
{
    char buf[] = {0x00, 0x00, 0x08, 0x40 + (mode % 0x0e), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    WirelessDevice *device;

    device = OSDynamicCast(WirelessDevice, getProvider());
    if (device != NULL)
    {
        device->SendPacket(buf, sizeof(buf));
        device->SendPacket(weirdStart, sizeof(weirdStart));
    }
}

unsigned char WirelessHIDDevice::GetBatteryLevel(void)
{
    return battery;
}
    
IOReturn WirelessHIDDevice::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options)
{
    char data[2];

    if (report->readBytes(0, data, 2) < 2)
        return kIOReturnUnsupported;
        
    // LED
    if (data[0] == 0x01)
    {
        if ((data[1] != report->getLength()) || (data[1] != 0x03))
            return kIOReturnUnsupported;
        report->readBytes(2, data, 1);
        SetLEDs(data[0]);
        return kIOReturnSuccess;
    }

    return super::setReport(report, reportType, options);
}

bool WirelessHIDDevice::handleStart(IOService *provider)
{
    WirelessDevice *device;
    
    if (!super::handleStart(provider))
        return false;

    device = OSDynamicCast(WirelessDevice, provider);
    if (device == NULL)
        return false;
        
    device->RegisterWatcher(this, _receivedData, NULL);
    
    device->SendPacket(weirdStart, sizeof(weirdStart));
    
    return true;
}

void WirelessHIDDevice::handleStop(IOService *provider)
{
    WirelessDevice *device;

    device = OSDynamicCast(WirelessDevice, provider);
    if (device != NULL)
        device->RegisterWatcher(NULL, NULL, NULL);
        
    super::handleStop(provider);
}

void WirelessHIDDevice::receivedData(void)
{
    IOMemoryDescriptor *data;
    WirelessDevice *device = OSDynamicCast(WirelessDevice, getProvider());
    if (device == NULL)
        return;
    
    while ((data = device->NextPacket()) != NULL)
    {
        receivedMessage(data);
        data->release();
    }
}

void WirelessHIDDevice::receivedMessage(IOMemoryDescriptor *data)
{
    unsigned char buf[29];
    
    if (data->getLength() != 29)
        return;
        
    data->readBytes(0, buf, 29);
    
    switch (buf[1])
    {
        case 0x0f:  // Initial info
            if (buf[16] == 0x13)
                receivedUpdate(0x13, buf + 17);
            break;
        case 0x01:  // HID info update
            if (buf[3] == 0xf0)
                receivedHIDupdate(buf + 4, buf[5]);
            break;
        case 0x00:  // Info update
            receivedUpdate(buf[3], buf + 4);
            break;
        default:
            break;
    }
}

void WirelessHIDDevice::receivedUpdate(unsigned char type, unsigned char *data)
{
    switch (type)
    {
        case 0x13:  // Battery level
            battery = data[0];
            {
                OSObject *prop = OSNumber::withNumber(battery, 8);
                if (prop != NULL)
                {
                    setProperty(kIOWirelessBatteryLevel, prop);
                    prop->release();
                }
            }
            break;
        default:
            break;
    }
}

void WirelessHIDDevice::receivedHIDupdate(unsigned char *data, int length)
{
    IOReturn err;
    IOMemoryDescriptor *report;
    
    report = IOMemoryDescriptor::withAddress(data, length, kIODirectionNone);
    err = handleReport(report, kIOHIDReportTypeInput);
    report->release();
    if (err != kIOReturnSuccess)
        IOLog("handleReport return: 0x%.8x\n", err);
}

void WirelessHIDDevice::_receivedData(void *target, WirelessDevice *sender, void *parameter)
{
    ((WirelessHIDDevice*)target)->receivedData();
}
