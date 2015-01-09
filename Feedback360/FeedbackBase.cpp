//
//  FeedbackBaseClass.cpp
//  360 Driver
//
//  Created by C.W. Betts on 1/8/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#include "FeedbackBase.h"

static IOCFPlugInInterface functionMapBase_IOCFPlugInInterface = {
    // Padding required for COM
    NULL,
    // IUnknown
    &FeedbackBase::sQueryInterface,
    &FeedbackBase::sAddRef,
    &FeedbackBase::sRelease,
    // IOCFPlugInInterface
    1,0,    // Version
    &FeedbackBase::sProbe,
    &FeedbackBase::sStart,
    &FeedbackBase::sStop
};

static IOForceFeedbackDeviceInterface functionMapBase_IOForceFeedbackDeviceInterface = {
    // Padding required for COM
    NULL,
    // IUnknown
    &FeedbackBase::sQueryInterface,
    &FeedbackBase::sAddRef,
    &FeedbackBase::sRelease,
    // IOForceFeedbackDevice
    &FeedbackBase::sGetVersion,
    &FeedbackBase::sInitializeTerminate,
    &FeedbackBase::sDestroyEffect,
    &FeedbackBase::sDownloadEffect,
    &FeedbackBase::sEscape,
    &FeedbackBase::sGetEffectStatus,
    &FeedbackBase::sGetForceFeedbackCapabilities,
    &FeedbackBase::sGetForceFeedbackState,
    &FeedbackBase::sSendForceFeedbackCommand,
    &FeedbackBase::sSetProperty,
    &FeedbackBase::sStartEffect,
    &FeedbackBase::sStopEffect
};

FeedbackBase::FeedbackBase(): fRefCount(1)
{
    iIOCFPlugInInterface.pseudoVTable = (IUnknownVTbl *) &functionMapBase_IOCFPlugInInterface;
    iIOCFPlugInInterface.obj = this;
    
    iIOForceFeedbackDeviceInterface.pseudoVTable = (IUnknownVTbl *) &functionMapBase_IOForceFeedbackDeviceInterface;
    iIOForceFeedbackDeviceInterface.obj = this;
}

FeedbackBase::~FeedbackBase()
{
    
}

HRESULT FeedbackBase::QueryInterface(REFIID iid, LPVOID *ppv)
{
    CFUUIDRef interface = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);
    
    if(CFEqual(interface, kIOForceFeedbackDeviceInterfaceID))
        *ppv = &this->iIOForceFeedbackDeviceInterface;
    // IUnknown || IOCFPlugInInterface
    else if(CFEqual(interface, IUnknownUUID) || CFEqual(interface, kIOCFPlugInInterfaceID))
        *ppv = &this->iIOCFPlugInInterface;
    else
        *ppv = NULL;
    
    // Done
    CFRelease(interface);
    if ((*ppv) == NULL) return E_NOINTERFACE;
    else {
        this->iIOCFPlugInInterface.pseudoVTable->AddRef(*ppv);
    }
    return FF_OK;
}

ULONG FeedbackBase::AddRef()
{
    return ++fRefCount;
}

ULONG FeedbackBase::Release()
{
    ULONG returnValue = fRefCount - 1;
    if(returnValue > 0) {
        fRefCount = returnValue;
    } else if(returnValue == 0) {
        fRefCount = returnValue;
        delete this;
    } else {
        returnValue = 0;
    }
    return returnValue;
}


// static c->c++ glue functions
HRESULT FeedbackBase::sQueryInterface(void *self, REFIID iid, LPVOID *ppv)
{
    FeedbackBase *obj = ((FeedbackInterfaceMap *)self)->obj;
    return obj->QueryInterface(iid, ppv);
}

ULONG FeedbackBase::sAddRef(void *self)
{
    FeedbackBase *obj = ( (FeedbackInterfaceMap *) self)->obj;
    return obj->AddRef();
}

ULONG FeedbackBase::sRelease(void *self)
{
    FeedbackBase *obj = ( (FeedbackInterfaceMap *) self)->obj;
    return obj->Release();
}

IOReturn FeedbackBase::sProbe(void *self, CFDictionaryRef propertyTable, io_service_t service, SInt32 *order)
{
    return getThis(self)->Probe(propertyTable, service, order);
}

IOReturn FeedbackBase::sStart(void *self, CFDictionaryRef propertyTable, io_service_t service)
{
    return getThis(self)->Start(propertyTable, service);
}

IOReturn FeedbackBase::sStop(void *self)
{
    return getThis(self)->Stop();
}

HRESULT FeedbackBase::sGetVersion(void * self, ForceFeedbackVersion * version)
{
    return FeedbackBase::getThis(self)->GetVersion(version);
}

HRESULT FeedbackBase::sInitializeTerminate(void * self, NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin)
{
    return FeedbackBase::getThis(self)->InitializeTerminate(forceFeedbackAPIVersion, hidDevice, begin);
}

HRESULT FeedbackBase::sDestroyEffect(void * self, FFEffectDownloadID downloadID)
{
    return FeedbackBase::getThis(self)->DestroyEffect(downloadID);
}

HRESULT FeedbackBase::sDownloadEffect(void * self, CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags)
{
    return FeedbackBase::getThis(self)->DownloadEffect(effectType, pDownloadID, pEffect, flags);
}

HRESULT FeedbackBase::sEscape(void * self, FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape)
{
    return FeedbackBase::getThis(self)->Escape(downloadID, pEscape);
}

HRESULT FeedbackBase::sGetEffectStatus(void * self, FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode)
{
    return FeedbackBase::getThis(self)->GetEffectStatus(downloadID, pStatusCode);
}

HRESULT FeedbackBase::sGetForceFeedbackState(void * self, ForceFeedbackDeviceState * pDeviceState)
{
    return FeedbackBase::getThis(self)->GetForceFeedbackState(pDeviceState);
}

HRESULT FeedbackBase::sGetForceFeedbackCapabilities(void * self, FFCAPABILITIES * capabilities)
{
    return FeedbackBase::getThis(self)->GetForceFeedbackCapabilities(capabilities);
}

HRESULT FeedbackBase::sSendForceFeedbackCommand(void * self, FFCommandFlag state)
{
    return FeedbackBase::getThis(self)->SendForceFeedbackCommand(state);
}

HRESULT FeedbackBase::sSetProperty(void * self, FFProperty property, void * pValue)
{
    return FeedbackBase::getThis(self)->SetProperty(property, pValue);
}

HRESULT FeedbackBase::sStartEffect(void * self, FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations)
{
    return FeedbackBase::getThis(self)->StartEffect(downloadID, mode, iterations);
}

HRESULT FeedbackBase::sStopEffect(void * self, UInt32 downloadID)
{
    return FeedbackBase::getThis(self)->StopEffect(downloadID);
}

