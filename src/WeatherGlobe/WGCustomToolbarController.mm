#import "WGCustomToolbarController.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "WGPaletteBaseController.h"
#import "WGCustomSlider.h"
#import "WGCloudsPianino.h"
#import "WGTexturedView.h"
#import "WGCustomToolbarButtonCell.h"
#import "NSMenu+NSMenuWGExtensions.h"

// The toolbar cells' tags.
enum {
	kToolbarSpace = 0,
	kToolbarDayNight = 1,
	kToolbarGrid = 2,
	kToolbarBorders = 3,
	kToolbarPoints = 4,
	kToolbarWeatherOnMap = 5,
	kToolbarClouds = 6,
	kToolbarMovie = 7,
	kToolbarTimeLine = 8,
	kToolbarSettings = 10,
	kToolbarImagesOnMap = 11
};

// The clouds of the last 120 hours can be gone back to.
static const int kBackInTimeHours = 120;

@implementation WGCustomToolbarController

- (CloudsController *)_cloudsController
{
	AppWindowInterfaceWrapper *window = [[[AtlasController atlasController] mainWindowController] windowInterface];
	SharedContextObjectsContainer *container = window ? window->contextObjectsContainer() : 0;
	return container ? container->cloudsController() : 0;
}

// Cells that don't fit are kept by columns in _hiddenCells.
- (id)_toolbarCelWithTag:(NSInteger)aTag
{
	id cell = [_toolbar cellWithTag:aTag];
	if (cell)
		return cell;
	int columnsCount = (int)[_hiddenCells count];
	for (int i = 0; i < columnsCount; i++) {
		NSArray *column = [_hiddenCells objectAtIndex:i];
		int count = (int)[column count];
		for (int j = 0; j < count; j++) {
			id hidden = [column objectAtIndex:j];
			if ([hidden tag] == aTag)
				return hidden;
		}
	}
	return nil;
}

- (void)_actionWithTag:(NSInteger)aTag
{
	WGPaletteBaseController *palette;
	switch (aTag) {
	case kToolbarSpace:
		Preferences::setSpaceEnabled(!Preferences::spaceEnabled());
		return;
	case kToolbarDayNight:
		Preferences::setDayNightEnabled(!Preferences::dayNightIsEnabled());
		return;
	case kToolbarGrid:
		Preferences::setGridEnabled(!Preferences::gridEnabled());
		return;
	case kToolbarBorders:
		Preferences::setShowAllBorders(!Preferences::showAllBorders());
		return;
	case kToolbarPoints:
		Preferences::setShowAllPoints(!Preferences::showAllPoints());
		return;
	case kToolbarWeatherOnMap:
		Preferences::setShowWeatherOnMapEnabled(!Preferences::showWeatherOnMapEnabled());
		return;
	case kToolbarClouds:
		Preferences::setCloudsEnabled(!Preferences::cloudsEnabled(), true);
		return;
	case kToolbarMovie:
		palette = [[AtlasController atlasController] movieController];
		break;
	case kToolbarTimeLine:
		palette = [[AtlasController atlasController] timeLineController];
		break;
	case kToolbarSettings:
		palette = [[AtlasController atlasController] settingsPaletteController];
		break;
	case kToolbarImagesOnMap:
		Preferences::setShowImagesOnMapEnabled(!Preferences::showImagesOnMapEnabled());
		return;
	default:
		return;
	}
	[palette togglePalette];
}

- (void)_validateToolbarCell:(id)aCell
{
	BOOL state;
	WGPaletteBaseController *palette;
	switch ([aCell tag]) {
	case kToolbarSpace:
		[aCell setState:Preferences::spaceEnabled() != 0];
		// The space is only around the globe.
		[aCell setEnabled:Preferences::mapMode() == 0];
		return;
	case kToolbarDayNight:
		state = Preferences::dayNightIsEnabled();
		break;
	case kToolbarGrid:
		state = Preferences::gridEnabled();
		break;
	case kToolbarBorders:
		state = Preferences::showAllBorders();
		break;
	case kToolbarPoints:
		state = Preferences::showAllPoints();
		break;
	case kToolbarWeatherOnMap:
		state = Preferences::showWeatherOnMapEnabled();
		break;
	case kToolbarClouds:
		state = Preferences::cloudsEnabled();
		break;
	case kToolbarMovie:
		palette = [[AtlasController atlasController] movieController];
		state = [palette isPaletteVisible];
		break;
	case kToolbarTimeLine:
		palette = [[AtlasController atlasController] timeLineController];
		state = [palette isPaletteVisible];
		break;
	case kToolbarSettings:
		palette = [[AtlasController atlasController] settingsPaletteController];
		state = [palette isPaletteVisible];
		break;
	case kToolbarImagesOnMap:
		state = Preferences::showImagesOnMapEnabled();
		break;
	default:
		return;
	}
	[aCell setState:state != NO];
}

- (void)_validateSpaceControlls
{
	[[_spaceSettingsMatrix cellWithTag:0] setState:Preferences::moonEnabled() != 0];
	[[_spaceSettingsMatrix cellWithTag:1] setState:Preferences::sunEnabled() != 0];
	[[_spaceSettingsMatrix cellWithTag:2] setState:Preferences::planetsEnabled() != 0];
	[[_spaceSettingsMatrix cellWithTag:3] setState:Preferences::starsEnabled() != 0];
	[[_spaceSettingsMatrix cellWithTag:4] setState:Preferences::constellationsEnabled() != 0];
	[_planetsNames setState:Preferences::showPlanetsNamesEnabled() != 0];
	[_constellationsNames setState:Preferences::showConstellationsNamesEnabled() != 0];
}

// The slider runs from the oldest clouds on the left to the newest.
- (void)_validateBITSliderValue
{
	if (_isBITSliderTracking)
		return;
	CloudsController *clouds = [self _cloudsController];
	if (!clouds)
		return;
	int offset = clouds->currentBackInTimeOffset();
	[_cloudsBackInTimeSlider setFloatValue:(float)([_cloudsBackInTimeSlider maxValue] - (double)offset)];
	[_cloudsPlayPauseButton setState:!clouds->_paused];
}

- (void)_updateCloudsPianino
{
	[_cloudsPianino setNeedsDisplay:YES];
}

- (void)_validateCountriesControlls
{
	[[_countriesSettingsMatrix cellWithTag:0] setState:Preferences::shapesDrawEnabled(CShapeManager::kShapeIDCountries) != 0];
	[[_countriesSettingsMatrix cellWithTag:1] setState:Preferences::shapesDrawEnabled(CShapeManager::kShapeIDStates) != 0];
	[[_countriesSettingsMatrix cellWithTag:2] setState:Preferences::shapesDrawEnabled(CShapeManager::kShapeIDTimezones) != 0];
	[_highlightAlwaysButt setState:Preferences::highlightBordersAnyway() != 0];
}

- (void)_validateSettingsControlls
{
	// The globe's transparency and sounds are for the globe only (the
	// sounds also needed 10.4, which is always there now).
	BOOL globe = Preferences::mapMode() == 0;
	[_globeTransparency setFloatValue:Preferences::globeTransparencyLevel()];
	[_globeTransparency setEnabled:globe];
	[_soundVolume setFloatValue:Preferences::soundVolume()];
	[_soundVolume setEnabled:globe];
	[_spinSpeed setFloatValue:Preferences::automoveSpeedSliderValue()];
	[_markerSize setFloatValue:Preferences::markersScale()];
	[_locationNames setState:Preferences::showLocationNamesEnabled() != 0];
}

- (void)_validateLocationsControlls
{
	[[_locationsSettingsMatrix cellWithTag:0] setState:Preferences::showCapitalPoints() != 0];
	[[_locationsSettingsMatrix cellWithTag:1] setState:Preferences::showAppleStorePoints() != 0];
	[[_locationsSettingsMatrix cellWithTag:2] setState:Preferences::showOtherPoints() != 0];
	[_nearestPoint setState:Preferences::showNearestPoint() != 0];
}

- (void)_validateToolbar
{
	NSArray *cells = [_toolbar cells];
	int count = (int)[cells count];
	for (int i = 0; i < count; i++)
		[self _validateToolbarCell:[cells objectAtIndex:i]];
	int columnsCount = (int)[_hiddenCells count];
	for (int i = 0; i < columnsCount; i++) {
		NSArray *column = [_hiddenCells objectAtIndex:i];
		int columnCount = (int)[column count];
		for (int j = 0; j < columnCount; j++)
			[self _validateToolbarCell:[column objectAtIndex:j]];
	}
}

- (void)_setupCustomSlider:(id)aSlider
{
	if (![aSlider numberOfTickMarks]) {
		[aSlider setKnobImage:[NSImage imageNamed:@"MWT_sliderknob"]];
		[aSlider setKnobDisabledImage:[NSImage imageNamed:@"MWT_sliderknob_disabl"]];
		[aSlider setKnobAlternateImage:[NSImage imageNamed:@"MWT_sliderknob_on"]];
	}
	else {
		[aSlider setKnobImage:[NSImage imageNamed:@"Knob_Normal"]];
		[aSlider setKnobDisabledImage:[NSImage imageNamed:@"Knob_Disable"]];
		[aSlider setKnobAlternateImage:[NSImage imageNamed:@"Knob_press"]];
	}
	[aSlider setBarFirstEdgeImage:[NSImage imageNamed:@"MWT_sliderback_left"]];
	[aSlider setBarMiddleImage:[NSImage imageNamed:@"MWT_sliderback_center"]];
	[aSlider setBarLastEdgeImage:[NSImage imageNamed:@"MWT_sliderback_right"]];
	[aSlider setBarDisabledFirstEdgeImage:[NSImage imageNamed:@"MWT_sliderback_left_disabl"]];
	[aSlider setBarDisabledMiddleImage:[NSImage imageNamed:@"MWT_sliderback_center_disabl"]];
	[aSlider setBarDisabledLastEdgeImage:[NSImage imageNamed:@"MWT_sliderback_right_disabl"]];
}

// One tick per clouds update; the pianino under the slider shows which
// frames are there.
- (void)_setupBackInTimeSlider
{
	[self _setupCustomSlider:_cloudsBackInTimeSlider];
	[_cloudsBackInTimeSlider setDrawsTickMarks:NO];
	int updateTime = Preferences::cloudsUpdateTime();
	[_cloudsBackInTimeSlider setMinValue:0.0];
	[_cloudsBackInTimeSlider setMaxValue:(double)(kBackInTimeHours - updateTime)];
	[_cloudsBackInTimeSlider setNumberOfTickMarks:kBackInTimeHours / updateTime];
	_cloudsPianino = [[WGCloudsPianino alloc] initWithFrame:[_cloudsPianinoPlaceView frame]];
	NSView *superview = [_cloudsPianinoPlaceView superview];
	[_cloudsPianinoPlaceView removeFromSuperview];
	[superview addSubview:_cloudsPianino];
	[_cloudsPianino setDelegate:self];
	[_cloudsPianino setEnabled:NO];
}

- (void)_setTextColor:(id)aColor forCell:(id)aCell
{
	NSMutableAttributedString *title = [[[NSMutableAttributedString alloc] initWithAttributedString:[aCell attributedTitle]] autorelease];
	[title addAttribute:NSForegroundColorAttributeName value:aColor range:NSMakeRange(0, [title length])];
	[aCell setAttributedTitle:title];
}

- (void)_setCellsTextColor:(id)aColor forMatrix:(id)aMatrix
{
	NSArray *cells = [aMatrix cells];
	int count = (int)[cells count];
	for (int i = 0; i < count; i++)
		[self _setTextColor:aColor forCell:[cells objectAtIndex:i]];
}

- (void)awakeFromNib
{
	[self _setupCustomSlider:_cloudsAnimSpeedSlider];
	[self _setupCustomSlider:_globeTransparency];
	[self _setupCustomSlider:_soundVolume];
	[self _setupCustomSlider:_spinSpeed];
	[self _setupCustomSlider:_markerSize];
	[self _setupBackInTimeSlider];
	[_toolbarView setImage:[NSImage imageNamed:@"MWT_Background"] forTexture:1];
	[_toolbarView setPostsFrameChangedNotifications:YES];
	[_expandButton sendActionOn:NSEventMaskLeftMouseDown];
	_isDynamicIntercellWidth = YES;

	NSColor *textColor = [[NSColor whiteColor] colorWithAlphaComponent:0.95f];
	[self _setCellsTextColor:textColor forMatrix:_toolbar];
	[self _setCellsTextColor:textColor forMatrix:_spaceSettingsMatrix];
	[self _setCellsTextColor:textColor forMatrix:_countriesSettingsMatrix];
	[self _setCellsTextColor:textColor forMatrix:_locationsSettingsMatrix];
	[self _setTextColor:textColor forCell:[_clouds5DHistoryButton cell]];
	[self _setTextColor:textColor forCell:[_locationNames cell]];
	[self _setTextColor:textColor forCell:[_planetsNames cell]];
	[self _setTextColor:textColor forCell:[_constellationsNames cell]];
	[self _setTextColor:textColor forCell:[_nearestPoint cell]];
	[self _setTextColor:textColor forCell:[_highlightAlwaysButt cell]];

	[[_toolbar cellWithTag:kToolbarSpace] setPopupWindowContentViewDelegate:self];
	[[_toolbar cellWithTag:kToolbarBorders] setPopupWindowContentViewDelegate:self];
	[[_toolbar cellWithTag:kToolbarPoints] setPopupWindowContentViewDelegate:self];
	[[_toolbar cellWithTag:kToolbarClouds] setPopupWindowContentViewDelegate:self];

	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center addObserver:self selector:@selector(_toolbarViewFrameWasChanged)
		name:NSViewFrameDidChangeNotification object:_toolbarView];
	[center addObserver:self selector:@selector(_validateToolbar) name:_WGPaletteDidCloseNotification object:nil];
	[center addObserver:self selector:@selector(_validateToolbar) name:_WGPaletteDidBecomeKey object:nil];
	LNotificationCenter *lcenter = LNotificationCenter::center();
	lcenter->addObserver(self, @selector(_validateToolbar), Preferences::kDrawOptionsWereChanged, 0);
	lcenter->addObserver(self, @selector(_validateToolbar), Preferences::kEnabledBordersSetWasChanged, 0);
	lcenter->addObserver(self, @selector(_validateBITSliderValue), CloudsController::kCloudsFrameWasChanged, 0);
	lcenter->addObserver(self, @selector(_updateCloudsPianino), CWManager::kCloudsWereUpdatedNotification, 0);
	lcenter->addObserver(self, @selector(_updateCloudsPianino), Preferences::kCloudsTextureSizeWasChanged, 0);
	[self _validateToolbar];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[[_toolbar cellWithTag:kToolbarSpace] setPopupWindowContentViewDelegate:nil];
	[[_toolbar cellWithTag:kToolbarBorders] setPopupWindowContentViewDelegate:nil];
	[[_toolbar cellWithTag:kToolbarPoints] setPopupWindowContentViewDelegate:nil];
	[[_toolbar cellWithTag:kToolbarClouds] setPopupWindowContentViewDelegate:nil];
	[_cloudsPianino setDelegate:nil];
	[_cloudsPianino release];
	[_toolbarView release];
	[_hiddenCells release];
	[_expandMenu release];
	[_spaceSettingsView release];
	[_cloudsSettingsView release];
	[_countriesSettingsView release];
	[super dealloc];
}

- (void)sliderDidEndTracking:(id)aTracking
{
	if (_cloudsBackInTimeSlider != aTracking)
		return;
	CloudsController *clouds = [self _cloudsController];
	if (clouds && !_isInPauseState)
		clouds->resumeCloudsAnimation();
	_isBITSliderTracking = NO;
}

- (NSInteger)numberOfKeysInPianino:(id)aPianino
{
	return [_cloudsBackInTimeSlider numberOfTickMarks];
}

// The keys go from the oldest clouds to the newest.
- (BOOL)pianino:(id)aPianino isEnabledKeyAtIndex:(NSInteger)aIndex
{
	int updateTime = Preferences::cloudsUpdateTime();
	NSInteger keysCount = [aPianino keysCount];
	CloudsManager *manager = CWManager::instance()->cloudsManager();
	int frame = -1;
	if (manager)
		frame = manager->frameIndexForBackInTimeOffset(updateTime * (int)(keysCount - 1 - aIndex));
	return frame != -1;
}

- (BOOL)validateMenuItem:(id)aItem
{
	if ([aItem action] == @selector(_expandMenuAction:)) {
		id cell = [aItem representedObject];
		[aItem setState:[cell state]];
		return [cell isEnabled];
	}
	return [self respondsToSelector:[aItem action]];
}

- (void)_expandMenuAction:(id)aAction
{
	id cell = [aAction representedObject];
	[cell setState:[cell state] != NSControlStateValueOn];
	[self _actionWithTag:[aAction tag]];
}

// The menu of the cells hidden for want of room.
- (id)_expandMenu
{
	if (!_expandMenu) {
		_expandMenu = [[NSMenu alloc] init];
		int count = (int)[_hiddenCells count];
		for (int i = 0; i < count; i++) {
			id cell = [[_hiddenCells objectAtIndex:i] objectAtIndex:0];
			NSMenuItem *item = [[[NSMenuItem alloc] init] autorelease];
			[item setTitle:[cell title]];
			[item setTag:[cell tag]];
			[item setRepresentedObject:cell];
			[item setTarget:self];
			[item setAction:@selector(_expandMenuAction:)];
			[_expandMenu addItem:item];
		}
	}
	return _expandMenu;
}

- (CGFloat)_requiredWidthForColumnsCount:(NSInteger)aCount
{
	CGFloat spacing = 0.0;
	NSInteger gaps = aCount > 1 ? aCount - 1 : 0;
	if (!_isDynamicIntercellWidth)
		spacing = [_toolbar intercellSpacing].width;
	return (float)aCount * [_toolbar cellSize].width + spacing * (float)gaps;
}

- (id)nibName
{
	return @"CustomToolbar";
}

- (id)toolbarView
{
	[self loadNibIfNeeded];
	return _toolbarView;
}

- (void)showSettings
{
}

- (void)toolbarAction:(id)aAction
{
	[self _actionWithTag:[[aAction keyCell] tag]];
}

- (void)expandButtonAction:(id)aAction
{
	[NSMenu wgPopUpContextMenu:[self _expandMenu] withEvent:[[aAction window] currentEvent] forView:aAction];
}

- (void)spaceAction:(id)aAction
{
	switch ([aAction tag]) {
	case 1:
		Preferences::setShowPlanetsNamesEnabled([aAction state] == NSControlStateValueOn);
		return;
	case 2:
		Preferences::setShowConstellationsNamesEnabled([aAction state] == NSControlStateValueOn);
		return;
	case 0:
		switch ([[aAction keyCell] tag]) {
		case 0:
			Preferences::setMoonEnabled(!Preferences::moonEnabled());
			return;
		case 1:
			Preferences::setSunEnabled(!Preferences::sunEnabled());
			return;
		case 2:
			Preferences::setPlanetsEnabled(!Preferences::planetsEnabled());
			return;
		case 3:
			Preferences::setStarsEnabled(!Preferences::starsEnabled());
			return;
		case 4:
			Preferences::setConstellationsEnabled(!Preferences::constellationsEnabled());
			return;
		}
	}
}

- (void)cloudsAction:(id)aAction
{
	CloudsController *clouds;
	switch ([aAction tag]) {
	case 1: {
		BOOL animated = [aAction state] == NSControlStateValueOn;
		Preferences::setCloudsAnimationQuality((TCloudsAnimation)(animated ? 2 : 0));
		if (animated)
			Preferences::setCloudsEnabled(true, true);
		[self _validateCloudsControlls];
		return;
	}
	case 2:
		Preferences::setCloudsAnimationSliderValue([aAction floatValue]);
		return;
	case 3:
		clouds = [self _cloudsController];
		if (clouds)
			clouds->setBackInTimeOffset((int)([_cloudsBackInTimeSlider maxValue] - [_cloudsBackInTimeSlider floatValue]));
		return;
	case 4:
		clouds = [self _cloudsController];
		if (!clouds)
			return;
		if ([aAction state] == NSControlStateValueOff)
			clouds->pauseCloudsAnimation();
		else
			clouds->resumeCloudsAnimation();
		[self _validateCloudsControlls];
		return;
	}
}

- (void)countriesAction:(id)aAction
{
	NSInteger tag = [aAction tag];
	if (tag == 1) {
		LString *shapes;
		switch ([[aAction keyCell] tag]) {
		case 0:
			shapes = CShapeManager::kShapeIDCountries;
			break;
		case 1:
			shapes = CShapeManager::kShapeIDStates;
			break;
		case 2:
			shapes = CShapeManager::kShapeIDTimezones;
			break;
		default:
			return;
		}
		if (shapes)
			Preferences::setShapesDrawEnabled(shapes, !Preferences::shapesDrawEnabled(shapes));
	}
	else if (tag == 2)
		Preferences::setHighlightBordersAnyway([aAction state] == NSControlStateValueOn);
}

- (void)settingsAction:(id)aAction
{
	switch ([aAction tag]) {
	case 1:
		Preferences::setGlobeTransparencyLevel([aAction floatValue]);
		return;
	case 2:
		Preferences::setAutomoveSpeedSliderValue([aAction floatValue]);
		return;
	case 3:
		Preferences::setMarkersScale([aAction floatValue]);
		return;
	case 4:
		Preferences::setShowLocationNamesEnabled([aAction state] == NSControlStateValueOn);
		return;
	case 5:
		Preferences::setSoundVolume([aAction floatValue]);
		return;
	}
}

- (void)locationsAction:(id)aAction
{
	NSInteger tag = [aAction tag];
	if (tag == 1) {
		switch ([[aAction keyCell] tag]) {
		case 0:
			Preferences::setShowCapitalPoints(!Preferences::showCapitalPoints());
			return;
		case 1:
			Preferences::setShowAppleStorePoints(!Preferences::showAppleStorePoints());
			return;
		case 2:
			Preferences::setShowOtherPoints(!Preferences::showOtherPoints());
			return;
		}
	}
	else if (tag == 2)
		Preferences::setShowNearestPoint([aAction state] == NSControlStateValueOn);
}

- (id)popupWindowContentViewForCell:(id)aCell
{
	switch ([aCell tag]) {
	case kToolbarSpace:
		[self _validateSpaceControlls];
		return _spaceSettingsView;
	case kToolbarBorders:
		[self _validateCountriesControlls];
		return _countriesSettingsView;
	case kToolbarPoints:
		[self _validateLocationsControlls];
		return _locationsSettingsView;
	case kToolbarClouds:
		[self _validateCloudsControlls];
		return _cloudsSettingsView;
	}
	return nil;
}

// The clouds popup stays while its slider is played with.
- (BOOL)isPopupWindowAutoHiddenForCell:(id)aCell
{
	return [aCell tag] != kToolbarClouds;
}

// Moves the toolbar's columns that don't fit to the expand menu, and back.
- (void)_toolbarViewFrameWasChanged
{
	NSRect bounds = [_toolbarView bounds];
	NSRect expandFrame = [_expandButton frame];
	CGFloat width = bounds.size.width;
	if (NSMaxX(bounds) - expandFrame.origin.x > width)
		return;
	NSRect toolbarFrame = [_toolbar frame];
	NSInteger columns = [_toolbar numberOfColumns];
	NSUInteger hidden = [_hiddenCells count];
	if ([self _requiredWidthForColumnsCount:columns + hidden] <= width)
		[_expandButton setHidden:YES];
	else {
		[_expandButton setHidden:NO];
		width = expandFrame.origin.x - toolbarFrame.origin.x;
	}
	toolbarFrame.size.width = width;
	[_toolbar setFrame:toolbarFrame];

	while ([_hiddenCells count]
		&& [self _requiredWidthForColumnsCount:[_toolbar numberOfColumns] + 1] <= width) {
		[_toolbar addColumnWithCells:[_hiddenCells objectAtIndex:0]];
		[_hiddenCells removeObjectAtIndex:0];
	}
	while ([self _requiredWidthForColumnsCount:[_toolbar numberOfColumns]] > width) {
		NSInteger rows = [_toolbar numberOfRows];
		NSInteger last = [_toolbar numberOfColumns] - 1;
		NSMutableArray *column = [NSMutableArray arrayWithCapacity:rows];
		for (NSInteger row = 0; row < rows; row++)
			[column addObject:[_toolbar cellAtRow:row column:last]];
		if (!_hiddenCells)
			_hiddenCells = [[NSMutableArray alloc] initWithCapacity:columns + hidden];
		[_hiddenCells insertObject:column atIndex:0];
		[_toolbar removeColumn:last];
	}
	if ([_expandMenu numberOfItems] != (NSInteger)[_hiddenCells count]) {
		[_expandMenu release];
		_expandMenu = nil;
	}

	if (!_isDynamicIntercellWidth)
		return;
	// Spreads the shown columns over the whole width.
	NSSize spacing = [_toolbar intercellSpacing];
	spacing.width = 0.0;
	if (![_hiddenCells count]) {
		NSInteger count = [_toolbar numberOfColumns];
		CGFloat required = [self _requiredWidthForColumnsCount:count];
		if (count > 1)
			spacing.width = (width - required) / (float)(count - 1);
	}
	[_toolbar setIntercellSpacing:spacing];
}

- (void)sliderWillStartTracking:(id)aTracking
{
	if (_cloudsBackInTimeSlider != aTracking)
		return;
	_isBITSliderTracking = YES;
	CloudsController *clouds = [self _cloudsController];
	if (clouds) {
		_isInPauseState = clouds->_paused;
		clouds->pauseCloudsAnimation();
	}
}

- (void)_validateCloudsControlls
{
	BOOL animated = Preferences::cloudsAnimationQuality() != 0;
	[_clouds5DHistoryButton setState:animated];
	[_cloudsAnimSpeedSlider setEnabled:animated];
	[_cloudsAnimSpeedSlider setFloatValue:Preferences::cloudsAnimationSliderValue()];
	[_cloudsPlayPauseButton setEnabled:animated];
	CloudsController *clouds = [self _cloudsController];
	BOOL paused = clouds ? clouds->_paused : YES;
	[_cloudsPlayPauseButton setImage:[NSImage imageNamed:paused ? @"Button_Play_Normal" : @"Button_Pause_Press"]];
	[_cloudsPlayPauseButton setAlternateImage:[NSImage imageNamed:paused ? @"Button_Play_Press" : @"Button_Pause_Normal"]];
	[_cloudsBackInTimeSlider setEnabled:animated];
	[self _validateBITSliderValue];
}

- (id)pianino:(id)aPianino dateForKeyAtIndex:(NSInteger)aIndex
{
	CloudsManager *manager = CWManager::instance()->cloudsManager();
	LDate *current = manager ? manager->currentCloudsDate() : 0;
	if (!current)
		return nil;
	int updateTime = Preferences::cloudsUpdateTime();
	NSInteger keysCount = [aPianino keysCount];
	LTimeZone *zone = current->timeZone();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	NSCalendarDate *date = [NSCalendarDate dateWithYear:current->year() month:current->month() day:current->day()
		hour:current->hour() minute:0 second:0 timeZone:(NSTimeZone *)zone->_cf];
	return [date dateByAddingYears:0 months:0 days:0 hours:-(updateTime * (int)(keysCount - 1 - aIndex))
		minutes:0 seconds:0];
#pragma clang diagnostic pop
}

@end
