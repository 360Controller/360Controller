/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    main.c - Main code for the FF plugin
    
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
#include <CoreFoundation/CoreFoundation.h>
#include "main.h"

static void dealloc360Feedback(Xbox360ForceFeedback *this);

// IUnknown

static HRESULT Feedback360QueryInterface(void *that,REFIID iid,LPVOID *ppv)
{
    Xbox360ForceFeedback *this=FFThis(that);
    CFUUIDRef interface;
    
    interface=CFUUIDCreateFromUUIDBytes(NULL,iid);
    // IOForceFeedbackDevice
    if(CFEqual(interface,kIOForceFeedbackDeviceInterfaceID))
        *ppv=&this->iIOForceFeedbackDeviceInterface;
    // IUnknown || IOCFPlugInInterface
    else if(CFEqual(interface,IUnknownUUID)||CFEqual(interface,kIOCFPlugInInterfaceID))
        *ppv=&this->iIOCFPlugInInterface;
    else
        *ppv=NULL;
    // Done
    CFRelease(interface);
    if((*ppv)==NULL) return E_NOINTERFACE;
    else {
        this->iIOCFPlugInInterface.pseudoVTable->AddRef(*ppv);
        return S_OK;
    }
}

static ULONG Feedback360AddRef(void *that)
{
    Xbox360ForceFeedback *this=FFThis(that);
    this->refCount++;
    return this->refCount;
}

static ULONG Feedback360Release(void *that)
{
    Xbox360ForceFeedback *this=FFThis(that);
    this->refCount--;
    if(this->refCount==0) {
        dealloc360Feedback(this);
        return 0;
    } else return this->refCount;
}

// IOCFPlugInInterface

static IOReturn Feedback360Probe(void *that,CFDictionaryRef propertyTable,io_service_t service,SInt32 *order)
{
    if ((service==0)
     || ((!IOObjectConformsTo(service,"Xbox360ControllerClass"))
     && (!IOObjectConformsTo(service,"Wireless360Controller")))) return kIOReturnBadArgument;
    return S_OK;
}

static IOReturn Feedback360Start(void *that,CFDictionaryRef propertyTable,io_service_t service)
{
    return S_OK;
}

static IOReturn Feedback360Stop(void *that)
{
    return S_OK;
}

// IOForceFeedbackDevice

static HRESULT Feedback360GetVersion(void *this,ForceFeedbackVersion *version)
{
    if((this==NULL)||(version==NULL)) return FFERR_INVALIDPARAM;
    version->apiVersion.majorRev=kFFPlugInAPIMajorRev;
    version->apiVersion.minorAndBugRev=kFFPlugInAPIMinorAndBugRev;
    version->apiVersion.stage=kFFPlugInAPIStage;
    version->apiVersion.nonRelRev=kFFPlugInAPINonRelRev;
    version->plugInVersion.majorRev=FeedbackDriverVersionMajor;
    version->plugInVersion.minorAndBugRev=FeedbackDriverVersionMinor;
    version->plugInVersion.stage=FeedbackDriverVersionStage;
    version->plugInVersion.nonRelRev=FeedbackDriverVersionNonRelRev;
    return S_OK;
}

static void Feedback360_Callback(void *that,unsigned char large,unsigned char small)
{
    Xbox360ForceFeedback *this=(Xbox360ForceFeedback*)that;
    char buf[]={0x00,0x04,large,small};
    if(!this->manual) Device_Send(&this->device,buf,sizeof(buf));
}

static HRESULT Feedback360InitializeTerminate(void *that,NumVersion APIversion,io_object_t hidDevice,boolean_t begin)
{
    Xbox360ForceFeedback *this=FFThis(that);
//    fprintf(stderr,"Feedback360InitializeTerminate(%p,[%i.%i.%i.%i],%p,%s)\n",this,APIversion.majorRev,APIversion.minorAndBugRev,APIversion.stage,APIversion.nonRelRev,hidDevice,begin?"true":"false");
    if(this==NULL) return FFERR_INVALIDPARAM;
    if(begin) {
//        fprintf(stderr,"Feedback: Initialising...\n");
        // Initialize
        if(APIversion.majorRev!=kFFPlugInAPIMajorRev)
        {
//            fprintf(stderr,"Feedback: Invalid version\n");
            return FFERR_INVALIDPARAM;
        }
        // From probe
            if( (hidDevice==0)
             || ((!IOObjectConformsTo(hidDevice,"Xbox360ControllerClass"))
             &&  (!IOObjectConformsTo(hidDevice,"Wireless360Controller"))) )
        {
//            fprintf(stderr,"Feedback: Invalid device\n");
            return FFERR_INVALIDPARAM;
        }
        Emulate_Initialise(&this->emulator,10,Feedback360_Callback,this);
        if(!Device_Initialise(&this->device,hidDevice)) {
//            fprintf(stderr,"Feedback: Failed to initialise\n");
            Emulate_Finalise(&this->emulator);
            return FFERR_NOINTERFACE;
        }
        this->manual=FALSE;
    } else {
//        fprintf(stderr,"Feedback: Terminating\n");
        // Terminate
        Emulate_Finalise(&this->emulator);
        Device_Finalise(&this->device);
    }
    return FF_OK;
}

static HRESULT Feedback360DestroyEffect(void *that,FFEffectDownloadID downloadID)
{
    Xbox360ForceFeedback *this=FFThis(that);
    Emulate_DestroyEffect(&this->emulator,downloadID);
    return FF_OK;
}

static bool Feedback360SupportedEffect(CFUUIDRef effect)
{
    if(CFEqual(effect,kFFEffectType_SawtoothDown_ID)) return TRUE;
    if(CFEqual(effect,kFFEffectType_SawtoothUp_ID)) return TRUE;
    if(CFEqual(effect,kFFEffectType_Sine_ID)) return TRUE;
    if(CFEqual(effect,kFFEffectType_Spring_ID)) return TRUE;
    if(CFEqual(effect,kFFEffectType_Square_ID)) return TRUE;
    if(CFEqual(effect,kFFEffectType_Triangle_ID)) return TRUE;
    return FALSE;
}

static bool Feedback360ConvertEffect(const FFEFFECT *effect,ForceParams *params)
{
    if((effect->cbTypeSpecificParams!=sizeof(FFPERIODIC))||(effect->lpvTypeSpecificParams==NULL)) return FALSE;
    // Settings
    params->startDelay=effect->dwStartDelay;
    params->gain=effect->dwGain;
    params->maxLevel=10000;
    // Envelope
    params->sustainLevel=((FFPERIODIC*)effect->lpvTypeSpecificParams)->dwMagnitude;
        // Documentation was a bit confusing here as to which value was right, compared to what a game actually used
//    params->sustainTime=effect->dwDuration;
    params->sustainTime=((FFPERIODIC*)effect->lpvTypeSpecificParams)->dwPeriod;
    if(effect->lpEnvelope==NULL) {
        params->attackLevel=params->sustainLevel;
        params->attackTime=0;
        params->fadeLevel=params->sustainLevel;
        params->fadeTime=0;
    } else {
        if(effect->lpEnvelope->dwSize<sizeof(FFENVELOPE)) return FALSE;
        params->attackLevel=effect->lpEnvelope->dwAttackLevel;
        params->attackTime=effect->lpEnvelope->dwAttackTime;
        params->fadeLevel=effect->lpEnvelope->dwFadeLevel;
        params->fadeTime=effect->lpEnvelope->dwFadeTime;
    }
    // Done
    return TRUE;
}

static HRESULT Feedback360DownloadEffect(void *that,CFUUIDRef effectType,FFEffectDownloadID *downloadID,FFEFFECT *effect,FFEffectParameterFlag flags)
{
    ForceParams params;
    Xbox360ForceFeedback *this=FFThis(that);

    if(!Feedback360SupportedEffect(effectType)) return FFERR_UNSUPPORTED;
    if(!(effect->dwFlags&FFEFF_SPHERICAL)) return FFERR_UNSUPPORTED;
    if(effect->dwTriggerButton!=FFEB_NOTRIGGER) return FFERR_UNSUPPORTED;
    if(!Feedback360ConvertEffect(effect,&params)) return FFERR_UNSUPPORTED;
    if(Emulate_IsPaused(&this->emulator)) return FFERR_DEVICEPAUSED;
    if(flags&FFEP_NODOWNLOAD) return FF_OK;
    if((*downloadID)==0) {
        *downloadID=Emulate_CreateEffect(&this->emulator,&params);
        if((*downloadID)==0) return FFERR_DEVICEFULL;
    } else {
        if(!Emulate_IsValidEffect(&this->emulator,*downloadID)) return FFERR_INVALIDDOWNLOADID;
        if(!Emulate_ChangeEffect(&this->emulator,*downloadID,&params)) return FFERR_EFFECTTYPEMISMATCH;
    }
    if(flags&FFEP_START) {
        // Restart
    }
    return FF_OK;
}

static HRESULT Feedback360Escape(void *that,FFEffectDownloadID downloadID,FFEFFESCAPE *escape)
{
    Xbox360ForceFeedback *this=FFThis(that);

    if(this==NULL) return FFERR_INVALIDPARAM;
    if(downloadID!=0) return FFERR_UNSUPPORTED;
    if(escape->dwSize<sizeof(FFEFFESCAPE)) return FFERR_INVALIDPARAM;
    escape->cbOutBuffer=0;
    switch(escape->dwCommand) {
        case 0x00:  // Control motors
            if(escape->cbInBuffer!=1) return FFERR_INVALIDPARAM;
            this->manual=((unsigned char*)escape->lpvInBuffer)[0]!=0x00;
            break;
        case 0x01:  // Set motors
            if(escape->cbInBuffer!=2) return FFERR_INVALIDPARAM;
            if(this->manual) {
                unsigned char *data=escape->lpvInBuffer;
                unsigned char buf[]={0x00,0x04,data[0],data[1]};
                Device_Send(&this->device,buf,sizeof(buf));
            }
            break;
        case 0x02:  // Set LED
            if(escape->cbInBuffer!=1) return FFERR_INVALIDPARAM;
            {
                unsigned char *data=escape->lpvInBuffer;
                unsigned char buf[]={0x01,0x03,data[0]};
                Device_Send(&this->device,buf,sizeof(buf));
            }
            break;
        case 0x03:  // Power off
        {
            unsigned char buf[] = {0x02, 0x02};
            Device_Send(&this->device, buf, sizeof(buf));
        }
            break;
        default:
            fprintf(stderr, "Xbox360Controller FF plugin: Unknown escape (%i)\n", (int)escape->dwCommand);
            return FFERR_UNSUPPORTED;
    }
    return FF_OK;
}

static HRESULT Feedback360GetEffectStatus(void *that,FFEffectDownloadID downloadID,FFEffectStatusFlag *statusCode)
{
    Xbox360ForceFeedback *this=FFThis(that);

    if(!Emulate_IsValidEffect(&this->emulator,downloadID)) return FFERR_INVALIDDOWNLOADID;
    *statusCode=Emulate_IsPlaying(&this->emulator,downloadID)?FFEGES_PLAYING:FFEGES_NOTPLAYING;
    return FF_OK;
}

static HRESULT Feedback360GetForceFeedbackCapabilities(void *this,FFCAPABILITIES *capabilities)
{
    capabilities->ffSpecVer.majorRev=kFFPlugInAPIMajorRev;
    capabilities->ffSpecVer.minorAndBugRev=kFFPlugInAPIMinorAndBugRev;
    capabilities->ffSpecVer.stage=kFFPlugInAPIStage;
    capabilities->ffSpecVer.nonRelRev=kFFPlugInAPINonRelRev;
    capabilities->supportedEffects=FFCAP_ET_SQUARE|FFCAP_ET_SINE|FFCAP_ET_TRIANGLE|FFCAP_ET_SAWTOOTHUP|FFCAP_ET_SAWTOOTHDOWN;
    capabilities->emulatedEffects=0;
    capabilities->subType=FFCAP_ST_VIBRATION;
    capabilities->numFfAxes=1;
    capabilities->ffAxes[0]=FFJOFS_X;
    capabilities->storageCapacity=256;
    capabilities->playbackCapacity=1;
    capabilities->driverVer.majorRev=FeedbackDriverVersionMajor;
    capabilities->driverVer.minorAndBugRev=FeedbackDriverVersionMinor;
    capabilities->driverVer.stage=FeedbackDriverVersionStage;
    capabilities->driverVer.nonRelRev=FeedbackDriverVersionNonRelRev;
    capabilities->firmwareVer.majorRev=1;
    capabilities->firmwareVer.minorAndBugRev=0;
    capabilities->firmwareVer.stage=developStage;
    capabilities->firmwareVer.nonRelRev=0;
    capabilities->hardwareVer.majorRev=1;
    capabilities->hardwareVer.minorAndBugRev=0;
    capabilities->hardwareVer.stage=developStage;
    capabilities->hardwareVer.nonRelRev=0;
    return S_OK;
}

static HRESULT Feedback360GetForceFeedbackState(void *that,ForceFeedbackDeviceState *deviceState)
{
    Xbox360ForceFeedback *this=FFThis(that);

    if(deviceState->dwSize<sizeof(ForceFeedbackDeviceState)) return FFERR_INVALIDPARAM;
    else deviceState->dwSize=sizeof(ForceFeedbackDeviceState);
    deviceState->dwState
        = Emulate_IsEmpty(&this->emulator)?FFGFFS_EMPTY:0
        | Emulate_IsStopped(&this->emulator)?FFGFFS_STOPPED:0
        | Emulate_IsPaused(&this->emulator)?FFGFFS_PAUSED:0
        | Emulate_IsEnabled(&this->emulator)?FFGFFS_ACTUATORSON:FFGFFS_ACTUATORSOFF;
    deviceState->dwLoad=(Emulate_Effects_Used(&this->emulator)*100)/Emulate_Effects_Total(&this->emulator);
    return S_OK;
}

static HRESULT Feedback360SendForceFeedbackCommand(void *that,FFCommandFlag state)
{
    Xbox360ForceFeedback *this=FFThis(that);

    switch(state) {
        case FFSFFC_RESET:
            Emulate_Reset(&this->emulator);
            return S_OK;
        case FFSFFC_STOPALL:
            Emulate_Stop(&this->emulator,0);    // 0 == all effects
            return S_OK;
        case FFSFFC_PAUSE:
        case FFSFFC_CONTINUE:
            Emulate_SetPaused(&this->emulator,state==FFSFFC_PAUSE);
            return S_OK;
        case FFSFFC_SETACTUATORSON:
        case FFSFFC_SETACTUATORSOFF:
            Emulate_SetEnable(&this->emulator,state==FFSFFC_SETACTUATORSON);
            return S_OK;
        default:
            return FFERR_UNSUPPORTED;
    }
}

static HRESULT Feedback360SetProperty(void *that,FFProperty property,void *value)
{
    Xbox360ForceFeedback *this=FFThis(that);

    switch(property) {
        case FFPROP_FFGAIN:
            {
                UInt32 uValue = *((UInt32*)value);
                if (uValue > 10000)
					return FFERR_INVALIDPARAM;
                Emulate_SetGain(&this->emulator, uValue, 10000);
            }
            return FF_OK;
        default:
            return FFERR_UNSUPPORTED;
    }
}

static HRESULT Feedback360StartEffect(void *that,FFEffectDownloadID downloadID,FFEffectStartFlag mode,UInt32 iterations)
{
    Xbox360ForceFeedback *this=FFThis(that);

    if(mode&FFES_SOLO) Feedback360SendForceFeedbackCommand(that,FFSFFC_RESET);
    Emulate_Start(&this->emulator,downloadID,iterations);
    return S_OK;
}

static HRESULT Feedback360StopEffect(void *that,FFEffectDownloadID downloadID)
{
    Xbox360ForceFeedback *this=FFThis(that);

    if(!Emulate_IsValidEffect(&this->emulator,downloadID)) return FFERR_INVALIDDOWNLOADID;
    Emulate_Stop(&this->emulator,downloadID);
    return S_OK;
}

// Function table

static IOCFPlugInInterface functionMap360_IOCFPlugInInterface={
    // Padding required for COM
    NULL,
    // IUnknown
    Feedback360QueryInterface,
    Feedback360AddRef,
    Feedback360Release,
    // IOCFPlugInInterface
    1,0,    // Version
    Feedback360Probe,
    Feedback360Start,
    Feedback360Stop
};

static IOForceFeedbackDeviceInterface functionMap360_IOForceFeedbackDeviceInterface={
    // Padding required for COM
    NULL,
    // IUnknown
    Feedback360QueryInterface,
    Feedback360AddRef,
    Feedback360Release,
    // IOForceFeedbackDevice
    Feedback360GetVersion,
    Feedback360InitializeTerminate,
    Feedback360DestroyEffect,
    Feedback360DownloadEffect,
    Feedback360Escape,
    Feedback360GetEffectStatus,
    Feedback360GetForceFeedbackCapabilities,
    Feedback360GetForceFeedbackState,
    Feedback360SendForceFeedbackCommand,
    Feedback360SetProperty,
    Feedback360StartEffect,
    Feedback360StopEffect
};

// Constructor and destructor

static IOCFPlugInInterface** alloc360Feedback(CFUUIDRef uuid)
{
    Xbox360ForceFeedback *item;
    
    item=(Xbox360ForceFeedback*)malloc(sizeof(Xbox360ForceFeedback));
    item->iIOCFPlugInInterface.pseudoVTable=(IUnknownVTbl*)&functionMap360_IOCFPlugInInterface;
    item->iIOCFPlugInInterface.obj=item;
    item->iIOForceFeedbackDeviceInterface.pseudoVTable=(IUnknownVTbl*)&functionMap360_IOForceFeedbackDeviceInterface;
    item->iIOForceFeedbackDeviceInterface.obj=item;
    item->factoryID=CFRetain(uuid);
    CFPlugInAddInstanceForFactory(uuid);
    item->refCount=1;
    return (IOCFPlugInInterface**)&item->iIOCFPlugInInterface.pseudoVTable;
}

static void dealloc360Feedback(Xbox360ForceFeedback *this)
{
    CFUUIDRef uuid;
    
    uuid=this->factoryID;
    free(this);
    if(uuid) {
        CFPlugInRemoveInstanceForFactory(uuid);
        CFRelease(uuid);
    }
}

// External factory function

void* Control360Factory(CFAllocatorRef allocator,CFUUIDRef uuid)
{
    if(CFEqual(uuid,kIOForceFeedbackLibTypeID)) return (void*)alloc360Feedback(uuid);
    else return NULL;
}
