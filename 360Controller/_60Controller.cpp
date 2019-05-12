/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro
 Bug fixes contributed by Cody "codeman38" Boisclair

 _60Controller.cpp - main source of the driver

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
#include <IOKit/IOLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/usb/IOUSBHostFamily.h>
#include "_60Controller.h"
#include "ChatPad.h"
#include "Controller.h"

#define kDriverSettingKey       "DeviceData"

#define kIOSerialDeviceType   "Serial360Device"

OSDefineMetaClassAndStructors(Xbox360Peripheral, IOService)
#define super IOService

class LockRequired
{
private:
    IOLock* _lock;
public:
    LockRequired(IOLock* lock)
    {
        _lock = lock;
        IOLockLock(_lock);
    }
    ~LockRequired()
    {
        IOLockUnlock(_lock);
    }
};

// Find the maximum packet size of this pipe
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

void Xbox360Peripheral::SendSpecial(UInt16 value)
{
    DeviceRequest controlReq = {};

    controlReq.bmRequestType = makeDeviceRequestbmRequestType(kRequestDirectionOut, kRequestTypeVendor, kRequestRecipientInterface);
    controlReq.bRequest = 0x00;
    controlReq.wValue = value;
    controlReq.wIndex = 0x0002;
    controlReq.wLength = 0;

    if (device->deviceRequest(this, controlReq, (void*)nullptr, nullptr, 100) != kIOReturnSuccess)
    {
        IOLog("Failed to send special message %.4x\n", value);
    }
}

void Xbox360Peripheral::SendInit(UInt16 value, UInt16 index)
{
    DeviceRequest controlReq = {};

    controlReq.bmRequestType = makeDeviceRequestbmRequestType(kRequestDirectionOut, kRequestTypeVendor, kRequestRecipientDevice);
    controlReq.bRequest = 0xa9;
    controlReq.wValue = value;
    controlReq.wIndex = index;
    controlReq.wLength = 0;

    device->deviceRequest(this, controlReq, (void*)nullptr, nullptr, 100);	// Will fail - but device should still act on it
}

bool Xbox360Peripheral::SendSwitch(bool sendOut)
{
    DeviceRequest controlReq = {};

    controlReq.bmRequestType = makeDeviceRequestbmRequestType(sendOut ? kRequestDirectionOut : kRequestDirectionIn, kRequestTypeVendor, kRequestRecipientDevice);
    controlReq.bRequest = 0xa1;
    controlReq.wValue = 0x0000;
    controlReq.wIndex = 0xe416;
    controlReq.wLength = sizeof(chatpadInit);

    IOReturn err = device->deviceRequest(this, controlReq, (void*)&chatpadInit, nullptr, 100);
    if (err == kIOReturnSuccess)
    {
        return true;
    }

    const char* errStr = device->stringFromReturn(err);
    IOLog("start - failed to %s chatpad setting (%x): %s\n", sendOut ? "write" : "read", err, errStr);
    return false;
}

void Xbox360Peripheral::SendToggle(void)
{
    SendSpecial(serialToggle ? 0x1F : 0x1E);
    serialToggle = !serialToggle;
}

void Xbox360Peripheral::ChatPadTimerActionWrapper(OSObject* owner, IOTimerEventSource* sender)
{
    Xbox360Peripheral* controller;

    controller = OSDynamicCast(Xbox360Peripheral, owner);
    controller->ChatPadTimerAction(sender);
}

void Xbox360Peripheral::ChatPadTimerAction(IOTimerEventSource* sender)
{
    int nextTime = 1000;
    int serialGot = 0;

    switch (serialTimerState)
    {
        case tsToggle:
        {
            SendToggle();
            if (serialActive)
            {
                if (!serialHeard)
                {
                    serialActive = false;
                    serialGot = 2;
                }
            }
            else
            {
                if (serialHeard)
                {
                    serialTimerState = tsReset1;
                    serialResetCount = 0;
                    nextTime = 40;
                }
            }
        } break;

        case tsMiniToggle:
        {
            SendToggle();
            if (serialHeard)
            {
                serialTimerState = tsSet1;
                nextTime = 40;
            }
            else
            {
                serialResetCount++;
                if (serialResetCount > 3)
                {
                    serialTimerState = tsToggle;
                }
                else
                {
                    serialTimerState = tsReset1;
                    nextTime = 40;
                }
            }
        } break;

        case tsReset1:
        {
            SendSpecial(0x1B);
            serialTimerState = tsReset2;
            nextTime = 35;
        } break;

        case tsReset2:
        {
            SendSpecial(0x1B);
            serialTimerState = tsMiniToggle;
            nextTime = 150;
        } break;

        case tsSet1:
        {
            SendSpecial(0x18);
            serialTimerState = tsSet2;
            nextTime = 10;
        } break;

        case tsSet2:
        {
            SendSpecial(0x10);
            serialTimerState = tsSet3;
            nextTime = 10;
        } break;

        case tsSet3:
        {
            SendSpecial(0x03);
            serialTimerState = tsToggle;
            nextTime = 940;
            serialActive = true;
            serialGot = 1;
        } break;
    }

    sender->setTimeoutMS(nextTime);	// Todo: Make it take into account function execution time?
    serialHeard = false;
    // Make it happen after the timer's set, for minimum impact
    switch (serialGot)
    {
        case 1:
        {
            SerialConnect();
        } break;

        case 2:
        {
            SerialDisconnect();
        } break;

        default:
        {

        } break;
    }
}

// Read the settings from the registry
void Xbox360Peripheral::readSettings(void)
{
    OSBoolean* value = NULL;
    OSNumber* number = NULL;
    OSDictionary* dataDictionary = OSDynamicCast(OSDictionary, getProperty(kDriverSettingKey));

    if (dataDictionary == NULL) return;
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertLeftX"));
    if (value != NULL) invertLeftX = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertLeftY"));
    if (value != NULL) invertLeftY = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertRightX"));
    if (value != NULL) invertRightX = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("InvertRightY"));
    if (value != NULL) invertRightY = value->getValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("DeadzoneLeft"));
    if (number != NULL) deadzoneLeft = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("DeadzoneRight"));
    if (number != NULL) deadzoneRight = number->unsigned32BitValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("RelativeLeft"));
    if (value != NULL) relativeLeft = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("RelativeRight"));
    if (value != NULL) relativeRight=value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("DeadOffLeft"));
    if (value != NULL) deadOffLeft = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("DeadOffRight"));
    if (value != NULL) deadOffRight = value->getValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("RumbleType"));
    if (number != NULL) rumbleType = number->unsigned8BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingUp"));
    if (number != NULL) mapping[0] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingDown"));
    if (number != NULL) mapping[1] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingLeft"));
    if (number != NULL) mapping[2] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingRight"));
    if (number != NULL) mapping[3] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingStart"));
    if (number != NULL) mapping[4] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingBack"));
    if (number != NULL) mapping[5] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingLSC"));
    if (number != NULL) mapping[6] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingRSC"));
    if (number != NULL) mapping[7] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingLB"));
    if (number != NULL) mapping[8] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingRB"));
    if (number != NULL) mapping[9] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingGuide"));
    if (number != NULL) mapping[10] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingA"));
    if (number != NULL) mapping[11] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingB"));
    if (number != NULL) mapping[12] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingX"));
    if (number != NULL) mapping[13] = number->unsigned32BitValue();
    number = OSDynamicCast(OSNumber, dataDictionary->getObject("BindingY"));
    if (number != NULL) mapping[14] = number->unsigned32BitValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("SwapSticks"));
    if (value != NULL) swapSticks = value->getValue();
    value = OSDynamicCast(OSBoolean, dataDictionary->getObject("Pretend360"));
    if (value != NULL) pretend360 = value->getValue();

#if 0
    IOLog("Xbox360Peripheral preferences loaded:\n  invertLeft X: %s, Y: %s\n   invertRight X: %s, Y:%s\n  deadzone Left: %d, Right: %d\n\n",
          invertLeftX?"True":"False",invertLeftY?"True":"False",
          invertRightX?"True":"False",invertRightY?"True":"False",
          deadzoneLeft,deadzoneRight);
#endif
}

// Initialise the extension
bool Xbox360Peripheral::init(OSDictionary* propTable)
{
    bool res = super::init(propTable);
    mainLock = IOLockAlloc();
    device = nullptr;
    interface = nullptr;
    inPipe = nullptr;
    outPipe = nullptr;
    inBuffer = nullptr;
    padHandler = nullptr;
    serialIn = nullptr;
    serialInPipe = nullptr;
    serialInBuffer = nullptr;
    serialTimer = nullptr;
    serialHandler = nullptr;
    // Default settings
    invertLeftX = invertLeftY = false;
    invertRightX = invertRightY = false;
    deadzoneLeft = deadzoneRight = 0;
    relativeLeft = relativeRight = false;
    deadOffLeft = false;
    deadOffRight = false;
    swapSticks = false;
    pretend360 = false;
    // Controller Specific
    rumbleType = 0;
    // Bindings
    noMapping = true;
    for (int i = 0; i < 11; i++)
    {
        mapping[i] = i;
    }
    for (int i = 12; i < 16; i++)
    {
        mapping[i-1] = i;
    }
    // Done
    return res;
}

// Free the extension
void Xbox360Peripheral::free(void)
{
    IOLockFree(mainLock);
    super::free();
}

bool Xbox360Peripheral::start(IOService* provider)
{
    const ConfigurationDescriptor* cd = nullptr;
    const InterfaceDescriptor* intf = nullptr;
    const EndpointDescriptor* pipe = nullptr;
    IOWorkLoop* workloop = nullptr;
    XBOX360_OUT_LED led = {};

    if (!super::start(provider))
    {
        kprintf("start - Failed super start method.\n");
        return false;
    }

    // Get device
    device = OSDynamicCast(IOUSBHostDevice, provider);
    if (device == nullptr)
    {
        kprintf("start - Failed to cast provider.\n");
        goto fail;
    }

    // TODO(Drew): Remove this for full release. Looking to see if we have a consistent class/subclass/protocol across devices for better matching.
//    IOLog("360Controller - start() - Starting for device with class: %d, sub class: %d and protocol: %d\n",
//          device->getDeviceDescriptor()->bDeviceClass,
//          device->getDeviceDescriptor()->bDeviceSubClass,
//          device->getDeviceDescriptor()->bDeviceProtocol);

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
        kprintf("start - Couldn't get configuration descriptor.\n");
        goto fail;
    }

    // Open
    if (!device->open(this))
    {
        device = nullptr;
        kprintf("start - Unable to open device.\n");
        goto fail;
    }

    if (device->setConfiguration(cd->bConfigurationValue, true) != kIOReturnSuccess)
    {
        kprintf("start - Unable to set configuration.\n");
        goto fail;
    }

    // Get release
    {
        UInt16 release = USBToHost16(device->getDeviceDescriptor()->bcdDevice);
        switch (release)
        {
            default:
            {
                kprintf("start - Unknown device release %.4x\n", release);
            } // fall through
            case 0x0110:
            {
                chatpadInit[0] = 0x01;
                chatpadInit[1] = 0x02;
            } break;
            case 0x0114:
            {
                chatpadInit[0] = 0x09;
                chatpadInit[1] = 0x00;
            } break;
        }
    }

    // Find correct interface
    controllerType = Xbox360;
    {
        OSIterator* iterator = device->getChildIterator(gIOServicePlane);
        OSObject* candidate = nullptr;
        while ((iterator != nullptr) && ((candidate = iterator->getNextObject()) != nullptr))
        {
            IOUSBHostInterface* interfaceCandidate = OSDynamicCast(IOUSBHostInterface, candidate);
            if (interfaceCandidate != nullptr)
            {
                const StandardUSB::InterfaceDescriptor* id = interfaceCandidate->getInterfaceDescriptor();

                if (id->bInterfaceSubClass == 93)
                {
                    if (id->bInterfaceProtocol == 1)
                    {
                        controllerType = Xbox360;
                        interface = interfaceCandidate;
                    }
                    else if (id->bInterfaceProtocol == 2)
                    {
                        controllerType = Xbox360;
                        serialIn = interfaceCandidate;
                    }
                }
                else if ((id->bInterfaceSubClass == 66) && (id->bInterfaceProtocol == 0))
                {
                    controllerType = XboxOriginal;
                    interface = interfaceCandidate;
                }
                else if ((id->bInterfaceClass == 255) && (id->bInterfaceSubClass == 71) && (id->bInterfaceProtocol == 208))
                {
                    if (id->bInterfaceNumber == 0)
                    {
                        controllerType = XboxOne;
                        interface = interfaceCandidate;
                    }
                    else if ((id->bInterfaceNumber == 1) && (id->bAlternateSetting == 1))
                    {
                        // TODO(Drew): Isoc pipes for controller audio
                    }
                    else if ((id->bInterfaceNumber == 2) && (id->bAlternateSetting == 1))
                    {
                        // TODO(Drew): Proprietary port at the bottom of the controller
                    }
                }
            }
        }

        OSSafeReleaseNULL(iterator);
    }

    if (interface == nullptr)
    {
        kprintf("start - Unable to find the interface.\n");
        goto fail;
    }

    if (!interface->open(this))
    {
        kprintf("start - unable to open interface.\n");
        goto fail;
    }

    cd = interface->getConfigurationDescriptor();
    intf = interface->getInterfaceDescriptor();
    if (!cd || !intf)
    {
        kprintf("start - Can't get configuration descriptor and interface descriptor.\n");
        goto fail;
    }

    while ((pipe = StandardUSB::getNextEndpointDescriptor(cd, intf, pipe)))
    {
        UInt8 pipeDirection = StandardUSB::getEndpointDirection(pipe);
        UInt8 pipeType = StandardUSB::getEndpointType(pipe);

        if ((pipeDirection == kEndpointDirectionIn) && (pipeType == kEndpointTypeInterrupt))
        {
            inPipe = interface->copyPipe(StandardUSB::getEndpointAddress(pipe));
        }
        else if ((pipeDirection == kEndpointDirectionOut) && (pipeType == kEndpointTypeInterrupt))
        {
            outPipe = interface->copyPipe(StandardUSB::getEndpointAddress(pipe));
        }
    }

    if (inPipe == nullptr)
    {
        kprintf("start - Unable to find in pipe.\n");
        goto fail;
    }
    inPipe->retain();

    if (outPipe == nullptr)
    {
        kprintf("start - Unable to find out pipe.\n");
        goto fail;
    }
    outPipe->retain();

    // Get a buffer
    inBuffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, GetMaxPacketSize(inPipe));
    if (inBuffer == nullptr)
    {
        kprintf("start - Failed to allocate input buffer.\n");
        goto fail;
    }

    if (serialIn == nullptr)
    {
        kprintf("start - unable to find chatpad interface.\n");
        goto nochat;
    }

    // Open
    if (!serialIn->open(this))
    {
        kprintf("start - unable to open chatpad interface.\n");
        ReleaseChatpad();
        goto nochat;
    }

    // Find chatpad pipe
    cd = serialIn->getConfigurationDescriptor();
    intf = serialIn->getInterfaceDescriptor();
    if (!cd || !intf)
    {
        kprintf("start - Can't get configuration descriptor and interface descriptor for chatpad.\n");
        ReleaseChatpad();
        goto nochat;
    }

    while ((pipe = StandardUSB::getNextEndpointDescriptor(cd, intf, pipe)))
    {
        UInt8 pipeDirection = StandardUSB::getEndpointDirection(pipe);
        UInt8 pipeType = StandardUSB::getEndpointType(pipe);

        if ((pipeDirection == kEndpointDirectionIn) && (pipeType == kEndpointTypeInterrupt))
        {
            serialInPipe = interface->copyPipe(StandardUSB::getEndpointAddress(pipe));
        }
    }

    if (serialInPipe == nullptr)
    {
        kprintf("start - Unable to find chatpad in pipe.\n");
        ReleaseChatpad();
        goto nochat;
    }
    serialInPipe->retain();

    // Get a buffer for the chatpad
    serialInBuffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, GetMaxPacketSize(serialInPipe));
    if (serialInBuffer == nullptr)
    {
        kprintf("start - failed to allocate input buffer for chatpad.\n");
        ReleaseChatpad();
        goto nochat;
    }

    // Create timer for chatpad
    serialTimer = IOTimerEventSource::timerEventSource(this, ChatPadTimerActionWrapper);
    if (serialTimer == nullptr)
    {
        kprintf("start - Failed to create timer for chatpad.\n");
        ReleaseChatpad();
        goto nochat;
    }

    workloop = getWorkLoop();
    if ((workloop == nullptr) || (workloop->addEventSource(serialTimer) != kIOReturnSuccess))
    {
        kprintf("start - Failed to connect timer for chatpad.\n");
        ReleaseChatpad();
        goto nochat;
    }

    // Configure ChatPad
    // Send 'configuration'
    SendInit(0xa30c, 0x4423);
    SendInit(0x2344, 0x7f03);
    SendInit(0x5839, 0x6832);
    // Set 'switch'
    if ((!SendSwitch(false)) || (!SendSwitch(true)) || (!SendSwitch(false)))
    {
        // Commenting goto fail fixes the driver for the Hori Real Arcade Pro EX
        ReleaseChatpad();
        goto nochat;
    }

    // Begin toggle
    serialHeard = false;
    serialActive = false;
    serialToggle = false;
    serialResetCount = 0;
    serialTimerState = tsToggle;
    serialTimer->setTimeoutMS(1000);
    // Begin reading
    if (!QueueSerialRead())
    {
        ReleaseChatpad();
        goto nochat;
    }

nochat:
    if (!QueueRead())
    {
        goto fail;
    }

    if ((XboxOne == controllerType) || (XboxOnePretend360 == controllerType))
    {
        UInt8 xoneInit0[] = { 0x01, 0x20, 0x00, 0x09, 0x00, 0x04, 0x20, 0x3a, 0x00, 0x00, 0x00, 0x80, 0x00 };
        UInt8 xoneInit1[] = { 0x05, 0x20, 0x00, 0x01, 0x00 };
        UInt8 xoneInit2[] = { 0x09, 0x00, 0x00, 0x09, 0x00, 0x0F, 0x00, 0x00, 0x1D, 0x1D, 0xFF, 0x00, 0x00 };
        UInt8 xoneInit3[] = { 0x09, 0x00, 0x00, 0x09, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        QueueWrite(&xoneInit0, sizeof(xoneInit0));
        QueueWrite(&xoneInit1, sizeof(xoneInit1));
        QueueWrite(&xoneInit2, sizeof(xoneInit2));
        QueueWrite(&xoneInit3, sizeof(xoneInit3));
    }
    else
    {
        // Disable LED
        Xbox360_Prepare(led, outLed);
        led.pattern = ledOff;
        QueueWrite(&led, sizeof(led));
    }

    // Done
    PadConnect();
    registerService();
    return true;

fail:
    ReleaseAll();
    return false;
}

// Set up an asynchronous read
bool Xbox360Peripheral::QueueRead(void)
{
    IOUSBHostCompletion complete = {};
    IOReturn err = kIOReturnError;

    if ((inPipe == nullptr) || (inBuffer == nullptr))
    {
        return false;
    }

    complete.owner = this;
    complete.action = ReadCompleteInternal;
    complete.parameter = inBuffer;

    err = inPipe->io(inBuffer, (UInt32)inBuffer->getLength(), &complete);
    if (err == kIOReturnSuccess)
    {
        return true;
    }
    else
    {
        IOLog("read - failed to start (0x%.8x)\n", err);
        return false;
    }
}

bool Xbox360Peripheral::QueueSerialRead(void)
{
    IOUSBHostCompletion complete = {};
    IOReturn err = kIOReturnError;

    if ((serialInPipe == nullptr) || (serialInBuffer == nullptr))
    {
        return false;
    }

    complete.owner = this;
    complete.action = SerialReadCompleteInternal;
    complete.parameter = serialInBuffer;

    err = serialInPipe->io(serialInBuffer, (UInt32)serialInBuffer->getLength(), &complete);
    if (err == kIOReturnSuccess)
    {
        return true;
    }
    else
    {
        IOLog("read - failed to start for chatpad (0x%.8x)\n",err);
        return false;
    }
}

// Set up an asynchronous write
bool Xbox360Peripheral::QueueWrite(const void* bytes, UInt32 length)
{
    IOBufferMemoryDescriptor* outBuffer = nullptr;
    IOUSBHostCompletion complete = {};
    IOReturn err = kIOReturnError;

    outBuffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, length);
    if (outBuffer == nullptr)
    {
        IOLog("send - unable to allocate buffer\n");
        return false;
    }

    outBuffer->writeBytes(0, bytes, length);

    complete.owner = this;
    complete.action = WriteCompleteInternal;
    complete.parameter = outBuffer;
    err = outPipe->io(outBuffer, (UInt32)length, &complete);
    if (err == kIOReturnSuccess)
    {
        return true;
    }
    else
    {
        IOLog("send - failed to start (0x%.8x)\n", err);
        return false;
    }
}

bool Xbox360Peripheral::willTerminate(IOService* provider, IOOptionBits options)
{
    ReleaseAll();

    return super::willTerminate(provider, options);
}

void Xbox360Peripheral::stop(IOService* provider)
{
    ReleaseAll();

    super::stop(provider);
}

// Releases all the objects used
void Xbox360Peripheral::ReleaseAll(void)
{
    LockRequired locker(mainLock);

    SerialDisconnect();
    PadDisconnect();

    ReleaseChatpad();

    if (serialTimer != nullptr)
    {
        serialTimer->cancelTimeout();
        getWorkLoop()->removeEventSource(serialTimer);
        serialTimer->release();
        serialTimer = nullptr;
    }

    if (serialInPipe != nullptr)
    {
        serialInPipe->abort();
        serialInPipe->release();
        serialInPipe = nullptr;
    }

    if (serialInBuffer != nullptr)
    {
        serialInBuffer->release();
        serialInBuffer = nullptr;
    }

    if (outPipe != nullptr)
    {
        outPipe->abort();
        outPipe->release();
        outPipe = nullptr;
    }

    if (inPipe != nullptr)
    {
        inPipe->abort();
        inPipe->release();
        inPipe = nullptr;
    }

    if (inBuffer != nullptr)
    {
        inBuffer->release();
        inBuffer = nullptr;
    }

    if (interface != nullptr)
    {
        interface->close(this);
        interface = nullptr;
    }

    if (device != nullptr)
    {
        device->close(this);
        device = nullptr;
    }
}

void Xbox360Peripheral::ReleaseChatpad(void)
{
    if (serialIn != nullptr)
    {
        serialIn->close(this);
        serialIn = nullptr;
    }
}

// Handle termination
bool Xbox360Peripheral::didTerminate(IOService* provider, IOOptionBits options, bool* defer)
{
    // release all objects used and close the device
    ReleaseAll();
    return super::didTerminate(provider, options, defer);
}


// Handle message sent to the driver
IOReturn Xbox360Peripheral::message(UInt32 type, IOService* provider, void* argument)
{
    switch (type)
    {
        case kIOMessageServiceIsTerminated:
        case kIOMessageServiceIsRequestingClose:
        default:
        {
            return super::message(type, provider, argument);
        }
    }
}

// This returns the abs() value of a short, swapping it if necessary
static inline Xbox360_SShort getAbsolute(Xbox360_SShort value)
{
    Xbox360_SShort reverse;

#ifdef __LITTLE_ENDIAN__
    reverse=value;
#elif __BIG_ENDIAN__
    reverse=((value&0xFF00)>>8)|((value&0x00FF)<<8);
#else
#error Unknown CPU byte order
#endif
    return (reverse<0)?~reverse:reverse;
}

void Xbox360Peripheral::normalizeAxis(SInt16& axis, short deadzone)
{
    static const UInt16 max16 = 32767;
    const float current = getAbsolute(axis);
    const float maxVal = max16 - deadzone;

    if (current > deadzone)
    {
        if (axis < 0)
        {
            axis = max16 * (current - deadzone) / maxVal;
            axis = ~axis;
        }
        else
        {
            axis = max16 * (current - deadzone) / maxVal;
        }
    }
    else
    {
        axis =0;
    }
}

void Xbox360Peripheral::fiddleReport(XBOX360_HAT& left, XBOX360_HAT& right)
{
    // deadOff - Normalize checkbox is checked if true
    // relative - Linked checkbox is checked if true

    if (invertLeftX) left.x = ~left.x;
    if (!invertLeftY) left.y = ~left.y;
    if (invertRightX) right.x = ~right.x;
    if (!invertRightY) right.y = ~right.y;

    if (deadzoneLeft != 0)
    {
        if (relativeLeft)
        {
            if ((getAbsolute(left.x) < deadzoneLeft) && (getAbsolute(left.y) < deadzoneLeft))
            {
                left.x = 0;
                left.y = 0;
            }
            else if (deadOffLeft)
            {
                normalizeAxis(left.x, deadzoneLeft);
                normalizeAxis(left.y, deadzoneLeft);
            }
        }
        else // Linked checkbox has no check
        {
            if (getAbsolute(left.x) < deadzoneLeft)
            {
                left.x = 0;
            }
            else if (deadOffLeft)
            {
                normalizeAxis(left.x, deadzoneLeft);
            }

            if (getAbsolute(left.y) < deadzoneLeft)
            {
                left.y = 0;
            }
            else if (deadOffLeft)
            {
                normalizeAxis(left.y, deadzoneLeft);
            }
        }
    }

    if (deadzoneRight != 0)
    {
        if (relativeRight)
        {
            if ((getAbsolute(right.x) < deadzoneRight) && (getAbsolute(right.y) < deadzoneRight))
            {
                right.x = 0;
                right.y = 0;
            }
            else if (deadOffRight)
            {
                normalizeAxis(left.x, deadzoneRight);
                normalizeAxis(left.y, deadzoneRight);
            }
        }
        else
        {
            if (getAbsolute(right.x) < deadzoneRight)
            {
                right.x = 0;
            }
            else if (deadOffRight)
            {
                normalizeAxis(right.x, deadzoneRight);
            }

            if (getAbsolute(right.y) < deadzoneRight)
            {
                right.y = 0;
            }
            else if (deadOffRight)
            {
                normalizeAxis(right.y, deadzoneRight);
            }
        }
    }
}

// This forwards a completed read notification to a member function
void Xbox360Peripheral::ReadCompleteInternal(void* target, void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    if (target != nullptr)
    {
        ((Xbox360Peripheral*)target)->ReadComplete(parameter, status, bufferSizeRemaining);
    }
}

void Xbox360Peripheral::SerialReadCompleteInternal(void* target, void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    if (target != nullptr)
    {
        ((Xbox360Peripheral*)target)->SerialReadComplete(parameter, status, bufferSizeRemaining);
    }
}

// This forwards a completed write notification to a member function
void Xbox360Peripheral::WriteCompleteInternal(void* target, void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    if (target != nullptr)
    {
        ((Xbox360Peripheral*)target)->WriteComplete(parameter, status, bufferSizeRemaining);
    }
}

// This handles a completed asynchronous read
void Xbox360Peripheral::ReadComplete(void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    if (padHandler) // avoid deadlock with release
    {
        LockRequired locker(mainLock);
        IOReturn err = kIOReturnError;
        bool reread = !isInactive();

        switch (status)
        {
            case kIOReturnOverrun:
            {
                IOLog("read - kIOReturnOverrun, clearing stall\n");
                if (inPipe != nullptr)
                {
                    inPipe->clearStall(false);
                }
            } // Fall through

            case kIOReturnSuccess:
            {
                if (inBuffer != nullptr)
                {
                    const XBOX360_IN_REPORT* report = (const XBOX360_IN_REPORT*)inBuffer->getBytesNoCopy();
                    if (((report->header.command == inReport) && (report->header.size == sizeof(XBOX360_IN_REPORT)))
                       || (report->header.command == 0x20) || (report->header.command == 0x07)) /* Xbox One */
                    {
                        err = padHandler->handleReport(inBuffer, kIOHIDReportTypeInput);
                        if (err != kIOReturnSuccess)
                        {
                            IOLog("read - failed to handle report: 0x%.8x\n", err);
                        }
                    }
                }
            } break;

            case kIOReturnNotResponding:
            {
                IOLog("read - kIOReturnNotResponding\n");
                reread = false;
            } break;

            default:
            {
                reread = false;
            } break;
        }

        if (reread)
        {
            QueueRead();
        }
    }
}

void Xbox360Peripheral::SerialReadComplete(void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    if (padHandler != nullptr) // avoid deadlock with release
    {
        LockRequired locker(mainLock);
        bool reread = !isInactive();

        switch (status)
        {
            case kIOReturnOverrun:
            {
                IOLog("read (serial) - kIOReturnOverrun, clearing stall\n");
                if (serialInPipe != nullptr)
                {
                    serialInPipe->clearStall(false);
                }
            } // Fall through

            case kIOReturnSuccess:
            {
                serialHeard = true;
                if (serialInBuffer != nullptr)
                {
                    SerialMessage(serialInBuffer, serialInBuffer->getCapacity() - bufferSizeRemaining);
                }
            } break;

            case kIOReturnNotResponding:
            {
                IOLog("read (serial) - kIOReturnNotResponding\n");
                reread = false;
            } break;

            default:
            {
                reread = false;
            } break;
        }

        if (reread)
        {
            QueueSerialRead();
        }
    }
}

// Handle a completed asynchronous write
void Xbox360Peripheral::WriteComplete(void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    IOMemoryDescriptor* memory = (IOMemoryDescriptor*)parameter;
    if (status != kIOReturnSuccess)
    {
        IOLog("write - Error writing: 0x%.8x\n", status);
    }
    memory->release();
}


void Xbox360Peripheral::MakeSettingsChanges()
{
    if (controllerType == XboxOne)
    {
        if (pretend360)
        {
            controllerType = XboxOnePretend360;
            PadConnect();
        }
    }
    else if (controllerType == XboxOnePretend360)
    {
        if (!pretend360)
        {
            controllerType = XboxOne;
            PadConnect();
        }
    }

    if (controllerType == Xbox360)
    {
        if (pretend360)
        {
            controllerType = Xbox360Pretend360;
            PadConnect();
        }
    }
    else if (controllerType == Xbox360Pretend360)
    {
        if (!pretend360)
        {
            controllerType = Xbox360;
            PadConnect();
        }
    }

    noMapping = true;
    UInt8 normalMapping[15] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15 };
    for (int i = 0; i < 15; i++)
    {
        if (normalMapping[i] != mapping[i])
        {
            noMapping = false;
            break;
        }
    }
}


// Called by the userspace IORegistryEntrySetCFProperties function
IOReturn Xbox360Peripheral::setProperties(OSObject* properties)
{
    OSDictionary* dictionary;

    dictionary = OSDynamicCast(OSDictionary, properties);

    if (dictionary != nullptr)
    {
        dictionary->setObject(OSString::withCString("ControllerType"), OSNumber::withNumber(controllerType, 8));
        setProperty(kDriverSettingKey, dictionary);
        readSettings();

        MakeSettingsChanges();

        return kIOReturnSuccess;
    }
    else
    {
        return kIOReturnBadArgument;
    }
}

IOHIDDevice* Xbox360Peripheral::getController(int index)
{
    switch (index)
    {
        case 0:
        {
            return padHandler;
        } break;

        case 1:
        {
            return serialHandler;
        } break;

        default:
        {
            return nullptr;
        } break;
    }
}

// Main controller support
void Xbox360Peripheral::PadConnect(void)
{
    PadDisconnect();

    if (controllerType == XboxOriginal)
    {
        padHandler = new XboxOriginalControllerClass;
    }
    else if (controllerType == XboxOne)
    {
        padHandler = new XboxOneControllerClass;
    }
    else if (controllerType == XboxOnePretend360)
    {
        padHandler = new XboxOnePretend360Class;
    }
    else if (controllerType == Xbox360Pretend360)
    {
        padHandler = new Xbox360Pretend360Class;
    }
    else
    {
        padHandler = new Xbox360ControllerClass;
    }

    if (padHandler != nullptr)
    {
        const OSString* keys[] = {
            OSString::withCString(kIOSerialDeviceType),
            OSString::withCString("IOCFPlugInTypes"),
            OSString::withCString("IOKitDebug"),
        };
        const OSObject* objects[] = {
            OSNumber::withNumber((unsigned long long)1, 32),
            getProperty("IOCFPlugInTypes"),
            OSNumber::withNumber((unsigned long long)65535, 32),
        };
        OSDictionary* dictionary = OSDictionary::withObjects(objects, keys, sizeof(keys) / sizeof(keys[0]));
        if (padHandler->init(dictionary))
        {
            padHandler->attach(this);
            padHandler->start(this);
        }
        else
        {
            padHandler->release();
            padHandler = nullptr;
        }
    }
}

void Xbox360Peripheral::PadDisconnect(void)
{
    if (padHandler != nullptr)
    {
        padHandler->terminate(kIOServiceRequired | kIOServiceSynchronous);
        padHandler->release();
        padHandler = nullptr;
    }
}

// Serial peripheral support

void Xbox360Peripheral::SerialConnect(void)
{
    SerialDisconnect();
    serialHandler = new ChatPadKeyboardClass;
    if (serialHandler != nullptr)
    {
        const OSString* keys[] = {
            OSString::withCString(kIOSerialDeviceType),
        };
        const OSObject* objects[] = {
            OSNumber::withNumber((unsigned long long)0, 32),
        };
        OSDictionary* dictionary = OSDictionary::withObjects(objects, keys, sizeof(keys) / sizeof(keys[0]), 0);
        if (serialHandler->init(dictionary))
        {
            serialHandler->attach(this);
            serialHandler->start(this);
        }
        else
        {
            serialHandler->release();
            serialHandler = nullptr;
        }
    }
}

void Xbox360Peripheral::SerialDisconnect(void)
{
    if (serialHandler != nullptr)
    {
        // Hope it's okay to terminate twice...
        serialHandler->terminate(kIOServiceRequired | kIOServiceSynchronous);
        serialHandler->release();
        serialHandler = nullptr;
    }
}

void Xbox360Peripheral::SerialMessage(IOBufferMemoryDescriptor* data, size_t length)
{
    if (serialHandler != nullptr)
    {
        char* buffer = (char*)data->getBytesNoCopy();
        if ((length == 5) && (buffer[0] == 0x00))
        {
            serialHandler->handleReport(data, kIOHIDReportTypeInput);
        }
    }
}
