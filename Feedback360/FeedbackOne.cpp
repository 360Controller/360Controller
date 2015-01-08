//
//  FeedbackOne.cpp
//  360 Driver
//
//  Created by C.W. Betts on 1/8/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#include "FeedbackOne.h"

IOCFPlugInInterface** FeedbackOne::Alloc(void)
{
    FeedbackOne *me = new FeedbackOne();
    if (!me) {
        return NULL;
    }
    return (IOCFPlugInInterface **)(&me->iIOCFPlugInInterface.pseudoVTable);
}

FeedbackOne::FeedbackOne(): FeedbackBase()
{
    FactoryID = kFeedbackOneUUID;
    CFRetain(FactoryID);
    CFPlugInAddInstanceForFactory(FactoryID);
}

FeedbackOne::~FeedbackOne()
{
    
}

HRESULT FeedbackOne::Escape(FFEffectDownloadID downloadID, FFEFFESCAPE *escape)
{
    return FFERR_UNSUPPORTED;
}

IOReturn FeedbackOne::Probe(CFDictionaryRef propertyTable, io_service_t service, SInt32 *order)
{
    if ((service==0)
        || ((!IOObjectConformsTo(service,"XboxOneControllerClass")))) return kIOReturnBadArgument;
    return S_OK;
}

IOReturn FeedbackOne::Start ( CFDictionaryRef propertyTable, io_service_t service )
{
    return 0;
}

IOReturn FeedbackOne::Stop ( void )
{
    return 0;
}

HRESULT FeedbackOne::GetVersion(ForceFeedbackVersion * version)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::InitializeTerminate(NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::DestroyEffect(FFEffectDownloadID downloadID)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::DownloadEffect(CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::GetEffectStatus(FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::GetForceFeedbackState(ForceFeedbackDeviceState * pDeviceState)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::GetForceFeedbackCapabilities(FFCAPABILITIES *capabilities)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::SendForceFeedbackCommand(FFCommandFlag state)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::SetProperty(FFProperty property, void * pValue)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::StartEffect(FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations)
{
    return FFERR_UNSUPPORTED;
}
HRESULT FeedbackOne::StopEffect(UInt32 downloadID)
{
    return FFERR_UNSUPPORTED;
}

void* ControlXboxOneFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
    void* result = NULL;
    if (CFEqual(typeID, kIOForceFeedbackLibTypeID))
        result = (void*)FeedbackOne::Alloc();
    return result;
}

