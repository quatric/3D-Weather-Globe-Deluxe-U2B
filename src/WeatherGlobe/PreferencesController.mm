#import "PreferencesController.h"
#import "PrefGenericController.h"

// The preferences window: a toolbar of panes (PrefGenericController
// subclasses, made on demand by class name); the window resizes to each.
@implementation PreferencesController

- (id)_identifiers
{
	if (!_identifiers)
		_identifiers = [[NSArray alloc] initWithObjects:@"PrefGeneralController", @"PrefGraphicsController",
			@"PrefApearanceController", @"PrefCloudsController", @"PrefMyLocationController",
			@"PrefOnlineController", @"PrefScreenSaverController", nil];
	return _identifiers;
}

- (id)_prefControllerForKey:(id)aKey
{
	if (![[self _identifiers] containsObject:aKey])
		return nil;
	if (!_prefControllers)
		_prefControllers = [[NSMutableDictionary alloc] init];
	PrefGenericController *controller = [_prefControllers objectForKey:aKey];
	if (!controller) {
		controller = [[[NSClassFromString(aKey) alloc] init] autorelease];
		[_prefControllers setObject:controller forKey:aKey];
	}
	return controller;
}

// Shows the first pane. (The original also shrank the window on AppKit
// 743 and earlier.)
- (void)awakeFromNib
{
	_toolbar = [[NSToolbar alloc] initWithIdentifier:@"com.mackiev.3dweatherglobe.preferences.toolbar"];
	[_toolbar setDisplayMode:NSToolbarDisplayModeIconAndLabel];
	[_toolbar setAllowsUserCustomization:NO];
	[_toolbar setDelegate:(id)self];
	if ([[self _identifiers] count]) {
		NSString *identifier = [_identifiers objectAtIndex:0];
		PrefGenericController *controller = [self _prefControllerForKey:identifier];
		NSView *view = [controller mainView];
		[_mainWindow setContentSize:[view frame].size];
		[controller viewWillBeInstalled];
		[[_mainWindow contentView] addSubview:view];
		_currentController = controller;
		[_toolbar setSelectedItemIdentifier:identifier];
		[_mainWindow makeFirstResponder:view];
	}
	if ([_mainWindow respondsToSelector:@selector(setShowsToolbarButton:)])
		[_mainWindow setShowsToolbarButton:NO];
	[_mainWindow setTitle:[_currentController toolbarLabel]];
	[_mainWindow setToolbar:_toolbar];
	[_mainWindow center];
}

- (void)dealloc
{
	[_mainWindow release];
	[_toolbar release];
	[_identifiers release];
	[[_prefControllers allValues] makeObjectsPerformSelector:@selector(preferencesWillTerminate)];
	[_prefControllers release];
	[super dealloc];
}

- (void)_loadNibIfNeeded
{
	if (!_mainWindow)
		[NSBundle loadNibNamed:@"Preferences" owner:self];
}

// Swaps the panes, resizing the window around the new one (its top stays).
- (void)_toolbarItemAction:(id)aAction
{
	PrefGenericController *controller = [self _prefControllerForKey:[aAction itemIdentifier]];
	if (controller == _currentController)
		return;
	NSView *view = [controller mainView];
	NSView *oldView = [_currentController mainView];
	[_currentController viewWillBeUninstalled];
	[oldView removeFromSuperview];
	NSRect frame = [_mainWindow frame];
	CGFloat top = frame.origin.y + frame.size.height;
	NSRect content = [_mainWindow contentRectForFrameRect:frame];
	content.size.height = [view frame].size.height;
	frame = [_mainWindow frameRectForContentRect:content];
	frame.origin.y = top - frame.size.height;
	[_mainWindow setFrame:frame display:YES animate:YES];
	[controller viewWillBeInstalled];
	[[_mainWindow contentView] addSubview:view];
	_currentController = controller;
	[_mainWindow setTitle:[controller toolbarLabel]];
	[_toolbar setSelectedItemIdentifier:[aAction itemIdentifier]];
	[_mainWindow makeFirstResponder:view];
}

- (void)_showPreferencesWithIdentifier:(id)aIdentifier
{
	NSUInteger index = [[self _identifiers] indexOfObject:aIdentifier];
	if (index == NSNotFound)
		return;
	[self _loadNibIfNeeded];
	[_mainWindow makeKeyAndOrderFront:self];
	[_toolbar setSelectedItemIdentifier:aIdentifier];
	[self _toolbarItemAction:[[_toolbar items] objectAtIndex:index]];
}

- (void)showPreferences
{
	[self _loadNibIfNeeded];
	[_mainWindow makeKeyAndOrderFront:self];
}

- (void)showSelectCityDialog
{
	[self _showPreferencesWithIdentifier:@"PrefMyLocationController"];
	[[self _prefControllerForKey:@"PrefMyLocationController"] primaryAction];
}

- (void)showWeatherPane
{
	[self _showPreferencesWithIdentifier:@"PrefCloudsController"];
}

- (id)toolbarDefaultItemIdentifiers:(id)aIdentifiers
{
	return [self _identifiers];
}

- (id)toolbarAllowedItemIdentifiers:(id)aIdentifiers
{
	return [self _identifiers];
}

- (id)toolbarSelectableItemIdentifiers:(id)aIdentifiers
{
	return [self _identifiers];
}

- (id)toolbar:(id)aToolbar itemForItemIdentifier:(id)aIdentifier willBeInsertedIntoToolbar:(BOOL)aToolbar2
{
	PrefGenericController *controller = [self _prefControllerForKey:aIdentifier];
	PreferencesToolbarItem *item = [[[PreferencesToolbarItem alloc] initWithItemIdentifier:aIdentifier] autorelease];
	[item setLabel:[controller toolbarLabel]];
	[item setImage:[controller toolbarImage]];
	[item setTarget:self];
	[item setAction:@selector(_toolbarItemAction:)];
	return item;
}

@end
