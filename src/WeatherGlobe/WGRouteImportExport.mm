#import "WGRouteImportExport.h"

NSString *_kRouteSetFileExt = @"smroute";

static NSString *const _kRoutesPathKey = @"RoutesPathKey";
static WGRouteImportExport *_sWGRouteImportExport = nil;
static BOOL _sAppTerminated = NO;

@interface WGRouteImportExport () <NSOpenSavePanelDelegate>
@end

// Saves routes as .smroute plists and loads them, remembering the folder.
@implementation WGRouteImportExport

// Nil once the application is quitting.
+ (id)sharedRouteImportExport
{
	if (!_sWGRouteImportExport && !_sAppTerminated) {
		_sWGRouteImportExport = [[WGRouteImportExport alloc] init];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_appWillTerminate)
			name:NSApplicationWillTerminateNotification object:nil];
	}
	return _sAppTerminated ? nil : _sWGRouteImportExport;
}

+ (void)_appWillTerminate
{
	_sAppTerminated = YES;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

// The last folder used, else the default one.
- (id)_path
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *path = [defaults stringForKey:_kRoutesPathKey];
	if (path && [[NSFileManager defaultManager] fileExistsAtPath:path])
		return path;
	path = [self defaultRoutesFolder];
	[defaults setObject:path forKey:_kRoutesPathKey];
	return path;
}

static void _WGRouteAlert(NSString *title, NSString *message)
{
	NSString *ok = [[NSBundle mainBundle] localizedStringForKey:@"Ok" value:@"" table:nil];
	NSRunAlertPanel(title, @"%@", ok, nil, nil, message);
}

static NSString *_WGRouteLocalized(NSString *key)
{
	return [[NSBundle mainBundle] localizedStringForKey:key value:@"" table:nil];
}

- (void)saveRouteObject:(RouteObject *)aObject atPath:(id)aPath hideExt:(BOOL)aExt
{
	NSFileManager *manager = [NSFileManager defaultManager];
	NSString *folder = [aPath stringByDeletingLastPathComponent];
	if (![manager isWritableFileAtPath:folder]) {
		_WGRouteAlert(_WGRouteLocalized(@"Cannot save to folder title"),
			[NSString stringWithFormat:_WGRouteLocalized(@"Cannot save to folder message"), folder]);
		return;
	}
	if ([manager fileExistsAtPath:aPath] && ![manager removeItemAtPath:aPath error:NULL]) {
		NSString *name = [[aPath lastPathComponent] stringByDeletingPathExtension];
		_WGRouteAlert([NSString stringWithFormat:_WGRouteLocalized(@"Cannot replace file title"), name],
			_WGRouteLocalized(@"Cannot replace file message"));
		return;
	}
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults setObject:folder forKey:_kRoutesPathKey];
	[defaults synchronize];
	if (!aObject->dictionaryRepresentation()->writeToFile([aPath LString])) {
		_WGRouteAlert(_WGRouteLocalized(@"Cannot save to full disk title"),
			_WGRouteLocalized(@"Cannot save to full disk message"));
		return;
	}
	if (aExt)
		[manager setAttributes:[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:YES],
			NSFileExtensionHidden, nil] ofItemAtPath:aPath error:NULL];
}

// The route comes as the context.
- (void)_savePanelDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	NSString *path = [[aEnd URL] path];
	if (aCode == NSModalResponseOK)
		[self saveRouteObject:(RouteObject *)aInfo atPath:path hideExt:[aEnd isExtensionHidden]];
}

// The target gets the route's NSObject wrapper, or nil.
- (void)_openPanelDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	NSString *path = [[aEnd URL] path];
	RouteObject *route = aCode == NSModalResponseOK ? [self loadRouteObjectAtPath:path] : 0;
	[_target performSelector:_endCallback withObject:route ? (id)route->detachNSWrapper() : nil];
}

- (void)exportRoute:(RouteObject *)aRoute withName:(id)aName hostWindow:(id)aWindow
{
	if (!aRoute)
		return;
	NSSavePanel *panel = [NSSavePanel savePanel];
	[panel setCanSelectHiddenExtension:YES];
	[panel setAllowedFileTypes:[NSArray arrayWithObjects:_kRouteSetFileExt, nil]];
	[panel setExtensionHidden:YES];
	if (!aName)
		aName = [NSString stringReferenceFromLString:aRoute->preferredName()];
	[panel setDirectoryURL:[NSURL fileURLWithPath:[self _path] isDirectory:YES]];
	[panel setNameFieldStringValue:aName];
	if (!aWindow) {
		[self _savePanelDidEnd:panel returnCode:[panel runModal] contextInfo:aRoute];
		return;
	}
	[panel beginSheetModalForWindow:aWindow completionHandler:^(NSModalResponse result) {
		[self _savePanelDidEnd:panel returnCode:result contextInfo:aRoute];
	}];
}

- (void)importRouteWithTarget:(id)aTarget didEndSelector:(SEL)aSelector hostWindow:(id)aWindow
{
	if (![aTarget respondsToSelector:aSelector])
		return;
	_endCallback = aSelector;
	_target = aTarget;
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setCanChooseDirectories:NO];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseFiles:YES];
	[panel setDelegate:self];
	[panel setDirectoryURL:[NSURL fileURLWithPath:[self _path] isDirectory:YES]];
	if (!aWindow) {
		[self _openPanelDidEnd:panel returnCode:[panel runModal] contextInfo:NULL];
		return;
	}
	[panel beginSheetModalForWindow:aWindow completionHandler:^(NSModalResponse result) {
		[self _openPanelDidEnd:panel returnCode:result contextInfo:NULL];
	}];
}

// Readable folders and .smroute files.
- (BOOL)panel:(id)aPanel shouldShowFilename:(id)aFilename
{
	NSFileManager *manager = [NSFileManager defaultManager];
	BOOL readable = [manager isReadableFileAtPath:aFilename];
	NSDictionary *attributes = [manager attributesOfItemAtPath:[aFilename stringByResolvingSymlinksInPath]
		error:NULL];
	NSString *type = [attributes objectForKey:NSFileType];
	if (readable && [type isEqual:NSFileTypeDirectory])
		return YES;
	return [type isEqual:NSFileTypeRegular]
		&& [[aFilename pathExtension] caseInsensitiveCompare:_kRouteSetFileExt] == NSOrderedSame;
}

- (BOOL)panel:(id)aSender shouldEnableURL:(NSURL *)aURL
{
	return [self panel:aSender shouldShowFilename:[aURL path]];
}

// ~/Documents/<RoutesFolderName>, created (rwxr-xr-x) when missing.
- (id)defaultRoutesFolder
{
	LString *path = LFileManager::pathToFolder(kDocumentsFolderType, kUserDomain, true);
	path->appendPathComponent(LString::stringFromTable("RoutesFolderName", "PathComponents",
		(void *)getAppModelModule()));
	LFileManager *manager = LFileManager::manager();
	if (!manager->objectExistsAtPath(path, 0)) {
		if (!manager->createFolderAtPath(path))
			return nil;
		manager->changeFileAttributesAtPath(path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0755),
			LFileManager::fileAttrPosixPermissions, 0));
	}
	return (NSString *)path->cfString();
}

// Autoreleased, or 0 after telling the user why.
- (RouteObject *)loadRouteObjectAtPath:(id)aPath
{
	NSString *name = [[aPath lastPathComponent] stringByDeletingPathExtension];
	NSString *title, *message;
	if (![[NSFileManager defaultManager] isReadableFileAtPath:aPath]) {
		message = _WGRouteLocalized(@"Cannot open message");
		title = _WGRouteLocalized(@"Cannot open title");
	} else {
		NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
		[defaults setObject:[aPath stringByDeletingLastPathComponent] forKey:_kRoutesPathKey];
		[defaults synchronize];
		LMap *rep = LMap::mapWithContentsOfFile([aPath LString]);
		RouteObject *route = RouteObject::_alloc()->initWithDictionaryRep(rep, [name LString]);
		if (!route)
			return 0;
		if (route->itemsCount())
			return (RouteObject *)route->l_autorelease();
		route->release();
		message = _WGRouteLocalized(@"Cannot open corrupted file message");
		title = _WGRouteLocalized(@"Cannot open corrupted file title");
	}
	_WGRouteAlert([NSString stringWithFormat:title, name], message);
	return 0;
}

@end
