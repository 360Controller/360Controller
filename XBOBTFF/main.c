//
//  main.c
//  XBoxBTFFPlug
//
//  Created by C.W. Betts on 9/20/17.
//  Copyright © 2017 C.W. Betts. All rights reserved.
//

#include "FFDriver.h"
#include <ForceFeedback/IOForceFeedbackLib.h>

// -----------------------------------------------------------------------------
//	typedefs
// -----------------------------------------------------------------------------

// The layout for an instance of MetaDataImporterPlugIn
typedef struct __MetadataImporterPluginType {
	void		*conduitInterface;
	CFUUIDRef	factoryID;
	UInt32		refCount;
} MDImportPlug;

// -----------------------------------------------------------------------------
//	prototypes
// -----------------------------------------------------------------------------
//	Forward declaration for the IUnknown implementation.
//

static MDImportPlug	*AllocMetadataImporterPluginType(CFUUIDRef inFactoryID);
static void			DeallocMetadataImporterPluginType(MDImportPlug *thisInstance);
static HRESULT		MetadataImporterQueryInterface(void *thisInstance,REFIID iid,LPVOID *ppv);
extern void			*FeedbackXBOBTFactory(CFAllocatorRef allocator,CFUUIDRef typeID) __attribute__((visibility ("default")));
static ULONG		MetadataImporterPluginAddRef(void *thisInstance);
static ULONG		MetadataImporterPluginRelease(void *thisInstance);

// -----------------------------------------------------------------------------
//	testInterfaceFtbl	definition
// -----------------------------------------------------------------------------
//	The TestInterface function table.
//

static IOForceFeedbackDeviceInterface testInterfaceFtbl = {
	NULL,
	MetadataImporterQueryInterface,
	MetadataImporterPluginAddRef,
	MetadataImporterPluginRelease,
	BTFFGetVersion,
	BTFFInitializeTerminate,
	BTFFDestroyEffect,
	BTFFDownloadEffect,
	BTFFEscape,
	BTFFGetEffectStatus,
	BTFFGetForceFeedbackCapabilities,
	BTFFGetForceFeedbackState,
	BTFFSendForceFeedbackCommand,
	BTFFSetProperty,
	BTFFStartEffect,
	BTFFStopEffect
};

// -----------------------------------------------------------------------------
//	AllocMetadataImporterPluginType
// -----------------------------------------------------------------------------
//	Utility function that allocates a new instance.
//      You can do some initial setup for the importer here if you wish
//      like allocating globals etc...
//
static MDImportPlug *AllocMetadataImporterPluginType(CFUUIDRef inFactoryID)
{
	MDImportPlug *theNewInstance = (MDImportPlug *)calloc(sizeof(MDImportPlug), 1);
	
	/* Point to the function table */
	theNewInstance->conduitInterface = &testInterfaceFtbl;
	
	/*  Retain and keep an open instance refcount for each factory. */
	theNewInstance->factoryID = CFRetain(inFactoryID);
	CFPlugInAddInstanceForFactory(inFactoryID);
	
	/* This function returns the IUnknown interface so set the refCount to one. */
	theNewInstance->refCount = 1;
	return theNewInstance;
}

// -----------------------------------------------------------------------------
//	DeallocMetadataImporterPluginType
// -----------------------------------------------------------------------------
//	Utility function that deallocates the instance when
//	the refCount goes to zero.
//      In the current implementation importer interfaces are never deallocated
//      but implement this as this might change in the future
//
static void DeallocMetadataImporterPluginType(MDImportPlug *thisInstance)
{
	CFUUIDRef theFactoryID = thisInstance->factoryID;
	
	free(thisInstance);
	if (theFactoryID) {
		CFPlugInRemoveInstanceForFactory(theFactoryID);
		CFRelease(theFactoryID);
	}
}

// -----------------------------------------------------------------------------
//	MetadataImporterQueryInterface
// -----------------------------------------------------------------------------
//	Implementation of the IUnknown QueryInterface function.
//
static HRESULT MetadataImporterQueryInterface(void *thisInstance, REFIID iid, LPVOID *ppv)
{
	CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);
	
	if (CFEqual(interfaceID, kIOForceFeedbackDeviceInterfaceID)) {
		/* If the right interface was requested, bump the ref count,
		 * set the ppv parameter equal to the instance, and
		 * return good status.
		 */
		((MDImportPlug*)thisInstance)->conduitInterface = &testInterfaceFtbl;
		((IOForceFeedbackDeviceInterface *)((MDImportPlug*)thisInstance)->conduitInterface)->AddRef(thisInstance);
		*ppv = thisInstance;
		CFRelease(interfaceID);
		return S_OK;
	} else if (CFEqual(interfaceID, IUnknownUUID)) {
		/* If the IUnknown interface was requested, same as above. */
		((MDImportPlug*)thisInstance)->conduitInterface = &testInterfaceFtbl;
		((IOForceFeedbackDeviceInterface *)((MDImportPlug*)thisInstance)->conduitInterface)->AddRef(thisInstance);
		*ppv = thisInstance;
		CFRelease(interfaceID);
		return S_OK;
	} else {
		/* Requested interface unknown, bail with error. */
		*ppv = NULL;
		CFRelease(interfaceID);
		return E_NOINTERFACE;
	}
}

// -----------------------------------------------------------------------------
//	MetadataImporterPluginAddRef
// -----------------------------------------------------------------------------
//	Implementation of reference counting for this type. Whenever an interface
//	is requested, bump the refCount for the instance. NOTE: returning the
//	refcount is a convention but is not required so don't rely on it.
//
static ULONG MetadataImporterPluginAddRef(void *thisInstance)
{
	return ++((MDImportPlug*)thisInstance)->refCount;
}

// -----------------------------------------------------------------------------
// MetadataImporterPluginRelease
// -----------------------------------------------------------------------------
//	When an interface is released, decrement the refCount.
//	If the refCount goes to zero, deallocate the instance.
//
static ULONG MetadataImporterPluginRelease(void *thisInstance)
{
	((MDImportPlug*)thisInstance)->refCount -= 1;
	if (((MDImportPlug*)thisInstance)->refCount == 0) {
		DeallocMetadataImporterPluginType((MDImportPlug*)thisInstance);
		return 0;
	} else {
		return ((MDImportPlug*)thisInstance)->refCount;
	}
}

// -----------------------------------------------------------------------------
//	PPMetadataImporterPluginFactory
// -----------------------------------------------------------------------------
//	Implementation of the factory function for this type.
//
void *FeedbackXBOBTFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
	MDImportPlug	*result;
	CFUUIDRef		uuid;
	
	/* If correct type is being requested, allocate an
	 * instance of TestType and return the IUnknown interface.
	 */
	
	if (CFEqual(typeID, kIOForceFeedbackLibTypeID)) {
		uuid = BTFFPLUGINTERFACE;
		result = AllocMetadataImporterPluginType(uuid);
		CFRelease(uuid);
		return result;
	}
	/* If the requested type is incorrect, return NULL. */
	return NULL;
}
