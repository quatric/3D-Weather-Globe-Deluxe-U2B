// AtlasController
#import "AtlasController.h"
#import "MainWindowController.h"
#import "CIPaletteController.h"
#import "WGRouteManagerController.h"
#import "PreferencesController.h"
#import "AboutController.h"
#import "WGTimeLineController.h"
#import "WGMovieController.h"
#import "IPhotoImportController.h"
#import "WGSettingsPaletteController.h"
#import "WGNewLocationController.h"
#import "SnapshotEditorController.h"
#import "WGSerialNumberController.h"
#import "WBLicenseController.h"
#import "WBAlert.h"
#import "WGHelpBook.h"
#import "WGRouteImportExport.h"
#import "WGCurrentSetController.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

static NSWindow *_sSplashWindow = nil;
// The controller, or kTerminatedController once the application is quitting.
static AtlasController *_sGlobalAtlasController = nil;
#define kTerminatedController ((AtlasController *)-1)

// Megabytes the caches folder needs to start.
enum { kMinFreeSpaceMB = 100 };

@implementation AtlasController

#pragma mark Startup

+ (void)showSplashScreen
{
	if ([_sSplashWindow isVisible])
		return;
	if (!_sSplashWindow) {
		NSImage *image = [NSImage imageNamed:@"3DWGsplash"];
		NSRect frame = { NSZeroPoint, [image size] };
		_sSplashWindow = [[NSWindow alloc] initWithContentRect:frame
		                                             styleMask:NSWindowStyleMaskBorderless
		                                               backing:NSBackingStoreBuffered
		                                                 defer:NO];
		[_sSplashWindow setBackgroundColor:[NSColor clearColor]];
		[_sSplashWindow setOpaque:NO];
		[_sSplashWindow setHasShadow:YES];
		[_sSplashWindow setReleasedWhenClosed:YES];
		NSImageView *imageView = [[NSImageView alloc] initWithFrame:frame];
		[imageView setImage:image];
		[[_sSplashWindow contentView] addSubview:imageView];
		[imageView release];
		[[NSNotificationCenter defaultCenter] addObserver:self
		                                         selector:@selector(hideSplashScreen)
		                                             name:NSWindowDidBecomeKeyNotification
		                                           object:nil];
		[[NSNotificationCenter defaultCenter] addObserver:self
		                                         selector:@selector(hideSplashScreen)
		                                             name:NSWindowDidBecomeMainNotification
		                                           object:nil];
	}
	[_sSplashWindow center];
	[_sSplashWindow orderFront:nil];
}

+ (void)hideSplashScreen
{
	if (!_sSplashWindow)
		return;
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidBecomeKeyNotification object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidBecomeMainNotification object:nil];
	[_sSplashWindow close];
	_sSplashWindow = nil;
}

+ (void)_appWillTerminate:(id)aTerminate
{
	if (!_sGlobalAtlasController || _sGlobalAtlasController == kTerminatedController)
		return;
	[NSApp setDelegate:nil];
	[NSApp setServicesProvider:nil];
	AtlasController *controller = _sGlobalAtlasController;
	_sGlobalAtlasController = kTerminatedController;
	// The controller can outlive this (autoreleased) until exit, after the
	// libraries are gone: stop the dock weather now.
	[controller _finilizeDockWeather];
	[controller release];
	[self hideSplashScreen];
}

+ (id)generalScreen
{
	NSArray *screens = [NSScreen screens];
	if (![screens count])
		return [NSScreen mainScreen];
	return [screens objectAtIndex:0];
}

+ (void)_assertUnsupportedResolution
{
	NSScreen *screen = [self generalScreen];
	if (screen) {
		NSSize size = [screen frame].size;
		if (size.width >= 1024.0 && size.height >= 600.0)
			return;
	}
	NSRunAlertPanel(NSLocalizedString(@"UnsupportedResolutionTitle", @""), @"%@",
	                NSLocalizedString(@"QuitButt", @""), nil, nil,
	                NSLocalizedString(@"UnsupportedResolutionMessage", @""));
	[NSApp terminate:self];
}

// Thousands of colors get a warning; fewer quit.
+ (void)_assertUnsupportedColors
{
	NSInteger bitsPerPixel = 0;
	NSScreen *screen = [self generalScreen];
	if (screen)
		bitsPerPixel = NSBitsPerPixelFromDepth([screen depth]);
	BOOL usable = bitsPerPixel > 11;
	if (usable && (!Preferences::doShowUnsupportedColorAlert() || bitsPerPixel > 23))
		return;
	NSString *button = NSLocalizedString(@"Ok", @"");
	NSString *checkbox = NSLocalizedString(@"DontShowAlertAgain", @"");
	NSString *title = NSLocalizedString(@"UnsupportedColorsTitle", @"");
	NSString *message = NSLocalizedString(@"LowColorsMessage", @"");
	if (!usable) {
		checkbox = nil;
		button = NSLocalizedString(@"QuitButt", @"");
		message = NSLocalizedString(@"UnsupportedColorsMessage", @"");
	}
	WBAlert *alert = [[[WBAlert alloc] initWithTitle:title
	                                            body:message
	                                       rightButt:button
	                                      middleButt:nil
	                                        leftButt:nil
	                                   checkboxTitle:checkbox
	                                     hasHelpButt:NO
	                                    defButtIndex:1
	                                 cancelButtIndex:2
	                                    hasCustomBgr:NO] autorelease];
	[alert runModal];
	if (!usable)
		[NSApp terminate:self];
	Preferences::setShowUnsupportedColorAlert(![alert dontShowAgain]);
}

+ (void)_checkFreeDiskSpace
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	if (!callbacks)
		return;
	LString *path = callbacks->_applicationDataPath(1);
	if (!path)
		return;
	unsigned long long freeBytes = 0;
	LFileSystemFileManager::getDiskSpaceInfo(path, &freeBytes, 0, 0);
	if ((freeBytes >> 20) >= kMinFreeSpaceMB)
		return;
	NSString *message = [NSString stringWithFormat:NSLocalizedString(@"NoSpaceOnStartMessage", @""), kMinFreeSpaceMB];
	NSRunAlertPanel(NSLocalizedString(@"NoSpaceOnStartTitle", @""), @"%@", NSLocalizedString(@"QuitButt", @""), nil, nil,
	                message);
	[NSApp terminate:self];
}

+ (id)atlasController
{
	if (!_sGlobalAtlasController) {
		[WGHelpBook registerHelpBook];
		AppController::instance();
		IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
		id terminator = nil;
		if (callbacks && callbacks->_applicationDataPath(1) && callbacks->_applicationDataPath(0)) {
			if (callbacks->_applicationCachesPath(1) && callbacks->_applicationCachesPath(0)) {
				[self _checkFreeDiskSpace];
				[self _assertUnsupportedResolution];
				[self _assertUnsupportedColors];
				[[NSNotificationCenter defaultCenter] addObserver:self
				                                         selector:@selector(_appWillTerminate:)
				                                             name:NSApplicationWillTerminateNotification
				                                           object:nil];
				_sGlobalAtlasController = [[AtlasController alloc] init];
				[NSApp setServicesProvider:_sGlobalAtlasController];
				[NSApp setDelegate:(id<NSApplicationDelegate>)_sGlobalAtlasController];
				WBLicenseController *license = [WBLicenseController sharedLicenseController];
				if (![license isAgreed])
					[license runLicense];
				[self showSplashScreen];
				if (Preferences::homeCity()) {
					[self showSplashScreen];
					if (Preferences::usesSoftwareUpdating())
						DownloadManager::instance()->refreshDatabase();
					[NSBundle loadNibNamed:@"CoreInterface" owner:_sGlobalAtlasController];
					[self hideSplashScreen];
					return _sGlobalAtlasController;
				}
				terminator = self;
			} else
				NSLog(@"Cannot access caches folder");
		}
		[NSApp terminate:terminator];
		return nil;
	}
	if (_sGlobalAtlasController == kTerminatedController)
		return nil;
	return _sGlobalAtlasController;
}

- (id)init
{
	self = [super init];
	if (self) {
		_mainWindowController = [MainWindowController new];
		@try {
			_citiesPaletteController = [CIPaletteController new];
			_routeManagerPaletteController = [WGRouteManagerController new];
			_preferencesController = [PreferencesController new];
			_aboutController = [AboutController new];
			_timeLineController = [WGTimeLineController new];
			_movieController = [WGMovieController new];
			_iPhotoImportController = [IPhotoImportController new];
			_newLocationController = [WGNewLocationController new];
			_settingsPaletteController = [WGSettingsPaletteController new];
			_snapshotEditorController = [SnapshotEditorController new];
		} @catch (NSException *exception) {
		}
	}
	return self;
}

- (void)awakeFromNib
{
	[self _initDockWeather];
	[_defaultStatusMenu setAutoenablesItems:NO];
	[self installStatusBarItems];
	[_copyingWindow setLevel:CGWindowLevelForKey(kCGFloatingWindowLevelKey)];
	if (Preferences::doShowMainWindowOnStartup())
		[self showMainWindow];
	if (Preferences::doShowWeatherOnStartup())
		[_citiesPaletteController showPalette];
	if ([_timeLineController doShowOnStartup])
		[_timeLineController showPalette];
	if ([_movieController doShowOnStartup])
		[_movieController showPalette];
	if ([_iPhotoImportController doShowOnStartup])
		[_iPhotoImportController showPalette];
	if ([_settingsPaletteController doShowOnStartup])
		[_settingsPaletteController showPalette];
	if ([_snapshotEditorController doShowOnStartup])
		[_snapshotEditorController showPalette];
	if ([_routeManagerPaletteController doShowOnStartup])
		[_routeManagerPaletteController showPalette];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[_citiesPaletteController invalidate];
	[_routeManagerPaletteController release];
	[_preferencesController release];
	[_aboutController release];
	[_timeLineController release];
	[_movieController cleanUp:_mainWindowController];
	[_movieController release];
	[_iPhotoImportController release];
	[_settingsPaletteController release];
	[_newLocationController release];
	[_snapshotEditorController release];
	[self hideStatusBarItems];
	[_mainWindowController release];
	[_defaultStatusMenu release];
	[_insertDiscDialog release];
	[_copyingWindow release];
	[self _finilizeDockWeather];
	[_dockMenu release];
	[super dealloc];
}

#pragma mark Services

- (void)_openSearchForTerm:(id)aTerm
{
	[NSApp activateIgnoringOtherApps:YES];
	[_mainWindowController openSearchForTerm:aTerm];
}

- (void)servicesLookup:(id)aLookup userData:(id)aData error:(id *)aError
{
	NSString *term = [aLookup stringForType:NSPasteboardTypeString];
	[self performSelector:@selector(_openSearchForTerm:) withObject:term afterDelay:0.001];
}

#pragma mark Menus

- (BOOL)validateMenuItem:(id)aItem
{
	SEL action = [aItem action];
	NSString *title;
	if (action == @selector(browseTheEarth:))
		return ![_mainWindowController isWindowVisible];
	if (action == @selector(toggleCitiesInfoPalette:))
		title = [_citiesPaletteController paletteIsVisible] ? @"Hide Cities Info Palette" : @"Show Cities Info Palette";
	else if (action == @selector(toggleRouteManagerPalette:))
		title = [_routeManagerPaletteController isPaletteVisible] ? @"Hide Route Palette" : @"Show Route Palette";
	else if (action == @selector(toggleIPhotoMediaPalette:) || action == @selector(toggleSnapshotEditorPalette:)) {
		WGPaletteBaseController *palette = action == @selector(toggleIPhotoMediaPalette:)
		    ? (WGPaletteBaseController *)_iPhotoImportController : (WGPaletteBaseController *)_snapshotEditorController;
		[aItem setState:[palette isPaletteVisible] ? NSControlStateValueOn : NSControlStateValueOff];
		return [NSApp modalWindow] == nil;
	} else if (action == @selector(showAboutPanel:))
		return [NSApp modalWindow] == nil;
	else if (action == @selector(showPreferences:))
		title = nil;
	else if (action == @selector(gotoGetMoreRoutes:)) {
		[aItem setState:NSControlStateValueOff];
		return YES;
	} else if (action == @selector(_dockEntryWasSelected:) || action == @selector(_selectHomeCityFromDock:))
		return ![_mainWindowController isInFullWindowMode];
	else if (action == @selector(installHiResMaps:))
		return !InstallationController::instance()->isHiResMapsValid();
	else
		return YES;
	if (title)
		[aItem setTitle:NSLocalizedString(title, @"")];
	return [NSApp modalWindow] == nil && ![_mainWindowController isInFullWindowMode];
}

- (id)_menuForStatusBar
{
	return _defaultStatusMenu;
}

- (void)installStatusBarItems
{
}

- (void)hideStatusBarItems
{
	if (!_barItems)
		return;
	[[NSStatusBar systemStatusBar] removeStatusItem:_barItems];
	[_barItems autorelease];
	_barItems = nil;
}

- (BOOL)statusBarItemsWereInstalled
{
	return _barItems != nil;
}

- (void)_invalidateDockMenu
{
	[_dockMenu release];
	_dockMenu = nil;
}

- (void)_processSerialNumber
{
	if ([WGSerialNumberController runModal])
		DownloadManager::instance()->postActivationTask([WGSerialNumberController serialNumber]);
	else
		[NSApp terminate:nil];
}

#pragma mark Actions

- (void)activateAtlas:(id)aAtlas
{
	[NSApp activateIgnoringOtherApps:YES];
}

- (void)browseTheEarth:(id)aEarth
{
	[NSApp activateIgnoringOtherApps:NO];
	[self showMainWindow];
}

- (void)changeOnlineStatus:(id)aStatus
{
}

- (void)hideStatusMenu:(id)aMenu
{
	[self hideStatusBarItems];
}

- (void)showCitiesInfoPalette:(id)aPalette
{
	[_citiesPaletteController showPalette];
}

- (void)hideCitiesInfoPalette:(id)aPalette
{
	[_citiesPaletteController hidePalette];
}

- (void)toggleCitiesInfoPalette:(id)aPalette
{
	[_citiesPaletteController togglePalette];
}

- (void)showRouteManagerPalette:(id)aPalette
{
	[_routeManagerPaletteController showPalette];
}

- (void)hideRouteManagerPalette:(id)aPalette
{
	[_routeManagerPaletteController hidePalette];
}

- (void)toggleRouteManagerPalette:(id)aPalette
{
	[_routeManagerPaletteController togglePalette];
}

- (void)toggleIPhotoMediaPalette:(id)aPalette
{
	[_iPhotoImportController togglePalette];
}

- (void)toggleSnapshotEditorPalette:(id)aPalette
{
	[_snapshotEditorController togglePalette];
}

- (void)showPreferences:(id)aPreferences
{
	[_preferencesController showPreferences];
}

- (void)showAboutPanel:(id)aPanel
{
	[_aboutController showPanel];
}

// The page with the version and the platform.
- (id)_fullURLForPageURLString:(id)aString
{
#if defined(__i386__) || defined(__x86_64__)
	NSString *platform = @"intel";
#else
	NSString *platform = @"arm";
#endif
	NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
	return [NSURL URLWithString:[NSString stringWithFormat:@"%@?ver=%@&mac=%@", aString, version, platform]];
}

- (void)provideFeedback:(id)aFeedback
{
	[[NSWorkspace sharedWorkspace] openURL:[self _fullURLForPageURLString:NSLocalizedString(@"Feedback Page", @"")]];
}

- (void)registerAtlas:(id)aAtlas
{
	[[NSWorkspace sharedWorkspace] openURL:[self _fullURLForPageURLString:NSLocalizedString(@"Registration Page", @"")]];
}

- (void)showSettingsHelp:(id)aHelp
{
	[WGHelpBook gotoPage:@"topics/settings.html" anchor:nil];
}

- (void)openUserGuide:(id)aGuide
{
	NSString *name = NSLocalizedString(@"UserGuideFileName", @"");
	NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:nil];
	if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
		[[NSWorkspace sharedWorkspace] openFile:path];
		return;
	}
	NSString *message = [NSString stringWithFormat:NSLocalizedString(@"UserGuideNotFoundMessage", @""), name];
	NSRunAlertPanel(NSLocalizedString(@"UserGuideNotFoundTitle", @""), @"%@", NSLocalizedString(@"OK", @""), nil, nil,
	                message);
}

- (void)gotoTechSupport:(id)aSupport
{
	[[NSWorkspace sharedWorkspace] openURL:[self _fullURLForPageURLString:NSLocalizedString(@"Tech Support Page", @"")]];
}

- (void)gotoGetMoreRoutes:(id)aRoutes
{
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:NSLocalizedString(@"Get More Routes Page", @"")]];
}

#pragma mark Controllers

- (void)showMainWindow
{
	[_mainWindowController showWindow];
}

- (void)hideMainWindow
{
	[_mainWindowController hideWindow];
}

- (id)mainWindowController
{
	return _mainWindowController;
}

- (id)preferencesController
{
	return _preferencesController;
}

- (id)citiesPaletteController
{
	return _citiesPaletteController;
}

- (id)timeLineController
{
	return _timeLineController;
}

- (id)routeManagerPaletteController
{
	return _routeManagerPaletteController;
}

- (id)movieController
{
	return _movieController;
}

- (id)iPhotoImportController
{
	return _iPhotoImportController;
}

- (id)settingsPaletteController
{
	return _settingsPaletteController;
}

- (id)newLocationController
{
	return _newLocationController;
}

#pragma mark Application delegate

// Route sets open into the current set.
- (BOOL)application:(id)aApplication openFile:(id)aFile
{
	if ([NSApp modalWindow])
		return NO;
	if (![[aFile pathExtension] isEqual:_kRouteSetFileExt])
		return NO;
	[self showMainWindow];
	RouteObject *route = [[WGRouteImportExport sharedRouteImportExport] loadRouteObjectAtPath:aFile];
	[[_mainWindowController currentSetController] replaceCurrentSet:route];
	return YES;
}

- (BOOL)application:(id)aApplication printFile:(id)aFile
{
	BOOL opened = [self application:aApplication openFile:aFile];
	if (opened)
		[_mainWindowController doPrint:nil];
	return opened;
}

#pragma mark Hi-res maps

- (void)insertDiscDialogAction:(id)aAction
{
	if ([aAction tag] == 1)
		[NSApp stopModalWithCode:NSModalResponseCancel];
}

// The ".Images" folder of a mounted disc with this name.
- (id)_hiResMapsRootFolderPathForMediaWithName:(id)aName
{
	for (NSString *media in [[NSWorkspace sharedWorkspace] mountedRemovableMedia]) {
		if (![[media lastPathComponent] isEqualTo:aName])
			continue;
		NSString *path = [media stringByAppendingPathComponent:@".Images"];
		BOOL isDirectory = NO;
		if ([[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory] && isDirectory)
			return path;
	}
	return nil;
}

- (id)_hiResMapsRootFolderSourcePath
{
	NSString *path = [self _hiResMapsRootFolderPathForMediaWithName:@"3D Weather Globe Installer DVD"];
	if (!path)
		path = [self _hiResMapsRootFolderPathForMediaWithName:@"3D Weather Globe 2 Installer DVD"];
	return path;
}

// Asks for the installer disc until it shows up or the user cancels.
- (LString *)hiResMapsRootFolderSourcePath
{
	NSString *path = [self _hiResMapsRootFolderSourcePath];
	if (!path) {
		[_insertDiscDialog center];
		NSModalSession session = [NSApp beginModalSessionForWindow:_insertDiscDialog];
		for (;;) {
			if ([NSApp runModalSession:session] != NSModalResponseContinue) {
				path = nil;
				break;
			}
			NSAutoreleasePool *pool = [NSAutoreleasePool new];
			path = [[self _hiResMapsRootFolderSourcePath] retain];
			[pool release];
			if (path) {
				[path autorelease];
				[NSApp stopModal];
				break;
			}
		}
		[NSApp endModalSession:session];
		[_insertDiscDialog orderOut:nil];
	}
	return [path LString];
}

- (void)_hiResMapsInstallationProgress:(id)aProgress
{
	LNotification *notification = lobj_cast<LNotification>([aProgress getLObject]);
	if (!notification || !notification->userInfo())
		return;
	LMap *info = notification->userInfo();
	LValue *index = lobj_cast<LValue>(info->objectForKey(InstallationController::kHiResMapsFileIndexKey));
	LValue *size = lobj_cast<LValue>(info->objectForKey(InstallationController::kHiResMapsFileSizeKey));
	LValue *copied = lobj_cast<LValue>(info->objectForKey(InstallationController::kHiResMapsFileCopiedSizeKey));
	_copyingCurrentFileIdx = index ? index->intValue() : 0;
	_copyingCurrentFileSize = size ? size->int64Value() : 0;
	_copyingCurrentFileCopiedSize = copied ? copied->int64Value() : 0;
	[self _updateCopyingProgress];
}

- (void)installHiResMaps:(id)aMaps
{
	NSInteger result = NSRunAlertPanel(NSLocalizedString(@"InstallHiResMapsTitle", @""), @"%@",
	                                   NSLocalizedString(@"Install", @""), NSLocalizedString(@"Cancel", @""), nil,
	                                   NSLocalizedString(@"InstallHiResMapsMessage", @""));
	if (result != NSAlertDefaultReturn)
		return;
	if (!InstallationController::instance()->installHiResMapsInBackground(1))
		return;
	LNotificationCenter::center()->addObserver(self, @selector(_hiResMapsInstallationProgress:),
	                                           InstallationController::kHiResMapsInstallationProgressNotification, 0);
	LNotificationCenter::center()->addObserver(self, @selector(_hiResMapsInstallationDidEnd:),
	                                           InstallationController::kHiResMapsInstallationDidEndNotification, 0);
	_copyingCurrentFileIdx = 0;
	_copyingCurrentFileSize = 0;
	_copyingCurrentFileCopiedSize = 0;
	[_copyingProgress setIndeterminate:NO];
	[self _updateCopyingProgress];
	[_copyingWindow center];
	[NSApp runModalForWindow:_copyingWindow];
}

- (void)_hiResMapsInstallationDidEnd:(id)aEnd
{
	LNotificationCenter::center()->removeObserver(self, InstallationController::kHiResMapsInstallationDidEndNotification,
	                                              (LObject *)0);
	LNotificationCenter::center()->removeObserver(
	    self, InstallationController::kHiResMapsInstallationProgressNotification, (LObject *)0);
	[_copyingStatus setStringValue:@""];
	[_copyingStatus display];
	[_copyingProgress setIndeterminate:YES];
	AppController::instance()->resetFileManager();
	[NSApp stopModal];
	[_copyingWindow orderOut:nil];
	if (InstallationController::instance()->isHiResMapsValid())
		NSRunAlertPanel(NSLocalizedString(@"InstallHiResMapsFinishedTitle", @""), @"%@", NSLocalizedString(@"Ok", @""),
		                nil, nil, NSLocalizedString(@"InstallHiResMapsFinishedMessage", @""));
}

- (void)_updateCopyingProgress
{
	InstallationController *installer = InstallationController::instance();
	int filesCount = installer->_filesToInstall ? installer->_filesToInstall->count() : 0;
	float fileProgress = 0.0f;
	if (_copyingCurrentFileSize)
		fileProgress = (float)_copyingCurrentFileCopiedSize / (float)_copyingCurrentFileSize;
	[_copyingProgress setDoubleValue:(int)(((_copyingCurrentFileIdx + fileProgress) * 100.0f) / filesCount)];
	[_copyingStatus setStringValue:[NSString stringWithFormat:NSLocalizedString(@"CopyingStatusTextFormat", @""),
	                                                          _copyingCurrentFileIdx + 1, filesCount,
	                                                          (int)(fileProgress * 100.0f)]];
}

#pragma mark Dock

// A rounded label with the text near the top of the Dock icon; no text restores the icon.
- (void)setDockIconOverlayText:(id)aText
{
	[NSApp setApplicationIconImage:nil];
	if (![aText length])
		return;
	NSSize textSize = [aText size];
	CGFloat radius = textSize.height * 0.5;
	CGFloat width = textSize.width + radius * 1.5;
	CGFloat bottom = 124.0 - textSize.height;
	CGFloat top = bottom + radius + radius;
	NSBezierPath *path = [NSBezierPath bezierPath];
	[path moveToPoint:NSMakePoint(radius, bottom)];
	[path lineToPoint:NSMakePoint(width - radius, bottom)];
	[path curveToPoint:NSMakePoint(width - radius, top)
	     controlPoint1:NSMakePoint(width, bottom + radius * 0.2)
	     controlPoint2:NSMakePoint(width, top - radius * 0.2)];
	[path lineToPoint:NSMakePoint(radius, top)];
	[path curveToPoint:NSMakePoint(radius, bottom)
	     controlPoint1:NSMakePoint(0.0, top - radius * 0.2)
	     controlPoint2:NSMakePoint(0.0, bottom + radius * 0.2)];
	[path closePath];

	// The original overlaid a CGImage with OverlayApplicationDockTileImage; draw over the icon instead.
	NSImage *icon = [[[NSImage alloc] initWithSize:NSMakeSize(128.0, 128.0)] autorelease];
	[icon lockFocus];
	[[NSImage imageNamed:NSImageNameApplicationIcon] drawInRect:NSMakeRect(0.0, 0.0, 128.0, 128.0)
	                                                    fromRect:NSZeroRect
	                                                   operation:NSCompositingOperationSourceOver
	                                                    fraction:1.0];
	[[NSColor colorWithDeviceRed:0.0 green:0.1 blue:0.2 alpha:0.85] set];
	[path fill];
	[path setLineWidth:1.5];
	[[NSColor whiteColor] set];
	[path stroke];
	[(NSAttributedString *)aText drawInRect:NSMakeRect(0.0, bottom, width, radius + radius)];
	[icon unlockFocus];
	[NSApp setApplicationIconImage:icon];
}

// The home city's weather and the Dock weather commands.
- (id)applicationDockMenu:(id)aMenu
{
	if (_dockMenu || !_dockEntry)
		return _dockMenu;
	_dockMenu = [[NSMenu alloc] initWithTitle:@""];
	LMap *report = _dockEntry->dictionaryForReport(kReportTypeCurrent);
	if (report && !UMapParsingUtils::loadIntFromObject(report->objectForKey(kReportCommonDataExpired))) {
		LString *temperature = Utils::temperatureDescription(report->objectForKey(kReportCurrentTemperature));
		LString *weather = Utils::weatherDescription(report);
		NSString *weatherString = weather ? [NSString stringReferenceFromLString:weather] : @"";
		NSString *temperatureString = temperature ? [NSString stringReferenceFromLString:temperature] : @"";
		NSString *city = [NSString stringReferenceFromLString:_dockEntry->shortCaption(0)];
		NSString *title = [NSString stringWithFormat:NSLocalizedString(@"DockMenuWeatherText", @""), city,
		                                             temperatureString, weatherString];
		[_dockMenu addItem:[[[NSMenuItem alloc] initWithTitle:title action:@selector(_dockEntryWasSelected:)
		                                         keyEquivalent:@""] autorelease]];
	}
	[_dockMenu addItem:[[[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"UpdateCurrentTemperature", @"")
	                                               action:@selector(_updateDockWeather:)
	                                        keyEquivalent:@""] autorelease]];
	[_dockMenu addItem:[[[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"DockMenuSelectCity", @"")
	                                               action:@selector(_selectHomeCityFromDock:)
	                                        keyEquivalent:@""] autorelease]];
	return _dockMenu;
}

@end

#pragma clang diagnostic pop
