// AppController: the libraries' callbacks into the application.

#import "WeatherGlobePrefix.h"
#include "WGAppClasses.h"
#import "AtlasController.h"
#import "SubscriptionController.h"
#import "SelectCityWindow.h"
#import "WGSerialNumberController.h"
#import "WGDBUpdateController.h"
#import "WBAlert.h"
#include "NSString_LStringExtension.h"
#include <Security/Authorization.h>

static LConstantString _sRootFolder("Contents/Resources/BASE");

LF_IMPLEMENT_CLASS(AppController, 0)
LF_SELECTOR(AppController, _dbUpdatesAreAvailable);

void AppController::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	{
		StNSAutoreleasePool pool;
		finalizeRenderLib();
		finalizeAppModel();
		finalizeFishHook();
	}
	IModelCallbacks::setCallbacks(0);
	IAlertCallback::setAlertCallback(0);
	ILImageCallback::setLImageCallback(0);
	if (_cachesPath) {
		_cachesPath->release();
		_cachesPath = 0;
	}
	if (_sharedDataPath) {
		_sharedDataPath->release();
		_sharedDataPath = 0;
	}
	if (_userDataPath) {
		_userDataPath->release();
		_userDataPath = 0;
	}
	if (_fileManager) {
		delete _fileManager;
		_fileManager = 0;
	}
	LObject::dealloc();
}

// Registers the defaults and the callbacks, then starts AppModel; the
// database cleans up at startup in the application.
void AppController::initAsSingleton(void *)
{
	Preferences::registerDefaults(0);
	AtlasDatabase::sDoCleanupAtStartup = true;
	_fileManager = new CZipFileManager(&_sRootFolder);
	ILImageCallback::setLImageCallback(this);
	IAlertCallback::setAlertCallback(this);
	IModelCallbacks::setCallbacks(this);
	initializeAppModel([[[NSBundle mainBundle] bundleIdentifier] LString]);
	LNotificationCenter::center()->addObserver(this, "_dbUpdatesAreAvailable",
	                                           DownloadManager::kNewUpdatesAreAvailableNotification, 0);
}

void AppController::resetFileManager()
{
	if (_fileManager) {
		delete _fileManager;
		_fileManager = 0;
	}
	_fileManager = new CZipFileManager(&_sRootFolder);
}

// With descriptions of the updates the update window offers them; without,
// a plain alert asks whether to download now.
intptr_t AppController::_dbUpdatesAreAvailable(char *args)
{
	LNotification *notification = LF_ARG(args, 0, LNotification *);
	if (!Preferences::usesSoftwareUpdating())
		return 0;
	LMap *descriptions = notification ? notification->userInfo() : 0;
	if (!descriptions) {
		NSBundle *bundle = [NSBundle mainBundle];
		NSString *later = [bundle localizedStringForKey:@"DownloadLater" value:@"" table:nil];
		NSString *now = [bundle localizedStringForKey:@"DownloadNow" value:@"" table:nil];
		NSString *message = [bundle localizedStringForKey:@"New database updates are available" value:@"" table:nil];
		NSString *title = [bundle localizedStringForKey:@"New Database Updates" value:@"" table:nil];
		if (NSRunAlertPanel(title, @"%@", now, later, nil, message) == NSAlertDefaultReturn) {
			AppController *controller = AppController::instance();
			DownloadManager::instance()->updateWithDelegate(controller, 0);
		}
	} else {
		[[WGDBUpdateController sharedInstance] updateWithDescriptions:descriptions];
	}
	return 0;
}

// A Folder Manager folder of the local (shared) or the user's domain; the
// application quits when there is none.
LString *AppController::_pathToFolderWithType(int folderType, unsigned char shared)
{
	LString *path = LFileManager::pathToFolder(folderType, shared ? kLocalDomain : kUserDomain, true);
	if (!path) {
		NSBundle *bundle = [NSBundle mainBundle];
		NSString *quit = [bundle localizedStringForKey:@"QuitButt" value:@"" table:nil];
		NSString *message = [bundle localizedStringForKey:@"SystemFoldersErrorMessage" value:@"" table:nil];
		NSString *title = [bundle localizedStringForKey:@"SystemFoldersErrorTitle" value:@"" table:nil];
		NSRunAlertPanel(title, @"%@", quit, nil, nil, message);
		[NSApp terminate:nil];
	}
	return path;
}

void AppController::updateWillStartWithFilesCount(int count)
{
	[[WGDBUpdateController sharedInstance] updateWillStartWithFilesCount:count];
}

void AppController::updateWasFinished(unsigned char success)
{
	[[WGDBUpdateController sharedInstance] updateWasFinished:success];
}

int AppController::errorWithCodeCatched(int status, LData *data)
{
	[[WGDBUpdateController sharedInstance] errorWithCodeCatched:status data:data];
	return 0;
}

int AppController::_applicationBuildNumber()
{
	CFTypeRef version = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), kCFBundleVersionKey);
	return version ? CFStringGetIntValue((CFStringRef)version) : 0;
}

void AppController::_applicationTerminate(int)
{
	[NSApp terminate:nil];
}

void AppController::_applicationRestart()
{
	[NSApp terminate:nil];
}

bool AppController::_showSubscriptionDialog()
{
	return [SubscriptionController runModal];
}

LMap *AppController::_registrationInfo()
{
	return [SubscriptionController registrationInfo];
}

void AppController::_showSelectCityDialog()
{
	[SelectCityDialogController runModalDialog];
}

bool AppController::_showSerialNumberDialog()
{
	return [WGSerialNumberController runModal];
}

LString *AppController::_serialNumber()
{
	return [WGSerialNumberController serialNumber];
}

LString *AppController::_hiResMapsRootFolderSourcePath()
{
	return [[AtlasController atlasController] hiResMapsRootFolderSourcePath];
}

static void _releaseDataCallback(void *info, const void *, size_t)
{
	[(id)info release];
}

// Any image NSImage reads, as a CGImage made from its PNG form.
CGImageRef AppController::_CGImageFromImageData(LData *data)
{
	if (!data)
		return 0;
	NSImage *image = [[[NSImage alloc] initWithData:(NSData *)data->cfData()] autorelease];
	NSArray *reps = [image representations];
	NSImageRep *rep = nil;
	if ([reps count] != 0)
		rep = [reps objectAtIndex:0];
	if (![rep isKindOfClass:[NSBitmapImageRep class]])
		rep = [NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]];
	if (!rep)
		return 0;
	NSData *png = [(NSBitmapImageRep *)rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
	if (!png)
		return 0;
	CGDataProviderRef provider = CGDataProviderCreateWithData([png retain], [png bytes], [png length], _releaseDataCallback);
	if (!provider)
		return 0;
	CGImageRef result = CGImageCreateWithPNGDataProvider(provider, 0, false, kCGRenderingIntentDefault);
	CGDataProviderRelease(provider);
	return result;
}

bool AppController::fileWillBeDownloadedWithName(LString *name, LConnection *connection)
{
	return [[WGDBUpdateController sharedInstance] fileWillBeDownloadedWithName:(NSString *)name->cfString()
	                                                               connection:connection];
}

int AppController::fileWasDownloadedWithName(LString *name, int status)
{
	[[WGDBUpdateController sharedInstance] fileWasDownloadedWithName:(NSString *)name->cfString() error:status];
	return 0;
}

void AppController::_openURL(LString *url)
{
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:(NSString *)url->cfString()]];
}

// Runs each task (an LArray: the tool's path, then its arguments) as root,
// after asking for an administrator's password once.
bool AppController::_runAuthorizedTasks(LArray *tasks)
{
	AuthorizationRef authorization = 0;
	if (AuthorizationCreate(0, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &authorization) != errAuthorizationSuccess)
		return false;
	AuthorizationItem right = { "system.privilege.admin", 0, 0, 0 };
	AuthorizationRights rights = { 1, &right };
	if (AuthorizationCopyRights(authorization, &rights, 0,
	                            kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights |
	                                kAuthorizationFlagPreAuthorize,
	                            0) != errAuthorizationSuccess)
		return false;
	int count = tasks ? (int)tasks->count() : 0;
	for (int i = 0; i < count; i++) {
		LArray *task = lobj_cast<LArray>(tasks->objectAtIndex(i));
		LString *tool = 0;
		int argc = 0;
		if (task) {
			tool = lobj_cast<LString>(task->objectAtIndex(0));
			argc = (int)task->count();
			if (argc > 99)
				return false;
		}
		char *args[101];
		int a;
		for (a = 1; a < argc; a++)
			args[a] = (char *)[(NSString *)lobj_cast<LString>(task->objectAtIndex(a))->cfString() fileSystemRepresentation];
		args[a] = 0;
		if (AuthorizationExecuteWithPrivileges(authorization, [(NSString *)tool->cfString() fileSystemRepresentation],
		                                       kAuthorizationFlagDefaults, args + 1, 0) != errAuthorizationSuccess)
			return false;
		[NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceReferenceDate:[NSDate timeIntervalSinceReferenceDate] + 0.4]];
	}
	return true;
}

// <user caches>/<bundle id>, made if needed.
LString *AppController::_applicationCachesPath(unsigned char)
{
	if (_cachesPath)
		return _cachesPath;
	LString *caches = _pathToFolderWithType(kCachedDataFolderType, 0);
	if (!caches)
		return 0;
	LFileManager *manager = LFileManager::manager();
	if (!manager->isReadableFileAtPath(caches))
		return 0;
	LString *path = caches->stringByAppendingPathComponent([[[NSBundle mainBundle] bundleIdentifier] LString]);
	bool ok;
	if (!manager->objectExistsAtPath(path, 0)) {
		ok = manager->createFolderAtPath(path);
	} else {
		if (!manager->isWritableFileAtPath(path))
			return 0;
		ok = manager->isReadableFileAtPath(path);
	}
	if (!ok)
		return 0;
	_cachesPath = path;
	if (path)
		path->retain();
	return path;
}

void AppController::_fillParamsFromTheCode(SAlertParams *params, int code)
{
	const char *message;
	switch (code) {
	case 0x1010000:
	case 0x1020002:
		params->title = LString::stringFromTable("No disk space title", "Localizable");
		message = "No disk space message";
		goto okAlert;
	case 0x1020000:
	case 0x1020001:
		if (code == 0x1020000) {
			params->title = LString::stringFromTable("AuthFailedOnCloudsRequestTitle", "Localizable");
			message = "AuthFailedOnCloudsRequestMessage";
		} else {
			params->title = LString::stringFromTable("CloudsDownloadFailedTitle", "Localizable");
			message = "CloudsDownloadFailedMessage";
		}
		params->message = LString::stringFromTable(message, "Localizable");
		params->defaultResult = 0;
		params->defaultButton = LString::stringFromTable("Ok", "Localizable");
		params->helpAnchor = LString::stringFromTable("DontShowAlertAgain", "Localizable");
		params->suppressionKey = LString::stringWithCString("ShowCloudsDownloadFailedAlert");
		return;
	case 0x1000000:
		params->title = LString::stringFromTable("IDS_NoInetForSubscrTitle", "Localizable");
		message = "IDS_NoInetForSubscrMessage";
		break;
	case 0x1000001:
		params->title = LString::stringFromTable("NoINetForActivationTitle", "Localizable");
		message = "NoINetForActivationMessage";
		goto okAlert;
	case 0x1030000:
		params->title = LString::stringFromTable("InstallHiResMapsNotEnoughSpaceTitle", "Localizable");
		message = "InstallHiResMapsNotEnoughSpaceMessage";
		break;
	case 0x1030001:
		params->title = LString::stringFromTable("InstallHiResMapsErrorTitle", "Localizable");
		message = "InstallHiResMapsErrorMessage";
		goto okAlert;
	case 0x1030002:
		params->title = LString::stringFromTable("InstallHiResMapsErrorTitle", "Localizable");
		message = "InstallHiResMapsReadErrorMessage";
		break;
	case 0x1030003:
		params->title = LString::stringFromTable("InstallHiResMapsErrorTitle", "Localizable");
		message = "InstallHiResMapsWriteErrorMessage";
		break;
	case 0x1040000:
		params->title = LString::stringFromTable("CannotCommitRoteNodeTitle", "Localizable");
		message = "CannotCommitRoteNodeMessage";
		goto okAlert;
	case 0x1050000:	// Custom3DView::kErrorCannotPerformRendering
		NSLog(@"No alert params for Custom3DView::kErrorCannotPerformRendering");
		return;
	default:
		NSLog(@"Unknown code for alert params");
		return;
	}
	// a retry alert
	params->message = LString::stringFromTable(message, "Localizable");
	params->defaultButton = LString::stringFromTable("RetryButt", "Localizable");
	params->alternateButton = LString::stringFromTable("Cancel", "Localizable");
	params->defaultResult = 0;
	return;
okAlert:
	params->message = LString::stringFromTable(message, "Localizable");
	params->defaultButton = LString::stringFromTable("Ok", "Localizable");
	params->defaultResult = 0;
}

// The support folder must be readable and writable; otherwise the user may
// repair its permissions (chmod, chflags nouchg as root) or quit.
bool AppController::_checkSupportFolderPermissions(LString *path, unsigned char shared)
{
	LFileManager *manager = LFileManager::manager();
	if (manager->isWritableFileAtPath(path) && manager->isReadableFileAtPath(path))
		return true;
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *quit = [bundle localizedStringForKey:@"QuitButt" value:@"" table:nil];
	NSString *repair = [bundle localizedStringForKey:@"RepairButt" value:@"" table:nil];
	NSString *body = [bundle localizedStringForKey:@"BadAtlasSuppPermissionsBody" value:@"" table:nil];
	NSString *title = [bundle localizedStringForKey:@"BadAtlasSuppPermissionsTitle" value:@"" table:nil];
	if (NSRunAlertPanel(title, @"%@", repair, quit, nil, body) == NSAlertDefaultReturn) {
		LArray *chflags = LArray::arrayWithObjects(LString::stringWithCString("/usr/bin/chflags"),
		                                           LString::stringWithCString("nouchg"), path, (LObject *)0);
		LArray *chmod = LArray::arrayWithObjects(LString::stringWithCString("/bin/chmod"),
		                                         LString::stringWithCString(shared ? "777" : "755"), path, (LObject *)0);
		if (_runAuthorizedTasks(LArray::arrayWithObjects(chmod, chflags, (LObject *)0)))
			return true;
	}
	[NSApp terminate:nil];
	return false;
}

// A WBAlert from the params; a suppression key makes it a "don't show
// again" alert that isn't shown once checked. Returns 0 for the default
// button, 1 for another, 2 when cancelled, kAlertNotShown without texts.
int AppController::_showAlert(const SAlertParams *params)
{
	if (params->suppressionKey && LUserDefaults::standardUserDefaults()->objectForKey(params->suppressionKey))
		return kAlertNotShown;
	NSString *title = params->title ? (NSString *)params->title->cfString() : nil;
	NSString *message = params->message ? (NSString *)params->message->cfString() : nil;
	if (!title || !message)
		return kAlertNotShown;
	NSString *defaultButton = params->defaultButton ? (NSString *)params->defaultButton->cfString() : nil;
	NSString *alternateButton = params->alternateButton ? (NSString *)params->alternateButton->cfString() : nil;
	NSString *otherButton = params->otherButton ? (NSString *)params->otherButton->cfString() : nil;
	NSString *checkbox = params->helpAnchor ? (NSString *)params->helpAnchor->cfString() : nil;
	int defaultIndex = params->defaultResult == 1 ? 2 : params->defaultResult == 2 ? 3 : params->defaultResult == 0;
	int cancelIndex = 0;
	if (alternateButton || otherButton)
		cancelIndex = (defaultIndex == 1) + 1;
	WBAlert *alert = [[[WBAlert alloc] initWithTitle:title body:message rightButt:defaultButton
	                                     middleButt:alternateButton leftButt:otherButton checkboxTitle:checkbox
	                                    hasHelpButt:NO defButtIndex:defaultIndex cancelButtIndex:cancelIndex
	                                   hasCustomBgr:NO] autorelease];
	NSInteger result = [alert runModal];
	if (checkbox && params->suppressionKey) {
		if ([alert dontShowAgain])
			LUserDefaults::standardUserDefaults()->setObjectForKey(LValue::valueWithInt(1), params->suppressionKey);
		else
			LUserDefaults::standardUserDefaults()->removeObjectForKey(params->suppressionKey);
	}
	if (result == -1)
		return 2;
	return result != 1;
}

// <domain's Application Support>/<SMKSupportFolderName>/<AtlasApplication
// SupportFolderName>, made if needed (world-writable in the local domain).
// Unreadable, locked or unwritable folders get an alert that can retry,
// repair as root, or quit.
LString *AppController::_applicationDataPath(unsigned char shared)
{
	LString *path = shared ? _sharedDataPath : _userDataPath;
	if (path)
		return path;
	LString *support = _pathToFolderWithType(kApplicationSupportFolderType, shared);
	if (!support)
		return 0;
	LFileManager *manager = LFileManager::manager();
	NSBundle *bundle = [NSBundle mainBundle];
	// The 64-bit port: only root can write /Library/Application Support now,
	// so unless the shared folder is there already the shared data lives with
	// the user's (their files don't share names).
	LString *smk = LString::stringFromTable("SMKSupportFolderName", "PathComponents", getAppModelModule());
	const bool local = shared && (manager->objectExistsAtPath(support->stringByAppendingPathComponent(smk), 0) ||
	                              manager->isWritableFileAtPath(support));
	if (shared && !local) {
		support = _pathToFolderWithType(kApplicationSupportFolderType, 0);
		if (!support)
			return 0;
	}
	while (!manager->isReadableFileAtPath(support)) {
		NSString *quit = [bundle localizedStringForKey:@"QuitButt" value:@"" table:nil];
		NSString *retry = [bundle localizedStringForKey:@"RetryButt" value:@"" table:nil];
		NSString *body = [NSString stringWithFormat:[bundle localizedStringForKey:@"BadAppSuppReadPermissionsBody" value:@"" table:nil],
		                                            (NSString *)support->cfString()];
		NSString *title = [bundle localizedStringForKey:@"BadAppSuppReadPermissionsTitle" value:@"" table:nil];
		if (NSRunAlertPanel(title, @"%@", retry, quit, nil, body) == NSAlertAlternateReturn) {
			[NSApp terminate:nil];
			return 0;
		}
	}
	path = support->stringByAppendingPathComponent(smk);
	if (manager->objectExistsAtPath(path, 0)) {
		if (!_checkSupportFolderPermissions(path, local))
			return 0;
	} else {
		for (;;) {
			if (!manager->isLockedFileAtPath(support)) {
				if (manager->isWritableFileAtPath(support)) {
					if (!manager->createFolderAtPath(path))
						return 0;
					if (local)
						manager->changeFileAttributesAtPath(path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777),
						                                                                       LFileManager::fileAttrPosixPermissions, (LObject *)0));
					break;
				}
				NSString *quit = [bundle localizedStringForKey:@"QuitButt" value:@"" table:nil];
				NSString *repair = [bundle localizedStringForKey:@"RepairButt" value:@"" table:nil];
				NSString *body = [bundle localizedStringForKey:@"BadAppSuppPermissionsBody" value:@"" table:nil];
				NSString *title = [bundle localizedStringForKey:@"BadAppSuppPermissionsTitle" value:@"" table:nil];
				if (NSRunAlertPanel(title, @"%@", repair, quit, nil, body) == NSAlertDefaultReturn) {
					LArray *chmod = 0;
					if (local)
						chmod = LArray::arrayWithObjects(LString::stringWithCString("/bin/chmod"),
						                                 LString::stringWithCString("777"), path, (LObject *)0);
					LArray *mkdir = LArray::arrayWithObjects(LString::stringWithCString("/bin/mkdir"), path, (LObject *)0);
					if (_runAuthorizedTasks(LArray::arrayWithObjects(mkdir, chmod, (LObject *)0)))
						break;
				}
				[NSApp terminate:nil];
				return 0;
			}
			NSString *quit = [bundle localizedStringForKey:@"QuitButt" value:@"" table:nil];
			NSString *retry = [bundle localizedStringForKey:@"RetryButt" value:@"" table:nil];
			NSString *body = [NSString stringWithFormat:[bundle localizedStringForKey:@"AppSuppIsLockedMessage" value:@"" table:nil],
			                                            (NSString *)support->cfString()];
			NSString *title = [bundle localizedStringForKey:@"AppSuppIsLockedTitle" value:@"" table:nil];
			if (NSRunAlertPanel(title, @"%@", retry, quit, nil, body) == NSAlertAlternateReturn) {
				[NSApp terminate:nil];
				return 0;
			}
		}
	}
	path->appendPathComponent(LString::stringFromTable("AtlasApplicationSupportFolderName", "PathComponents", getAppModelModule()));
	if (!manager->objectExistsAtPath(path, 0)) {
		if (!manager->createFolderAtPath(path))
			return 0;
		if (local)
			manager->changeFileAttributesAtPath(path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777),
			                                                                       LFileManager::fileAttrPosixPermissions, (LObject *)0));
	} else if (!_checkSupportFolderPermissions(path, local)) {
		return 0;
	}
	if (shared)
		_sharedDataPath = path;
	else
		_userDataPath = path;
	path->retain();
	return path;
}

// An RGBA bitmap of the image (premultiplied from its BGRA pixels) as PNG,
// JPEG or TIFF.
LData *AppController::_representationOfTypeForBitmapImage(TBitmapRepresentationType type, LBitmapImage *image)
{
	if (!image)
		return 0;
	LData *result;
	{
		LBitmapImage::StLocker locker(image, LBitmapImage::StLocker::kRead, (TPixelFormat)0);
		NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:0 pixelsWide:image->width()
		                                                               pixelsHigh:image->height() bitsPerSample:8
		                                                          samplesPerPixel:4 hasAlpha:YES isPlanar:NO
		                                                           colorSpaceName:NSDeviceRGBColorSpace
		                                                              bytesPerRow:locker.bytesPerRow()
		                                                             bitsPerPixel:locker.bitsPerPixel()];
		const unsigned char *src = (const unsigned char *)locker.bitmapData();
		unsigned char *dst = [rep bitmapData];
		int pixels = (int)image->width() * (int)image->height();
		for (int i = 0; i < pixels; i++) {
			const unsigned char *s = src + i * 4;
			unsigned char *d = dst + i * 4;
			float alpha = (float)s[3] / 255.0f;
			d[0] = (unsigned char)(int)((float)s[2] * alpha);
			d[1] = (unsigned char)(int)((float)s[1] * alpha);
			d[2] = (unsigned char)(int)((float)s[0] * alpha);
			d[3] = s[3];
		}
		NSBitmapImageFileType fileType = type == kPNGRepresentation ? NSBitmapImageFileTypePNG
		                                 : type == kJPEGRepresentation ? NSBitmapImageFileTypeJPEG
		                                                               : NSBitmapImageFileTypeTIFF;
		NSData *data = [rep representationUsingType:fileType properties:@{}];
		result = LData::_alloc()->initWithCFDataNoCopy((CFDataRef)data);
		[rep release];
	}
	if (!result)
		return 0;
	return (LData *)result->l_autorelease();
}
