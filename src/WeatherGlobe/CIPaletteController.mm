// CIPaletteController
#import "CIPaletteController.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "CIPaletteWindow.h"
#import "CIPaletteView.h"
#import "CIPaletteTitleView.h"
#import "CIBackgroundImageView.h"
#import "CityInfoController.h"
#import "CityInfoView.h"
#import "PrintController.h"
#import "WeatherPalettePrintDataSourceProvider.h"
#import "NSMenu+NSMenuWGExtensions.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

// The tag of the application menu's Weather Palette item.
enum { kWeatherPaletteMenuTag = 1980 };

// _paletteActionsMenu: tags.
enum {
	kPaletteActionAddSelected = 1,
	kPaletteActionUpdateWeather = 2,
	kPaletteActionRemoveSelected = 3,
	kPaletteActionRestore = 4,
	kPaletteActionPrintShort = 5,
	kPaletteActionPrintExtended = 6
};

// _fontSizeMenu: tags.
enum { kFontSizeMini = 0, kFontSizeSmall = 1, kFontSizeRegular = 2 };

@implementation CIPaletteController

- (void)_loadNibIfNeeded
{
	static BOOL wasHere = NO;
	if (wasHere)
		return;
	wasHere = YES;
	[self awakeFromNib];
}

// Renders one city view's cached image per tick.
- (void)_cacheNextImageIfNeeded
{
	static NSUInteger index = 0;
	if (index >= [_cityInfoControllers count]) {
		index = 0;
		return;
	}
	[[[_cityInfoControllers objectAtIndex:index++] mainView] createImageIfNeeded];
}

- (void)_constrainKeyViewsChain
{
	NSArray *components = [_citiesPaletteTitleView viewComponents];
	NSInteger count = [components count];
	for (NSInteger i = 0; i < count - 1; i++)
		[[components objectAtIndex:i] setNextKeyView:[components objectAtIndex:i + 1]];
	if (count > 0) {
		[_citiesPaletteView setNextKeyView:[components objectAtIndex:0]];
		[[components lastObject] setNextKeyView:_citiesPaletteView];
	}
}

- (id)stringWithLocaleShortDateFormatForAbsoluteTime:(double)aTime
{
	CFDateRef date = CFDateCreate(kCFAllocatorDefault, aTime);
	CFLocaleRef locale = CFLocaleCopyCurrent();
	CFDateFormatterRef formatter = CFDateFormatterCreate(kCFAllocatorDefault, locale, kCFDateFormatterShortStyle,
	                                                     kCFDateFormatterNoStyle);
	NSString *string = [(NSString *)CFDateFormatterCreateStringWithDate(kCFAllocatorDefault, formatter, date) autorelease];
	CFRelease(formatter);
	CFRelease(locale);
	CFRelease(date);
	return string;
}

- (id)_screen
{
	NSScreen *screen = [_citiesPalette screen];
	return screen ? screen : [NSScreen mainScreen];
}

// Keeps the palette on its screen.
- (void)_adjustPalettePosition
{
	NSRect frame = [_citiesPalette frame];
	NSRect visible = [[self _screen] visibleFrame];
	NSPoint origin = frame.origin;
	CGFloat top = NSMaxY(frame);
	if (top < visible.origin.y) {
		origin.y = visible.origin.y;
		top = frame.size.height + visible.origin.y;
	}
	if (NSMaxX(frame) < visible.origin.x)
		origin.x = visible.origin.x;
	if (NSMaxX(visible) < origin.x)
		origin.x = NSMaxX(visible) - frame.size.width;
	if (NSMaxY(visible) < top)
		origin.y = NSMaxY(visible) - frame.size.height;
	[_citiesPalette setFrameOrigin:origin];
}

// Fits the window to _contentSize, keeping its top.
- (void)_resizePalette
{
	NSRect frame = [_citiesPalette frame];
	NSRect content = [_citiesPalette contentRectForFrameRect:frame];
	content.size = _contentSize;
	NSRect newFrame = [_citiesPalette frameRectForContentRect:content];
	newFrame.origin.y = NSMaxY(frame) - newFrame.size.height;
	[_citiesPalette setFrame:newFrame display:YES];
	[self _adjustPalettePosition];
}

// Lays the city views out in a grid as large as the screen allows, with separators
// between the columns, and computes _contentSize.
- (void)_reconstructPalette
{
	NSRect visible = [[self _screen] visibleFrame];
	CGFloat cellHeight = _maxCityViewSize.height;
	CGFloat cellWidth = _maxCityViewSize.width;
	BOOL vertical = Preferences::verticalStateEnabled();
	[_citiesPaletteTitleView setVertical:!vertical];
	_minContentSize = [_citiesPaletteTitleView sizeForBestFit];
	CGFloat separatorWidth = [_verticalSeparatorImage size].width;

	int count = (int)[_cityInfoControllers count];
	if (_draggedCityInfo && _mouseOutside)
		count++;
	int columns, rows, separators;
	if (!vertical) {
		int perRow = MAX(1, (int)((visible.size.width - _minContentSize.width) / (cellWidth + separatorWidth)));
		rows = MAX(1, count / perRow + (count % perRow > 0));
		columns = count / rows;
		separators = columns - 1;
		if (count % rows) {
			columns++;
			separators = columns - 1;
		}
	} else {
		int perColumn = MAX(1, (int)((visible.size.height - _minContentSize.height) / (cellHeight + 1.0)));
		columns = MAX(1, count / perColumn + (count % perColumn != 0));
		rows = count / columns + (count % columns != 0);
		separators = columns - 1;
		if (cellWidth * columns + separators * separatorWidth < _minContentSize.width)
			cellWidth = _minContentSize.width;
	}
	if (_draggedCityInfo && _mouseOutside)
		count--;

	NSUInteger separatorIndex = 0;
	for (int i = 0; i < count; i++) {
		CityInfoController *controller = [_cityInfoControllers objectAtIndex:i];
		int column;
		CGFloat x, y;
		if (!vertical) {
			column = i % columns;
			x = column * (cellWidth + separatorWidth) + _minContentSize.width;
			y = (cellHeight + 1.0) * (i / columns);
		} else {
			column = i / rows;
			x = column * (cellWidth + separatorWidth);
			y = (cellHeight + 1.0) * (i % rows) + _minContentSize.height;
		}
		y += 1.0;
		[[controller mainView] setFrame:NSMakeRect(x, y, cellWidth, cellHeight)];
		if (column >= separators)
			continue;
		NSRect separatorFrame = NSMakeRect(x + cellWidth, y, separatorWidth, cellHeight);
		while ([_separatorViews count] <= separatorIndex) {
			CIBackgroundImageView *separator = [[[CIBackgroundImageView alloc] initWithFrame:separatorFrame] autorelease];
			[separator setImage:_verticalSeparatorImage];
			[separator setImageFrameStyle:NSImageFramePhoto];
			[separator setImageScaling:NSImageScaleAxesIndependently];
			[_separatorViews addObject:separator];
		}
		NSView *separator = [_separatorViews objectAtIndex:separatorIndex++];
		[separator setFrame:separatorFrame];
		[_citiesPaletteView addSubview:separator];
	}
	for (NSUInteger count = [_separatorViews count]; separatorIndex < count; separatorIndex++)
		[[_separatorViews objectAtIndex:separatorIndex] removeFromSuperview];

	_contentSize = _minContentSize;
	if (!vertical) {
		_contentSize.width += separators * separatorWidth + columns * cellWidth;
		CGFloat height = rows * (cellHeight + 1.0);
		if (_contentSize.height < height)
			_contentSize.height = height;
	} else {
		_contentSize.height += rows * (cellHeight + 1.0);
		CGFloat width = columns * cellWidth + separators * separatorWidth;
		if (_contentSize.width < width)
			_contentSize.width = width;
	}
}

// The largest city view; YES when it changed.
- (BOOL)_refreshMaxSizeWithNeedReconstruct:(BOOL)aReconstruct
{
	NSSize size = NSZeroSize;
	for (CityInfoController *controller in _cityInfoControllers) {
		if (aReconstruct)
			[controller reconstructMainView];
		NSSize fit = [controller sizeForBestFit];
		size.width = MAX(size.width, fit.width);
		size.height = MAX(size.height, fit.height);
	}
	if (NSEqualSizes(size, _maxCityViewSize))
		return NO;
	_maxCityViewSize = size;
	return YES;
}

- (void)_immediateReconstruct:(id)aReconstruct
{
	if (_reconstructTimer != aReconstruct)
		[_reconstructTimer invalidate];
	_reconstructTimer = nil;
	if (![_cityInfoControllers count])
		return;
	[[NSNotificationCenter defaultCenter] postNotificationName:@"NeedStopAnimationsNotification" object:self userInfo:nil];
	if ([self _refreshMaxSizeWithNeedReconstruct:YES])
		[_citiesPaletteView setNeedsDisplay:YES];
	[self _reconstructPalette];
	[self _resizePalette];
}

- (void)_postponedReconstruct
{
	[_reconstructTimer invalidate];
	_reconstructTimer = [NSTimer scheduledTimerWithTimeInterval:0.001
	                                                     target:self
	                                                   selector:@selector(_immediateReconstruct:)
	                                                   userInfo:nil
	                                                    repeats:NO];
}

- (void)_changeVerticalState
{
	BOOL wasVertical = Preferences::verticalStateEnabled();
	Preferences::setVerticalStateEnabled(!wasVertical);
	[self _reconstructPalette];
	[self _resizePalette];
	NSRect visible = [[self _screen] visibleFrame];
	NSRect frame = [_citiesPalette frame];
	NSPoint origin = frame.origin;
	if (!wasVertical)
		origin.y = MAX(visible.origin.y, frame.origin.y);
	else if (visible.origin.x > frame.origin.x)
		origin.x = visible.origin.x;
	else if (NSMaxX(visible) < NSMaxX(frame))
		origin.x = NSMaxX(visible) - frame.size.width;
	[_citiesPalette setFrameOrigin:origin];
}

- (void)_addCityInfo:(id)aInfo
{
	[aInfo setMenu:_paletteContextMenu];
	NSSize size = [aInfo sizeForBestFit];
	if (![_cityInfoControllers count])
		_maxCityViewSize = size;
	else {
		_maxCityViewSize.width = MAX(_maxCityViewSize.width, size.width);
		_maxCityViewSize.height = MAX(_maxCityViewSize.height, size.height);
	}
	[_cityInfoControllers addObject:aInfo];
	[_citiesPaletteView addSubview:[aInfo mainView]];
}

- (void)_addDefaultCityIfNeeded
{
	if ([_cityInfoControllers count])
		return;
	[_citiesPalette disableFlushWindow];
	PointEntry *home = Preferences::homeCity();
	[_citiesPalette enableFlushWindow];
	if (home)
		[self _addCityInfo:[[[CityInfoController alloc] initWithCityEntry:home] autorelease]];
}

#pragma mark Title view

- (id)_buttonWithTitle:(id)aTitle action:(SEL)aAction image:(id)aImage tag:(NSInteger)aTag enabled:(BOOL)aEnabled sendActionOn:(NSInteger)aOn
{
	NSButton *button = [[NSButton new] autorelease];
	[button setTarget:self];
	[button setAction:aAction];
	[button setButtonType:NSButtonTypeMomentaryChange];
	[button setTitle:aTitle];
	[button setImage:aImage];
	[button setBordered:NO];
	[button setTag:aTag];
	[button setEnabled:aEnabled];
	[button setFrame:_titleViewComponentRect];
	if (aOn)
		[button sendActionOn:aOn];
	return button;
}

// The component rect grows to the largest button image.
- (id)_buttonImageWithName:(id)aName
{
	NSImage *image = [NSImage imageNamed:aName];
	NSSize size = [image size];
	_titleViewComponentRect.size.width = MAX(_titleViewComponentRect.size.width, size.width);
	_titleViewComponentRect.size.height = MAX(_titleViewComponentRect.size.height, size.height);
	return image;
}

- (id)_titleViewComponents
{
	_titleViewComponentRect = NSMakeRect(0.0, 0.0, 11.0, 11.0);
	NSImage *red = [self _buttonImageWithName:@"Red"];
	NSImage *gray = [self _buttonImageWithName:@"Gray"];
	NSImage *green = [self _buttonImageWithName:@"Green"];
	NSImage *triangle = [self _buttonImageWithName:@"Triangle"];
	NSButton *menu = [self _buttonWithTitle:NSLocalizedString(@"Menu", @"")
	                                 action:@selector(_acceptMenuAction:)
	                                  image:triangle
	                                    tag:1
	                                enabled:YES
	                           sendActionOn:NSEventMaskLeftMouseDown];
	NSButton *zoom = [self _buttonWithTitle:NSLocalizedString(@"Zoom", @"")
	                                 action:@selector(_acceptVerticalAction:)
	                                  image:green
	                                    tag:0
	                                enabled:YES
	                           sendActionOn:0];
	NSButton *minimize = [self _buttonWithTitle:NSLocalizedString(@"Minimize", @"")
	                                     action:NULL
	                                      image:gray
	                                        tag:0
	                                    enabled:NO
	                               sendActionOn:0];
	NSButton *close = [self _buttonWithTitle:NSLocalizedString(@"Close", @"")
	                                  action:@selector(_acceptCloseAction:)
	                                   image:red
	                                     tag:0
	                                 enabled:YES
	                            sendActionOn:0];
	return [NSArray arrayWithObjects:close, minimize, zoom, menu, nil];
}

- (void)_createTitleView
{
	// Retained by the palette view it is added to.
	_citiesPaletteTitleView = [[CIPaletteTitleView new] autorelease];
	[_citiesPaletteTitleView setViewComponents:[self _titleViewComponents]];
	[_citiesPaletteTitleView setVertical:!Preferences::verticalStateEnabled()];
	[_citiesPaletteTitleView setHorizontalTitleImage:[NSImage imageNamed:@"title_horisontal"]];
	[_citiesPaletteTitleView setVerticalTitleImage:[NSImage imageNamed:@"title_vertical"]];
	[_citiesPaletteTitleView setTopOffset:0.0];
}

#pragma mark Menus

- (BOOL)validateMenuItem:(id)aItem
{
	NSInteger tag = [aItem tag];
	SEL action = [aItem action];
	if (action == @selector(_fontSizeMenu:)) {
		float size = Preferences::paletteFontSize();
		float sizes[] = { 9.0f, 11.0f, 13.0f };
		if (tag >= kFontSizeMini && tag <= kFontSizeRegular)
			[aItem setState:size == sizes[tag] ? NSControlStateValueOn : NSControlStateValueOff];
		return Preferences::doShowWeatherOnStartup();
	}
	if (action == @selector(_paletteActionsMenu:)) {
		switch (tag) {
		case 0:
			return NO;
		case kPaletteActionAddSelected:
			return [[[AtlasController atlasController] mainWindowController] validateMenuItem:aItem];
		case kPaletteActionUpdateWeather:
			return Preferences::usesInternet();
		case kPaletteActionRemoveSelected:
			return [_citiesPalette isKeyWindow] && [CityInfoView selectedView];
		case kPaletteActionPrintShort:
		case kPaletteActionPrintExtended:
			return [self paletteIsVisible] && Preferences::usesInternet()
			    && ![[[AtlasController atlasController] mainWindowController] isInFullWindowMode];
		default:
			return Preferences::doShowWeatherOnStartup();
		}
	}
	if (action == @selector(showCityDetails:))
		return [_citiesPalette isKeyWindow] && [CityInfoView selectedView];
	if (action == @selector(runPageLayout:))
		return ![[[AtlasController atlasController] mainWindowController] isInFullWindowMode];
	return Preferences::doShowWeatherOnStartup();
}

- (void)showCityDetails:(id)aDetails
{
	[[[AtlasController atlasController] mainWindowController]
	    showDetailsForEntry:[[[CityInfoView selectedView] owner] cityEntry]
	               openPane:YES
	   openInfoForBillboard:YES];
}

- (void)_fontSizeMenu:(id)aMenu
{
	if ([aMenu state] != NSControlStateValueOff)
		return;
	switch ([aMenu tag]) {
	case kFontSizeMini:
		Preferences::setPaletteFontSize(9.0f);
		break;
	case kFontSizeSmall:
		Preferences::setPaletteFontSize(11.0f);
		break;
	case kFontSizeRegular:
		Preferences::setPaletteFontSize(13.0f);
		break;
	}
}

- (void)_paletteActionsMenu:(id)aMenu
{
	switch ([aMenu tag]) {
	case kPaletteActionAddSelected:
		[[[AtlasController atlasController] mainWindowController] _paletteActionsMenu:aMenu];
		break;
	case kPaletteActionUpdateWeather:
		CWManager::instance()->updateScheduledEntries();
		break;
	case kPaletteActionRemoveSelected:
		[self deleteForward:aMenu];
		break;
	case kPaletteActionRestore:
		[self removeAllCities];
		break;
	case kPaletteActionPrintShort:
		_isPrintShortForecast = YES;
		// fall through
	case kPaletteActionPrintExtended:
		[NSApp activateIgnoringOtherApps:YES];
		if ([_citiesPalette isVisible])
			[self doPrint:self];
		_isPrintShortForecast = NO;
		break;
	}
}

- (id)_newFontSizeMenuItemWithTitle:(id)aTitle tag:(NSInteger)aTag
{
	NSMenuItem *item = [[NSMenuItem new] autorelease];
	[item setTitle:aTitle];
	[item setTag:aTag];
	[item setTarget:self];
	[item setAction:@selector(_fontSizeMenu:)];
	return item;
}

- (id)_fontSizeMenuItem
{
	NSMenu *menu = [[NSMenu new] autorelease];
	[menu addItem:[self _newFontSizeMenuItemWithTitle:NSLocalizedString(@"Mini Font Size", @"") tag:kFontSizeMini]];
	[menu addItem:[self _newFontSizeMenuItemWithTitle:NSLocalizedString(@"Small Font Size", @"") tag:kFontSizeSmall]];
	[menu addItem:[self _newFontSizeMenuItemWithTitle:NSLocalizedString(@"Regular Font Size", @"") tag:kFontSizeRegular]];
	NSMenuItem *item = [[NSMenuItem new] autorelease];
	[item setTitle:NSLocalizedString(@"Font Size", @"")];
	[item setSubmenu:menu];
	return item;
}

- (id)_menuItemWithTitle:(id)aTitle selector:(SEL)aSelector tag:(NSInteger)aTag targer:(id)aTarger
{
	NSMenuItem *item = [[NSMenuItem new] autorelease];
	[item setTitle:aTitle];
	[item setTarget:aTarger];
	[item setAction:aSelector];
	[item setTag:aTag];
	return item;
}

- (void)_createSelfMenu
{
	_mainMenu = [NSMenu new];
	NSMenuItem *item = [self _menuItemWithTitle:NSLocalizedString(@"Weather Palette", @"")
	                                   selector:@selector(toggleCitiesInfoPalette:)
	                                        tag:0
	                                     targer:nil];
	[item setKeyEquivalent:@"I"];
	[item setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
	[_mainMenu addItem:item];
	[_mainMenu addItem:[NSMenuItem separatorItem]];
	[_mainMenu addItem:[self _menuItemWithTitle:NSLocalizedString(@"Add Selected", @"")
	                                   selector:@selector(_paletteActionsMenu:)
	                                        tag:kPaletteActionAddSelected
	                                     targer:nil]];
	[_mainMenu addItem:[self _menuItemWithTitle:NSLocalizedString(@"Remove Selected", @"")
	                                   selector:@selector(_paletteActionsMenu:)
	                                        tag:kPaletteActionRemoveSelected
	                                     targer:self]];
	[_mainMenu addItem:[self _menuItemWithTitle:NSLocalizedString(@"Restore", @"")
	                                   selector:@selector(_paletteActionsMenu:)
	                                        tag:kPaletteActionRestore
	                                     targer:self]];
	[_mainMenu addItem:[NSMenuItem separatorItem]];
}

- (void)_addSelfMenu
{
	[_mainMenu addItem:[NSMenuItem separatorItem]];
	[_mainMenu addItem:[self _menuItemWithTitle:NSLocalizedString(@"PrintWeatherShort", @"")
	                                   selector:@selector(_paletteActionsMenu:)
	                                        tag:kPaletteActionPrintShort
	                                     targer:self]];
	[_mainMenu addItem:[self _menuItemWithTitle:NSLocalizedString(@"PrintWeatherExtended", @"")
	                                   selector:@selector(_paletteActionsMenu:)
	                                        tag:kPaletteActionPrintExtended
	                                     targer:self]];
	[_mainMenu addItem:[NSMenuItem separatorItem]];
	NSMenuItem *item = [self _menuItemWithTitle:NSLocalizedString(@"Update Weather", @"")
	                                   selector:@selector(_paletteActionsMenu:)
	                                        tag:kPaletteActionUpdateWeather
	                                     targer:self];
	[item setKeyEquivalent:@"U"];
	[item setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
	[_mainMenu addItem:item];
}

// The palette's menu replaces the application menu's Weather Palette item; the
// context menu is the same without its first three and last two items.
- (void)_createMenu
{
	[self _createSelfMenu];
	for (NSArray *group in [CityInfoController optionsMenuItems])
		for (NSMenuItem *item in group)
			[_mainMenu addItem:item];
	[self _addSelfMenu];

	_paletteContextMenu = [NSMenu new];
	[_paletteContextMenu addItems:[_mainMenu itemArray]];
	for (int i = 0; i < 3; i++)
		[_paletteContextMenu removeItemAtIndex:0];
	for (int i = 0; i < 2; i++)
		[_paletteContextMenu removeItemAtIndex:[_paletteContextMenu numberOfItems] - 1];

	NSMenuItem *oldItem = [[NSApp mainMenu] recursiveFindItemWithTag:kWeatherPaletteMenuTag];
	NSMenuItem *newItem = [[NSMenuItem new] autorelease];
	[_mainMenu setTitle:[oldItem title]];
	[newItem setTitle:[oldItem title]];
	[newItem setTag:[oldItem tag]];
	[newItem setSubmenu:_mainMenu];
	NSInteger index = [[NSApp mainMenu] indexOfItem:oldItem];
	[[NSApp mainMenu] removeItem:oldItem];
	[[NSApp mainMenu] insertItem:newItem atIndex:index];
}

- (void)_acceptCloseAction:(id)aAction
{
	[_citiesPalette performClose:aAction];
}

- (void)_acceptVerticalAction:(id)aAction
{
	[self _changeVerticalState];
}

- (void)_acceptMenuAction:(id)aAction
{
	[NSMenu wgPopUpContextMenu:_paletteContextMenu withEvent:[[aAction window] currentEvent] forView:aAction];
}

// Every second: animations, cached images, and a rebuild when the date format or the minute changes.
- (void)_timeTick:(id)aTick
{
	static NSString *lastLocaleDate = nil;
	if ([CityInfoView isDraggingInProgress])
		return;
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	if ([_citiesPalette isVisible])
		[center postNotificationName:@"NeedStartAnimationsNotification" object:self userInfo:nil];
	[_citiesPalette setViewsNeedDisplay:NO];
	[self _cacheNextImageIfNeeded];
	double now = [NSDate timeIntervalSinceReferenceDate];
	NSString *localeDate = [self stringWithLocaleShortDateFormatForAbsoluteTime:now];
	BOOL dateChanged;
	if (![lastLocaleDate isEqualToString:localeDate]) {
		[lastLocaleDate release];
		lastLocaleDate = [localeDate retain];
		dateChanged = YES;
	} else {
		static int lastMinutesCount = (int)(now / 60.0);
		int minutes = (int)(now / 60.0);
		if (minutes == lastMinutesCount)
			return;
		dateChanged = NO;
		lastMinutesCount = minutes;
	}
	[center postNotificationName:@"AdjustTimeNotification" object:[NSCalendarDate calendarDate] userInfo:nil];
	if (dateChanged || _reconstructTimer)
		[self _immediateReconstruct:nil];
}

#pragma mark Dragging

// The dragged city comes back in, before the view under the mouse.
- (void)_acceptDraggingEntered:(id)aEntered
{
	if (!_mouseOutside)
		return;
	_mouseOutside = NO;
	id info = [aEntered object];
	NSView *view = [_citiesPaletteView hitTest:[info draggingLocation]];
	if ([view isMemberOfClass:[CityInfoView class]])
		[_cityInfoControllers insertObject:_draggedCityInfo
		                           atIndex:[_cityInfoControllers indexOfObject:[(CityInfoView *)view owner]]];
	else
		[_cityInfoControllers addObject:_draggedCityInfo];
	[self _reconstructPalette];
	[_citiesPaletteView setNeedsDisplay:YES];
	[self _resizePalette];
	[_draggedCityInfo release];
	_draggedCityInfo = nil;
}

// Swaps the dragged city with the one under the mouse.
- (void)_acceptDraggingUpdated:(id)aUpdated
{
	id info = [aUpdated object];
	CityInfoView *source = [info draggingSource];
	[source tearOffImageIfNeeded];
	NSPoint location = [info draggingLocation];
	CityInfoView *view = (CityInfoView *)[_citiesPaletteView hitTest:location];
	if (![view isMemberOfClass:[CityInfoView class]])
		return;
	NSPoint point = [view convertPoint:location fromView:nil];
	if (![view mouse:point inRect:[view opaqueBounds]])
		return;
	NSRect viewFrame = [view frame];
	[view setFrameAnimated:[source frame]];
	[source setFrame:viewFrame];
	id sourceOwner = [[source owner] retain];
	id viewOwner = [[view owner] retain];
	NSUInteger sourceIndex = [_cityInfoControllers indexOfObject:sourceOwner];
	NSUInteger viewIndex = [_cityInfoControllers indexOfObject:viewOwner];
	[_cityInfoControllers replaceObjectAtIndex:sourceIndex withObject:viewOwner];
	[_cityInfoControllers replaceObjectAtIndex:viewIndex withObject:sourceOwner];
	[sourceOwner release];
	[viewOwner release];
	[_citiesPalette setViewsNeedDisplay:YES];
}

- (void)_acceptDraggingExited:(id)aExited
{
	_draggedCityInfo = [aExited object];
	if (![[_draggedCityInfo mainView] isDraggingAborted]) {
		if (_mouseOutside)
			return;
		_mouseOutside = YES;
		[_draggedCityInfo retain];
		[_cityInfoControllers removeObject:_draggedCityInfo];
	} else {
		_mouseOutside = NO;
		[_cityInfoControllers addObject:_draggedCityInfo];
		[_draggedCityInfo release];
		_draggedCityInfo = nil;
	}
	[self _reconstructPalette];
	[_citiesPaletteView setNeedsDisplay:YES];
	[self _resizePalette];
}

// Dropped outside: the city is gone.
- (void)_acceptDraggingRemoveCity:(id)aCity
{
	if (!_mouseOutside)
		return;
	[_draggedCityInfo release];
	_draggedCityInfo = nil;
	_mouseOutside = NO;
	[self _refreshMaxSizeWithNeedReconstruct:NO];
	[self _reconstructPalette];
	[_citiesPaletteView setNeedsDisplay:YES];
	[self _resizePalette];
	[_citiesPaletteView displayIfNeeded];
	[self _saveCitiesInPrefs];
}

#pragma mark Palette

- (void)showPalette
{
	[self _loadNibIfNeeded];
	[self _adjustPalettePosition];
	if ([_citiesPalette isVisible])
		return;
	[_citiesPalette makeKeyAndOrderFront:self];
	Preferences::setShowWeatherOnStartup(1);
	[self manualUpdate];
}

- (void)hidePalette
{
	[_citiesPalette performClose:self];
}

- (void)togglePalette
{
	if ([_citiesPalette isVisible])
		[self hidePalette];
	else
		[self showPalette];
}

- (BOOL)paletteIsVisible
{
	return [_citiesPalette isVisible];
}

- (void)invalidate
{
	if ([_citiesPalette isVisible])
		[_citiesPalette close];
	[_reconstructTimer invalidate];
	[_timeTick invalidate];
	[self release];
}

- (void)_selectCityAtIndex:(NSInteger)aIndex
{
	[CityInfoView setSelectedView:[[_cityInfoControllers objectAtIndex:aIndex] mainView]];
}

- (void)moveDown:(id)aDown
{
	NSUInteger index = 0;
	id owner = [[CityInfoView selectedView] owner];
	if (owner)
		index = [_cityInfoControllers indexOfObject:owner] + 1;
	if (index < [_cityInfoControllers count])
		[self _selectCityAtIndex:index];
}

- (void)moveLeft:(id)aLeft
{
	NSInteger index = [_cityInfoControllers count];
	id owner = [[CityInfoView selectedView] owner];
	if (owner)
		index = [_cityInfoControllers indexOfObject:owner];
	if (index - 1 >= 0)
		[self _selectCityAtIndex:index - 1];
}

- (void)moveRight:(id)aRight
{
	[self moveDown:aRight];
}

- (void)moveUp:(id)aUp
{
	[self moveLeft:aUp];
}

#pragma mark Window delegate

- (BOOL)windowShouldClose:(id)aClose
{
	Preferences::setShowWeatherOnStartup(0);
	[[NSNotificationCenter defaultCenter] postNotificationName:@"NeedStopAnimationsNotification" object:self userInfo:nil];
	return YES;
}

// The title bar spans the content along its side.
- (void)windowDidResize:(id)aResize
{
	NSSize size = [_citiesPaletteTitleView sizeForBestFit];
	NSRect content = [[_citiesPalette contentView] frame];
	if (![_citiesPaletteTitleView isVertical])
		size.width = content.size.width;
	else
		size.height = content.size.height;
	[_citiesPaletteTitleView setFrameSize:size];
}

- (void)windowDidResignKey:(id)aKey
{
	[[CityInfoView selectedView] setNeedsDisplay:YES];
}

- (void)windowDidBecomeKey:(id)aKey
{
	[[CityInfoView selectedView] setNeedsDisplay:YES];
}

#pragma mark Life cycle

- (id)init
{
	self = [super init];
	if (self) {
		_citiesPalette = [CIPaletteWindow new];
		_cityInfoControllers = [NSMutableArray new];
		[self _createMenu];
		[self _createTitleView];
		_citiesPaletteView = [[CIPaletteView alloc] initWithController:self];
		[_citiesPaletteView addSubview:_citiesPaletteTitleView];
		[_citiesPalette setDelegate:(id<NSWindowDelegate>)self];
		[_citiesPalette setContentView:_citiesPaletteView];
		_timeTick = [NSTimer scheduledTimerWithTimeInterval:1.0
		                                             target:self
		                                           selector:@selector(_timeTick:)
		                                           userInfo:nil
		                                            repeats:YES];
		NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
		[center addObserver:self selector:@selector(_acceptDraggingEntered:) name:@"DraggingEnteredNotification" object:nil];
		[center addObserver:self selector:@selector(_acceptDraggingUpdated:) name:@"DraggingUpdatedNotification" object:nil];
		[center addObserver:self selector:@selector(_acceptDraggingExited:) name:@"DraggingExitedNotification" object:nil];
		[center addObserver:self
		           selector:@selector(_acceptDraggingRemoveCity:)
		               name:@"DraggingRemoveCityNotification"
		             object:nil];
		LString *names[] = { Preferences::kClockFormatWasChanged, Preferences::kInternetStateWasChanged,
			                 Preferences::kCurrentConditionsStateWasChanged, Preferences::kExpandedForecastStateWasChanged,
			                 Preferences::kCIPaletteFontSizeWasChanged, Preferences::kMainPropertiesStateWasChanged };
		for (LString *name : names)
			LNotificationCenter::center()->addObserver(self, @selector(_postponedReconstruct), name, (LObject *)0);
	}
	return self;
}

- (void)awakeFromNib
{
	_verticalSeparatorImage = [[NSImage imageNamed:@"city_info_separartor"] retain];
	_separatorViews = [NSMutableArray new];
	[_citiesPaletteTitleView setMenu:_paletteContextMenu];
	[_citiesPaletteView setMenu:_paletteContextMenu];
	NSSize size = [_citiesPaletteTitleView sizeForBestFit];
	[_citiesPalette setTitleOffset:[_citiesPaletteTitleView isVertical] ? size.width : size.height];
	_minContentSize = size;
	[self _setCitiesFromPreferences];
	[self _reconstructPalette];
	[self _resizePalette];
	[_citiesPalette center];
	[_citiesPalette setFrameAutosaveName:@"Cities Info Palette"];
	[self _constrainKeyViewsChain];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[_cityInfoControllers release];
	[_mainMenu release];
	[_paletteContextMenu release];
	[_citiesPalette release];
	[_citiesPaletteView release];
	[_separatorViews release];
	[_verticalSeparatorImage release];
	[super dealloc];
}

#pragma mark Printing

- (void)runPageLayout:(id)aLayout
{
	[[NSPageLayout pageLayout] runModal];
}

- (void)doPrint:(id)aPrint
{
	NSUInteger count = [_cityInfoControllers count];
	TLArray<PointEntry *> *entries = (TLArray<PointEntry *> *)LArray::arrayWithCapacity((unsigned int)count);
	for (CityInfoController *controller in _cityInfoControllers)
		entries->addObject([controller cityEntry]);
	[PrintController printWithDataSourceProvider:[WeatherPalettePrintDataSourceProvider
	                                                  weatherPalettePrintDataSourceProviderWithEntriesList:entries
	                                                                                  isPrintShortForecast:_isPrintShortForecast]];
}

#pragma mark Cities

- (void)_setCitiesFromPreferences
{
	LArray *entries = Preferences::entriesList();
	int count = entries ? entries->count() : 0;
	for (int i = 0; i < count; i++) {
		PointEntry *entry = lobj_cast<PointEntry>(entries->objectAtIndex(i));
		if (entry)
			[self _addCityInfo:[[[CityInfoController alloc] initWithCityEntry:entry] autorelease]];
	}
}

- (void)_saveCitiesInPrefs
{
	TLArray<GenericEntry *> *entries = (TLArray<GenericEntry *> *)LArray::array();
	for (CityInfoController *controller in _cityInfoControllers)
		entries->addObject([controller cityEntry]);
	Preferences::setEntriesList(entries);
}

- (void)addEntries:(TLArray<GenericEntry*> *)aEntries
{
	TLArray<LMap *> *descriptions = (TLArray<LMap *> *)LArray::array();
	int count = aEntries ? aEntries->count() : 0;
	int limit = Preferences::citiesCountLimit();
	for (int i = 0; i < count && (int)descriptions->count() < limit; i++) {
		PointEntry *entry = lobj_cast<PointEntry>(aEntries->objectAtIndex(i));
		if (entry)
			descriptions->addObject(entry->entryDictionary());
	}
	[self addCities:descriptions];
}

// Some location that is not in the palette yet.
- (BOOL)canAddEntries:(TLArray<GenericEntry*> *)aEntries
{
	int count = aEntries ? aEntries->count() : 0;
	if (count <= 0)
		return NO;
	LComparableSet *uids = [self _entryUIDsInPalette];
	for (int i = 0; i < count; i++) {
		PointEntry *entry = lobj_cast<PointEntry>(aEntries->objectAtIndex(i));
		if (entry && !uids->containsObject(entry->uid(0)))
			return YES;
	}
	return NO;
}

- (void)addCities:(TLArray<LF::LMap*> *)aCities
{
	[self showPalette];
	int count = aCities ? aCities->count() : 0;
	if (count <= 0)
		return;
	int citiesCount = (int)[_cityInfoControllers count];
	int limit = Preferences::citiesCountLimit();
	LComparableSet *uids = [self _entryUIDsInPalette];
	BOOL added = NO, overLimit = NO;
	for (int i = 0; i < count; i++) {
		if (citiesCount >= limit) {
			overLimit = YES;
			break;
		}
		LString *uid = lobj_cast<LString>(aCities->objectAtIndex(i)->objectForKey(GenericEntry::kGenericEntryUID));
		if (!uid || uids->containsObject(uid))
			continue;
		PointEntry *entry = lobj_cast<PointEntry>(GenericEntry::entryWithUID(uid));
		if (!entry)
			continue;
		if (!added) {
			[[NSNotificationCenter defaultCenter] postNotificationName:@"NeedStopAnimationsNotification" object:self userInfo:nil];
			added = YES;
		}
		citiesCount++;
		[self _addCityInfo:[[[CityInfoController alloc] initWithCityEntry:entry] autorelease]];
	}
	if (added) {
		[self _reconstructPalette];
		[self _resizePalette];
		[self _saveCitiesInPrefs];
	}
	if (overLimit)
		NSRunAlertPanel(NSLocalizedString(@"Cannot add city title", @""), @"%@", NSLocalizedString(@"Ok", @""), nil, nil,
		                [NSString stringWithFormat:NSLocalizedString(@"Cannot add city message", @""), limit]);
}

- (BOOL)canAddEntriesFromPropertyList:(TLArray<LF::LMap*> *)aList
{
	int count = aList ? aList->count() : 0;
	if (count <= 0)
		return NO;
	LComparableSet *uids = [self _entryUIDsInPalette];
	for (int i = 0; i < count; i++) {
		LString *uid = lobj_cast<LString>(aList->objectAtIndex(i)->objectForKey(GenericEntry::kGenericEntryUID));
		PointEntry *entry = lobj_cast<PointEntry>(GenericEntry::entryWithUID(uid));
		if (entry && !uids->containsObject(entry->uid(0)))
			return YES;
	}
	return NO;
}

- (void)removeAllCities
{
	TLArray<LMap *> *descriptions = (TLArray<LMap *> *)LArray::array();
	for (CityInfoController *controller in _cityInfoControllers)
		descriptions->addObject([controller cityEntry]->entryDictionary());
	[self resetCities:descriptions];
}

- (void)manualUpdate
{
	CWManager::instance()->resetWeatherInternetState();
	for (CityInfoController *controller in _cityInfoControllers)
		[controller manualUpdateWeather];
}

- (void)deleteForward:(id)aForward
{
	id owner = [[CityInfoView selectedView] owner];
	if (!owner)
		return;
	NSUInteger index = [_cityInfoControllers indexOfObject:owner];
	[self resetCities:(TLArray<LMap *> *)LArray::arrayWithObjects([owner cityEntry]->entryDictionary(), (LObject *)0)];
	if (index >= [_cityInfoControllers count])
		index--;
	[self _selectCityAtIndex:index];
}

- (LComparableSet *)_entryUIDsInPalette
{
	LComparableSet *uids = LAutorelease(LComparableSet::_alloc()->init());
	uids->preserveCapacity((unsigned int)[_cityInfoControllers count]);
	for (CityInfoController *controller in _cityInfoControllers)
		uids->addObject([controller cityEntry]->uid(0));
	return uids;
}

// Removes these cities; an emptied palette gets the home city back (or the last one removed).
- (void)resetCities:(TLArray<LF::LMap*> *)aCities
{
	int count = aCities ? aCities->count() : 0;
	if (count <= 0)
		return;
	LComparableSet *uids = LAutorelease(LComparableSet::_alloc()->init());
	uids->preserveCapacity(count);
	for (int i = 0; i < count; i++) {
		LString *uid = lobj_cast<LString>(aCities->objectAtIndex(i)->objectForKey(GenericEntry::kGenericEntryUID));
		if (uid)
			uids->addObject(uid);
	}
	PointEntry *lastEntry = 0;
	for (NSInteger i = (NSInteger)[_cityInfoControllers count] - 1; i >= 0; i--) {
		CityInfoController *controller = [_cityInfoControllers objectAtIndex:i];
		lastEntry = [controller cityEntry];
		if (uids->containsObject(lastEntry->uid(0))) {
			[[controller mainView] removeFromSuperview];
			[_cityInfoControllers removeObject:controller];
		}
	}
	if (![_cityInfoControllers count]) {
		PointEntry *home = Preferences::homeCity();
		[self _addCityInfo:[[[CityInfoController alloc] initWithCityEntry:home ? home : lastEntry] autorelease]];
	}
	[self _refreshMaxSizeWithNeedReconstruct:NO];
	[self _reconstructPalette];
	[_citiesPaletteView setNeedsDisplay:YES];
	[self _resizePalette];
	[self _saveCitiesInPrefs];
}

@end

#pragma clang diagnostic pop
