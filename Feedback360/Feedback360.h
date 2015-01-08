/*
    MICE Xbox 360 Controller driver for Mac OS X
    Force Feedback module
    Copyright (C) 2013 David Ryskalczyk
    based on xi, Copyright (C) 2011 Masahiko Morii

    Feedback360.h - defines the structure used for the driver (COM object and emulator)

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
    along with Xbox360Controller; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef Feedback360_Feedback360_h
#define Feedback360_Feedback360_h

#include <ForceFeedback/IOForceFeedbackLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <vector>

#include "FeedbackBase.h"
#include "devlink.h"
#include "Feedback360Effect.h"

#define FeedbackDriverVersionMajor      1
#define FeedbackDriverVersionMinor      0
#define FeedbackDriverVersionStage      developStage
#define FeedbackDriverVersionNonRelRev  0

class Feedback360 : public FeedbackBase
{
public:
    // constructor/destructor
    Feedback360(void);
    virtual ~Feedback360(void);

private:
    //disable copy constructor
    Feedback360(Feedback360 &src);
    void operator = (Feedback360 &src);

    // IOCFPlugin interfacing variables and functions
public:
    static IOCFPlugInInterface** Alloc(void);
    virtual HRESULT QueryInterface(REFIID iid, LPVOID* ppv);

private:
    typedef std::vector<Feedback360Effect> Feedback360EffectVector;
    typedef Feedback360EffectVector::iterator Feedback360EffectIterator;

    // interfacing
    DeviceLink          device;

    // GCD queue and timer
    dispatch_queue_t    Queue;
    dispatch_source_t   Timer;

    // effects handling
    Feedback360EffectVector EffectList;
    UInt32              EffectIndex;

    DWORD   Gain;
    bool    Actuator;

    LONG            PrvLeftLevel, PrvRightLevel;
    bool            Stopped;
    bool            Paused;
    bool            Manual;
    CFAbsoluteTime  LastTime;
    CFAbsoluteTime  PausedTime;
    CFUUIDRef       FactoryID;

    void            SetForce(LONG LeftLevel, LONG RightLevel);

    // event loop func
    static void EffectProc( void *params );
    
    // actual member functions ultimately called by the FF API (through the static functions)
    virtual IOReturn Probe ( CFDictionaryRef propertyTable, io_service_t service, SInt32 * order );
    virtual IOReturn Start ( CFDictionaryRef propertyTable, io_service_t service );
    virtual IOReturn Stop ( void );

    virtual HRESULT GetVersion(ForceFeedbackVersion * version);
    virtual HRESULT InitializeTerminate(NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin);
    virtual HRESULT DestroyEffect(FFEffectDownloadID downloadID);
    virtual HRESULT DownloadEffect(CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags);
    virtual HRESULT Escape(FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape);
    virtual HRESULT GetEffectStatus(FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode);
    virtual HRESULT GetForceFeedbackState(ForceFeedbackDeviceState * pDeviceState);
    virtual HRESULT GetForceFeedbackCapabilities(FFCAPABILITIES *capabilities);
    virtual HRESULT SendForceFeedbackCommand(FFCommandFlag state);
    virtual HRESULT SetProperty(FFProperty property, void * pValue);
    virtual HRESULT StartEffect(FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations);
    virtual HRESULT StopEffect(UInt32 downloadID);
};

// B8ED278F-EC8A-4E8E-B4CF-13E2A9D68E83
#define kFeedback360Uuid CFUUIDGetConstantUUIDWithBytes(kCFAllocatorSystemDefault, \
0xB8, 0xED, 0x27, 0x8F, 0xEC, 0x8A, 0x4E, 0x8E, \
0xB4, 0xCF, 0x13, 0xE2, 0xA9, 0xD6, 0x8E, 0x83)

// Factory function
extern "C" {
    void* Control360Factory(CFAllocatorRef allocator, CFUUIDRef uuid);
}

#endif
