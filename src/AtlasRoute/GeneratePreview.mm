// The Quick Look entry points and the usual CFPlugIn boilerplate. Previews
// are 512 x 512; thumbnails take the requested size.
#import <CoreFoundation/CFPlugInCOM.h>
#import <QuickLook/QuickLook.h>
#import "AtlasRoute.h"

#define PLUGIN_ID "D47FCAD2-24AA-4ACF-BBAD-F87514941581"

static const CGFloat kPreviewSize = 512.0;

extern "C" {
OSStatus GenerateThumbnailForURL(void *thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url,
                                 CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize);
void CancelThumbnailGeneration(void *thisInterface, QLThumbnailRequestRef thumbnail);
OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url,
                               CFStringRef contentTypeUTI, CFDictionaryRef options);
void CancelPreviewGeneration(void *thisInterface, QLPreviewRequestRef preview);
void *QuickLookGeneratorPluginFactory(CFAllocatorRef allocator, CFUUIDRef typeID);
}

OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url,
                               CFStringRef contentTypeUTI, CFDictionaryRef options)
{
	CGContextRef context = QLPreviewRequestCreateContext(preview, CGSizeMake(kPreviewSize, kPreviewSize), false, NULL);
	if (context) {
		drawRouteInContextWithSize(url, context, CGSizeMake(kPreviewSize, kPreviewSize));
		QLPreviewRequestFlushContext(preview, context);
		CFRelease(context);
	}
	return noErr;
}

void CancelPreviewGeneration(void *thisInterface, QLPreviewRequestRef preview)
{
}

OSStatus GenerateThumbnailForURL(void *thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url,
                                 CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize)
{
	CGContextRef context = QLThumbnailRequestCreateContext(thumbnail, maxSize, true, NULL);
	if (context) {
		drawRouteInContextWithSize(url, context, maxSize);
		QLThumbnailRequestFlushContext(thumbnail, context);
		CFRelease(context);
	}
	return noErr;
}

void CancelThumbnailGeneration(void *thisInterface, QLThumbnailRequestRef thumbnail)
{
}

typedef struct __QuickLookGeneratorPluginType {
	void *conduitInterface;
	CFUUIDRef factoryID;
	UInt32 refCount;
} QuickLookGeneratorPluginType;

static HRESULT QuickLookGeneratorQueryInterface(void *thisInstance, REFIID iid, LPVOID *ppv);
static ULONG QuickLookGeneratorPluginAddRef(void *thisInstance);
static ULONG QuickLookGeneratorPluginRelease(void *thisInstance);

static QLGeneratorInterfaceStruct myInterfaceFtbl = {
	NULL,
	QuickLookGeneratorQueryInterface,
	QuickLookGeneratorPluginAddRef,
	QuickLookGeneratorPluginRelease,
	NULL,
	NULL,
	NULL,
	NULL
};

static QuickLookGeneratorPluginType *AllocQuickLookGeneratorPluginType(CFUUIDRef inFactoryID)
{
	QuickLookGeneratorPluginType *instance = (QuickLookGeneratorPluginType *)calloc(1, sizeof(QuickLookGeneratorPluginType));
	instance->conduitInterface = malloc(sizeof(QLGeneratorInterfaceStruct));
	memcpy(instance->conduitInterface, &myInterfaceFtbl, sizeof(QLGeneratorInterfaceStruct));
	instance->factoryID = (CFUUIDRef)CFRetain(inFactoryID);
	CFPlugInAddInstanceForFactory(inFactoryID);
	instance->refCount = 1;
	return instance;
}

static void DeallocQuickLookGeneratorPluginType(QuickLookGeneratorPluginType *thisInstance)
{
	CFUUIDRef factoryID = thisInstance->factoryID;
	free(thisInstance->conduitInterface);
	free(thisInstance);
	if (factoryID) {
		CFPlugInRemoveInstanceForFactory(factoryID);
		CFRelease(factoryID);
	}
}

// Fills in the callbacks when Quick Look asks for them.
static HRESULT QuickLookGeneratorQueryInterface(void *thisInstance, REFIID iid, LPVOID *ppv)
{
	CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);
	if (!CFEqual(interfaceID, kQLGeneratorCallbacksInterfaceID)) {
		*ppv = NULL;
		CFRelease(interfaceID);
		return E_NOINTERFACE;
	}
	QLGeneratorInterfaceStruct *callbacks =
	    (QLGeneratorInterfaceStruct *)((QuickLookGeneratorPluginType *)thisInstance)->conduitInterface;
	callbacks->GenerateThumbnailForURL = GenerateThumbnailForURL;
	callbacks->CancelThumbnailGeneration = CancelThumbnailGeneration;
	callbacks->GeneratePreviewForURL = GeneratePreviewForURL;
	callbacks->CancelPreviewGeneration = CancelPreviewGeneration;
	callbacks->AddRef(thisInstance);
	*ppv = thisInstance;
	CFRelease(interfaceID);
	return S_OK;
}

static ULONG QuickLookGeneratorPluginAddRef(void *thisInstance)
{
	return ++((QuickLookGeneratorPluginType *)thisInstance)->refCount;
}

static ULONG QuickLookGeneratorPluginRelease(void *thisInstance)
{
	QuickLookGeneratorPluginType *instance = (QuickLookGeneratorPluginType *)thisInstance;
	ULONG refCount = --instance->refCount;
	if (refCount == 0)
		DeallocQuickLookGeneratorPluginType(instance);
	return refCount;
}

void *QuickLookGeneratorPluginFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
	if (!CFEqual(typeID, kQLGeneratorTypeID))
		return NULL;
	CFUUIDRef factoryID = CFUUIDCreateFromString(kCFAllocatorDefault, CFSTR(PLUGIN_ID));
	QuickLookGeneratorPluginType *result = AllocQuickLookGeneratorPluginType(factoryID);
	CFRelease(factoryID);
	return result;
}
