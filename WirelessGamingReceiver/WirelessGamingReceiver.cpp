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

//#define PROTOCOL_DEBUG

OSDefineMetaClassAndStructors(WirelessGamingReceiver, IOService)

// Holds data for asynchronous reads
typedef struct WGRREAD
{
    int index;
    IOBufferMemoryDescriptor *buffer;
} WGRREAD;

// Get maximum packet size for a pipe
static UInt32 GetMaxPacketSize(IOUSBPipe *pipe)
{
    const IOUSBEndpointDescriptor *ed = pipe->GetEndpointDescriptor();
    
    if (ed == NULL) return 0;
    else return ed->wMaxPacketSize;
}

// Start device
bool WirelessGamingReceiver::start(IOService *provider)
{
    const IOUSBConfigurationDescriptor *cd;
    IOUSBFindInterfaceRequest interfaceRequest;
    IOUSBFindEndpointRequest pipeRequest;
    IOUSBInterface *interface;
    int iConnection, iOther, i;
    
    if (!IOService::start(provider))
    {
        // IOLog("start - superclass failed\n");
        return false;
    }

    device = OSDynamicCast(IOUSBDevice, provider);
    if (device == NULL)
    {
        // IOLog("start - invalid provider\n");
        goto fail;
    }

    // Check for configurations
    if (device->GetNumConfigurations() < 1)
    {
        device = NULL;
        // IOLog("start - device has no configurations!\n");
        goto fail;
    }
    
    // Set configuration
    cd = device->GetFullConfigurationDescriptor(0);
    if (cd == NULL)
    {
        device = NULL;
        // IOLog("start - couldn't get configuration descriptor\n");
        goto fail;
    }
    
    if (!device->open(this))
    {
        device = NULL;
        // IOLog("start - failed to open device\n");
        goto fail;
    }
    if (device->SetConfiguration(this, cd->bConfigurationValue, true) != kIOReturnSuccess)
    {
        // IOLog("start - unable to set configuration\n");
        goto fail;
    }
    
    for (i = 0; i < WIRELESS_CONNECTIONS; i++)
    {
        connections[i].controller = NULL;
        connections[i].controllerIn = NULL;
        connections[i].controllerOut = NULL;
        connections[i].other = NULL;
        connections[i].otherIn = NULL;
        connections[i].otherOut = NULL;
        connections[i].inputArray = NULL;
        connections[i].service = NULL;
        connections[i].controllerStarted = false;
    }
    
    pipeRequest.interval = 0;
    pipeRequest.maxPacketSize = 0;
    pipeRequest.type = kUSBInterrupt;
    interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bAlternateSetting = 0;
    interface = NULL;
    iConnection = 0;
    iOther = 0;
    while ((interface = device->FindNextInterface(interface, &interfaceRequest)) != NULL)
    {
        switch (interface->GetInterfaceProtocol())
        {
            case 129:   // Controller
                if (!interface->open(this))
                {
                    // IOLog("start: Failed to open control interface\n");
                    goto fail;
                }
                connections[iConnection].controller = interface;
                pipeRequest.direction = kUSBIn;
                connections[iConnection].controllerIn = interface->FindNextPipe(NULL, &pipeRequest);
                if (connections[iConnection].controllerIn == NULL)
                {
                    // IOLog("start: Failed to open control input pipe\n");
                    goto fail;
                }
                else
                    connections[iConnection].controllerIn->retain();
                pipeRequest.direction = kUSBOut;
                connections[iConnection].controllerOut = interface->FindNextPipe(NULL, &pipeRequest);
                if (connections[iConnection].controllerOut == NULL)
                {
                    // IOLog("start: Failed to open control output pipe\n");
                    goto fail;
                }
                else
                    connections[iConnection].controllerOut->retain();
                iConnection++;
                break;
				
            case 130:   // It is a mystery
                if (!interface->open(this))
                {
                    // IOLog("start: Failed to open mystery interface\n");
                    goto fail;
                }
                connections[iOther].other = interface;
                pipeRequest.direction = kUSBIn;
                connections[iOther].otherIn = interface->FindNextPipe(NULL, &pipeRequest);
                if (connections[iOther].otherIn == NULL)
                {
                    // IOLog("start: Failed to open mystery input pipe\n");
                    goto fail;
                }
                else
                    connections[iOther].otherIn->retain();
                pipeRequest.direction = kUSBOut;
                connections[iOther].otherOut = interface->FindNextPipe(NULL, &pipeRequest);
                if (connections[iOther].otherOut == NULL)
                {
                    // IOLog("start: Failed to open mystery output pipe\n");
                    goto fail;
                }
                else
                    connections[iOther].otherOut->retain();
                iOther++;
                break;
				
            default:
                // IOLog("start: Ignoring interface (protocol %d)\n", interface->GetInterfaceProtocol());
                break;
        }
    }
    
    if (iConnection != iOther)
        IOLog("start - interface mismatch?\n");
    connectionCount = iConnection;
    
    for (i = 0; i < connectionCount; i++)
    {
        connections[i].inputArray = OSArray::withCapacity(5);
        if (connections[i].inputArray == NULL)
        {
            // IOLog("start: Failed to allocate packet buffer %d\n", i);
            goto fail;
        }
        if (!QueueRead(i))
        {
            // IOLog("start: Failed to start read %d\n", i);
            goto fail;
        }
    }
    
    // IOLog("start: Transform and roll out (%d interfaces)\n", connectionCount);
    return true;
    
fail:
    ReleaseAll();
    return false;
}

// Stop the device
void WirelessGamingReceiver::stop(IOService *provider)
{
    ReleaseAll();
    IOService::stop(provider);
}

// Handle termination
bool WirelessGamingReceiver::didTerminate(IOService *provider, IOOptionBits options, bool *defer)
{
    // release all objects used and close the device
    ReleaseAll();
    return IOService::didTerminate(provider, options, defer);
}


// Handle message from provider
IOReturn WirelessGamingReceiver::message(UInt32 type,IOService *provider,void *argument)
{
    // IOLog("Message\n");
#if 0
    switch(type) {
        case kIOMessageServiceIsTerminated:
        case kIOMessageServiceIsRequestingClose:
            if(device->isOpen(this)) ReleaseAll();
            return kIOReturnSuccess;
        default:
            return IOService::message(type,provider,argument);
    }
#else
    return IOService::message(type,provider,argument);
#endif
}

// Queue a read on a controller
bool WirelessGamingReceiver::QueueRead(int index)
{
    IOUSBCompletion complete;
    IOReturn err;
    WGRREAD *data = (WGRREAD*)IOMalloc(sizeof(WGRREAD));
    
    if (data == NULL)
        return false;
    data->index = index;
    data->buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, GetMaxPacketSize(connections[index].controllerIn));
    if (data->buffer == NULL)
    {
        IOFree(data, sizeof(WGRREAD));
        return false;
    }

    complete.target = this;
    complete.action = _ReadComplete;
    complete.parameter = data;
    
    err = connections[index].controllerIn->Read(data->buffer, 0, 0, data->buffer->getLength(), &complete);
    if (err == kIOReturnSuccess)
        return true;
        
    data->buffer->release();
    IOFree(data, sizeof(WGRREAD));
    
    // IOLog("read - failed to start (0x%.8x)\n", err);
    return false;
}

// Handle a completed read on a controller
void WirelessGamingReceiver::ReadComplete(void *parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    WGRREAD *data = (WGRREAD*)parameter;
    bool reread = true;
    
    switch (status)
    {
        case kIOReturnOverrun:
            // IOLog("read - kIOReturnOverrun, clearing stall\n");
            connections[data->index].controllerIn->ClearStall();
            // fall through
        case kIOReturnSuccess:
            ProcessMessage(data->index, (unsigned char*)data->buffer->getBytesNoCopy(), (int)data->buffer->getLength() - bufferSizeRemaining);
            break;
            
        case kIOReturnNotResponding:
            // IOLog("read - kIOReturnNotResponding\n");
            // fall through
        default:
            reread = false;
            break;
    }
    
    int newIndex = data->index;
    data->buffer->release();
    IOFree(data, sizeof(WGRREAD));
    
    if (reread)
        QueueRead(newIndex);
}

// Queue an asynchronous write on a controller
bool WirelessGamingReceiver::QueueWrite(int index, const void *bytes, UInt32 length)
{
    IOBufferMemoryDescriptor *outBuffer;
    IOUSBCompletion complete;
    IOReturn err;
    
    outBuffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, length);
    if (outBuffer == NULL)
    {
        // IOLog("send - unable to allocate buffer\n");
        return false;
    }
    outBuffer->writeBytes(0, bytes, length);
    
    complete.target = this;
    complete.action = _WriteComplete;
    complete.parameter = outBuffer;
    
    err = connections[index].controllerOut->Write(outBuffer, 0, 0, length, &complete);
    if (err == kIOReturnSuccess)
        return true;
    else
    {
        // IOLog("send - failed to start (0x%.8x)\n",err);
        return false;
    }
}

// Handle a completed write on a controller
void WirelessGamingReceiver::WriteComplete(void *parameter,IOReturn status,UInt32 bufferSizeRemaining)
{
    IOMemoryDescriptor *memory=(IOMemoryDescriptor*)parameter;
    if(status!=kIOReturnSuccess) {
        IOLog("write - Error writing: 0x%.8x\n",status);
    }
    memory->release();
}

// Release any allocated objects
void WirelessGamingReceiver::ReleaseAll(void)
{
    for (int i = 0; i < connectionCount; i++)
    {
        if (connections[i].service != NULL)
        {
            connections[i].service->terminate(kIOServiceRequired);
            connections[i].service->detachAll(gIOServicePlane);
            connections[i].service->release();
            connections[i].service = NULL;
        }
        if (connections[i].controllerIn != NULL)
        {
            connections[i].controllerIn->Abort();
            connections[i].controllerIn->release();
            connections[i].controllerIn = NULL;
        }
        if (connections[i].controllerOut != NULL)
        {
            connections[i].controllerOut->Abort();
            connections[i].controllerOut->release();
            connections[i].controllerOut = NULL;
        }
        if (connections[i].controller != NULL)
        {
            connections[i].controller->close(this);
            connections[i].controller = NULL;
        }
        if (connections[i].otherIn != NULL)
        {
            connections[i].otherIn->Abort();
            connections[i].otherIn->release();
            connections[i].otherIn = NULL;
        }
        if (connections[i].otherOut != NULL)
        {
            connections[i].otherOut->Abort();
            connections[i].otherOut->release();
            connections[i].otherOut = NULL;
        }
        if (connections[i].other != NULL)
        {
            connections[i].other->close(this);
            connections[i].other = NULL;
        }
        if (connections[i].inputArray != NULL)
        {
            connections[i].inputArray->release();
            connections[i].inputArray = NULL;
        }
        connections[i].controllerStarted = false;
    }
    if (device != NULL)
    {
        device->close(this);
        device = NULL;
    }
}

// Static wrapper for read notifications
void WirelessGamingReceiver::_ReadComplete(void *target, void *parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    if (target != NULL)
        ((WirelessGamingReceiver*)target)->ReadComplete(parameter, status, bufferSizeRemaining);
}

// Static wrapper for write notifications
void WirelessGamingReceiver::_WriteComplete(void *target, void *parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    if (target != NULL)
        ((WirelessGamingReceiver*)target)->WriteComplete(parameter, status, bufferSizeRemaining);
}

// Processes a message for a controller
void WirelessGamingReceiver::ProcessMessage(int index, const unsigned char *data, int length)
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
            IOLog("process: Device detached\n");
#endif
            if (connections[index].service != NULL)
            {
                connections[index].service->SetIndex(-1);
                if (connections[index].controllerStarted)
                    connections[index].service->terminate(kIOServiceRequired | kIOServiceSynchronous);
                connections[index].service->detach(this);
                connections[index].service->release();
                connections[index].service = NULL;
                connections[index].controllerStarted = false;
            }
        }
        else
        {
            // Device connected
#ifdef PROTOCOL_DEBUG
            IOLog("process: Attempting to add new device\n");
#endif
            if (connections[index].service == NULL)
            {
                bool ready;
                int i, j;
                IOMemoryDescriptor *data;
                char c;
                
                ready = false;
                j = connections[index].inputArray->getCount();
                for (i = 0; !ready && (i < j); i++)
                {
                    data = OSDynamicCast(IOMemoryDescriptor, connections[index].inputArray->getObject(i));
                    data->readBytes(1, &c, 1);
                    if (c == 0x0f)
                        ready = true;
                }
                InstantiateService(index);
                if (ready && connections[index].service != NULL)
                {
#ifdef PROTOCOL_DEBUG
                    IOLog("Registering wireless device");
#endif
                    connections[index].controllerStarted = true;
                    connections[index].service->registerService();
                }
            }
        }
        return;
    }
    
    // Add anything else to the queue
    IOMemoryDescriptor *copy = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, length);
    copy->writeBytes(0, data, length);
    connections[index].inputArray->setObject(copy);
    if (connections[index].service == NULL)
        InstantiateService(index);
    if (connections[index].service != NULL)
    {
        connections[index].service->NewData();
        if (!connections[index].controllerStarted)
        {
            char c;
            
            copy->readBytes(1, &c, 1);
            if (c == 0x0f)
            {
#ifdef PROTOCOL_DEBUG
                IOLog("Registering wireless device");
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
    connections[index].service = new WirelessDevice;
    if (connections[index].service != NULL)
    {
        const OSString *keys[1] = {
            OSString::withCString(kIOWirelessDeviceType),
        };
        const OSObject *objects[1] = {
            OSNumber::withNumber((unsigned long long)0, 32),
        };
        OSDictionary *dictionary = OSDictionary::withObjects(objects, keys, 1, 0);
        if (connections[index].service->init(dictionary))
        {
            connections[index].service->attach(this);
            connections[index].service->SetIndex(index);
            // connections[index].service->registerService();
            // IOLog("process: Device attached\n");
            if (IsDataQueued(index))
                connections[index].service->NewData();
            connections[index].service->start(this);
        }
        else
        {
            connections[index].service->release();
            connections[index].service = NULL;
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
    IOMemoryDescriptor *data;
    
    data = OSDynamicCast(IOMemoryDescriptor, connections[index].inputArray->getObject(0));
    if (data != NULL)
        data->retain();
    connections[index].inputArray->removeObject(0);
    return data;
}

// Get our location ID
OSNumber* WirelessGamingReceiver::newLocationIDNumber() const
{
    OSNumber *number;
    UInt32    location = 0;
    
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
