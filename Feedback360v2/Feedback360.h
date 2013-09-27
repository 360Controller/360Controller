/*
 MICE Xbox 360 Controller driver for Mac OS X
 Copyright (C) 2006-2013 Colin Munro

 main.h - defines the structure used for the plugin COM object

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

#ifndef Feedback360_Feedback360_h
#define Feedback360_Feedback360_h

#include <ForceFeedback/IOForceFeedbackLib.h>
#include <IOKit/IOCFPlugin.h>

#include "devlink.h"
#include "Feedback360Effect.h"


#define FeedbackDriverVersionMajor              1
#define FeedbackDriverVersionMinor              0
#define FeedbackDriverVersionStage              developStage
#define FeedbackDriverVersionNonRelRev          0


class Feedback360
{
public:
    Feedback360();

    typedef struct _Xbox360InterfaceMap
    {
        IUnknownVTbl *pseudoVTable;
        Feedback360 *obj;
    } Xbox360InterfaceMap;

    /*
     typedef struct _Xbox360ForceFeedback {
     Xbox360InterfaceMap iIOCFPlugInInterface;
     Xbox360InterfaceMap iIOForceFeedbackDeviceInterface;
     CFUUIDRef factoryID;
     UInt32 refCount;
     DeviceLink device;
     bool manual;
     } Xbox360ForceFeedback;
     */

    LONG EffectCount;
    Feedback360Effect **EffectList;
    DWORD Gain;
    bool Actuator;

private:
    
    //disable copy constructor
    //Feedback360(Feedback360 &src);
    void operator = (Feedback360 &src);

    int EffectIndex;

    int PrvLeftLevel, PrvRightLevel;
    bool Stopped;
    bool Paused;
    bool Manual;
    CFAbsoluteTime LastTime;
    CFAbsoluteTime PausedTime;
    DeviceLink device;
    dispatch_source_t Timer;
    dispatch_queue_t Queue;
    void SetForce(LONG LeftLevel, LONG RightLevel);

protected:

    static void EffectProc( void *params );

    // glue code (static functions)
    static UInt32 sFactoryRefCount;
    static void sFactoryAddRef(void);
    static void sFactoryRelease(void);


    virtual ~Feedback360(void);

    UInt32 fRefCount;
    Xbox360InterfaceMap iIOCFPlugInInterface;
    Xbox360InterfaceMap iIOForceFeedbackDeviceInterface;


    // member functions

public:

    static inline Feedback360 *getThis (void *self) { return (Feedback360 *) ((Xbox360InterfaceMap *) self)->obj; }

    static inline int Scale(int from,int to,int max,int val)
    {
        int upper=max-1;
        return from+(((to-from)*(upper-val))/upper);
    }


    static IOCFPlugInInterface** Alloc(void);

    // static glue functions

    static HRESULT sQueryInterface(void *self, REFIID iid, LPVOID *ppv);
    static ULONG sAddRef(void *self);
    static ULONG sRelease(void *self);

    static IOReturn 	sProbe ( void * self, CFDictionaryRef propertyTable, io_service_t service, SInt32 * order );
    static IOReturn 	sStart ( void * self, CFDictionaryRef propertyTable, io_service_t service );
    static IOReturn 	sStop ( void * self );

    static HRESULT sGetVersion(void * interface, ForceFeedbackVersion * version);
    static HRESULT sInitializeTerminate(void * interface, NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin );
    static HRESULT sDestroyEffect(void * interface, FFEffectDownloadID downloadID );
    static HRESULT sDownloadEffect(  void * interface, CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags );
    static HRESULT	sEscape( void * interface, FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape	);
    static HRESULT	sGetEffectStatus( void * interface, FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode );
    static HRESULT	sGetForceFeedbackState( void * interface, ForceFeedbackDeviceState * pDeviceState );
    static HRESULT	sGetForceFeedbackCapabilities( void * interface, FFCAPABILITIES *capabilities );
    static HRESULT	sSendForceFeedbackCommand( void * interface, FFCommandFlag state );
    static HRESULT	sSetProperty( void * interface, FFProperty property, void * pValue );
    static HRESULT	sStartEffect( void * interface, FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations );
    static HRESULT	sStopEffect( void * interface, UInt32 downloadID );


    // actual member functions

    virtual HRESULT QueryInterface(REFIID iid, LPVOID* ppv);
    virtual ULONG AddRef(void);
    virtual ULONG Release(void);

    virtual IOReturn Probe ( CFDictionaryRef propertyTable, io_service_t service, SInt32 * order );
    virtual IOReturn Start ( CFDictionaryRef propertyTable, io_service_t service );
    virtual IOReturn Stop ( void );



    virtual HRESULT	GetVersion(ForceFeedbackVersion * version);
    virtual HRESULT InitializeTerminate(NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin );
    virtual HRESULT DestroyEffect(FFEffectDownloadID downloadID );
    virtual HRESULT DownloadEffect(CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags );
    virtual HRESULT	Escape(FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape	);
    virtual HRESULT	GetEffectStatus(FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode );
    virtual HRESULT	GetForceFeedbackState(ForceFeedbackDeviceState * pDeviceState );
    virtual HRESULT	GetForceFeedbackCapabilities(FFCAPABILITIES *capabilities );
    virtual HRESULT	SendForceFeedbackCommand(FFCommandFlag state );
    virtual HRESULT	SetProperty(FFProperty property, void * pValue );
    virtual HRESULT	StartEffect(FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations );
    virtual HRESULT	StopEffect(UInt32 downloadID );
};

// B8ED278F-EC8A-4E8E-B4CF-13E2A9D68E83
#define kFeedback360Uuid CFUUIDGetConstantUUIDWithBytes(NULL, 	\
0xB8, 0xED, 0x27, 0x8F, 0xEC, 0x8A, 0x4E, 0x8E, 					\
0xB4, 0xCF, 0x13, 0xE2, 0xA9, 0xD6, 0x8E, 0x83)


// Factory function
extern "C" {
    void* Control360Factory(CFAllocatorRef allocator,CFUUIDRef uuid);
}

#endif