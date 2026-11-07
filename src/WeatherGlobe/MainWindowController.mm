// MainWindowController
#import "MainWindowController.h"
#import "AtlasController.h"
#import "AtlasGLView.h"
#import "CIPaletteController.h"
#import "WGRouteManagerController.h"
#import "WGEarthView.h"
#import "WGLeftAreaController.h"
#import "WGMainEntriesListController.h"
#import "WGCurrentSetController.h"
#import "WGCommonEntryDetailsController.h"
#import "WGNewLocationController.h"
#import "WGExportImageController.h"
#import "WGCustomToolbarController.h"
#import "WGCustomFieldEditor.h"
#import "WGCustomTitleView.h"
#import "WGCustomSlider.h"
#import "WGSplitView.h"
#import "WGFilledView.h"
#import "WGTexturedView.h"
#import "WBAlert.h"
#import "MainPrintDataSourceProvider.h"
#import "PrintController.h"
#import "NSMenu+NSMenuWGExtensions.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

// The left area's width, and its limits while dragging the divider.
static const CGFloat kLeftAreaWidth = 320.0;
static const CGFloat kLeftAreaMaxWidth = 500.0;
// The scale step of the zoom menu items.
static const float kScaleStep = 0.05f;

// drawElementsMenu: tags.
enum {
	kDrawImagesOnMap = 0,
	kDrawDayNight = 2,
	kDrawGrid = 3,
	kDrawWeatherOnMap = 7,
	kDrawClouds = 8,
	kDrawMoviePalette = 9,
	kDrawTimeLinePalette = 10,
	kDrawNavigator = 11,
	kDrawInfoDevice = 12,
	kDrawMinimap = 13,
	kDrawLockNormal = 14,
	kDrawLockTilted = 15,
	kDrawSettingsPalette = 16
};

// spaceMenu: tags.
enum { kSpaceAll = 0, kSpaceStars, kSpaceMoon, kSpaceSun, kSpacePlanets, kSpaceConstellations };

// regionHighlightMenu: tags.
enum { kRegionAllBorders = 0, kRegionCountries, kRegionStates, kRegionTimezones, kRegionHighlightAnyway };

// locationsMenu: tags.
enum { kLocationsAll = 0, kLocationsCapitals, kLocationsAppleStores, kLocationsOther };

// cloudsMenu: tags; the last three pick the animation quality.
enum { kCloudsOff = 1, kCloudsQualityLow = 2, kCloudsQualityMedium = 3, kCloudsQualityHigh = 4 };

// viewControlMenu: tags; the mouse behaviour items are the control tag
// plus kViewMouseBehaviourFirst.
enum {
	kViewZoomIn = 1,
	kViewZoomOut = 2,
	kViewToggleMapMode = 3,
	kViewToggleSettings = 4,
	kViewMouseBehaviourFirst = 5,
	kViewMouseBehaviourLast = 7,
	kViewFullWindow = 8
};

// mainWindowMenu: tags.
enum { kMainWindowExportImage = 0, kMainWindowEmailImage = 1, kMainWindowDesktopPicture = 2, kMainWindowPrintView = 3 };

// leftArea: tags.
enum { kLeftAreaCurrentSet = 0, kLeftAreaDetails = 1, kLeftAreaSettings = 2, kLeftAreaCurrentSetMode = 3 };

// routeMenu: tags handled here; the others go to the current set.
// The current set's commands that open windows are refused in full window
// mode; importing first shows the current set.
enum {
	kRouteImport = 1,
	kRouteExport = 2,
	kRouteChangeColor = 6,
	kRouteToggleFlying = 10,
	kRoutePrintCurrentSet = 11,
	kRoutePrintLocationInfo = 12
};

// entriesListMenu: the print item; the others go to the entries list.
enum { kEntriesListPrintLocationInfo = 6 };

// _paletteActionsMenu: the Add To Weather Palette item.
enum { kPaletteActionAddSelected = 1 };

// The map mode that hides the cursor in full window mode.
enum { kMapModeLogo = 4 };

// Billboard mode of showDetailsForEntry: that doesn't leave full window mode.
enum { kBillboardInfoInPlace = 2 };

@implementation MainWindowController

- (BOOL)_loadNibIfNeeded
{
	if (!_nibWasLoaded)
		_nibWasLoaded = [NSBundle loadNibNamed:@"MainWindow" owner:self];
	return _nibWasLoaded;
}

- (void)_cleanupOnWindowClose
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[_mainSplitView release];
	_mainSplitView = nil;
	[_fieldEditor release];
	_fieldEditor = nil;
	[_toolbarController release];
	_toolbarController = nil;
	[_leftAreaController release];
	_leftAreaController = nil;
	[_exportImageController release];
	_exportImageController = nil;
	[_popupMenu release];
	_popupMenu = nil;
	if (_wndInterface) {
		delete _wndInterface;
		_wndInterface = 0;
	}
	[_mainWindow setDelegate:nil];
	[_mainWindow autorelease];
	_mainWindow = nil;
	[_fullScreenWindow autorelease];
	_fullScreenWindow = nil;
	_nibWasLoaded = NO;
}

- (void)windowWillClose:(id)aClose
{
	[NSApp terminate:nil];
}

- (void)windowWillMiniaturize:(id)aMiniaturize
{
	LNotificationCenter::center()->postNotification(kRecordingMustBeStoppedNotification, (LObject *)0, (LMap *)0);
}

// The full window stays above other windows, except while another window
// (not the main one) is key.
- (void)windowDidBecomeKey:(id)aKey
{
	id window = [aKey object];
	if ([self isInFullWindowMode] && window != _fullScreenWindow && window != _mainWindow)
		[_fullScreenWindow setLevel:CGWindowLevelForKey(kCGNormalWindowLevelKey)];
}

- (void)windowDidResignKey:(id)aKey
{
	id window = [aKey object];
	if ([self isInFullWindowMode] && window != _fullScreenWindow && window != _mainWindow)
		[_fullScreenWindow setLevel:CGWindowLevelForKey(kCGPopUpMenuWindowLevelKey)];
}

// The Leopard title images; the Tiger ones ("MW_title_") are gone with Tiger.
- (void)_setupCustomTitle
{
	NSString *prefix = @"Leopard_MW_title_";
	WGCustomTitleView *titleView = [_mainWindow customTitleView];
	[titleView setImage:[NSImage imageNamed:[prefix stringByAppendingString:@"left"]] forTexture:0];
	[titleView setImage:[NSImage imageNamed:[prefix stringByAppendingString:@"center"]] forTexture:1];
	[titleView setImage:[NSImage imageNamed:[prefix stringByAppendingString:@"right"]] forTexture:2];
	[titleView setTitleShadowColor:[[NSColor blackColor] colorWithAlphaComponent:0.6]];
	[titleView setTitleShadowOffset:NSMakeSize(0.0, 1.0)];
	[titleView setDrawsTitle:YES];
}

- (void)_setupBottomArea
{
	[_bottomArea setImage:[NSImage imageNamed:@"MW_bottom_left"] forTexture:6];
	[_bottomArea setImage:[NSImage imageNamed:@"MW_bottom_center"] forTexture:7];
	[_bottomArea setImage:[NSImage imageNamed:@"MW_bottom_right"] forTexture:8];
	[_bottomArea setCompositeOperation:NSCompositingOperationCopy];
	[_popupMenuButton sendActionOn:NSEventMaskLeftMouseDown];
	[_mainWindow setShowsResizeIndicator:NO];
}

- (void)_setupScaleSlider
{
	[_scaleSlider setKnobImage:[NSImage imageNamed:@"MWB_slider_knob"]];
	[_scaleSlider setKnobAlternateImage:[NSImage imageNamed:@"MWB_slider_knob_on"]];
	[_scaleSlider setKnobDisabledImage:[NSImage imageNamed:@"MWB_slider_knob_Disable"]];
	[_scaleSlider setBarMiddleImage:[NSImage imageNamed:@"SliderBG_Center"]];
	[_scaleSlider setBarFirstEdgeImage:[NSImage imageNamed:@"SliderBG_Left"]];
	[_scaleSlider setBarLastEdgeImage:[NSImage imageNamed:@"SliderBG_Right"]];
}

- (void)_setupSeparators
{
	[_titleSeparator setFillColor:[NSColor colorWithCalibratedWhite:0.0745 alpha:1.0]];
}

// Resizes the split view's first subview to aWidth, the second one taking
// the difference.
- (void)_setLeftWidth:(CGFloat)aWidth forSplitView:(id)aView
{
	NSArray *subviews = [aView subviews];
	NSView *left = [subviews objectAtIndex:0];
	NSRect frame = [left frame];
	CGFloat delta = aWidth - frame.size.width;
	frame.size.width = aWidth;
	[left setFrame:frame];
	NSView *right = [subviews objectAtIndex:1];
	frame = [right frame];
	frame.origin.x += delta;
	frame.size.width -= delta;
	[right setFrame:frame];
}

- (void)dealloc
{
	[self _cleanupOnWindowClose];
	[super dealloc];
}

- (id)glView
{
	return _glView;
}

- (id)mainWindow
{
	return _mainWindow;
}

- (AppWindowInterfaceWrapper *)windowInterface
{
	return _wndInterface;
}

- (id)entriesListController
{
	return [_leftAreaController entriesListController];
}

- (id)currentSetController
{
	return [_leftAreaController currentSetController];
}

- (id)entryDetailsController
{
	return [_leftAreaController entryDetailsController];
}

- (id)toolbarController
{
	if (!_toolbarController)
		_toolbarController = [WGCustomToolbarController new];
	return _toolbarController;
}

- (id)exportImageController
{
	if (!_exportImageController)
		_exportImageController = [WGExportImageController new];
	return _exportImageController;
}

// The first time, opens the map panel (or plays the intro) before showing
// the window.
- (void)showWindow
{
	if ([self isWindowVisible])
		return;
	BOOL wasLoaded = _nibWasLoaded;
	if (![self _loadNibIfNeeded])
		return;
	Preferences::setShowMainWindow(true);
	if (!wasLoaded) {
		bool intro = Preferences::showIntroOnStartup();
		Preferences::setShowDemoOnStartup(false);
		if (!intro) {
			_wndInterface->openPanel(Preferences::mapMode());
			[_mainWindow displayIfNeeded];
			if (Preferences::openFullScreenByDefault() && ![self isInFullWindowMode]) {
				[self toggleFullWindowMode:nil];
				return;
			}
		} else {
			_wndInterface->startIntro();
			[_mainWindow displayIfNeeded];
		}
	}
	[_mainWindow makeKeyAndOrderFront:self];
}

- (void)hideWindow
{
	[_mainWindow close];
}

- (BOOL)isWindowVisible
{
	return [_mainWindow isVisible] || [_fullScreenWindow isVisible];
}

- (BOOL)isLeftAreaVisible
{
	return [_mainSplitView window] != nil;
}

- (BOOL)isInFullWindowMode
{
	return _fullScreenWindow && [_glView window] == _fullScreenWindow;
}

- (void)_showInNormalWindowModeForced
{
	if ([self isInFullWindowMode])
		[self toggleFullWindowMode:nil];
	else
		[self showWindow];
}

- (void)setScaleValue:(CGFloat)aValue
{
	if ([_glView window] != _mainWindow)
		return;
	[_scaleSlider setFloatValue:aValue];
	[_glView setNeedsDisplay:YES];
}

- (void)openSearchForTerm:(id)aTerm
{
	[self _showInNormalWindowModeForced];
	if ([_leftAreaController isSettingsMode])
		[_leftAreaController switchToSettingsMode:NO];
	[[self entriesListController] setSearchTerm:[aTerm LString] andRefreshResults:YES];
}

- (void)showDetailsForEntry:(GenericEntry *)aEntry openPane:(BOOL)aPane openInfoForBillboard:(NSInteger)aBillboard
{
	if (aPane && aBillboard != kBillboardInfoInPlace)
		[self _showInNormalWindowModeForced];
	[[self entryDetailsController] setCurrentEntry:aEntry openPane:aPane mode:aBillboard];
}

- (void)gotoLocation:(GenericEntry *)aLocation forced:(BOOL)aForced
{
	if (!_nibWasLoaded)
		return;
	if (!aForced && aLocation == _wndInterface->contextObjectsContainer()->currentEntry())
		return;
	_wndInterface->currentPanel()->setCurrentEntry(aLocation, aLocation != 0);
}

- (void)spaceMenu:(id)aMenu
{
	switch ([aMenu tag]) {
	case kSpaceAll:
		Preferences::setSpaceEnabled(!Preferences::spaceEnabled());
		break;
	case kSpaceStars:
		Preferences::setStarsEnabled(!Preferences::starsEnabled());
		break;
	case kSpaceMoon:
		Preferences::setMoonEnabled(!Preferences::moonEnabled());
		break;
	case kSpaceSun:
		Preferences::setSunEnabled(!Preferences::sunEnabled());
		break;
	case kSpacePlanets:
		Preferences::setPlanetsEnabled(!Preferences::planetsEnabled());
		break;
	case kSpaceConstellations:
		Preferences::setConstellationsEnabled(!Preferences::constellationsEnabled());
		break;
	}
}

- (void)gridMenu:(id)aMenu
{
}

static LString *_shapeIDForRegionTag(NSInteger aTag)
{
	switch (aTag) {
	case kRegionCountries:
		return CShapeManager::kShapeIDCountries;
	case kRegionStates:
		return CShapeManager::kShapeIDStates;
	case kRegionTimezones:
		return CShapeManager::kShapeIDTimezones;
	}
	return 0;
}

- (void)regionHighlightMenu:(id)aMenu
{
	NSInteger tag = [aMenu tag];
	if (tag == kRegionAllBorders) {
		Preferences::setShowAllBorders(!Preferences::showAllBorders());
	} else if (tag == kRegionHighlightAnyway) {
		Preferences::setHighlightBordersAnyway(!Preferences::highlightBordersAnyway());
	} else if (LString *shapeID = _shapeIDForRegionTag(tag)) {
		Preferences::setShapesDrawEnabled(shapeID, !Preferences::shapesDrawEnabled(shapeID));
	}
}

- (void)locationsMenu:(id)aMenu
{
	switch ([aMenu tag]) {
	case kLocationsAll:
		Preferences::setShowAllPoints(!Preferences::showAllPoints());
		break;
	case kLocationsCapitals:
		Preferences::setShowCapitalPoints(!Preferences::showCapitalPoints());
		break;
	case kLocationsAppleStores:
		Preferences::setShowAppleStorePoints(!Preferences::showAppleStorePoints());
		break;
	case kLocationsOther:
		Preferences::setShowOtherPoints(!Preferences::showOtherPoints());
		break;
	}
}

- (void)cloudsMenu:(id)aMenu
{
	switch ([aMenu tag]) {
	case kCloudsOff:
		Preferences::setCloudsEnabled(false, true);
		break;
	case kCloudsQualityLow:
	case kCloudsQualityMedium:
	case kCloudsQualityHigh:
		Preferences::setCloudsAnimationQuality((TCloudsAnimation)([aMenu tag] - kCloudsQualityLow));
		Preferences::setCloudsEnabled(true, true);
		break;
	}
}

// Adds the selected entries to the weather palette: those of the focused
// list, else the first list's that can be added.
- (void)_paletteActionsMenu:(id)aMenu
{
	if ([aMenu tag] != kPaletteActionAddSelected)
		return;
	id firstResponder = [[self mainWindow] firstResponder];
	WGCurrentSetController *currentSet = [self currentSetController];
	WGMainEntriesListController *entriesList = [self entriesListController];
	CIPaletteController *palette = [[AtlasController atlasController] citiesPaletteController];
	id currentSetTable = [currentSet tableView];
	id entriesTable = [entriesList entriesTableView];
	TLArray<GenericEntry*> *listEntries =
	    [entriesList selectedItemsWithLimit:Preferences::citiesCountLimit() * 2 createIfNeeded:YES];
	TLArray<GenericEntry*> *currentSetEntries = [currentSet selectedItems];
	TLArray<GenericEntry*> *entries;
	if (firstResponder != currentSetTable && firstResponder != entriesTable) {
		if ([palette canAddEntries:listEntries])
			entries = listEntries;
		else if ([palette canAddEntries:currentSetEntries])
			entries = currentSetEntries;
		else
			entries = 0;
	} else {
		entries = firstResponder == entriesTable ? listEntries : currentSetEntries;
	}
	[palette addEntries:entries];
}

// Takes the split view (and the left area with it) out of the window, or
// puts it back.
- (void)toggleLeftArea:(id)aArea
{
	if ([self isInFullWindowMode])
		return;
	BOOL cached = [_mainWindow isVisible];
	if (cached)
		[_glView createCachedImage];
	if (![self isLeftAreaVisible]) {
		NSRect frame = [_rightAreaView frame];
		NSView *superview = [_rightAreaView superview];
		[_mainSplitView addSubview:_rightAreaView];
		[_rightAreaView setFrame:_rightAreaRect];
		[superview addSubview:_mainSplitView];
		[_mainSplitView setFrame:frame];
	} else {
		NSView *superview = [_mainSplitView superview];
		[_mainSplitView removeFromSuperview];
		_rightAreaRect = [_rightAreaView frame];
		[superview addSubview:_rightAreaView];
		[_rightAreaView setFrame:[_mainSplitView frame]];
	}
	if (cached)
		[_glView discardCachedImage];
	[_leftAreaButton setState:[self isLeftAreaVisible] ? NSControlStateValueOn : NSControlStateValueOff];
	Preferences::setLeftAreaVisible([self isLeftAreaVisible]);
}

// Tag 0 the globe, 1 the flat map.
- (void)viewModeChanged:(id)aChanged
{
	NSInteger tag = [[aChanged keyCell] tag];
	if (tag == 0 || tag == 1)
		_wndInterface->openPanel((int)tag);
}

- (void)popupMenuButtonAction:(id)aAction
{
	[NSMenu wgPopUpContextMenu:_popupMenu withEvent:[[aAction window] currentEvent] forView:aAction];
}

- (void)toggleWeatherPalette:(id)aPalette
{
	[[AtlasController atlasController] toggleCitiesInfoPalette:aPalette];
}

// "Name.JPEG", or "Name2.JPEG", "Name3.JPEG"... when it exists.
static NSString *_uniqueImagePath(NSString *aDirectory, NSString *aName)
{
	NSString *path = [aDirectory stringByAppendingPathComponent:[aName stringByAppendingPathExtension:@"JPEG"]];
	NSString *uniquePath = path;
	for (int i = 2; [[NSFileManager defaultManager] fileExistsAtPath:uniquePath]; i++) {
		NSRange extension = [path rangeOfString:@"." options:NSBackwardsSearch];
		if (extension.length != 0)
			uniquePath = [[[path substringToIndex:extension.location]
			    stringByAppendingString:[NSString stringWithFormat:@"%d", i]] stringByAppendingPathExtension:@"JPEG"];
	}
	return uniquePath;
}

- (void)mainWindowMenu:(id)aMenu
{
	switch ([aMenu tag]) {
	case kMainWindowExportImage:
		[self exportAsImage:aMenu];
		break;
	case kMainWindowEmailImage: {
		WGExportImageController *exporter = [self exportImageController];
		NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent:
		    [NSLocalizedString(@"Image Filename", @"") stringByAppendingPathExtension:@"JPEG"]];
		[exporter setExportedImageFilename:[path lastPathComponent]];
		NSData *data = [exporter glViewAsImageData:[self glView] forSize:[[self glView] bounds].size];
		if (data && [data writeToFile:path atomically:YES])
			wgCreateMailWithAttachment(path, @"");
		break;
	}
	case kMainWindowDesktopPicture: {
		NSString *directory = [NSHomeDirectoryForUser(NSUserName()) stringByAppendingPathComponent:@"Pictures"];
		if (![[NSFileManager defaultManager] fileExistsAtPath:directory])
			directory = [directory stringByDeletingLastPathComponent];
		NSString *path = _uniqueImagePath(directory, NSLocalizedString(@"Desktop ScreenShoot Name", @""));
		WGExportImageController *exporter = [self exportImageController];
		[exporter setExportedImageFilename:[path lastPathComponent]];
		NSData *data = [exporter glViewAsImageData:[self glView] forSize:[[AtlasController generalScreen] frame].size];
		if (data && [data writeToFile:path atomically:YES])
			wgSetDesktopImageAtPath(path);
		break;
	}
	case kMainWindowPrintView:
		[self printGLViewOnly];
		break;
	}
}

- (void)editEntryAction:(id)aAction
{
	[[self entryDetailsController] setIsEditing:![[self entryDetailsController] isEditing]];
	[self _validateEditModeButton];
}

- (void)entriesListMenu:(id)aMenu
{
	if ([aMenu tag] == kEntriesListPrintLocationInfo)
		[self printLocationInfoOnly];
	else
		[[self entriesListController] entriesListCommand:[aMenu tag]];
}

- (void)leftArea:(id)aArea
{
	switch ([aArea tag]) {
	case kLeftAreaCurrentSet:
		[_leftAreaController showCurrentSet:![_leftAreaController isCurrentSetPaneVisible]];
		break;
	case kLeftAreaDetails:
		[_leftAreaController showDetails:![_leftAreaController isDetailsPaneVisible]];
		break;
	case kLeftAreaSettings:
		[_leftAreaController switchToSettingsMode:[[aArea selectedCell] tag] != 0];
		break;
	case kLeftAreaCurrentSetMode:
		[[self currentSetController] setMode:[[aArea selectedCell] tag] != 0];
		break;
	}
}

- (void)showDetails:(id)aDetails
{
	[_leftAreaController showDetails:YES];
}

- (void)onAddNewEntryButton:(id)aButton
{
	[[[AtlasController atlasController] newLocationController] togglePalette];
}

- (void)exportAsImage:(id)aImage
{
	[[self exportImageController] exportGlViewAsImage:[self glView]];
}

static MainPrintDataSourceProvider *_printProvider(BOOL aMainView, BOOL aLocationInfo, BOOL aCurrentRoute)
{
	MainPrintDataSourceProvider *provider = [MainPrintDataSourceProvider new];
	[provider setDoShowCustomDialog:NO];
	[provider setMainViewBtnState:aMainView];
	[provider setLocationInfoBtnState:aLocationInfo];
	[provider setCurrentRouteBtnState:aCurrentRoute];
	return provider;
}

- (void)printCurrentSetOnly
{
	RouteObject *route = [[self currentSetController] currentRoute];
	if (!route || route->locationsCount() == 0)
		return;
	MainPrintDataSourceProvider *provider = _printProvider(NO, NO, YES);
	[provider setPrintTitle:[NSString stringWithLString:route->preferredName()]];
	[PrintController printWithDataSourceProvider:[provider autorelease]];
}

- (void)printLocationInfoOnly
{
	[PrintController printWithDataSourceProvider:[_printProvider(NO, YES, NO) autorelease]];
}

- (void)printGLViewOnly
{
	[PrintController printWithDataSourceProvider:[_printProvider(YES, NO, NO) autorelease]];
}

// The print dialog with the parts there is something to print for.
- (void)doPrint:(id)aPrint
{
	MainPrintDataSourceProvider *provider = [MainPrintDataSourceProvider new];
	RouteObject *route = [[self currentSetController] currentRoute];
	BOOL hasRoute = route && route->locationsCount() > 0;
	BOOL hasEntry = [[self entryDetailsController] currentEntry] != 0;
	[provider setDoShowCustomDialog:YES];
	[provider setMainViewBtnState:YES];
	[provider setCurrentRouteBtnState:hasRoute];
	[provider setLocationInfoBtnState:hasEntry];
	if (hasRoute)
		[provider setPrintTitle:[NSString stringWithLString:route->preferredName()]];
	[PrintController printWithDataSourceProvider:[provider autorelease]];
}

- (void)runPageLayout:(id)aLayout
{
	[[NSPageLayout pageLayout] runModal];
}

- (void)startDemo:(id)aDemo
{
	if (!_wndInterface)
		return;
	[_mapModeButtons selectCellWithTag:0];
	_wndInterface->startDemo(0);
	if ([self isInFullWindowMode])
		[NSCursor setHiddenUntilMouseMoves:YES];
}

// The left area keeps its width.
- (void)splitView:(id)aView resizeSubviewsWithOldSize:(NSSize)aSize
{
	if (NSEqualSizes([aView frame].size, aSize))
		return;
	CGFloat leftWidth = [[[aView subviews] objectAtIndex:0] frame].size.width;
	[aView adjustSubviews];
	[self _setLeftWidth:leftWidth forSplitView:aView];
}

- (CGFloat)splitView:(id)aView constrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	return kLeftAreaWidth;
}

- (CGFloat)splitView:(id)aView constrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	return kLeftAreaMaxWidth;
}

- (NSRect)splitView:(id)aView effectiveRect:(NSRect)aRect forDrawnRect:(NSRect)aRect2 ofDividerAtIndex:(NSInteger)aIndex
{
	return NSInsetRect(aRect, -1.0, 0.0);
}

- (void)contentDidChangeInController:(id)aController
{
	[self _validateLeftAreaControls];
}

// The details show the current entry of the active owner (the entries list
// or the current set), else of the other one.
- (void)currentEntryDidChangeIn:(id)aIn
{
	if (!_nibWasLoaded || !aIn)
		return;
	if (![aIn isActive] && aIn != _currentEntryOwner)
		return;
	GenericEntry *entry = [aIn currentEntry];
	if (!entry) {
		if (aIn == [self entriesListController])
			aIn = [self currentSetController];
		else if (aIn == [self currentSetController])
			aIn = [self entriesListController];
		entry = [aIn currentEntry];
		if (entry)
			_currentEntryOwner = aIn;
	} else {
		_currentEntryOwner = aIn;
	}
	if (_currentEntryOwner && _currentEntryOwner != [self entryDetailsController])
		[self showDetailsForEntry:entry openPane:NO openInfoForBillboard:0];
}

- (BOOL)_canEdit
{
	return [_leftAreaController isDetailsPaneVisible] && [[self entryDetailsController] isMounted];
}

- (void)_validateEditModeButton
{
	BOOL editing = [_leftAreaController isDetailsPaneVisible] && [[self entryDetailsController] isEditing];
	[_editEntryButton setState:editing ? NSControlStateValueOn : NSControlStateValueOff];
	[_editEntryButton setEnabled:[self _canEdit]];
}

- (void)_validateLeftAreaControls
{
	[_leftAreaButton setState:[self isLeftAreaVisible] ? NSControlStateValueOn : NSControlStateValueOff];
	[_currentSetButton setState:[_leftAreaController isCurrentSetPaneVisible] ? NSControlStateValueOn : NSControlStateValueOff];
	[_entryDetailsButton setState:[_leftAreaController isDetailsPaneVisible] ? NSControlStateValueOn : NSControlStateValueOff];
	[_currentSetButton setEnabled:![_leftAreaController isSettingsMode]];
	[_entryDetailsButton setEnabled:![_leftAreaController isSettingsMode]];
	[self _validateEditModeButton];
}

// The mouse behaviour's control tag: 0 for the behaviour 1, 1 for 0, 2 for 2.
static NSInteger _controlTagForMouseBehaviour(int aBehaviour)
{
	if (aBehaviour == 1)
		return 0;
	return aBehaviour == 0 ? 1 : 2;
}

- (void)_validateInterface
{
	[_glView setNeedsDisplay:YES];
	[_mapModeButtons selectCellWithTag:Preferences::mapMode() != 0];
	[_mouseBehaviourButtons selectCellWithTag:_controlTagForMouseBehaviour(Preferences::mouseBehaviourOnMap())];
	[_weatherButton setState:Preferences::doShowWeatherOnStartup() ? NSControlStateValueOn : NSControlStateValueOff];
}

- (void)_appDidBecomeActive
{
	if ([self isInFullWindowMode])
		[_fullScreenWindow setFrame:[[_fullScreenWindow screen] frame] display:YES];
}

- (void)_glViewFrameDidChange
{
	LNotificationCenter::center()->postNotification(IWindowInterface::frameWasChangedNotification,
	                                                _wndInterface->acceleratedView(), (LMap *)0);
}

- (void)_setMouseBehaviourWithControlTag:(NSInteger)aTag
{
	switch (aTag) {
	case 0:
		Preferences::setMouseBehaviourOnMap((TMapMouseMode)1);
		break;
	case 1:
		Preferences::setMouseBehaviourOnMap((TMapMouseMode)0);
		break;
	case 2:
		Preferences::setMouseBehaviourOnMap((TMapMouseMode)2);
		break;
	}
}

- (void)_newLocationCoordsPickerModeChanged
{
	if (!_wndInterface)
		return;
	BOOL enabled = [[[AtlasController atlasController] newLocationController] isCoordsPickerEnabled];
	if (enabled)
		[_mainWindow makeKeyAndOrderFront:nil];
	_wndInterface->setCoordinatesPickerMode(enabled);
}

- (void)_newLocationPaletteDidClose
{
	if (_wndInterface)
		_wndInterface->hideAnimatedMarker();
}

- (void)addEntryToCurrentSet:(GenericEntry *)aSet
{
	_isAddingEntryToCurrentSet = YES;
	[[self currentSetController] addEntryToRoute:aSet];
	[self showDetailsForEntry:aSet openPane:YES openInfoForBillboard:0];
	_isAddingEntryToCurrentSet = NO;
}

- (void)awakeFromNib
{
	_wndInterface = new AppWindowInterfaceWrapper(self);
	if (!_wndInterface->isVideoCardSupported() && Preferences::doShowBadVideoAlert()) {
		WBAlert *alert = [[[WBAlert alloc] initWithTitle:NSLocalizedString(@"UnsupportedVideoCardTitle", @"")
		                                            body:NSLocalizedString(@"UnsupportedVideoCardMessage", @"")
		                                       rightButt:NSLocalizedString(@"LaunchButt", @"")
		                                      middleButt:NSLocalizedString(@"QuitButt", @"")
		                                        leftButt:nil
		                                   checkboxTitle:NSLocalizedString(@"DontShowAlertAgain", @"")
		                                     hasHelpButt:NO
		                                    defButtIndex:1
		                                 cancelButtIndex:2
		                                    hasCustomBgr:NO] autorelease];
		NSInteger button = [alert runModal];
		Preferences::setShowBadVideoAlert(![alert dontShowAgain]);
		if (button != 1)
			[NSApp terminate:nil];
		[AtlasController showSplashScreen];
	}
	CContextCaps *caps = _wndInterface->contextCaps();
	CGFloat maxSize = caps ? (CGFloat)caps->GetMaxTextureSize() : 1024.0;
	[_mainWindow setMaxSize:NSMakeSize(maxSize, maxSize)];
	if (![_mainWindow setFrameUsingName:[_mainWindow frameAutosaveName]])
		[_mainWindow zoom:nil];
	[_mainSplitView retain];
	[_mainSplitView setDividerThickness:1.0];
	[_mainSplitView setDividerFillColor:[NSColor blackColor]];
	[self _setLeftWidth:kLeftAreaWidth forSplitView:_mainSplitView];
	[self _setupToolbar];
	[self _setupBottomArea];
	[self _setupCustomTitle];
	[self _setupScaleSlider];
	[_mainWindow setAcceptsMouseMovedEvents:YES];
	[_glView setPostsFrameChangedNotifications:YES];
	if (!Preferences::isLeftAreaVisible())
		[self toggleLeftArea:nil];

	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center addObserver:self selector:@selector(_glViewFrameDidChange) name:NSViewFrameDidChangeNotification object:_glView];
	[center addObserver:self
	           selector:@selector(_currentSetRouteWasAdded:)
	               name:_WGCurrentSetRouteWasAddedNotification
	             object:[self currentSetController]];
	[center addObserver:self
	           selector:@selector(_validateEditModeButton)
	               name:_WGEntryDetailsControllerAttributesDidChangeNotification
	             object:[self entryDetailsController]];
	[center addObserver:self selector:@selector(_appDidBecomeActive) name:NSApplicationDidBecomeActiveNotification object:nil];
	[center addObserver:self selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification object:nil];
	[center addObserver:self selector:@selector(windowDidResignKey:) name:NSWindowDidResignKeyNotification object:nil];
	id newLocationController = [[AtlasController atlasController] newLocationController];
	[center addObserver:self
	           selector:@selector(_newLocationCoordsPickerModeChanged)
	               name:_WGNewLocationCoordsPickerModeChangedNotification
	             object:newLocationController];
	[center addObserver:self
	           selector:@selector(_newLocationPaletteDidClose)
	               name:_WGPaletteDidCloseNotification
	             object:newLocationController];
	LNotificationCenter::center()->addObserver(self, @selector(_validateInterface),
	                                           Preferences::kDrawOptionsWereChanged, (LObject *)0);
	[self _validateInterface];
	[self _validateLeftAreaControls];
	[self _currentSetRouteWasAdded:nil];
	[self showDetailsForEntry:Preferences::homeCity() openPane:NO openInfoForBillboard:0];
}

// Moves the GL view into a borderless window over the screen, or back.
- (void)toggleFullWindowMode:(id)aMode
{
	if ([self isInFullWindowMode]) {
		[_mainWindow makeKeyAndOrderFront:nil];
		NSPoint origin = [_mainWindow convertBaseToScreen:[_rightAreaView convertPoint:_glViewRect.origin toView:nil]];
		NSRect frame = [_fullScreenWindow frameRectForContentRect:NSMakeRect(origin.x, origin.y, _glViewRect.size.width,
		                                                                      _glViewRect.size.height)];
		[_fullScreenWindow setFrame:frame display:YES animate:YES];
		NSDisableScreenUpdates();
		[_rightAreaView addSubview:_glView];
		[_fullScreenWindow setOpaque:NO];
		[_fullScreenWindow orderOut:nil];
		[_glView setFrame:_glViewRect];
		_wndInterface->exitFullScreenMode();
		[_glView display];
		NSEnableScreenUpdates();
		[_mainWindow makeFirstResponder:_glView];
		return;
	}

	// A hidden main window first fits the screen, as the animation starts
	// from it.
	if (![_mainWindow isVisible]) {
		NSArray *screens = [NSScreen screens];
		NSScreen *screen = [screens count] ? [screens objectAtIndex:0] : [NSScreen mainScreen];
		NSRect visibleFrame = [screen visibleFrame];
		NSRect frame = [_mainWindow frame];
		if (frame.size.height > visibleFrame.size.height) {
			NSSize minSize = [_mainWindow minSize];
			[_mainWindow setFrame:NSMakeRect(frame.origin.x, MAX(visibleFrame.origin.y, frame.origin.y), frame.size.width,
			                                 MAX(minSize.height, visibleFrame.size.height))
			              display:NO];
		}
	}
	_glViewRect = [_glView frame];
	NSPoint origin = [_mainWindow convertBaseToScreen:[_rightAreaView convertPoint:_glViewRect.origin toView:nil]];
	NSRect contentRect = NSMakeRect(origin.x, origin.y, _glViewRect.size.width, _glViewRect.size.height);
	if (!_fullScreenWindow) {
		_fullScreenWindow = [[_WGFullScreenWindow alloc] initWithContentRect:contentRect
		                                                           styleMask:NSWindowStyleMaskBorderless
		                                                             backing:NSBackingStoreBuffered
		                                                               defer:NO];
		[_fullScreenWindow setDelegate:(id)self];
		[_fullScreenWindow setAcceptsMouseMovedEvents:YES];
		[_fullScreenWindow setLevel:CGWindowLevelForKey(kCGPopUpMenuWindowLevelKey)];
		[_fullScreenWindow setReleasedWhenClosed:NO];
		[_fullScreenWindow setHidesOnDeactivate:YES];
		[_fullScreenWindow setBackgroundColor:[NSColor clearColor]];
		[_fullScreenWindow setHasShadow:NO];
		[_fullScreenWindow setOpaque:NO];
	} else {
		[_fullScreenWindow setFrame:[_fullScreenWindow frameRectForContentRect:contentRect] display:NO];
	}
	[_fullScreenWindow makeKeyAndOrderFront:nil];
	NSRect screenFrame = [[_mainWindow screen] frame];
	if (![_mainWindow isVisible])
		[_fullScreenWindow setFrame:screenFrame display:YES];
	if (Preferences::mapMode() == kMapModeLogo)
		[NSCursor setHiddenUntilMouseMoves:YES];
	NSDisableScreenUpdates();
	[[_fullScreenWindow contentView] addSubview:_glView];
	[_glView setFrame:[[_fullScreenWindow contentView] bounds]];
	[_fullScreenWindow setOpaque:YES];
	[_fullScreenWindow displayIfNeeded];
	NSEnableScreenUpdates();
	if ([_mainWindow isVisible])
		[_fullScreenWindow setFrame:screenFrame display:YES animate:YES];
	[_fullScreenWindow makeFirstResponder:_glView];
	_wndInterface->enterFullScreenMode();
	[_mainWindow orderOut:nil];
}

// The toolbar goes over the top of the GL view.
- (void)_setupToolbar
{
	NSView *toolbarView = [[self toolbarController] toolbarView];
	CGFloat toolbarHeight = [toolbarView frame].size.height;
	NSRect glFrame = [_glView frame];
	NSRect bounds = [[_glView superview] bounds];
	CGFloat width = bounds.size.width + bounds.origin.x - glFrame.origin.x;
	[toolbarView setFrame:NSMakeRect(glFrame.origin.x, glFrame.origin.y, width, toolbarHeight)];
	[_glView setFrame:NSMakeRect(glFrame.origin.x, glFrame.origin.y + toolbarHeight, width,
	                             glFrame.size.height - toolbarHeight)];
	[[_glView superview] addSubview:toolbarView];
}

// The globe shows the current set's route, and zooms to a newly added one.
- (void)_currentSetRouteWasAdded:(id)aAdded
{
	CRoutePointsContainer *routePoints = 0;
	if (_wndInterface) {
		SharedContextObjectsContainer *objects = _wndInterface->contextObjectsContainer();
		if (objects)
			routePoints = objects->routePointsContainer();
	}
	RouteObject *route = [[self currentSetController] currentRoute];
	if (routePoints && route)
		routePoints->setCurrentRoute(route);
	if (aAdded && route && route->itemsCount() != 0)
		_wndInterface->currentPanel()->zoomToGeoRect(route->boundsGeoRect(), 1);
}

- (void)routeToggleFlying:(id)aFlying
{
	AtlasPanelController *panel = _wndInterface->currentPanel();
	if ([aFlying state] == NSControlStateValueOff)
		panel->stopFlying();
	else
		panel->startFlying();
}

- (void)mouseBehaviourChanged:(id)aChanged
{
	[self _setMouseBehaviourWithControlTag:[[aChanged keyCell] tag]];
	_wndInterface->currentPanel()->updateCursor();
}

- (void)scaleWasChanged:(id)aChanged
{
	_wndInterface->currentPanel()->setScaleFromUI([aChanged floatValue]);
}

// Tag 1 zooms all the way out, the other one all the way in.
- (void)scaleButtonAction:(id)aAction
{
	_wndInterface->currentPanel()->setScale([aAction tag] != 1 ? 1.0f : 0.0f);
}

- (void)resetGLView:(id)aView
{
	if (_wndInterface && _wndInterface->currentPanel())
		_wndInterface->currentPanel()->resetView();
}

// The zoom items zoom the route manager's preview while it is visible.
- (void)viewControlMenu:(id)aMenu
{
	NSInteger tag = [aMenu tag];
	switch (tag) {
	case kViewZoomIn:
	case kViewZoomOut: {
		WGRouteManagerController *routeManager = [[AtlasController atlasController] routeManagerPaletteController];
		if ([routeManager isPaletteVisible]) {
			if (tag == kViewZoomIn)
				[[routeManager earthPreview] zoomIn:aMenu];
			else
				[[routeManager earthPreview] zoomOut:aMenu];
			break;
		}
		AtlasPanelController *panel = _wndInterface->currentPanel();
		float scale = panel->scale() + (tag == kViewZoomIn ? kScaleStep : -kScaleStep);
		panel->setScale(scale >= 0.0f ? MIN(1.0f, scale) : 0.0f);
		break;
	}
	case kViewToggleMapMode:
		_wndInterface->openPanel(Preferences::mapMode() == 0);
		break;
	case kViewToggleSettings:
		[_leftAreaController switchToSettingsMode:![_leftAreaController isSettingsMode]];
		break;
	case kViewMouseBehaviourFirst:
	case kViewMouseBehaviourFirst + 1:
	case kViewMouseBehaviourLast:
		[self _setMouseBehaviourWithControlTag:tag - kViewMouseBehaviourFirst];
		break;
	case kViewFullWindow:
		[self toggleFullWindowMode:self];
		break;
	}
}

- (void)drawElementsMenu:(id)aMenu
{
	AtlasPanelController *panel = _wndInterface ? _wndInterface->currentPanel() : 0;
	switch ([aMenu tag]) {
	case kDrawImagesOnMap:
		Preferences::setShowImagesOnMapEnabled(!Preferences::showImagesOnMapEnabled());
		break;
	case kDrawDayNight:
		Preferences::setDayNightEnabled(!Preferences::dayNightIsEnabled());
		break;
	case kDrawGrid:
		Preferences::setGridEnabled(!Preferences::gridEnabled());
		break;
	case kDrawWeatherOnMap:
		Preferences::setShowWeatherOnMapEnabled(!Preferences::showWeatherOnMapEnabled());
		break;
	case kDrawClouds:
		Preferences::setCloudsEnabled(!Preferences::cloudsEnabled(), true);
		break;
	case kDrawMoviePalette:
		[[[AtlasController atlasController] movieController] togglePalette];
		break;
	case kDrawTimeLinePalette:
		[[[AtlasController atlasController] timeLineController] togglePalette];
		break;
	case kDrawNavigator:
		if (panel)
			panel->toggleNavDevice();
		break;
	case kDrawInfoDevice:
		if (panel)
			panel->toggleInfo();
		break;
	case kDrawMinimap:
		if (panel)
			panel->cycleMinimapMode();
		break;
	case kDrawLockNormal:
		if (panel)
			panel->setLockMode(panel->_lockMode != kEarthLockNormal ? kEarthLockNormal : kEarthLockNone);
		break;
	case kDrawLockTilted:
		if (panel)
			panel->setLockMode(panel->_lockMode != kEarthLockTilted ? kEarthLockTilted : kEarthLockNone);
		break;
	case kDrawSettingsPalette:
		[[[AtlasController atlasController] settingsPaletteController] togglePalette];
		break;
	}
}

static NSString *_showHideTitle(BOOL aShown, NSString *aHide, NSString *aShow)
{
	return NSLocalizedString(aShown ? aHide : aShow, @"");
}

- (BOOL)validateMenuItem:(id)aItem
{
	NSInteger tag = [aItem tag];
	SEL action = [aItem action];
	AtlasPanelController *panel = _wndInterface ? _wndInterface->currentPanel() : 0;

	if (action == @selector(toggleLeftArea:)) {
		[aItem setTitle:_showHideTitle([self isLeftAreaVisible], @"Hide Left Area", @"Show Left Area")];
		return ![self isInFullWindowMode];
	}
	if (action == @selector(resetGLView:))
		return Preferences::mapMode() == 0;

	if (action == @selector(drawElementsMenu:)) {
		switch (tag) {
		case kDrawImagesOnMap:
			[aItem setState:Preferences::showImagesOnMapEnabled()];
			break;
		case kDrawDayNight:
			[aItem setState:Preferences::dayNightIsEnabled()];
			break;
		case kDrawGrid:
			[aItem setState:Preferences::gridEnabled()];
			break;
		case kDrawWeatherOnMap:
			[aItem setState:Preferences::showWeatherOnMapEnabled()];
			break;
		case kDrawClouds:
			[aItem setState:Preferences::cloudsEnabled()];
			break;
		case kDrawMoviePalette:
			[aItem setState:[[[AtlasController atlasController] movieController] isPaletteVisible]];
			break;
		case kDrawTimeLinePalette:
			[aItem setState:[[[AtlasController atlasController] timeLineController] isPaletteVisible]];
			break;
		case kDrawNavigator:
			[aItem setTitle:_showHideTitle((panel ? panel->navDevicesVisibility() : 0.0f) > 0.0f, @"Hide Navigator",
			                               @"Show Navigator")];
			break;
		case kDrawInfoDevice:
			[aItem setState:(panel ? panel->infoDeviceVisibility() : 0.0f) > 0.0f];
			break;
		case kDrawMinimap:
			[aItem setState:(panel ? panel->minimapVisibility() : 0.0f) > 0.0f];
			break;
		case kDrawLockNormal:
			[aItem setState:(panel ? panel->_lockMode : kEarthLockNone) == kEarthLockNormal];
			return Preferences::mapMode() == 0;
		case kDrawLockTilted:
			[aItem setState:(panel ? panel->_lockMode : kEarthLockNone) == kEarthLockTilted];
			return Preferences::mapMode() == 0;
		case kDrawSettingsPalette:
			[aItem setState:[[[AtlasController atlasController] settingsPaletteController] isPaletteVisible]];
			return ![self isInFullWindowMode];
		}
		return YES;
	}

	// The space elements show on the globe only.
	if (action == @selector(spaceMenu:)) {
		switch (tag) {
		case kSpaceAll:
			[aItem setTitle:_showHideTitle(Preferences::spaceEnabled(), @"Hide Elements", @"Show Elements")];
			break;
		case kSpaceStars:
			[aItem setState:Preferences::starsEnabled()];
			break;
		case kSpaceMoon:
			[aItem setState:Preferences::moonEnabled()];
			break;
		case kSpaceSun:
			[aItem setState:Preferences::sunEnabled()];
			break;
		case kSpacePlanets:
			[aItem setState:Preferences::planetsEnabled()];
			break;
		case kSpaceConstellations:
			[aItem setState:Preferences::constellationsEnabled()];
			break;
		}
		return Preferences::mapMode() == 0;
	}
	if (action == @selector(gridMenu:))
		return NO;

	if (action == @selector(regionHighlightMenu:)) {
		if (tag == kRegionAllBorders)
			[aItem setTitle:_showHideTitle(Preferences::showAllBorders(), @"Hide Boundaries", @"Show Boundaries")];
		else if (tag == kRegionHighlightAnyway)
			[aItem setState:Preferences::highlightBordersAnyway()];
		else if (LString *shapeID = _shapeIDForRegionTag(tag))
			[aItem setState:Preferences::shapesDrawEnabled(shapeID)];
		return YES;
	}

	if (action == @selector(locationsMenu:)) {
		switch (tag) {
		case kLocationsAll:
			[aItem setTitle:_showHideTitle(Preferences::showAllPoints(), @"Hide Locations", @"Show Locations")];
			break;
		case kLocationsCapitals:
			[aItem setState:Preferences::showCapitalPoints()];
			break;
		case kLocationsAppleStores:
			[aItem setState:Preferences::showAppleStorePoints()];
			break;
		case kLocationsOther:
			[aItem setState:Preferences::showOtherPoints()];
			break;
		}
		return YES;
	}

	if (action == @selector(cloudsMenu:)) {
		if (!Preferences::cloudsEnabled()) {
			[aItem setState:tag == kCloudsOff];
		} else if (tag == kCloudsOff) {
			[aItem setState:NSControlStateValueOff];
		} else if (tag >= kCloudsQualityLow && tag <= kCloudsQualityHigh) {
			[aItem setState:Preferences::cloudsAnimationQuality() == tag - kCloudsQualityLow];
		}
		return YES;
	}

	if (action == @selector(viewControlMenu:)) {
		switch (tag) {
		case kViewZoomIn:
		case kViewZoomOut: {
			WGRouteManagerController *routeManager = [[AtlasController atlasController] routeManagerPaletteController];
			if ([routeManager isPaletteVisible]) {
				WGEarthView *preview = [routeManager earthPreview];
				return tag == kViewZoomIn ? [preview canZoomIn] : [preview canZoomOut];
			}
			float scale = panel->scale();
			return tag == kViewZoomIn ? scale < 1.0f : scale > 0.0f;
		}
		case kViewMouseBehaviourFirst:
		case kViewMouseBehaviourFirst + 1:
		case kViewMouseBehaviourLast:
			[aItem setState:_controlTagForMouseBehaviour(Preferences::mouseBehaviourOnMap()) == tag - kViewMouseBehaviourFirst];
			break;
		case kViewFullWindow:
			[aItem setState:[self isInFullWindowMode]];
			break;
		}
		return YES;
	}

	if (action == @selector(routeMenu:)) {
		if (tag == kRouteToggleFlying) {
			[aItem setState:panel->_flying];
			RouteObject *route = [[self currentSetController] currentRoute];
			return route && route->locationsCount() > 1;
		}
		if (tag == kRoutePrintLocationInfo)
			return [[self entryDetailsController] currentEntry] != 0;
		if (tag == kRouteImport)
			return ![self isInFullWindowMode];
		BOOL valid = [[self currentSetController] validateRouteCommand:aItem];
		if (tag == kRouteExport || tag == kRouteChangeColor)
			return valid && ![self isInFullWindowMode];
		return valid;
	}

	if (action == @selector(entriesListMenu:)) {
		if (tag == kEntriesListPrintLocationInfo)
			return [[self entryDetailsController] currentEntry] != 0;
		return [[self entriesListController] validateEntriesListCommand:aItem];
	}

	if (action == @selector(editEntryAction:)) {
		[aItem setState:[[self entryDetailsController] isEditing]];
		return [self _canEdit];
	}

	if (action == @selector(leftArea:)) {
		switch (tag) {
		case kLeftAreaCurrentSet:
			[aItem setTitle:_showHideTitle([_leftAreaController isCurrentSetPaneVisible], @"Hide Current Set",
			                               @"Show Current Set")];
			return ![_leftAreaController isSettingsMode];
		case kLeftAreaDetails:
			[aItem setTitle:_showHideTitle([_leftAreaController isDetailsPaneVisible], @"Hide Details", @"Show Details")];
			return ![_leftAreaController isSettingsMode];
		case kLeftAreaSettings:
			[aItem setState:[_leftAreaController isSettingsMode]];
			break;
		}
		return YES;
	}

	if (action == @selector(_paletteActionsMenu:)) {
		if (tag != kPaletteActionAddSelected)
			return NO;
		CIPaletteController *palette = [[AtlasController atlasController] citiesPaletteController];
		TLArray<GenericEntry*> *listEntries =
		    [[self entriesListController] selectedItemsWithLimit:Preferences::citiesCountLimit() * 2 createIfNeeded:YES];
		return [palette canAddEntries:listEntries]
		    || [palette canAddEntries:[(WGCurrentSetController *)[self currentSetController] selectedItems]];
	}

	if (action == @selector(onAddNewEntryButton:) || action == @selector(exportAsImage:)
	    || action == @selector(runPageLayout:) || action == @selector(doPrint:))
		return ![self isInFullWindowMode];
	return YES;
}

- (void)routeMenu:(id)aMenu
{
	NSInteger tag = [aMenu tag];
	switch (tag) {
	case kRouteToggleFlying: {
		AtlasPanelController *panel = _wndInterface->currentPanel();
		if (panel->_flying)
			panel->stopFlying();
		else
			panel->startFlying();
		return;
	}
	case kRoutePrintCurrentSet:
		[self printCurrentSetOnly];
		return;
	case kRoutePrintLocationInfo:
		[self printLocationInfoOnly];
		return;
	case kRouteImport:
		if (![[self currentSetController] isMounted])
			[_leftAreaController showCurrentSet:YES];
		break;
	}
	[[self currentSetController] doRouteCommand:tag];
}

// The details pane's editable fields share a field editor undoing with the
// application's undo manager.
- (id)windowWillReturnFieldEditor:(id)aEditor toObject:(id)aObject
{
	if ([aObject isKindOfClass:[NSView class]]
	    && [aObject isDescendantOf:[[self entryDetailsController] pane]]
	    && [aObject respondsToSelector:@selector(isEditable)]
	    && [aObject isEditable]) {
		if (!_fieldEditor) {
			_fieldEditor = [WGCustomFieldEditor new];
			[_fieldEditor setFieldEditor:YES];
			[_fieldEditor setUndoManager:LUndoManager::instance()];
		}
		return _fieldEditor;
	}
	return nil;
}

@end

#pragma clang diagnostic pop
