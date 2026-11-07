#import "SelectCityWindow.h"
#import "WGTexturedView.h"
#import <AddressBook/AddressBook.h>

@interface NSObject (SelectCityDialogDelegate)
- (void)selectCitySheetDidEnd;
@end

// The shared controller; (id)-1 once the app terminated.
static id _sSelectCityDialogController = nil;

enum {
	kSelectCityModal = 0,
	kSelectCitySheet = 1,
};

// The home city chooser: a city list in a textured window, run modally or
// as a sheet; choosing sets the home city.
@implementation SelectCityDialogController

+ (id)_sharedController
{
	if (!_sSelectCityDialogController) {
		_sSelectCityDialogController = [[SelectCityDialogController alloc] init];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_appWillTerminate)
			name:NSApplicationWillTerminateNotification object:nil];
	}
	return _sSelectCityDialogController == (id)-1 ? nil : _sSelectCityDialogController;
}

+ (void)_appWillTerminate
{
	if (_sSelectCityDialogController && _sSelectCityDialogController != (id)-1) {
		[_sSelectCityDialogController release];
		_sSelectCityDialogController = (id)-1;
	}
}

// Moves the window's views onto the "home_city_bg" texture.
- (void)awakeFromNib
{
	[super awakeFromNib];
	NSImage *image = [NSImage imageNamed:@"home_city_bg"];
	NSSize size = [image size];
	WGTexturedView *background = [[[WGTexturedView alloc] initWithFrame:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y,
		size.width, size.height)] autorelease];
	[background setImage:image forTexture:0];
	NSView *contentView = [_window contentView];
	// (The original compared the content and image sizes here and ignored
	// the result.)
	while ([[contentView subviews] count])
		[background addSubview:[[contentView subviews] objectAtIndex:0]];
	[contentView addSubview:background];
	[_window center];
	[_window setExcludedFromWindowsMenu:YES];
	[[self entriesTableView] setDoubleAction:@selector(_doubleClickAction:)];
}

- (void)dealloc
{
	[_window release];
	[super dealloc];
}

- (id)nibName
{
	return @"SelectCityDialog";
}

- (void)_doubleClickAction:(id)aAction
{
	if ([[self entriesTableView] clickedRow] >= 0)
		[self selectAction:aAction];
}

// Searches for the city (or a guess at the user's) and focuses the list.
- (void)_prepareWindowForCity:(PointEntry *)aCity
{
	LString *name = aCity ? aCity->ownName(0) : [self _defaultCityName];
	_entriesDataSource->setSearchString(name);
	[self loadNibIfNeeded];
	[_filterTextField setStringValue:(NSString *)name->cfString()];
	[_window makeFirstResponder:nil];
	[_window selectNextKeyView:nil];
}

- (void)_beginSheetWithDefaultCity:(PointEntry *)aCity modalForWindow:(id)aWindow modalDelegate:(id)aDelegate
{
	[self _prepareWindowForCity:aCity];
	_dialogMode = kSelectCitySheet;
	_modalDelegate = aDelegate;
	[aWindow beginSheet:_window completionHandler:^(NSModalResponse returnCode) {
		[self sheetDidEnd:_window returnCode:returnCode contextInfo:aCity];
	}];
}

- (void)selectAction:(id)aAction
{
	if (![self selectedItem]) {
		NSBeep();
		return;
	}
	if (_dialogMode == kSelectCityModal)
		[NSApp stopModalWithCode:NSModalResponseOK];
	else if (_dialogMode == kSelectCitySheet)
		[[_window sheetParent] endSheet:_window returnCode:NSModalResponseOK];
}

- (void)tableViewSelectionDidChange:(id)aChange
{
	[_selectButton setEnabled:[self selectedItem] != 0];
}

+ (void)runModalDialog
{
	[[self _sharedController] _runModalDialogWithDefaultCity:[self _homeCity]];
}

+ (void)beginSheetModalForWindow:(id)aWindow modalDelegate:(id)aDelegate
{
	[[self _sharedController] _beginSheetWithDefaultCity:[self _homeCity] modalForWindow:aWindow modalDelegate:aDelegate];
}

- (void)_runModalDialogWithDefaultCity:(PointEntry *)aCity
{
	[self _prepareWindowForCity:aCity];
	_dialogMode = kSelectCityModal;
	if ([NSApp runModalForWindow:_window] == NSModalResponseOK)
		aCity = lobj_cast<PointEntry>([self selectedItem]);
	[_window orderOut:self];
	if (aCity)
		Preferences::setHomeCityUID(aCity->uid(0));
}

// The context is the default city, kept when cancelled.
- (void)sheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	PointEntry *city = (PointEntry *)aInfo;
	if (aCode == NSModalResponseOK)
		city = lobj_cast<PointEntry>([self selectedItem]);
	[_window orderOut:self];
	if (city)
		Preferences::setHomeCityUID(city->uid(0));
	if ([_modalDelegate respondsToSelector:@selector(selectCitySheetDidEnd)])
		[_modalDelegate performSelector:@selector(selectCitySheetDidEnd)];
	_modalDelegate = nil;
}

+ (PointEntry *)_homeCity
{
	LString *uid = Preferences::homeCityUID();
	return uid ? lobj_cast<PointEntry>(GenericEntry::entryWithUID(uid)) : 0;
}

// Whether the atlas has a city of the name.
static bool _atlasHasCity(LString *name)
{
	SQLiteResultsContainer *results = AtlasDatabase::instance()->searchWithRequest(
		SQL_DB_Strings::selectCitiesUIDsWithCityName(name), (LMap *)0);
	return results && results->count();
}

// The city of the user's card, else the last city picked in the Date &
// Time time zone pane, else Cupertino.
- (LString *)_defaultCityName
{
	// (primaryValue, as the original called it, is no longer declared.)
	ABMultiValue *addresses = [[[ABAddressBook sharedAddressBook] me] valueForProperty:kABAddressProperty];
	NSDictionary *address = [addresses valueForIdentifier:[addresses primaryIdentifier]];
	LString *name = [[address objectForKey:kABAddressCityKey] LString];
	if (name && _atlasHasCity(name))
		return name;
	[NSUserDefaults resetStandardUserDefaults];
	NSArray *lastCity = [[NSUserDefaults standardUserDefaults] objectForKey:@"com.apple.TimeZonePref.Last_Selected_City"];
	NSInteger count = [lastCity count];
	if (count > 5) {
		name = [[lastCity objectAtIndex:count > 7 ? 7 : 5] LString];
		if (name && _atlasHasCity(name))
			return name;
	}
	return LString::stringWithCString("Cupertino");
}

@end
