// The Spotlight importer of route set files (.smroute): their cities' names
// (and, in the newer format, the name map's values) as the text content.
// CFPlugInCOM.h first: MDImporter.h declares the interface struct only after it.
#import <CoreFoundation/CFPlugInCOM.h>
#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>

#define PLUGIN_ID "53572418-2503-4A54-B021-BDAA584CED27"

extern "C" {
Boolean GetMetadataForFile(void *thisInterface, CFMutableDictionaryRef attributes, CFStringRef contentTypeUTI,
                           CFStringRef pathToFile);
void *MetadataImporterPluginFactory(CFAllocatorRef allocator, CFUUIDRef typeID);
}

static NSString *const _space = @" ";

// Appends the non-empty strings, each followed by a space. True when there
// was one.
static BOOL _appendStrings(NSMutableString *aText, NSArray *aStrings)
{
	BOOL appended = NO;
	NSUInteger count = [aStrings count];
	for (NSUInteger i = 0; i < count; i++) {
		NSString *string = [aStrings objectAtIndex:i];
		if ([string length]) {
			[aText appendString:string];
			[aText appendString:_space];
			appended = YES;
		}
	}
	return appended;
}

// A route set is an array of entries, or a dictionary with the entries and a
// name map.
Boolean GetMetadataForFile(void *thisInterface, CFMutableDictionaryRef attributes, CFStringRef contentTypeUTI,
                           CFStringRef pathToFile)
{
	@autoreleasepool {
		NSString *path = (NSString *)pathToFile;
		NSArray *entries = [NSArray arrayWithContentsOfFile:path];
		NSArray *names = nil;
		if (!entries) {
			NSDictionary *routeSet = [NSDictionary dictionaryWithContentsOfFile:path];
			entries = [routeSet objectForKey:@"entries"];
			names = [[routeSet objectForKey:@"nameMap"] allValues];
		}
		NSMutableString *text = [NSMutableString string];
		NSMutableArray *cityNames = [NSMutableArray array];
		NSUInteger count = [entries count];
		for (NSUInteger i = 0; i < count; i++) {
			NSString *name = [[entries objectAtIndex:i] objectForKey:@"CityName"];
			if (name)
				[cityNames addObject:name];
		}
		BOOL found = _appendStrings(text, cityNames);
		found = _appendStrings(text, names) || found;
		if (found)
			[(NSMutableDictionary *)attributes setObject:text forKey:(NSString *)kMDItemTextContent];
		return found;
	}
}

// The usual CFPlugIn boilerplate.

typedef struct __MetadataImporterPluginType {
	MDImporterInterfaceStruct *conduitInterface;
	CFUUIDRef factoryID;
	UInt32 refCount;
} MetadataImporterPluginType;

static HRESULT MetadataImporterQueryInterface(void *thisInstance, REFIID iid, LPVOID *ppv);
static ULONG MetadataImporterPluginAddRef(void *thisInstance);
static ULONG MetadataImporterPluginRelease(void *thisInstance);

static MDImporterInterfaceStruct testInterfaceFtbl = {
	NULL,
	MetadataImporterQueryInterface,
	MetadataImporterPluginAddRef,
	MetadataImporterPluginRelease,
	GetMetadataForFile
};

static MetadataImporterPluginType *AllocMetadataImporterPluginType(CFUUIDRef inFactoryID)
{
	MetadataImporterPluginType *instance = (MetadataImporterPluginType *)malloc(sizeof(MetadataImporterPluginType));
	instance->factoryID = NULL;
	instance->refCount = 0;
	instance->conduitInterface = &testInterfaceFtbl;
	instance->factoryID = (CFUUIDRef)CFRetain(inFactoryID);
	CFPlugInAddInstanceForFactory(inFactoryID);
	instance->refCount = 1;
	return instance;
}

static void DeallocMetadataImporterPluginType(MetadataImporterPluginType *thisInstance)
{
	CFUUIDRef factoryID = thisInstance->factoryID;
	free(thisInstance);
	if (factoryID) {
		CFPlugInRemoveInstanceForFactory(factoryID);
		CFRelease(factoryID);
	}
}

static HRESULT MetadataImporterQueryInterface(void *thisInstance, REFIID iid, LPVOID *ppv)
{
	CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);
	if (CFEqual(interfaceID, kMDImporterInterfaceID) || CFEqual(interfaceID, IUnknownUUID)) {
		MetadataImporterPluginAddRef(thisInstance);
		*ppv = thisInstance;
		CFRelease(interfaceID);
		return S_OK;
	}
	*ppv = NULL;
	CFRelease(interfaceID);
	return E_NOINTERFACE;
}

static ULONG MetadataImporterPluginAddRef(void *thisInstance)
{
	return ++((MetadataImporterPluginType *)thisInstance)->refCount;
}

static ULONG MetadataImporterPluginRelease(void *thisInstance)
{
	MetadataImporterPluginType *instance = (MetadataImporterPluginType *)thisInstance;
	ULONG refCount = --instance->refCount;
	if (refCount == 0)
		DeallocMetadataImporterPluginType(instance);
	return refCount;
}

void *MetadataImporterPluginFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
	if (!CFEqual(typeID, kMDImporterTypeID))
		return NULL;
	CFUUIDRef factoryID = CFUUIDCreateFromString(kCFAllocatorDefault, CFSTR(PLUGIN_ID));
	MetadataImporterPluginType *result = AllocMetadataImporterPluginType(factoryID);
	CFRelease(factoryID);
	return result;
}
