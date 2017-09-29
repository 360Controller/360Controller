//
//  FFDriver.h
//  XBoxBTFFPlug
//
//  Created by C.W. Betts on 9/20/17.
//  Copyright © 2017 C.W. Betts. All rights reserved.
//

#ifndef FFDriver_h
#define FFDriver_h

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <ForceFeedback/IOForceFeedbackLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <vector>
#include "FeedbackXBOEffect.hpp"

// 0F793F56-8C17-4BA0-9201-D52FEC6C2702
#define BTFFPLUGINTERFACE CFUUIDGetConstantUUIDWithBytes(kCFAllocatorSystemDefault, 0x0F, 0x79, 0x3F, 0x56, 0x8C, 0x17, 0x4B, 0xA0, 0x92, 0x01, 0xD5, 0x2F, 0xEC, 0x6C, 0x27, 0x02)

#define FeedbackDriverVersionMajor      1
#define FeedbackDriverVersionMinor      0
#define FeedbackDriverVersionStage      developStage
#define FeedbackDriverVersionNonRelRev  0

class FeedbackXBOBT : IUnknown
{
public:
    // constructor/destructor
    FeedbackXBOBT(void);
    virtual ~FeedbackXBOBT(void);
    
private:
    //disable copy constructor
    FeedbackXBOBT(FeedbackXBOBT &src);
    void operator = (FeedbackXBOBT &src);
    
    UInt32 fRefCount;
    
    typedef struct _Xbox360InterfaceMap
    {
        IUnknownVTbl *pseudoVTable;
        FeedbackXBOBT *obj;
    } XboxOneBTInterfaceMap;
    
    // IOCFPlugin interfacing variables and functions
public:
    static void** Alloc(void);
    
    // static functions called by the ForceFeedback API
    static HRESULT  sQueryInterface(void *self, REFIID iid, LPVOID *ppv);
    static ULONG sAddRef(void *self);
    static ULONG sRelease(void *self);
	
	static IOReturn sProbe ( void * self, CFDictionaryRef propertyTable, io_service_t service, SInt32 * order );
	static IOReturn sStart ( void * self, CFDictionaryRef propertyTable, io_service_t service );
	static IOReturn sStop ( void * self );
	
    static HRESULT  sGetVersion(void * interface, ForceFeedbackVersion * version);
    static HRESULT  sInitializeTerminate(void * interface, NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin );
    static HRESULT  sDestroyEffect(void * interface, FFEffectDownloadID downloadID );
    static HRESULT  sDownloadEffect(  void * interface, CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags );
    static HRESULT  sEscape( void * interface, FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape );
    static HRESULT  sGetEffectStatus( void * interface, FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode );
    static HRESULT  sGetForceFeedbackState( void * interface, ForceFeedbackDeviceState * pDeviceState );
    static HRESULT  sGetForceFeedbackCapabilities( void * interface, FFCAPABILITIES *capabilities );
    static HRESULT  sSendForceFeedbackCommand( void * interface, FFCommandFlag state );
    static HRESULT  sSetProperty( void * interface, FFProperty property, void * pValue );
    static HRESULT  sStartEffect( void * interface, FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations );
    static HRESULT  sStopEffect( void * interface, UInt32 downloadID );
    
    virtual HRESULT QueryInterface(REFIID iid, LPVOID* ppv);
    virtual ULONG   AddRef(void);
    virtual ULONG   Release(void);
    
private:
    typedef std::vector<FeedbackXBOEffect> FeedbackXBOEffectVector;
    typedef FeedbackXBOEffectVector::iterator FeedbackXBOEffectIterator;
    // helper function
    static inline FeedbackXBOBT *getThis (void *self) { return (FeedbackXBOBT *) ((XboxOneBTInterfaceMap *) self)->obj; }
    
    // interfacing
    XboxOneBTInterfaceMap iIOCFPlugInInterface;
    XboxOneBTInterfaceMap iIOForceFeedbackDeviceInterface;
    IOHIDDeviceRef      device;
    
    // GCD queue and timer
    dispatch_queue_t    Queue;
    dispatch_source_t   Timer;
    
    // effects handling
    FeedbackXBOEffectVector EffectList;
    UInt32              EffectIndex;
    
    DWORD   Gain;
    bool    Actuator;
    
    LONG            PrvLeftLevel, PrvRightLevel;
    bool            Stopped;
    bool            Paused;
    bool            Manual;
    double          LastTime;
    double          PausedTime;
    CFUUIDRef       FactoryID;
    
    void            SetForce(LONG LeftLevel, LONG RightLevel, LONG ltLevel, LONG rtLevel);
    
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

#endif /* FFDriver_h */
