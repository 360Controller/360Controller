//
//  FeedbackBaseClass.h
//  360 Driver
//
//  Created by C.W. Betts on 1/8/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//  A bare-bones class that helps with making Force Feedback subclasses
//  for OS X.
//

#ifndef ___60_Driver__FeedbackBaseClass__
#define ___60_Driver__FeedbackBaseClass__

#include <ForceFeedback/IOForceFeedbackLib.h>
#include <IOKit/IOCFPlugIn.h>


class FeedbackBase : public IUnknown
{
private:
    UInt32 fRefCount;
    
    static inline FeedbackBase *getThis (void *self) { return (FeedbackBase *) ((FeedbackInterfaceMap *) self)->obj; }
    
protected:
    typedef struct _FeedbackInterfaceMap
    {
        IUnknownVTbl *pseudoVTable;
        FeedbackBase *obj;
    } FeedbackInterfaceMap;

    // interfacing
    FeedbackInterfaceMap iIOCFPlugInInterface;
    FeedbackInterfaceMap iIOForceFeedbackDeviceInterface;
    
public:
    //constructor/destructor
    FeedbackBase();
    virtual ~FeedbackBase();
    
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

#pragma mark - IUnknown calls
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);
    
protected:
    // actual member functions ultimately called by the FF API (through the static functions)
#pragma mark - IOCFPlugInInterface calls
    virtual IOReturn Probe ( CFDictionaryRef propertyTable, io_service_t service, SInt32 * order ) = 0;
    virtual IOReturn Start ( CFDictionaryRef propertyTable, io_service_t service ) = 0;
    virtual IOReturn Stop ( void ) = 0;
    
#pragma mark - IOForceFeedbackDeviceInterface calls
    virtual HRESULT GetVersion(ForceFeedbackVersion * version) = 0;
    virtual HRESULT InitializeTerminate(NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin) = 0;
    virtual HRESULT DestroyEffect(FFEffectDownloadID downloadID) = 0;
    virtual HRESULT DownloadEffect(CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags) = 0;
    virtual HRESULT Escape(FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape) = 0;
    virtual HRESULT GetEffectStatus(FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode) = 0;
    virtual HRESULT GetForceFeedbackState(ForceFeedbackDeviceState * pDeviceState) = 0;
    virtual HRESULT GetForceFeedbackCapabilities(FFCAPABILITIES *capabilities) = 0;
    virtual HRESULT SendForceFeedbackCommand(FFCommandFlag state) = 0;
    virtual HRESULT SetProperty(FFProperty property, void * pValue) = 0;
    virtual HRESULT StartEffect(FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations) = 0;
    virtual HRESULT StopEffect(UInt32 downloadID) = 0;
};

#endif /* defined(___60_Driver__FeedbackBaseClass__) */
