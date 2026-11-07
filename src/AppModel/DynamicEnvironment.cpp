// DynamicEnvironment: loads the URL creator plug-in, which makes the URLs of
// the weather service. The library ships with the application (BASE/URLCreator)
// and is wrapped into a bundle in the application data folder; the MacKiev
// server could send a newer library.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(DynamicEnvironment, 0)

void DynamicEnvironment::initAsSingleton(void *)
{
}

LString *DynamicEnvironment::_pathBundled()
{
	return &kOwnLibName;
}

LString *DynamicEnvironment::_dylibRootPath()
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	if (callbacks == 0)
		return 0;
	LString *path = callbacks->_applicationDataPath(1);
	return path ? path->stringByAppendingPathComponent(&kLibBundleName) : 0;
}

void DynamicEnvironment::_unloadBundle()
{
	if (_bundle == 0)
		return;
	CFBundleUnloadExecutable(_bundle);
	CFRelease(_bundle);
	_bundle = 0;
	_getURLForClouds = 0;
	_getURLForReport = 0;
	_getURLForReportByID = 0;
	_getXMLForReportData = 0;
	_getURLDLLVersion = 0;
	_getJPGForCloudsData = 0;
	_getReportUN = 0;
	_getReportP = 0;
	_getMapsUN = 0;
	_getMapsP = 0;
}

void DynamicEnvironment::dealloc()
{
	_unloadBundle();
	LObject::dealloc();
}

// The library inside the bundle.
LString *DynamicEnvironment::_pathCached()
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	if (callbacks == 0)
		return 0;
	LString *root = callbacks->_applicationDataPath(1);
	if (root == 0)
		return 0;
	LString *path = LString::stringWithString(root);
	path->appendPathComponent(&kLibBundleName);
	path->appendPathComponent(LString::stringWithCString("Contents"));
	path->appendPathComponent(LString::stringWithCString("MacOS"));
	path->appendPathComponent(&kLibName);
	return path;
}

void DynamicEnvironment::_createPluginBundleWithDYLIBBody(LData *body, const LString *root)
{
	LMap *folderAttrs = LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777),
	                                                LFileManager::fileAttrPosixPermissions, 0);
	LMap *fileAttrs = LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0664),
	                                              LFileManager::fileAttrPosixPermissions, 0);
	LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0775), LFileManager::fileAttrPosixPermissions, (LObject *)0);
	LFileManager *fm = LFileManager::manager();

	LString *path = LString::stringWithString(root);
	fm->createFolderAtPath(path);
	fm->changeFileAttributesAtPath(path, folderAttrs);
	path->appendPathComponent(LString::stringWithCString("Contents"));
	fm->createFolderAtPath(path);
	fm->changeFileAttributesAtPath(path, folderAttrs);

	LString *plistPath = path->stringByAppendingPathComponent(LString::stringWithCString("Info.plist"));
	LMap *info = LMap::mapWithObjectsAndKeys(
	    LString::stringWithCString("6.0"), LString::stringWithCString("CFBundleInfoDictionaryVersion"),
	    LString::stringWithCString("BNDL"), LString::stringWithCString("CFBundlePackageType"), &kLibName,
	    LString::stringWithCString("CFBundleExecutable"), LString::stringWithCString("com.mackiev.3DWG2.URLCreator"),
	    LString::stringWithCString("CFBundleIdentifier"), 0);
	info->writeToFile(plistPath);
	fm->changeFileAttributesAtPath(plistPath, fileAttrs);

	path->appendPathComponent(LString::stringWithCString("MacOS"));
	fm->createFolderAtPath(path);
	fm->changeFileAttributesAtPath(path, folderAttrs);
	path->appendPathComponent(&kLibName);
	body->writeToFile(path);
	fm->changeFileAttributesAtPath(path, folderAttrs);
}

void DynamicEnvironment::replaceDYLIBWithNew(LData *body)
{
	_unloadBundle();
	if (body)
		_createPluginBundleWithDYLIBBody(body, _dylibRootPath());
}

// Copies the shipped library into the bundle when the bundle's is missing or
// older, then loads it.
void DynamicEnvironment::_loadBundleIfNeeded()
{
	if (_bundle)
		return;
	LString *cached = _pathCached();
	LF_CHECK_PTR(cached);
	LFileManager *fm = LFileManager::manager();
	double cachedTime = fm->lastUpdateTimeForFile(cached);
	double bundledTime = fm->lastUpdateTimeForFile(&kOwnLibName);
	if (!fm->objectExistsAtPath(cached, 0) || cachedTime < bundledTime) {
		LData *body = LData::dataWithContentsOfFile(&kOwnLibName);
		LF_CHECK_PTR(body);
		_createPluginBundleWithDYLIBBody(body, _dylibRootPath());
	}

	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, _dylibRootPath()->cfString(),
	                                             kCFURLPOSIXPathStyle, false);
	if (url) {
		_bundle = CFBundleCreate(kCFAllocatorDefault, url);
		CFRelease(url);
		if (_bundle == 0)
			return;
		if (!CFBundleLoadExecutable(_bundle)) {
			CFRelease(_bundle);
			_bundle = 0;
		}
	}
	if (_bundle == 0)
		return;
	_getURLForClouds = (TCloudsURLFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetURLForCloudsDownload"));
	_getURLForReport = (TReportURLFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetURLForReportDownload"));
	_getURLForReportByID =
	    (TReportByIDURLFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetURLForReportDownloadByID"));
	_getURLDLLVersion = (TVersionFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetURLDLLVersion"));
	_getXMLForReportData = (TDataFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetXMLForReportData"));
	_getJPGForCloudsData = (TDataFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetJPGForCloudsData"));
	_getReportUN = (TStringFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetReportUN"));
	_getReportP = (TStringFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetReportP"));
	_getMapsUN = (TStringFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetMapsUN"));
	_getMapsP = (TStringFunc)CFBundleGetFunctionPointerForName(_bundle, CFSTR("GetMapsP"));
}

LData *DynamicEnvironment::jpegDataForCloudsData(LData *data)
{
	_loadBundleIfNeeded();
	return _getJPGForCloudsData ? _getJPGForCloudsData(data) : data;
}

LData *DynamicEnvironment::xmlDataForReport(TReportType, LData *data)
{
	_loadBundleIfNeeded();
	return _getXMLForReportData ? _getXMLForReportData(data) : data;
}

LString *DynamicEnvironment::mapsP()
{
	_loadBundleIfNeeded();
	return _getMapsP ? _getMapsP() : 0;
}

LString *DynamicEnvironment::mapsUN()
{
	_loadBundleIfNeeded();
	return _getMapsUN ? _getMapsUN() : 0;
}

LString *DynamicEnvironment::reportP()
{
	_loadBundleIfNeeded();
	return _getReportP ? _getReportP() : 0;
}

LString *DynamicEnvironment::reportUN()
{
	_loadBundleIfNeeded();
	return _getReportUN ? _getReportUN() : 0;
}

// The plug-in numbers the reports from 0.
LString *DynamicEnvironment::URLForReportDownload(TReportType type, int customWeatherID)
{
	_loadBundleIfNeeded();
	return _getURLForReportByID ? _getURLForReportByID(customWeatherID, type - 1) : 0;
}

LString *DynamicEnvironment::URLForReportDownload(TReportType type, LPoint geoPoint)
{
	_loadBundleIfNeeded();
	return _getURLForReport ? _getURLForReport(geoPoint, type - 1) : 0;
}

LString *DynamicEnvironment::URLForClouds(unsigned char west, LDate *date, int size)
{
	_loadBundleIfNeeded();
	return _getURLForClouds ? _getURLForClouds(west, date, size) : 0;
}

int DynamicEnvironment::currentVersion()
{
	_loadBundleIfNeeded();
	return _getURLDLLVersion ? _getURLDLLVersion() : 0;
}
