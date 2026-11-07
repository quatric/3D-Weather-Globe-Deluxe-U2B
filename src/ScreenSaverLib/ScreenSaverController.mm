// ScreenSaverController: the saver's model callbacks. The libraries come
// from the application the saver framework is inside of
// (X.app/Contents/Frameworks/ScreenSaverLib.framework), and so do the
// resources (BASE) and the build number.

#import <Cocoa/Cocoa.h>
#include "ScreenSaverLib.h"
#include "NSString_LStringExtension.h"

static NSBundle *_screenSaverLibBundle()
{
	return [NSBundle bundleForClass:[ScreenSaverGLView class]];
}

static NSBundle *_applicationBundle()
{
	NSString *path = [[[[_screenSaverLibBundle() bundlePath] stringByDeletingLastPathComponent]
	    stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
	return [NSBundle bundleWithPath:path];
}

ScreenSaverController::ScreenSaverController() : _fileManager(0), _cachesPath(0), _sharedDataPath(0), _userDataPath(0)
{
	initializeLFoundation(1, 0);
	_fileManager = new CZipFileManager(_resourcePath());
	LUserDefaults::setUniqueRootKey(_screenSaverLibBundleID());
	_validateFrameworks();
	IModelCallbacks::setCallbacks(this);
	Preferences::registerDefaults(1);
	LAutoreleasePool::addNewPoolToTop();
	((DynamicEnvironment *)LF_CLASSOF(DynamicEnvironment)->singleton(0))->currentVersion();
	LAutoreleasePool::removeTopPool();
}

ScreenSaverController::~ScreenSaverController()
{
	CHashTableIterator it(&_interfaces);
	while (it.next()) {
		LAutoreleasePool::addNewPoolToTop();
		delete (ScreenSaverInterface *)it.value();
		LAutoreleasePool::removeTopPool();
	}
	releaseAndClear(_userDataPath);
	releaseAndClear(_sharedDataPath);
	releaseAndClear(_cachesPath);
	finalizeRenderLib();
	finalizeAppModel();
	finalizeFishHook();
	IModelCallbacks::setCallbacks(0);
	delete _fileManager;
	_fileManager = 0;
	finilizeLFoundation();
}

void ScreenSaverController::openPanel(int mode, void *systemView)
{
	intptr_t wnd = 0;
	if (!_interfaces.getValueForKey(&wnd, (intptr_t)systemView)) {
		LLog("Accelerated view not found for system view <0x%08x>\n", systemView);
		return;
	}
	LAutoreleasePool::addNewPoolToTop();
	((ScreenSaverInterface *)wnd)->openPanel(mode);
	LAutoreleasePool::removeTopPool();
}

// The saver's panel is the globe's saver version (TMapMode 2).
bool ScreenSaverController::createAcceleratedView(void *systemView, CDeviceFormat *format)
{
	LAutoreleasePool::addNewPoolToTop();
	intptr_t wnd = 0;
	bool exists = _interfaces.getValueForKey(&wnd, (intptr_t)systemView);
	if (!exists) {
		ScreenSaverInterface *window = new ScreenSaverInterface(systemView, format);
		_interfaces.setValueForKey((intptr_t)window, (intptr_t)systemView);
		openPanel(2, systemView);
	} else
		LLog("Accelerated view already created for system view <0x%08x>\n", systemView);
	LAutoreleasePool::removeTopPool();
	return !exists;
}

bool ScreenSaverController::destroyAcceleratedView(void *systemView)
{
	intptr_t wnd = 0;
	if (!_interfaces.getValueForKey(&wnd, (intptr_t)systemView)) {
		LLog("Accelerated view was not created for system view <0x%08x>\n", systemView);
		return false;
	}
	LAutoreleasePool::addNewPoolToTop();
	delete (ScreenSaverInterface *)wnd;
	_interfaces.removeValueForKey((intptr_t)systemView);
	LAutoreleasePool::removeTopPool();
	return true;
}

LString *ScreenSaverController::_screenSaverLibBundleID()
{
	return [[_screenSaverLibBundle() bundleIdentifier] LString];
}

LString *ScreenSaverController::_resourcePath()
{
	return [[[_applicationBundle() resourcePath] stringByAppendingPathComponent:@"BASE"] LString];
}

// Gives each library its framework bundle (for its resources).
void ScreenSaverController::_validateFrameworks()
{
	NSString *frameworks = [_applicationBundle() privateFrameworksPath];
	static const struct {
		NSString *name;
		void (*setModule)(CFBundleRef);
	} kFrameworks[] = {
		{ @"LFoundation", setLFoundationModule }, { @"FishHook", setFishHookModule },
		{ @"SQLiteLib", setSQLiteLibModule },     { @"AppModel", setAppModelModule },
		{ @"RenderLib", setRenderLibModule },
	};
	for (size_t i = 0; i < sizeof(kFrameworks) / sizeof(kFrameworks[0]); i++) {
		NSString *path = [[frameworks stringByAppendingPathComponent:kFrameworks[i].name]
		    stringByAppendingPathExtension:@"framework"];
		CFBundleRef bundle = CFBundleCreate(kCFAllocatorDefault, (CFURLRef)[NSURL fileURLWithPath:path]);
		if (bundle) {
			kFrameworks[i].setModule(bundle);
			CFRelease(bundle);
		}
	}
}

int ScreenSaverController::_applicationBuildNumber()
{
	return [[_applicationBundle() objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey] intValue];
}

// ~/Library/Caches/<the application's identifier>.
LString *ScreenSaverController::_applicationCachesPath(unsigned char)
{
	if (_cachesPath)
		return _cachesPath;
	LString *caches = LFileManager::pathToFolder('cach', -32763 /* kUserDomain */, true);
	if (caches == 0)
		return 0;
	LFileManager *files = LFileManager::manager();
	if (!files->isReadableFileAtPath(caches))
		return 0;
	LString *path = caches->stringByAppendingPathComponent([[_applicationBundle() bundleIdentifier] LString]);
	bool ok;
	if (!files->objectExistsAtPath(path, 0))
		ok = files->createFolderAtPath(path);
	else {
		if (!files->isWritableFileAtPath(path))
			return 0;
		ok = files->isReadableFileAtPath(path);
	}
	if (!ok)
		return 0;
	_cachesPath = LRetain(path);
	return _cachesPath;
}

// Creates `path` open to everyone, or checks it can be used.
static bool _prepareFolder(LFileManager *files, LString *path)
{
	if (!files->objectExistsAtPath(path, 0)) {
		if (!files->createFolderAtPath(path))
			return false;
		files->changeFileAttributesAtPath(
		    path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777), LFileManager::fileAttrPosixPermissions, (LObject *)0));
		return true;
	}
	return files->isWritableFileAtPath(path) && files->isReadableFileAtPath(path);
}

// <Application Support>/<SMKSupportFolderName>/<AtlasApplicationSupportFolderName>.
LString *ScreenSaverController::_applicationDataPath(unsigned char shared)
{
	LString **cached = shared ? &_sharedDataPath : &_userDataPath;
	if (*cached)
		return *cached;
	short domain = shared ? -32765 /* kLocalDomain */ : -32763 /* kUserDomain */;
	LString *support = LFileManager::pathToFolder('asup', domain, true);
	if (support == 0)
		return 0;
	LFileManager *files = LFileManager::manager();
	LString *smk = LString::stringFromTable("SMKSupportFolderName", "PathComponents", getAppModelModule());
	// As in the application (the 64-bit port): without a shared folder made
	// already, the shared data is in the user's domain.
	if (shared && !files->objectExistsAtPath(support->stringByAppendingPathComponent(smk), 0) &&
	    !files->isWritableFileAtPath(support)) {
		support = LFileManager::pathToFolder('asup', -32763 /* kUserDomain */, true);
		if (support == 0)
			return 0;
	}
	if (!files->isReadableFileAtPath(support))
		return 0;
	LString *path = support->stringByAppendingPathComponent(smk);
	if (!_prepareFolder(files, path))
		return 0;
	path->appendPathComponent(
	    LString::stringFromTable("AtlasApplicationSupportFolderName", "PathComponents", getAppModelModule()));
	if (!_prepareFolder(files, path))
		return 0;
	*cached = LRetain(path);
	return path;
}
