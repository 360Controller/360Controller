//
//  FFDriver.c
//  XBoxBTFFPlug
//
//  Created by C.W. Betts on 9/20/17.
//  Copyright © 2017 C.W. Betts. All rights reserved.
//

#include <ForceFeedback/IOForceFeedbackLib.h>
#include <IOKit/IOCFPlugIn.h>
#include "FFDriver.h"
#include "XBoxOneBTHID.h"
#include <mach/mach.h>
#include <mach/mach_time.h>
using std::max;
using std::min;

static bool goodProbe(io_service_t theService)
{
	if (!IOObjectConformsTo(theService, kIOHIDDeviceKey)) {
		return false;
	}
	CFMutableDictionaryRef dict;
	
	IORegistryEntryCreateCFProperties(theService, &dict, kCFAllocatorDefault, 0);
	CFTypeRef aNum = CFDictionaryGetValue(dict, CFSTR(kIOHIDProductIDKey));
	int tmpNum;
	::CFNumberGetValue((CFNumberRef)aNum, kCFNumberIntType, &tmpNum);
	if (tmpNum != 765) {
		CFRelease(dict);
		return false;
	}
	aNum = CFDictionaryGetValue(dict, CFSTR(kIOHIDVendorIDKey));
	::CFNumberGetValue((CFNumberRef)aNum, kCFNumberIntType, &tmpNum);
	if (tmpNum != 1118) {
		CFRelease(dict);
		return false;
	}
	
	CFRelease(dict);
	return true;
}

#define LoopGranularity 10000 // Microseconds

double CurrentTimeUsingMach()
{
	static mach_timebase_info_data_t info = {0};
	if (!info.denom)
	{
		if (mach_timebase_info(&info) != KERN_SUCCESS)
		{
			//Generally it can't fail here. Look at XNU sources //FIXME
			info.denom  = 0;
			return -1.0;
		}
	}
	
	uint64_t start = mach_absolute_time();
	
	uint64_t nanos = start * info.numer / info.denom;
	return (double)nanos / NSEC_PER_SEC;
}

static IOCFPlugInInterface functionMapXBOBT_IOCFPlugInInterface = {
	// Padding required for COM
	NULL,
	// IUnknown
	&FeedbackXBOBT::sQueryInterface,
	&FeedbackXBOBT::sAddRef,
	&FeedbackXBOBT::sRelease,
	// IOCFPlugInInterface
	1,0,    // Version
	&FeedbackXBOBT::sProbe,
	&FeedbackXBOBT::sStart,
	&FeedbackXBOBT::sStop
};


static IOForceFeedbackDeviceInterface functionMapXBOBT_IOForceFeedbackDeviceInterface = {
	// Padding required for COM
	NULL,
	// IUnknown
	&FeedbackXBOBT::sQueryInterface,
	&FeedbackXBOBT::sAddRef,
	&FeedbackXBOBT::sRelease,
	// IOForceFeedbackDevice
	&FeedbackXBOBT::sGetVersion,
	&FeedbackXBOBT::sInitializeTerminate,
	&FeedbackXBOBT::sDestroyEffect,
	&FeedbackXBOBT::sDownloadEffect,
	&FeedbackXBOBT::sEscape,
	&FeedbackXBOBT::sGetEffectStatus,
	&FeedbackXBOBT::sGetForceFeedbackCapabilities,
	&FeedbackXBOBT::sGetForceFeedbackState,
	&FeedbackXBOBT::sSendForceFeedbackCommand,
	&FeedbackXBOBT::sSetProperty,
	&FeedbackXBOBT::sStartEffect,
	&FeedbackXBOBT::sStopEffect
};

FeedbackXBOBT::FeedbackXBOBT() : fRefCount(1),  EffectIndex(1), Stopped(true),
Paused(false), PausedTime(0), LastTime(0), Gain(10000), PrvLeftLevel(0),
PrvRightLevel(0), Actuator(true), Manual(false)
{
	EffectList = FeedbackXBOEffectVector();
	
	iIOCFPlugInInterface.pseudoVTable = (IUnknownVTbl *) &functionMapXBOBT_IOCFPlugInInterface;
	iIOCFPlugInInterface.obj = this;
	
	iIOForceFeedbackDeviceInterface.pseudoVTable = (IUnknownVTbl *) &functionMapXBOBT_IOForceFeedbackDeviceInterface;
	iIOForceFeedbackDeviceInterface.obj = this;
	
	FactoryID = BTFFPLUGINTERFACE;
	CFRetain(FactoryID);
	CFPlugInAddInstanceForFactory(FactoryID);
}

FeedbackXBOBT::~FeedbackXBOBT()
{
	CFPlugInRemoveInstanceForFactory(FactoryID);
	CFRelease(FactoryID);
}

HRESULT FeedbackXBOBT::QueryInterface(REFIID iid, LPVOID *ppv)
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

ULONG FeedbackXBOBT::AddRef()
{
	return ++fRefCount;
}

ULONG FeedbackXBOBT::Release()
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

void** FeedbackXBOBT::Alloc(void)
{
	FeedbackXBOBT *me = new FeedbackXBOBT();
	if (!me) {
		return NULL;
	}
	return (void **)(&me->iIOCFPlugInInterface.pseudoVTable);
}

IOReturn FeedbackXBOBT::Probe(CFDictionaryRef propertyTable, io_service_t service, SInt32 *order)
{
	if (service == 0) {
		return kIOReturnBadArgument;
	}
	if (goodProbe(service)) {
		return 0;
	} else {
		return kIOReturnBadArgument;
	}
}

IOReturn FeedbackXBOBT::Start(CFDictionaryRef propertyTable,io_service_t service)
{
	return FF_OK;
}

IOReturn FeedbackXBOBT::Stop(void)
{
	return FF_OK;
}

HRESULT FeedbackXBOBT::SetProperty(FFProperty property, void *value)
{
	if(property != FFPROP_FFGAIN) {
		return FFERR_UNSUPPORTED;
	}
	
	UInt32 NewGain = *((UInt32*)value);
	__block HRESULT Result = FF_OK;
	
	dispatch_sync(Queue, ^{
		if (1 <= NewGain && NewGain <= 10000)
		{
			Gain = NewGain;
		} else {
			Gain = max((UInt32)1, min(NewGain, (UInt32)10000));
			Result = FF_TRUNCATED;
		}
	});
	
	return Result;
}

HRESULT FeedbackXBOBT::StartEffect(FFEffectDownloadID EffectHandle, FFEffectStartFlag Mode, UInt32 Count)
{
	dispatch_sync(Queue, ^{
		for (FeedbackXBOEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
		{
			if (effectIterator->Handle == EffectHandle)
			{
				effectIterator->Status  = FFEGES_PLAYING;
				effectIterator->PlayCount = Count;
				effectIterator->StartTime = CurrentTimeUsingMach();
				Stopped = false;
			} else {
				if (Mode & FFES_SOLO) {
					effectIterator->Status = NULL;
				}
			}
		}
	});
	return FF_OK;
}

HRESULT FeedbackXBOBT::StopEffect(UInt32 EffectHandle)
{
	dispatch_sync(Queue, ^{
		for (FeedbackXBOEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
		{
			if (effectIterator->Handle == EffectHandle)
			{
				effectIterator->Status = NULL;
				break;
			}
		}
	});
	return FF_OK;
}

HRESULT FeedbackXBOBT::DownloadEffect(CFUUIDRef EffectType, FFEffectDownloadID *EffectHandle, FFEFFECT *DiEffect, FFEffectParameterFlag Flags)
{
	__block HRESULT Result = FF_OK;
	
	if (Flags & FFEP_NODOWNLOAD)
	{
		return FF_OK;
	}
	
	dispatch_sync(Queue, ^{
		FeedbackXBOEffect *Effect = NULL;
		if (*EffectHandle == 0) {
			EffectList.push_back(FeedbackXBOEffect(EffectIndex++));
			Effect = &(EffectList.back());
			*EffectHandle = Effect->Handle;
		} else {
			for (LONG Index = 0; Index < EffectList.size(); Index++) {
				if (EffectList[Index].Handle == *EffectHandle) {
					Effect = &(EffectList[Index]);
					break;
				}
			}
		}
		
		if (Effect == NULL || Result == -1) {
			Result = FFERR_INTERNAL;
		}
		else {
			Effect->Type = EffectType;
			Effect->DiEffect.dwFlags = DiEffect->dwFlags;
			
			if( Flags & FFEP_DURATION )
			{
				Effect->DiEffect.dwDuration = DiEffect->dwDuration;
			}
			
			if( Flags & FFEP_SAMPLEPERIOD )
			{
				Effect->DiEffect.dwSamplePeriod = DiEffect->dwSamplePeriod;
			}
			
			if( Flags & FFEP_GAIN )
			{
				Effect->DiEffect.dwGain = DiEffect->dwGain;
			}
			
			if( Flags & FFEP_TRIGGERBUTTON )
			{
				Effect->DiEffect.dwTriggerButton = DiEffect->dwTriggerButton;
			}
			
			if( Flags & FFEP_TRIGGERREPEATINTERVAL )
			{
				Effect->DiEffect.dwTriggerRepeatInterval = DiEffect->dwTriggerRepeatInterval;
			}
			
			if( Flags & FFEP_AXES )
			{
				Effect->DiEffect.cAxes  = DiEffect->cAxes;
				Effect->DiEffect.rgdwAxes = NULL;
			}
			
			if( Flags & FFEP_DIRECTION )
			{
				Effect->DiEffect.cAxes   = DiEffect->cAxes;
				Effect->DiEffect.rglDirection = NULL;
			}
			
			if( ( Flags & FFEP_ENVELOPE ) && DiEffect->lpEnvelope != NULL )
			{
				memcpy( &Effect->DiEnvelope, DiEffect->lpEnvelope, sizeof( FFENVELOPE ) );
				if( Effect->DiEffect.dwDuration - Effect->DiEnvelope.dwFadeTime
				   < Effect->DiEnvelope.dwAttackTime )
				{
					Effect->DiEnvelope.dwFadeTime = Effect->DiEnvelope.dwAttackTime;
				}
				Effect->DiEffect.lpEnvelope = &Effect->DiEnvelope;
			}
			
			Effect->DiEffect.cbTypeSpecificParams = DiEffect->cbTypeSpecificParams;
			
			if( Flags & FFEP_TYPESPECIFICPARAMS )
			{
				if(CFEqual(EffectType, kFFEffectType_CustomForce_ID)) {
					memcpy(
						   &Effect->DiCustomForce
						   ,DiEffect->lpvTypeSpecificParams
						   ,DiEffect->cbTypeSpecificParams );
					Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiCustomForce;
				}
				
				else if(CFEqual(EffectType, kFFEffectType_ConstantForce_ID)) {
					memcpy(
						   &Effect->DiConstantForce
						   ,DiEffect->lpvTypeSpecificParams
						   ,DiEffect->cbTypeSpecificParams );
					Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiConstantForce;
				}
				else if(CFEqual(EffectType, kFFEffectType_Square_ID) || CFEqual(EffectType, kFFEffectType_Sine_ID) || CFEqual(EffectType, kFFEffectType_Triangle_ID) || CFEqual(EffectType, kFFEffectType_SawtoothUp_ID) || CFEqual(EffectType, kFFEffectType_SawtoothDown_ID) ) {
					memcpy(
						   &Effect->DiPeriodic
						   ,DiEffect->lpvTypeSpecificParams
						   ,DiEffect->cbTypeSpecificParams );
					Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiPeriodic;
				}
				else if(CFEqual(EffectType, kFFEffectType_RampForce_ID)) {
					memcpy(
						   &Effect->DiRampforce
						   ,DiEffect->lpvTypeSpecificParams
						   ,DiEffect->cbTypeSpecificParams );
					Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiRampforce;
				}
			}
			
			if( Flags & FFEP_STARTDELAY )
			{
				Effect->DiEffect.dwStartDelay = DiEffect->dwStartDelay;
			}
			
			if( Flags & FFEP_START )
			{
				Effect->Status  = FFEGES_PLAYING;
				Effect->PlayCount = 1;
				Effect->StartTime = CurrentTimeUsingMach();
			}
			
			if( Flags & FFEP_NORESTART )
			{
				;
			}
			Result = FF_OK;
		}
	});
	return Result;
}

HRESULT FeedbackXBOBT::GetForceFeedbackState(ForceFeedbackDeviceState *DeviceState)
{
	if (DeviceState->dwSize != sizeof(FFDEVICESTATE))
	{
		return FFERR_INVALIDPARAM;
	}
	
	dispatch_sync(Queue, ^{
		DeviceState->dwState = NULL;
		if( EffectList.size() == 0 )
		{
			DeviceState->dwState |= FFGFFS_EMPTY;
		}
		if( Stopped == true )
		{
			DeviceState->dwState |= FFGFFS_STOPPED;
		}
		if( Paused == true )
		{
			DeviceState->dwState |= FFGFFS_PAUSED;
		}
		if (Actuator == true)
		{
			DeviceState->dwState |= FFGFFS_ACTUATORSON;
		} else {
			DeviceState->dwState |= FFGFFS_ACTUATORSOFF;
		}
		DeviceState->dwState |= FFGFFS_POWERON;
		DeviceState->dwState |= FFGFFS_SAFETYSWITCHOFF;
		DeviceState->dwState |= FFGFFS_USERFFSWITCHON;
		
		DeviceState->dwLoad  = 0;
	});
	
	return FF_OK;
}

HRESULT FeedbackXBOBT::GetForceFeedbackCapabilities(FFCAPABILITIES *capabilities)
{
	capabilities->ffSpecVer.majorRev=kFFPlugInAPIMajorRev;
	capabilities->ffSpecVer.minorAndBugRev=kFFPlugInAPIMinorAndBugRev;
	capabilities->ffSpecVer.stage=kFFPlugInAPIStage;
	capabilities->ffSpecVer.nonRelRev=kFFPlugInAPINonRelRev;
	capabilities->supportedEffects=FFCAP_ET_CUSTOMFORCE|FFCAP_ET_CONSTANTFORCE|FFCAP_ET_RAMPFORCE|FFCAP_ET_SQUARE|FFCAP_ET_SINE|FFCAP_ET_TRIANGLE|FFCAP_ET_SAWTOOTHUP|FFCAP_ET_SAWTOOTHDOWN;
	capabilities->emulatedEffects=0;
	capabilities->subType=FFCAP_ST_VIBRATION;
	capabilities->numFfAxes=4;
	capabilities->ffAxes[0]=FFJOFS_X;
	capabilities->ffAxes[1]=FFJOFS_Y;
	capabilities->ffAxes[2]=FFJOFS_Z;
	capabilities->ffAxes[3]=FFJOFS_RZ;
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
	return FF_OK;
}

HRESULT FeedbackXBOBT::SendForceFeedbackCommand(FFCommandFlag state)
{
	__block HRESULT Result = FF_OK;
	
	dispatch_sync(Queue, ^{
		switch (state) {
			case FFSFFC_RESET:
				EffectList.clear();
				Stopped = true;
				Paused = false;
				break;
				
			case FFSFFC_STOPALL:
				for (FeedbackXBOEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
				{
					effectIterator->Status = NULL;
				}
				Stopped = true;
				Paused = false;
				break;
				
			case FFSFFC_PAUSE:
				Paused  = true;
				PausedTime = CurrentTimeUsingMach();
				break;
				
			case FFSFFC_CONTINUE:
				for (FeedbackXBOEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
				{
					effectIterator->StartTime += ( CurrentTimeUsingMach() - PausedTime );
				}
				Paused = false;
				break;
				
			case FFSFFC_SETACTUATORSON:
				Actuator = true;
				break;
				
			case FFSFFC_SETACTUATORSOFF:
				Actuator = false;
				break;
				
			default:
				Result = FFERR_INVALIDPARAM;
				break;
		}
	});
	//return Result;
	return FF_OK;
}

HRESULT FeedbackXBOBT::InitializeTerminate(NumVersion APIversion, io_object_t hidDevice, boolean_t begin)
{
	if(begin) {
		if (APIversion.majorRev != kFFPlugInAPIMajorRev)
		{
			// fprintf(stderr,"Feedback: Invalid version\n");
			return FFERR_INVALIDPARAM;
		}
		// From probe
		if( (hidDevice==0)
		   || !goodProbe(hidDevice) )
		{
			// fprintf(stderr,"Feedback: Invalid device\n");
			return FFERR_INVALIDPARAM;
		}
		this->device = IOHIDDeviceCreate(kCFAllocatorDefault, hidDevice);
		if(!this->device) {
			// fprintf(stderr,"Feedback: Failed to initialise\n");
			return FFERR_NOINTERFACE;
		}
		IOHIDDeviceOpen(this->device, 0);
		Queue = dispatch_queue_create("com.mice.driver.FeedbackXBOBT", NULL);
		Timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, Queue);
		dispatch_source_set_timer(Timer, dispatch_walltime(NULL, 0), LoopGranularity*1000, 10);
		dispatch_set_context(Timer, this);
		dispatch_source_set_event_handler_f(Timer, EffectProc);
		dispatch_resume(Timer);
	}
	else {
		dispatch_sync(Queue, ^{
			dispatch_source_cancel(Timer);
			SetForce(0, 0, 0, 0);
			IOHIDDeviceClose(this->device, 0);
			CFRelease(this->device);
		});
		
	}
	return FF_OK;
}

HRESULT FeedbackXBOBT::DestroyEffect(FFEffectDownloadID EffectHandle)
{
	__block HRESULT Result = FF_OK;
	dispatch_sync(Queue, ^{
		for (FeedbackXBOEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
		{
			if (effectIterator->Handle == EffectHandle)
			{
				EffectList.erase(effectIterator);
				break;
			}
		}
	});
	return Result;
}

HRESULT FeedbackXBOBT::Escape(FFEffectDownloadID downloadID, FFEFFESCAPE *escape)
{
	if (downloadID!=0) return FFERR_UNSUPPORTED;
	if (escape->dwSize < sizeof(FFEFFESCAPE)) return FFERR_INVALIDPARAM;
	escape->cbOutBuffer=0;
	switch (escape->dwCommand) {
#if 0
		case 0x00:  // Control motors
			if(escape->cbInBuffer!=1) return FFERR_INVALIDPARAM;
			dispatch_sync(Queue, ^{
				Manual=((unsigned char*)escape->lpvInBuffer)[0]!=0x00;
			});
			break;
			
		case 0x01:  // Set motors
			if (escape->cbInBuffer!=2) return FFERR_INVALIDPARAM;
			dispatch_sync(Queue, ^{
				if(Manual) {
					unsigned char *data=(unsigned char *)escape->lpvInBuffer;
					unsigned char buf[]={0x00,0x04,data[0],data[1]};
					Device_Send(&this->device,buf,sizeof(buf));
				}
			});
			break;
			
		case 0x02:  // Set LED
			if (escape->cbInBuffer!=1) return FFERR_INVALIDPARAM;
		{
			dispatch_sync(Queue, ^{
				unsigned char *data=(unsigned char *)escape->lpvInBuffer;
				unsigned char buf[]={0x01,0x03,data[0]};
				Device_Send(&this->device,buf,sizeof(buf));
			});
		}
			break;
			
		case 0x03:  // Power off
		{
			dispatch_sync(Queue, ^{
				unsigned char buf[] = {0x02, 0x02};
				Device_Send(&this->device, buf, sizeof(buf));
			});
		}
			break;
#endif
		default:
			fprintf(stderr, "XboxOneBTController FF plugin: Unknown escape (%i)\n", (int)escape->dwCommand);
			return FFERR_UNSUPPORTED;
	}
	return FF_OK;
}

void FeedbackXBOBT::SetForce(LONG LeftLevel, LONG RightLevel, LONG ltLevel, LONG rtLevel)
{
	//fprintf(stderr, "LS: %d; RS: %d\n", (unsigned char)MIN( 255, LeftLevel * Gain / 10000 ), (unsigned char)MIN( 255, RightLevel * Gain / 10000 ));
	XboxOneBluetoothReport_t report = {0};
	report.reportID = 0x03;
	report.activationMask = 0x0f;
	report.ltMagnitude = (unsigned char)min(SCALE_MAX, ltLevel * (LONG)Gain / 10000 );
	report.rtMagnitude = (unsigned char)min(SCALE_MAX, rtLevel * (LONG)Gain / 10000 );
	report.leftMagnitude = (unsigned char)min(SCALE_MAX, LeftLevel * (LONG)Gain / 10000 );
	report.rightMagnitude = (unsigned char)min(SCALE_MAX, RightLevel * (LONG)Gain / 10000 );
	report.duration = 0x7f;
	report.loopCount = 10;

	if (!Manual) {
		IOReturn retVal = IOHIDDeviceSetReport(device, kIOHIDReportTypeOutput, report.reportID, (const uint8_t*)&report, sizeof(XboxOneBluetoothReport_t));
		if (retVal != 0) {
			printf("IOHIDDeviceSetReport returned %d (system %d, subsystem %d, code %d)\n", retVal, err_get_system(retVal), err_get_sub(retVal), err_get_code(retVal));
		}
	}
}

void FeedbackXBOBT::EffectProc( void *params )
{
	FeedbackXBOBT *cThis = (FeedbackXBOBT *)params;
	
	LONG LeftLevel = 0;
	LONG RightLevel = 0;
	LONG ltLevel = 0;
	LONG rtLevel = 0;
	LONG Gain  = cThis->Gain;
	LONG CalcResult = 0;
	
	if (cThis->Actuator == true)
	{
		for (FeedbackXBOEffectIterator effectIterator = cThis->EffectList.begin(); effectIterator != cThis->EffectList.end(); ++effectIterator)
		{
			if(((CurrentTimeUsingMach() - cThis->LastTime)*1000*1000) >= effectIterator->DiEffect.dwSamplePeriod) {
				CalcResult = effectIterator->Calc(&LeftLevel, &RightLevel, &ltLevel, &rtLevel);
			}
		}
	}
	
	if ((cThis->PrvLeftLevel != LeftLevel || cThis->PrvRightLevel != RightLevel) && (CalcResult != -1))
	{
		//fprintf(stderr, "PL: %d, PR: %d; L: %d, R: %d; \n", cThis->PrvLeftLevel, cThis->PrvRightLevel, LeftLevel, RightLevel);
		cThis->SetForce((unsigned char)min(SCALE_MAX, LeftLevel * Gain / 10000),(unsigned char)min(SCALE_MAX, RightLevel * Gain / 10000 ), (unsigned char)min(SCALE_MAX, ltLevel * Gain / 10000), (unsigned char)min(SCALE_MAX, rtLevel * Gain / 10000));
		
		cThis->PrvLeftLevel = LeftLevel;
		cThis->PrvRightLevel = RightLevel;
	}
}

HRESULT FeedbackXBOBT::GetEffectStatus(FFEffectDownloadID EffectHandle, FFEffectStatusFlag *Status)
{
	dispatch_sync(Queue, ^{
		for (FeedbackXBOEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
		{
			if (effectIterator->Handle == EffectHandle)
			{
				*Status = effectIterator->Status;
				break;
			}
		}
	});
	return FF_OK;
}

HRESULT FeedbackXBOBT::GetVersion(ForceFeedbackVersion *version)
{
	version->apiVersion.majorRev = kFFPlugInAPIMajorRev;
	version->apiVersion.minorAndBugRev = kFFPlugInAPIMinorAndBugRev;
	version->apiVersion.stage = kFFPlugInAPIStage;
	version->apiVersion.nonRelRev = kFFPlugInAPINonRelRev;
	version->plugInVersion.majorRev = FeedbackDriverVersionMajor;
	version->plugInVersion.minorAndBugRev = FeedbackDriverVersionMinor;
	version->plugInVersion.stage = FeedbackDriverVersionStage;
	version->plugInVersion.nonRelRev = FeedbackDriverVersionNonRelRev;
	return FF_OK;
}

// static c->c++ glue functions
HRESULT FeedbackXBOBT::sQueryInterface(void *self, REFIID iid, LPVOID *ppv)
{
	FeedbackXBOBT *obj = ((XboxOneBTInterfaceMap *)self)->obj;
	return obj->QueryInterface(iid, ppv);
}

ULONG FeedbackXBOBT::sAddRef(void *self)
{
	FeedbackXBOBT *obj = ( (XboxOneBTInterfaceMap *) self)->obj;
	return obj->AddRef();
}

ULONG FeedbackXBOBT::sRelease(void *self)
{
	FeedbackXBOBT *obj = ( (XboxOneBTInterfaceMap *) self)->obj;
	return obj->Release();
}

IOReturn FeedbackXBOBT::sProbe(void *self, CFDictionaryRef propertyTable, io_service_t service, SInt32 *order)
{
	return getThis(self)->Probe(propertyTable, service, order);
}

IOReturn FeedbackXBOBT::sStart(void *self, CFDictionaryRef propertyTable, io_service_t service)
{
	return getThis(self)->Start(propertyTable, service);
}

IOReturn FeedbackXBOBT::sStop(void *self)
{
	return getThis(self)->Stop();
}

HRESULT FeedbackXBOBT::sGetVersion(void * self, ForceFeedbackVersion * version)
{
	return FeedbackXBOBT::getThis(self)->GetVersion(version);
}

HRESULT FeedbackXBOBT::sInitializeTerminate(void * self, NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin)
{
	return FeedbackXBOBT::getThis(self)->InitializeTerminate(forceFeedbackAPIVersion, hidDevice, begin);
}

HRESULT FeedbackXBOBT::sDestroyEffect(void * self, FFEffectDownloadID downloadID)
{
	return FeedbackXBOBT::getThis(self)->DestroyEffect(downloadID);
}

HRESULT FeedbackXBOBT::sDownloadEffect(void * self, CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags)
{
	return FeedbackXBOBT::getThis(self)->DownloadEffect(effectType, pDownloadID, pEffect, flags);
}

HRESULT FeedbackXBOBT::sEscape(void * self, FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape)
{
	return FeedbackXBOBT::getThis(self)->Escape(downloadID, pEscape);
}

HRESULT FeedbackXBOBT::sGetEffectStatus(void * self, FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode)
{
	return FeedbackXBOBT::getThis(self)->GetEffectStatus(downloadID, pStatusCode);
}

HRESULT FeedbackXBOBT::sGetForceFeedbackState(void * self, ForceFeedbackDeviceState * pDeviceState)
{
	return FeedbackXBOBT::getThis(self)->GetForceFeedbackState(pDeviceState);
}

HRESULT FeedbackXBOBT::sGetForceFeedbackCapabilities(void * self, FFCAPABILITIES * capabilities)
{
	return FeedbackXBOBT::getThis(self)->GetForceFeedbackCapabilities(capabilities);
}

HRESULT FeedbackXBOBT::sSendForceFeedbackCommand(void * self, FFCommandFlag state)
{
	return FeedbackXBOBT::getThis(self)->SendForceFeedbackCommand(state);
}

HRESULT FeedbackXBOBT::sSetProperty(void * self, FFProperty property, void * pValue)
{
	return FeedbackXBOBT::getThis(self)->SetProperty(property, pValue);
}

HRESULT FeedbackXBOBT::sStartEffect(void * self, FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations)
{
	return FeedbackXBOBT::getThis(self)->StartEffect(downloadID, mode, iterations);
}

HRESULT FeedbackXBOBT::sStopEffect(void * self, UInt32 downloadID)
{
	return FeedbackXBOBT::getThis(self)->StopEffect(downloadID);
}

// External factory function
extern "C" void* FeedbackXBOBTFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
	void* result = NULL;
	if (CFEqual(typeID, kIOForceFeedbackLibTypeID))
		result = (void*)FeedbackXBOBT::Alloc();
	return result;
}

