#include "WirelessDevice.h"
#include "WirelessGamingReceiver.h"

OSDefineMetaClassAndStructors(WirelessDevice, IOService)
#define super IOService

bool WirelessDevice::init(OSDictionary *dictionary)
{
    if (!super::init(dictionary))
        return false;
    index = -1;
    function = NULL;
    return true;
}

bool WirelessDevice::IsDataAvailable(void)
{
    if (index == -1)
        return false;
    WirelessGamingReceiver *receiver = OSDynamicCast(WirelessGamingReceiver, getProvider());
    if (receiver == NULL)
        return false;
    return receiver->IsDataQueued(index);
}

IOMemoryDescriptor* WirelessDevice::NextPacket(void)
{
    if (index == -1)
        return NULL;
    WirelessGamingReceiver *receiver = OSDynamicCast(WirelessGamingReceiver, getProvider());
    if (receiver == NULL)
        return false;
    return receiver->ReadBuffer(index);
}

void WirelessDevice::SendPacket(const void *data, size_t length)
{
    if (index == -1)
        return;
    WirelessGamingReceiver *receiver = OSDynamicCast(WirelessGamingReceiver, getProvider());
    if (receiver == NULL)
        return;
    receiver->QueueWrite(index, data, length);
}

void WirelessDevice::RegisterWatcher(void *target, WirelessDeviceWatcher function, void *parameter)
{
    this->target = target;
    this->parameter = parameter;
    this->function = function;
}

void WirelessDevice::SetIndex(int i)
{
    index = i;
}

void WirelessDevice::NewData(void)
{
    if (function != NULL)
        function(target, this, parameter);
}
