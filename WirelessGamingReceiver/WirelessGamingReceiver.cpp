/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro

    WirelessGamingReceiver.cpp - main source of the wireless receiver driver

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
#include "WirelessGamingReceiver.h"
#include "WirelessDevice.h"
#include "devices.h"
#include <IOKit/usb/IOUSBHostFamily.h>

//#define PROTOCOL_DEBUG

OSDefineMetaClassAndStructors(WirelessGamingReceiver, IOService)

// Holds data for asynchronous reads
typedef struct WGRREAD
{
    int index;
    IOBufferMemoryDescriptor* buffer;
} WGRREAD;

// Get maximum packet size for a pipe
static UInt32 GetMaxPacketSize(IOUSBHostPipe* pipe)
{
    const EndpointDescriptor* ed = pipe->getEndpointDescriptor();

    if (ed == nullptr)
    {
        return 0;
    }
    else
    {
        return ed->wMaxPacketSize;
    }
}

// Start device
bool WirelessGamingReceiver::start(IOService* provider)
{
    const ConfigurationDescriptor* cd = nullptr;
    const InterfaceDescriptor* intf = nullptr;
    const EndpointDescriptor* pipe = nullptr;
    int iConnection, iOther, i;
    OSIterator* iterator = nullptr;
    OSObject* candidate = nullptr;
    IOReturn error = kIOReturnError;

    if (!IOService::start(provider))
    {
        kprintf("start - superclass failed\n");
        return false;
    }

    device = OSDynamicCast(IOUSBHostDevice, provider);
    if (device == nullptr)
    {
        kprintf("start - invalid provider\n");
        goto fail;
    }

    // Check for configurations
    if (device->getDeviceDescriptor()->bNumConfigurations < 1)
    {
        device = nullptr;
        kprintf("start - device has no configurations!\n");
        goto fail;
    }

    // Set configuration
    cd = device->getConfigurationDescriptor(0);
    if (cd == nullptr)
    {
        device = nullptr;
        kprintf("start - couldn't get configuration descriptor\n");
        goto fail;
    }

    if (!device->open(this))
    {
        device = nullptr;
        kprintf("start - failed to open device\n");
        goto fail;
    }

    error = device->setConfiguration(cd->bConfigurationValue, true);
    if (error != kIOReturnSuccess)
    {
        kprintf("start - unable to set configuration with error: 0x%.8x\n", error);
        goto fail;
    }

    for (i = 0; i < WIRELESS_CONNECTIONS; i++)
    {
        connections[i].controller = nullptr;
        connections[i].controllerIn = nullptr;
        connections[i].controllerOut = nullptr;
        connections[i].other = nullptr;
        connections[i].otherIn = nullptr;
        connections[i].otherOut = nullptr;
        connections[i].inputArray = nullptr;
        connections[i].service = nullptr;
        connections[i].controllerStarted = false;
    }

    iConnection = 0;
    iOther = 0;
    iterator = device->getChildIterator(gIOServicePlane);
    while ((iterator != nullptr) && ((candidate = iterator->getNextObject()) != nullptr))
    {
        IOUSBHostInterface* interfaceCandidate = OSDynamicCast(IOUSBHostInterface, candidate);
        if (interfaceCandidate != nullptr)
        {
            // TODO(Drew): There's a bunch of duplicated code in here. Can we pull it out? What makes these two different?
            switch (interfaceCandidate->getInterfaceDescriptor()->bInterfaceProtocol)
            {
                case 129:   // Controller
                {
                    pipe = nullptr;
                    
                    if (!interfaceCandidate->open(this))
                    {
                        kprintf("start - failed to open control interface\n");
                        goto fail;
                    }
                    connections[iConnection].controller = interfaceCandidate;
                    
                    cd = interfaceCandidate->getConfigurationDescriptor();
                    intf = interfaceCandidate->getInterfaceDescriptor();
                    if (!cd || !intf)
                    {
                        kprintf("start - Can't get configuration descriptor and interface descriptor.\n");
                        goto fail;
                    }
                    
                    while ((pipe = StandardUSB::getNextEndpointDescriptor(cd, intf, pipe)))
                    {
                        UInt8 pipeDirection = StandardUSB::getEndpointDirection(pipe);
                        UInt8 pipeType = StandardUSB::getEndpointType(pipe);

                        kprintf("Pipe type: %d, pipe direction: %d\n", pipeType, pipeDirection); // TODO(Drew): Delete me
                        
                        if ((pipeDirection == kEndpointDirectionIn) && (pipeType == kEndpointTypeInterrupt))
                        {
                            connections[iConnection].controllerIn = interfaceCandidate->copyPipe(StandardUSB::getEndpointAddress(pipe));
                        }
                        else if ((pipeDirection == kEndpointDirectionOut) && (pipeType == kEndpointTypeInterrupt))
                        {
                            connections[iConnection].controllerOut = interfaceCandidate->copyPipe(StandardUSB::getEndpointAddress(pipe));
                        }
                    }
                    
                    if (connections[iConnection].controllerIn == nullptr)
                    {
                        kprintf("start - Failed to open control input pipe\n");
                        goto fail;
                    }
                    connections[iConnection].controllerIn->retain();
                    
                    if (connections[iConnection].controllerOut == nullptr)
                    {
                        kprintf("start - Failed to open control output pipe\n");
                        goto fail;
                    }
                    connections[iConnection].controllerOut->retain();
                    
                    iConnection++;
                } break;

                case 130:   // It is a mystery
                {
                    pipe = nullptr;
                    if (!interfaceCandidate->open(this))
                    {
                        kprintf("start - Failed to open mystery interface\n");
                        goto fail;
                    }
                    connections[iOther].other = interfaceCandidate;
                    
                    cd = interfaceCandidate->getConfigurationDescriptor();
                    intf = interfaceCandidate->getInterfaceDescriptor();
                    if (!cd || !intf)
                    {
                        kprintf("start - Can't get configuration descriptor and interface descriptor.\n");
                        goto fail;
                    }
                    
                    while ((pipe = StandardUSB::getNextEndpointDescriptor(cd, intf, pipe)))
                    {
                        UInt8 pipeDirection = StandardUSB::getEndpointDirection(pipe);
                        UInt8 pipeType = StandardUSB::getEndpointType(pipe);

                        kprintf("Pipe type: %d, pipe direction: %d\n", pipeType, pipeDirection); // TODO(Drew): Delete me

                        if ((pipeDirection == kEndpointDirectionIn) && (pipeType == kEndpointTypeInterrupt))
                        {
                            connections[iOther].otherIn = interfaceCandidate->copyPipe(StandardUSB::getEndpointAddress(pipe));
                        }
                        else if ((pipeDirection == kEndpointDirectionOut) && (pipeType == kEndpointTypeInterrupt))
                        {
                            connections[iOther].otherOut = interfaceCandidate->copyPipe(StandardUSB::getEndpointAddress(pipe));
                        }
                    }
                    
                    if (connections[iOther].otherIn == nullptr)
                    {
                        kprintf("start - Failed to open control input pipe\n");
                        goto fail;
                    }
                    connections[iOther].otherIn->retain();
                    
                    if (connections[iOther].otherOut == nullptr)
                    {
                        kprintf("start - Failed to open control output pipe\n");
                        goto fail;
                    }
                    connections[iOther].otherOut->retain();
                    
                    iOther++;
                } break;

                default:
                {
                    kprintf("start - Ignoring interface (protocol %d)\n", interfaceCandidate->getInterfaceDescriptor()->bInterfaceProtocol);
                } break;
            }
        }
    }
    OSSafeReleaseNULL(iterator);

    if (iConnection != iOther)
    {
        kprintf("start - interface mismatch?\n");
    }
    connectionCount = iConnection;

    for (i = 0; i < connectionCount; i++)
    {
        connections[i].inputArray = OSArray::withCapacity(5);
        if (connections[i].inputArray == nullptr)
        {
            kprintf("start - Failed to allocate packet buffer %d\n", i);
            goto fail;
        }
        if (!QueueRead(i))
        {
            kprintf("start - Failed to start read %d\n", i);
            goto fail;
        }
    }

    kprintf("start - Transform and roll out (%d interfaces)\n", connectionCount);
    return true;

fail:
    if (iterator != nullptr)
    {
        OSSafeReleaseNULL(iterator);
    }
    
    ReleaseAll();
    return false;
}

// Stop the device
void WirelessGamingReceiver::stop(IOService* provider)
{
    ReleaseAll();
    IOService::stop(provider);
}

// Handle termination
bool WirelessGamingReceiver::didTerminate(IOService* provider, IOOptionBits options, bool* defer)
{
    // release all objects used and close the device
    ReleaseAll();
    return IOService::didTerminate(provider, options, defer);
}


// Handle message from provider
IOReturn WirelessGamingReceiver::message(UInt32 type, IOService* provider, void* argument)
{
    // IOLog("Message\n");
#if 0
    switch(type)
    {
        case kIOMessageServiceIsTerminated:
        case kIOMessageServiceIsRequestingClose:
        {
            if (device->isOpen(this))
            {
                ReleaseAll();
            }
            return kIOReturnSuccess;
        } break;
            
        default:
        {
            return IOService::message(type, provider, argument);
        } break;
    }
#else
    return IOService::message(type, provider, argument);
#endif
}

// Queue a read on a controller
bool WirelessGamingReceiver::QueueRead(int index)
{
    IOUSBHostCompletion complete = {};
    IOReturn err = kIOReturnError;
    WGRREAD* data = (WGRREAD*)IOMalloc(sizeof(WGRREAD));

    if (data == nullptr)
    {
        kprintf("QueueRead - data is nullptr\n");
        return false;
    }
    
    data->index = index;
    data->buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, GetMaxPacketSize(connections[index].controllerIn));
    if (data->buffer == nullptr)
    {
        IOFree(data, sizeof(WGRREAD));
        kprintf("QueueRead - data->buffer is nullptr\n");
        return false;
    }

    complete.owner = this;
    complete.action = _ReadComplete;
    complete.parameter = data;

    err = connections[index].controllerIn->io(data->buffer, (UInt32)data->buffer->getLength(), &complete);
    if (err == kIOReturnSuccess)
    {
        kprintf("QueueRead - started\n");
        return true;
    }

    data->buffer->release();
    IOFree(data, sizeof(WGRREAD));

    kprintf("QueueRead - failed to start (0x%.8x)\n", err);
    return false;
}

// Handle a completed read on a controller
void WirelessGamingReceiver::ReadComplete(void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    WGRREAD* data = (WGRREAD*)parameter;
    bool reread = true;

    switch (status)
    {
        case kIOReturnOverrun:
        {
            // IOLog("read - kIOReturnOverrun, clearing stall\n");
            kprintf("ReadComplete - kIOReturnOverrun\n");
            connections[data->index].controllerIn->clearStall(false);
        } // fall through
        case kIOReturnSuccess:
        {
            kprintf("ReadComplete - kIOReturnSuccess\n");
            ProcessMessage(data->index, (unsigned char*)data->buffer->getBytesNoCopy(), (int)data->buffer->getLength() - bufferSizeRemaining);
        } break;

        case kIOReturnNotResponding:
        {
            // IOLog("read - kIOReturnNotResponding\n");
            kprintf("ReadComplete - kIOReturnNotResponding\n");
        } // fall through
        default:
        {
            kprintf("ReadComplete - default\n");
            reread = false;
        } break;
    }

    int newIndex = data->index;
    data->buffer->release();
    IOFree(data, sizeof(WGRREAD));

    if (reread)
    {
        QueueRead(newIndex);
    }
}

// Queue an asynchronous write on a controller
bool WirelessGamingReceiver::QueueWrite(int index, const void* bytes, UInt32 length)
{
    IOBufferMemoryDescriptor* outBuffer = nullptr;
    IOUSBHostCompletion complete = {};
    IOReturn err = kIOReturnError;

    outBuffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, length);
    if (outBuffer == nullptr)
    {
        // IOLog("send - unable to allocate buffer\n");
        return false;
    }
    outBuffer->writeBytes(0, bytes, length);

    complete.owner = this;
    complete.action = _WriteComplete;
    complete.parameter = outBuffer;

    err = connections[index].controllerOut->io(outBuffer, length, &complete);
    if (err == kIOReturnSuccess)
    {
        return true;
    }
    else
    {
        // IOLog("send - failed to start (0x%.8x)\n",err);
        return false;
    }
}

// Handle a completed write on a controller
void WirelessGamingReceiver::WriteComplete(void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    IOMemoryDescriptor* memory = (IOMemoryDescriptor*)parameter;
    if (status != kIOReturnSuccess)
    {
        IOLog("write - Error writing: 0x%.8x\n", status);
    }
    memory->release();
}

// Release any allocated objects
void WirelessGamingReceiver::ReleaseAll(void)
{
    for (int i = 0; i < connectionCount; i++)
    {
        if (connections[i].service != nullptr)
        {
            connections[i].service->terminate(kIOServiceRequired);
            connections[i].service->detachAll(gIOServicePlane);
            connections[i].service->release();
            connections[i].service = nullptr;
        }
        if (connections[i].controllerIn != nullptr)
        {
            connections[i].controllerIn->abort();
            connections[i].controllerIn->release();
            connections[i].controllerIn = nullptr;
        }
        if (connections[i].controllerOut != nullptr)
        {
            connections[i].controllerOut->abort();
            connections[i].controllerOut->release();
            connections[i].controllerOut = nullptr;
        }
        if (connections[i].controller != nullptr)
        {
            connections[i].controller->close(this);
            connections[i].controller = nullptr;
        }
        if (connections[i].otherIn != nullptr)
        {
            connections[i].otherIn->abort();
            connections[i].otherIn->release();
            connections[i].otherIn = nullptr;
        }
        if (connections[i].otherOut != nullptr)
        {
            connections[i].otherOut->abort();
            connections[i].otherOut->release();
            connections[i].otherOut = nullptr;
        }
        if (connections[i].other != nullptr)
        {
            connections[i].other->close(this);
            connections[i].other = nullptr;
        }
        if (connections[i].inputArray != nullptr)
        {
            connections[i].inputArray->release();
            connections[i].inputArray = nullptr;
        }
        connections[i].controllerStarted = false;
    }
    if (device != nullptr)
    {
        device->close(this);
        device = nullptr;
    }
}

// Static wrapper for read notifications
void WirelessGamingReceiver::_ReadComplete(void* target, void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    kprintf("_ReadComplete\n");
    if (target != nullptr)
    {
        ((WirelessGamingReceiver*)target)->ReadComplete(parameter, status, bufferSizeRemaining);
    }
}

// Static wrapper for write notifications
void WirelessGamingReceiver::_WriteComplete(void* target, void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    if (target != nullptr)
    {
        ((WirelessGamingReceiver*)target)->WriteComplete(parameter, status, bufferSizeRemaining);
    }
}
#define PROTOCOL_DEBUG 1
// Processes a message for a controller
void WirelessGamingReceiver::ProcessMessage(int index, const unsigned char* data, int length)
{
#ifdef PROTOCOL_DEBUG
    char s[1024];
    int i;

    for (i = 0; i < length; i++)
    {
        s[(i * 2) + 0] = "0123456789ABCDEF"[(data[i] & 0xF0) >> 4];
        s[(i * 2) + 1] = "0123456789ABCDEF"[data[i] & 0x0F];
    }
    s[i * 2] = '\0';
    IOLog("Got data (%d, %d bytes): %s\n", index, length, s);
#endif
    // Handle device connections
    if ((length == 2) && (data[0] == 0x08))
    {
        if (data[1] == 0x00)
        {
            // Device disconnected
#ifdef PROTOCOL_DEBUG
            kprintf("ProcessMessage - Device detached\n");
#endif
            if (connections[index].service != nullptr)
            {
                connections[index].service->SetIndex(-1);
                if (connections[index].controllerStarted)
                {
                    connections[index].service->terminate(kIOServiceRequired | kIOServiceSynchronous);
                }
                connections[index].service->detach(this);
                connections[index].service->release();
                connections[index].service = nullptr;
                connections[index].controllerStarted = false;
            }
        }
        else
        {
            // Device connected
#ifdef PROTOCOL_DEBUG
            kprintf("ProcessMessage - Attempting to add new device\n");
#endif
            if (connections[index].service == nullptr)
            {
                bool ready = false;
                int i, j;
                IOMemoryDescriptor* data = nullptr;
                char c;

                j = connections[index].inputArray->getCount();
                for (i = 0; !ready && (i < j); i++)
                {
                    data = OSDynamicCast(IOMemoryDescriptor, connections[index].inputArray->getObject(i));
                    data->readBytes(1, &c, 1);
                    if (c == 0x0f)
                    {
                        ready = true;
                    }
                }
                InstantiateService(index);
                if (ready && (connections[index].service != nullptr))
                {
#ifdef PROTOCOL_DEBUG
                    kprintf("ProcessMessage - Registering wireless device");
#endif
                    connections[index].controllerStarted = true;
                    connections[index].service->registerService();
                }
            }
        }
        return;
    }

    // Add anything else to the queue
    IOMemoryDescriptor* copy = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, length);
    copy->writeBytes(0, data, length);
    connections[index].inputArray->setObject(copy);
    if (connections[index].service == nullptr)
    {
        InstantiateService(index);
    }
    if (connections[index].service != nullptr)
    {
        connections[index].service->NewData();
        if (!connections[index].controllerStarted)
        {
            char c;

            copy->readBytes(1, &c, 1);
            if (c == 0x0f)
            {
#ifdef PROTOCOL_DEBUG
                IOLog("ProcessMessage - Registering wireless device");
#endif
                connections[index].controllerStarted = true;
                connections[index].service->registerService();
            }
        }
    }
    copy->release();
}

// Create a new node for the attached controller
void WirelessGamingReceiver::InstantiateService(int index)
{
    kprintf("InstantiateService - !!!\n");
    connections[index].service = new WirelessDevice;
    if (connections[index].service != nullptr)
    {
        const OSString* keys[1] = {
            OSString::withCString(kIOWirelessDeviceType),
        };
        const OSObject* objects[1] = {
            OSNumber::withNumber((unsigned long long)0, 32),
        };
        OSDictionary* dictionary = OSDictionary::withObjects(objects, keys, 1, 0);
        kprintf("InstantiateService - attempting to init device.\n");
        if (connections[index].service->init(dictionary))
        {
            connections[index].service->attach(this);
            connections[index].service->SetIndex(index);
            // connections[index].service->registerService();
            kprintf("InstantiateService - attached device.\n");
            // IOLog("process: Device attached\n");
            if (IsDataQueued(index))
            {
                connections[index].service->NewData();
            }
            connections[index].service->start(this);
        }
        else
        {
            connections[index].service->release();
            connections[index].service = nullptr;
            kprintf("InstantiateService - device attach failure.\n");
            // IOLog("process: Device attach failure\n");
        }
    }
}

// Check a controller's queue
bool WirelessGamingReceiver::IsDataQueued(int index)
{
    return connections[index].inputArray->getCount() > 0;
}

// Read a controller's queue
IOMemoryDescriptor* WirelessGamingReceiver::ReadBuffer(int index)
{
    IOMemoryDescriptor* data = nullptr;

    data = OSDynamicCast(IOMemoryDescriptor, connections[index].inputArray->getObject(0));
    if (data != nullptr)
    {
        data->retain();
    }
    connections[index].inputArray->removeObject(0);
    
    return data;
}

// Get our location ID
OSNumber* WirelessGamingReceiver::newLocationIDNumber() const
{
    OSNumber* number = nullptr;
    UInt32 location = 0;

    if (device)
    {
        if ((number = OSDynamicCast(OSNumber, device->getProperty("locationID"))))
        {
            location = number->unsigned32BitValue();
        }
        else
        {
            // Make up an address
            if ((number = OSDynamicCast(OSNumber, device->getProperty("USB Address"))))
                location |= number->unsigned8BitValue() << 24;

            if ((number = OSDynamicCast(OSNumber, device->getProperty("idProduct"))))
                location |= number->unsigned8BitValue() << 16;
        }
    }

    return OSNumber::withNumber(location, 32);
}
