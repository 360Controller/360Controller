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
    if (!acknowledgePacket(macAddress, nullptr) ||
        !setPowerMode(macAddress, POWER_ON)
    ) {
        LOG("failed to execute handshake");
        
        return;
    }
    
    if (!requestSerialNumber(macAddress))
    {
        LOG("failed to request serial number");
        
        return;
    }
    
    LedModeData ledMode = {};
    
    // Dim the LED a little bit, like the original driver
    // Brightness ranges from 0x00 to 0x20
    ledMode.mode = LED_ON;
    ledMode.brightness = 0x14;
    
    if (!setLedMode(macAddress, ledMode))
    {
        LOG("failed to set LED mode");
        
        return;
    }
    
    LOG("handshake sent");
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
    ControllerFrame *frame = (ControllerFrame*)data;
    
    // Serial number response, start controller
    if (frame->command == CMD_SERIAL_NUM && frame->length == sizeof(SerialData))
    {
        if (!acknowledgePacket(macAddress, frame))
        {
            LOG("failed to acknowledge serial number packet");
            
            return;
        }
        
        SerialData *serial = (SerialData*)(data + sizeof(ControllerFrame));
        
        if (!initController(macAddress, serial->serialNumber))
        {
            LOG("failed to init controller");
        }
        
        return;
    }
    
    iterateControllers([&](WirelessOneController *controller)
    {
        if (memcmp(controller->macAddress, macAddress, sizeof(controller->macAddress)))
        {
            return;
        }

        // Status packet
        if (frame->command == CMD_STATUS && frame->length == sizeof(StatusData))
        {
            controller->handleStatus((StatusData*)(data + sizeof(ControllerFrame)));
        }

        // Input packet
        if (frame->command == CMD_INPUT && frame->length == sizeof(InputData))
        {
            controller->handleInput((InputData*)(data + sizeof(ControllerFrame)));
        }

        // Guide button packet
        else if (frame->command == CMD_GUIDE_BTN && frame->length == sizeof(GuideButtonData))
        {
            if (!acknowledgePacket(macAddress, frame))
            {
                LOG("failed to acknowledge guide button packet");
               
                return;
            }
           
           controller->handleGuideButton((GuideButtonData*)(data + sizeof(ControllerFrame)));
        }
    });
}

uint32_t WirelessOneDongle::generateLocationId()
{
    uint32_t id = mt->getLocationId() + locationIdCounter;
    
    locationIdCounter++;
    
    return id;
}

bool WirelessOneDongle::initController(uint8_t macAddress[], char serialNumber[])
{
    WirelessOneController *controller = new WirelessOneController;
    
    memcpy(controller->macAddress, macAddress, sizeof(controller->macAddress));
    memcpy(controller->serialNumber, serialNumber, sizeof(controller->serialNumber) - 1);
    
    const OSString *keys[] = {
        OSString::withCString("IOCFPlugInTypes")
    };
    const OSObject *objects[] = {
        getProperty("IOCFPlugInTypes")
    };
    
    OSDictionary *dictionary = OSDictionary::withObjects(objects, keys, 1);
    
    if (!controller->init(dictionary))
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

bool WirelessOneDongle::acknowledgePacket(uint8_t macAddress[], ControllerFrame *frame)
{
    ControllerFrame out = {};
    
    out.command = CMD_ACKNOWLEDGE;
    out.type = TYPE_REQUEST;
    
    // Acknowledge empty frame
    if (!frame)
    {
        return send(macAddress, out, nullptr);
    }
    
    OSData *data = OSData::withCapacity(sizeof(ControllerFrame) + 6);
    uint64_t zero = 0;
    
    // Acknowledgement includes the received frame
    data->appendBytes(&zero, 1);
    data->appendBytes(frame, sizeof(ControllerFrame));
    data->appendBytes(&zero, 5);
    
    out.sequence = frame->sequence;
    out.length = data->getLength();
    
    bool result = send(macAddress, out, (uint8_t*)data->getBytesNoCopy());
    
    data->release();
    
    return result;
}

bool WirelessOneDongle::requestSerialNumber(uint8_t macAddress[])
{
    ControllerFrame frame = {};
    uint8_t data[] = { 0x04 };
    
    frame.command = CMD_SERIAL_NUM;
    frame.type = TYPE_REQUEST_ACK;
    frame.length = sizeof(data);
    
    return send(macAddress, frame, data);
}

bool WirelessOneDongle::setPowerMode(uint8_t macAddress[], uint8_t mode)
{
    ControllerFrame frame = {};
    uint8_t data[] = { mode };
    
    frame.command = CMD_POWER_MODE;
    frame.type = TYPE_REQUEST;
    frame.length = sizeof(data);
    
    return send(macAddress, frame, (uint8_t*)&data);
}

bool WirelessOneDongle::setLedMode(uint8_t macAddress[], LedModeData data)
{
    ControllerFrame frame = {};
    
    frame.command = CMD_LED_MODE;
    frame.type = TYPE_REQUEST;
    frame.length = sizeof(data);
    
    return send(macAddress, frame, (uint8_t*)&data);
}

bool WirelessOneDongle::rumble(uint8_t macAddress[], RumbleData data)
{
    ControllerFrame frame = {};
    
    frame.command = CMD_RUMBLE;
    frame.type = TYPE_COMMAND;
    frame.length = sizeof(data);
    
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
