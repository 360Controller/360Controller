/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    _60Controller.h - declaration of the driver main class
    
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
#ifndef __XBOX360CONTROLLER_H__
#define __XBOX360CONTROLLER_H__

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>

class Xbox360ControllerClass;
class ChatPadKeyboardClass;

class Xbox360Peripheral : public IOService
{
    OSDeclareDefaultStructors(Xbox360Peripheral)

private:
    void ReleaseAll(void);
    bool QueueRead(void);
	bool QueueSerialRead(void);

	static void SerialReadCompleteInternal(void *target,void *parameter,IOReturn status,UInt32 bufferSizeRemaining);
    static void ReadCompleteInternal(void *target,void *parameter,IOReturn status,UInt32 bufferSizeRemaining);
    static void WriteCompleteInternal(void *target,void *parameter,IOReturn status,UInt32 bufferSizeRemaining);
    
	void SerialReadComplete(void *parameter, IOReturn status, UInt32 bufferSizeRemaining);

    void readSettings(void);

	static void ChatPadTimerActionWrapper(OSObject *owner, IOTimerEventSource *sender);
	void ChatPadTimerAction(IOTimerEventSource *sender);
	void SendToggle(void);
	void SendSpecial(UInt16 value);
	void SendInit(UInt16 value, UInt16 index);
    bool SendSwitch(bool sendOut);
	
	void PadConnect(void);
	void PadDisconnect(void);
	
	void SerialConnect(void);
	void SerialDisconnect(void);
	void SerialMessage(IOBufferMemoryDescriptor *data, size_t length);

protected:
	typedef enum TIMER_STATE {
		tsToggle,
		tsReset1,
		tsReset2,
		tsMiniToggle,
		tsSet1,
		tsSet2,
		tsSet3,
	} TIMER_STATE;
    
    typedef enum CONTROLLER_TYPE {
        Xbox360 = 0,
        XboxOriginal = 1,
        XboxOne = 2
    } CONTROLLER_TYPE;
	
    IOUSBDevice *device;
    IOLock *mainLock;
	
	// Joypad
    IOUSBInterface *interface;
    IOUSBPipe *inPipe,*outPipe;
    IOBufferMemoryDescriptor *inBuffer;
	
	// Keyboard
	IOUSBInterface *serialIn;
	IOUSBPipe *serialInPipe;
    IOBufferMemoryDescriptor *serialInBuffer;
	IOTimerEventSource *serialTimer;
	bool serialToggle, serialHeard, serialActive;
	int serialResetCount;
	TIMER_STATE serialTimerState;
	ChatPadKeyboardClass *serialHandler;
	Xbox360ControllerClass *padHandler;
    UInt8 chatpadInit[2];
    CONTROLLER_TYPE controllerType;

    // Settings
    bool invertLeftX,invertLeftY;
    bool invertRightX,invertRightY;
    short deadzoneLeft,deadzoneRight;
    bool relativeLeft,relativeRight;
    bool deadOffLeft, deadOffRight;
    
public:
    // Controller specific
    UInt8 rumbleType;

    UInt8 mapping[15];
    
    // this is from the IORegistryEntry - no provider yet
    virtual bool init(OSDictionary *propTable);
    virtual void free(void);

	bool start(IOService *provider);
	void stop(IOService *provider);
	
    // IOKit methods. These methods are defines in <IOKit/IOService.h>

    virtual IOReturn setProperties(OSObject *properties);

    virtual IOReturn message(UInt32 type, IOService *provider, void *argument);

    virtual bool didTerminate(IOService *provider, IOOptionBits options, bool *defer);
    
	// Hooks
    virtual void ReadComplete(void *parameter,IOReturn status,UInt32 bufferSizeRemaining);
    virtual void WriteComplete(void *parameter,IOReturn status,UInt32 bufferSizeRemaining);

    bool QueueWrite(const void *bytes,UInt32 length);
    virtual void fiddleReport(IOBufferMemoryDescriptor *buffer);
	
	IOHIDDevice* getController(int index);
};

#endif /* __XBOX360CONTROLLER_H__ */
