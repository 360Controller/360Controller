/*
 * Copyright (C) 2019 Medusalix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "WirelessOneDongle.h"
#include "WirelessOneMT76.h"
#include "WirelessOneController.h"

#define LOG(format, ...) IOLog("WirelessOne (Dongle): %s - " format "\n", __func__, ##__VA_ARGS__)

OSDefineMetaClassAndStructors(WirelessOneDongle, IOService)

bool WirelessOneDongle::start(IOService *provider)
{
    if (!IOService::start(provider))
    {
        LOG("super failed");
        
        return false;
    }
    
    mt = OSDynamicCast(WirelessOneMT76, provider);
    
    if (!mt)
    {
        LOG("invalid provider");
        stop(provider);
        
        return false;
    }
    
    return true;
}

void WirelessOneDongle::stop(IOService *provider)
{
    iterateControllers([](WirelessOneController *controller)
    {
        controller->terminate(kIOServiceRequired);
        controller->detachAll(gIOServicePlane);
        controller->release();
    });
    
    IOService::stop(provider);
}

void WirelessOneDongle::handleConnect(uint8_t macAddress[])
{
    if (!initController(macAddress))
    {
        LOG("failed to init controller");
        
        return;
    }
    
    if (!executeHandshake(macAddress))
    {
        LOG("failed to execute handshake");
        
        return;
    }
    
    if (!requestSerialNumber(macAddress))
    {
        LOG("failed to request serial number");
        
        return;
    }
    
    // Dim the LED a little bit, like the original driver
    if (!setLedMode(macAddress, 0x01, 0x14))
    {
        LOG("failed to set LED mode");
        
        return;
    }
    
    LOG("handshake complete");
}

void WirelessOneDongle::handleDisconnect(uint8_t macAddress[])
{
    iterateControllers([&](WirelessOneController *controller)
    {
        if (memcmp(controller->macAddress, macAddress, sizeof(controller->macAddress)))
        {
            return;
        }
        
        controller->terminate(kIOServiceRequired);
        controller->detachAll(gIOServicePlane);
        controller->release();
    });
}

void WirelessOneDongle::handleData(uint8_t macAddress[], uint8_t data[])
{
    iterateControllers([&](WirelessOneController *controller)
    {
        if (memcmp(controller->macAddress, macAddress, sizeof(controller->macAddress)))
        {
            return;
        }
        
        ControllerFrame *frame = (ControllerFrame*)data;
        
        // Input data response
        if (frame->command == 0x20 && frame->message == 0 && frame->length == 0x0e)
        {
            controller->handleInput(data + sizeof(ControllerFrame));
        }
        
        // Guide button response
        else if (frame->command == 0x07 && frame->message == 0x30 && frame->length == 0x02)
        {
            uint8_t response[] = { 0x00, 0x07, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 };
            
            ControllerFrame out = {};
            
            out.command = 0x01;
            out.message = 0x20;
            out.sequence = frame->sequence;
            out.length = 0x09;
            
            if (!send(macAddress, out, response))
            {
                LOG("failed to send guide button response");
                
                return;
            }
            
            controller->handleGuideButton(data + sizeof(ControllerFrame));
        }
        
        // Serial number response
        else if (frame->command == 0x1e && frame->message == 0x30 && frame->length == 0x10)
        {
            uint8_t response[] = { 0x00, 0x1e, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };
            
            ControllerFrame out = {};
            
            out.command = 0x01;
            out.message = 0x20;
            out.sequence = frame->sequence;
            out.length = 0x09;
            
            if (!send(macAddress, out, response))
            {
                LOG("failed to send serial number response");
                
                return;
            }
            
            controller->handleSerialNumber(data + sizeof(ControllerFrame));
        }
    });
}

uint32_t WirelessOneDongle::generateLocationId()
{
    uint32_t id = mt->getLocationId() + locationIdCounter;
    
    locationIdCounter++;
    
    return id;
}

bool WirelessOneDongle::initController(uint8_t macAddress[])
{
    WirelessOneController *controller = new WirelessOneController;
    
    if (!controller->init())
    {
        LOG("failed to init controller");
        
        return false;
    }
    
    if (!controller->attach(this))
    {
        LOG("failed to attach controller");
        
        return false;
    }
    
    controller->registerService();
    
    if (!controller->start(this))
    {
        LOG("failed to start controller");
        
        return false;
    }
    
    memcpy(controller->macAddress, macAddress, sizeof(controller->macAddress));
    
    return true;
}

template<typename T>
void WirelessOneDongle::iterateControllers(T each)
{
    OSIterator *iterator = getClientIterator();
    
    if (!iterator)
    {
        LOG("missing client iterator");
        
        return;
    }
    
    OSObject *object = nullptr;
    
    while ((object = iterator->getNextObject()))
    {
        WirelessOneController *controller = OSDynamicCast(WirelessOneController, object);
        
        if (controller)
        {
            each(controller);
        }
    }
    
    iterator->release();
}

bool WirelessOneDongle::executeHandshake(uint8_t macAddress[])
{
    ControllerFrame frame1 = {};
    ControllerFrame frame2 = {};
    uint8_t frame2Data[] = { 0x00 };
    
    frame1.command = 0x01;
    frame1.message = 0x20;
    
    frame2.command = 0x05;
    frame2.message = 0x20;
    frame2.length = sizeof(frame2Data);
    
    return send(macAddress, frame1, nullptr) && send(macAddress, frame2, frame2Data);
}

bool WirelessOneDongle::requestSerialNumber(uint8_t macAddress[])
{
    ControllerFrame frame = {};
    uint8_t data[] = { 0x04 };
    
    frame.command = 0x1e;
    frame.message = 0x30;
    frame.length = sizeof(data);
    
    return send(macAddress, frame, data);
}

bool WirelessOneDongle::setLedMode(uint8_t macAddress[], uint8_t mode, uint8_t brightness)
{
    ControllerFrame frame = {};
    
    frame.command = 0x0a;
    frame.message = 0x20;
    frame.length = sizeof(LedModeData);
    
    LedModeData data = {};
    
    data.mode = mode;
    data.brightness = brightness;
    
    return send(macAddress, frame, (uint8_t*)&data);
}

bool WirelessOneDongle::send(uint8_t macAddress[], ControllerFrame frame, uint8_t data[])
{
    OSData *out = OSData::withCapacity(sizeof(frame) + frame.length);
    
    out->appendBytes(&frame, sizeof(frame));
    out->appendBytes(data, frame.length);
    
    if (!mt->sendControllerPacket(macAddress, out))
    {
        LOG("failed to send controller packet");
        out->release();
        
        return false;
    }
    
    out->release();
    
    return true;
}
