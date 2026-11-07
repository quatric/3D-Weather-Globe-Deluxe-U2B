#import "WGEntryDetailsWeatherPaneController.h"
#import "WGEntryDetailsController.h"
#import "WGEFMatrix.h"
#import "WGEFView.h"
#import "SMKEngravedTextFieldCell.h"
#import "NSImage+NSImageWGExtensions.h"
#include "AMOnline.h"

static LConstantString _k24TimeDateFormat("%H:%M, %B %e");
static LConstantString _k12TimeDateFormat("%I:%M %p, %B %e");

// Pane heights with the extended forecast shown and hidden.
static const CGFloat kExpandedHeight = 333.0;
static const CGFloat kMinimizedHeight = 190.0;

// "--" for an empty value.
static NSString *_valueOrDashes(LString *value)
{
	NSString *string = value ? (NSString *)value->cfString() : nil;
	return [string length] ? string : @"--";
}

// The weather of the current city in the entry details: the current
// conditions, and below them the forecast for the day selected in the
// weekdays strip (the strip's disclosure button hides the forecast).
@implementation WGEntryDetailsWeatherPaneController

- (id)initWithOwner:(id)aOwner
{
	return [super initWithOwner:aOwner];
}

- (void)dealloc
{
	[_refreshTimer invalidate];
	[_refreshTimer release];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[self _unschedule];
	[_extendedForecastSubview release];
	[_extendedWindDirectionImageView release];
	if (_entry) {
		_entry->release();
		_entry = 0;
	}
	if (_waitingEntry) {
		_waitingEntry->release();
		_waitingEntry = 0;
	}
	[super dealloc];
}

- (id)title
{
	return [[NSBundle mainBundle] localizedStringForKey:@"Weather" value:@"" table:nil];
}

- (id)iconName
{
	return @"icon_info_weather";
}

- (id)nibName
{
	return @"WGEFEntryInfo";
}

- (BOOL)wantsUpdatesWhenInactive
{
	return NO;
}

- (void)awakeFromNib
{
	[self _setupCellForField:_genaralDayOfWeek];
	[self _setupCellForField:_genaralDateTime];
	[self _setupCellForField:_genaralTemperature];
	[self _setupCellForField:_genaralWindSpeed];
	[self _setupCellForField:_genaralHumidity];
	[self _setupCellForField:_genaralPressure];
	[self _setupCellForField:_genaralVisibility];
	[pane addSubview:_extendedForecastfView];
	[_extendedForecastSubview retain];
	_weekdaysMatrix = [[WGEFMatrix alloc] initWithFrame:NSZeroRect];
	[_weekdaysMatrix setupCells];
	[_weekdaysMatrix setAction:@selector(weekdayAction:)];
	[_weekdaysMatrix setTarget:self];
	[_extendedForecastfView placeEFMatrix:_weekdaysMatrix];
	[_progressIndicator setDisplayedWhenStopped:NO];
	[self _checkInternet];
	NSBundle *bundle = [NSBundle mainBundle];
	[_extendedWindDirectionImageView setToolTip:[bundle localizedStringForKey:@"Wind direction" value:@"" table:nil]];
	[self _refreshToolTipRects];
	[_genaralSummaryImageView setImage:[NSImage imageNamed:@"weather_upd"]];
	[_extendedWindDirectionImageView retain];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_acceptAdjustTime:)
		name:@"AdjustTimeNotification" object:nil];
	LNotificationCenter *center = LNotificationCenter::center();
	center->addObserver(self, @selector(_clocksMeasureWereChanged:), Preferences::kClockFormatWasChanged, 0);
	center->addObserver(self, @selector(_unitsOfMeasureWereChanged:), Preferences::kUnitsOfMeasureWereChanged, 0);
	center->addObserver(self, @selector(_internetStateWasChanged:), Preferences::kInternetStateWasChanged, 0);
	center->addObserver(self, @selector(_reportsWereChanged:), CWManager::kWeatherInternetStateWasChangedNotification, 0);
	// The original set these through accessibilitySetOverrideValue:forAttribute:.
	[[_extendedWindDirectionImageView cell] setAccessibilityRoleDescription:[bundle localizedStringForKey:@"Wind Icon"
		value:@"" table:nil]];
	[[_genaralSummaryImageView cell] setAccessibilityRoleDescription:[bundle localizedStringForKey:@"Weather Icon"
		value:@"" table:nil]];
	[self toggleMinimizeAction:self];
	[self _refresh];
}

- (void)weekdayAction:(id)aAction
{
	if (_minimized)
		[self toggleMinimizeAction:self];
	[self _refresh];
}

- (id)view:(id)aView stringForToolTip:(NSInteger)aTip point:(NSPoint)aPoint userData:(void *)aData
{
	if (aView != _genaralSummaryImageView || !_entry)
		return nil;
	LMap *report = _entry->dictionaryForReport(kReportTypeCurrent);
	LString *description = report ? Utils::weatherDescription(report) : 0;
	return description ? (NSString *)description->cfString() : @"";
}

// Hiding the forecast deselects the weekday; showing it again (from the
// button) restores the selection.
- (void)toggleMinimizeAction:(id)aAction
{
	NSRect frame = [pane frame];
	if (!_minimized) {
		[_extendedForecastSubview removeFromSuperviewWithoutNeedingDisplay];
		frame.size.height = kMinimizedHeight;
		[pane setFrame:frame];
		[[self owner] rebuildLayout];
		_weekdaysMatrixSelectedCell = [_weekdaysMatrix selectedCell];
		[_weekdaysMatrix setAllowsEmptySelection:YES];
		[_weekdaysMatrix deselectAllCells];
		[_minimizeButton setImage:[NSImage imageNamed:@"discl_tr_norm_dwn"]];
		[_minimizeButton setAlternateImage:[NSImage imageNamed:@"discl_tr_press_dwn"]];
		_minimized = YES;
	} else {
		frame.size.height = kExpandedHeight;
		[pane setFrame:frame];
		[[self owner] rebuildLayout];
		[_extendedForecastfView addSubview:_extendedForecastSubview];
		if (aAction != self) {
			NSInteger row = 0, column = 0;
			[_weekdaysMatrix getRow:&row column:&column ofCell:_weekdaysMatrixSelectedCell];
			[_weekdaysMatrix setAllowsEmptySelection:NO];
			[_weekdaysMatrix selectCellAtRow:row column:column];
		}
		[_minimizeButton setImage:[NSImage imageNamed:@"discl_tr_norm_up"]];
		[_minimizeButton setAlternateImage:[NSImage imageNamed:@"discl_tr_press_up"]];
		_minimized = NO;
	}
}

- (void)_setupCellForField:(id)aField
{
	SMKEngravedTextFieldCell *cell = [[[SMKEngravedTextFieldCell alloc] initWithTextFieldCell:[aField cell]]
		autorelease];
	[cell setSecondaryTextColor:[NSColor colorWithCalibratedRed:0.45f green:0.52f blue:0.6f alpha:1.0f]];
	[cell setEngraveOffset:1.0];
	[aField setCell:cell];
}

- (void)_checkInternet
{
	_usesInternet = Preferences::usesInternet() != 0;
}

- (void)_refreshToolTipRects
{
	[_genaralSummaryImageView removeAllToolTips];
	[_genaralSummaryImageView addToolTipRect:[_genaralSummaryImageView bounds] owner:self userData:0];
}

// The current conditions and the extended forecast.
- (void)_schedule
{
	if (_scheduled || !_usesInternet || !_entry)
		return;
	LNotificationCenter::center()->addObserver(self, @selector(_reportsWereChanged:),
		PointEntry::kPointEntryWasUpdatedViaOnlineNotification, _entry);
	LNotificationCenter::center()->addObserver(self, @selector(_reportsWereChanged:),
		PointEntry::kPointEntryWasExpiredViaOnlineNotification, _entry);
	_entry->scheduleReports(5);
	_scheduled = YES;
}

- (void)_unschedule
{
	if (!_scheduled || !_entry)
		return;
	_entry->unscheduleReports(5);
	LNotificationCenter::center()->removeObserver(self, PointEntry::kPointEntryWasUpdatedViaOnlineNotification, _entry);
	LNotificationCenter::center()->removeObserver(self, PointEntry::kPointEntryWasExpiredViaOnlineNotification, _entry);
	_scheduled = NO;
}

static BOOL _isReportExpired(PointEntry *entry, TReportType type)
{
	LMap *report = entry ? entry->dictionaryForReport(type) : 0;
	return !report || UMapParsingUtils::loadIntFromObject(report->objectForKey(kReportCommonDataExpired)) != 0;
}

- (BOOL)_isCurrentConditionExpired
{
	return _isReportExpired(_entry, kReportTypeCurrent);
}

- (BOOL)_isExtendedForecastExpired
{
	return _isReportExpired(_entry, kReportTypeExpanded);
}

- (BOOL)_isWeatherExpired
{
	return [self _isCurrentConditionExpired] || [self _isExtendedForecastExpired];
}

- (void)_checkProgressIndicatorAnimation
{
	if ([self _isWeatherExpired] && _usesInternet && CWManager::instance()->isWeatherInternetStateOK()) {
		[_progressIndicator setHidden:NO];
		[_progressIndicator startAnimation:self];
	} else {
		[_progressIndicator stopAnimation:self];
		[_progressIndicator setHidden:YES];
	}
}

- (id)_weatherImageFromObject:(LObject *)aObject
{
	if (!aObject)
		return nil;
	int icon = UMapParsingUtils::loadIntFromObject(aObject);
	if (!icon)
		return nil;
	return [NSImage imageNamed:[NSString stringWithFormat:@"weather_icon%d", icon - 1]];
}

- (void)_timeForRefresh:(id)aRefresh
{
	[_refreshTimer release];
	_refreshTimer = nil;
	[self _refreshWithCityEntry:_waitingEntry];
	if (_waitingEntry) {
		_waitingEntry->release();
		_waitingEntry = 0;
	}
}

- (void)_internetStateWasChanged:(id)aChanged
{
	[self _refresh];
	[self _checkInternet];
	if (_usesInternet)
		[self _schedule];
	else
		[self _unschedule];
	[self _checkProgressIndicatorAnimation];
	[[self owner] validateCurrentPane];
}

- (void)_unitsOfMeasureWereChanged:(id)aChanged
{
	[self _refresh];
}

- (void)_clocksMeasureWereChanged:(id)aChanged
{
	[self _adjustTime];
}

- (void)_acceptAdjustTime:(id)aTime
{
	[self _adjustTime];
}

- (void)_reportsWereChanged:(id)aChanged
{
	[self _refresh];
	[self _checkProgressIndicatorAnimation];
}

- (BOOL)isEnabled
{
	return Preferences::usesInternet() && lobj_cast<PointEntry>([owner currentEntry]);
}

// The entry is refreshed half a second after the last change, so that
// browsing through cities doesn't schedule each one.
- (void)update
{
	if (_waitingEntry) {
		_waitingEntry->release();
		_waitingEntry = 0;
	}
	[_refreshTimer invalidate];
	[_refreshTimer release];
	_refreshTimer = nil;
	PointEntry *entry = lobj_cast<PointEntry>([[self owner] currentEntry]);
	if (entry) {
		entry->retain();
		_waitingEntry = entry;
		_refreshTimer = [[NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(_timeForRefresh:)
			userInfo:nil repeats:NO] retain];
	}
}

- (void)_setExtendedForecast:(LMap *)aForecast
{
	[_extendedFeelsLike setStringValue:_valueOrDashes(aForecast
		? Utils::temperatureDescription(aForecast->objectForKey(kReportComfortLevel)) : 0)];
	[_extendedWindSpeed setStringValue:_valueOrDashes(aForecast
		? Utils::speedDescription(aForecast->objectForKey(kReportWindSpeed)) : 0)];
	[_extendedHumidity setStringValue:_valueOrDashes(aForecast
		? Utils::percentsDescription(aForecast->objectForKey(kReportHumidity)) : 0)];
	int windDirection = aForecast ? UMapParsingUtils::loadIntFromObject(aForecast->objectForKey(kReportWindDir)) : 0;
	[_extendedWindDirectionImageView setImage:aForecast ? [self _windImageForValue:windDirection] : nil];
	if (![_extendedWindDirectionImageView image])
		[_extendedWindDirectionImageView removeFromSuperview];
	else {
		if (![_extendedWindDirectionImageView superview])
			[_extendedForecastSubview addSubview:_extendedWindDirectionImageView];
		[_extendedWindDirectionImageView setToolTip:(NSString *)Utils::windDescription(windDirection)->cfString()];
	}
	[_extendedPrecipitation setStringValue:_valueOrDashes(aForecast
		? Utils::percentsDescription(aForecast->objectForKey(kReportFCPreciptProbability)) : 0)];
	[_extendedDewPoint setStringValue:_valueOrDashes(aForecast
		? Utils::temperatureDescription(aForecast->objectForKey(kReportDewPoint)) : 0)];
	[_extendedUVIndex setStringValue:_valueOrDashes(aForecast
		? lobj_cast<LString>(aForecast->objectForKey(kReportFCUVDescription)) : 0)];
	NSString *description = nil;
	if (aForecast) {
		LString *weather = Utils::weatherDescription(aForecast);
		description = weather ? (NSString *)weather->cfString() : nil;
	}
	if (![description length])
		description = [[NSBundle mainBundle] localizedStringForKey:@"No forecast available." value:@"" table:nil];
	[_extendedDescription setStringValue:description];
	[_extendedDescription setToolTip:description];
}

- (void)_setCurrentConditions:(LMap *)aConditions
{
	[_genaralWindSpeed setStringValue:_valueOrDashes(aConditions
		? Utils::speedDescription(aConditions->objectForKey(kReportWindSpeed)) : 0)];
	[_genaralHumidity setStringValue:_valueOrDashes(aConditions
		? Utils::percentsDescription(aConditions->objectForKey(kReportHumidity)) : 0)];
	[_genaralSummaryImageView setImage:aConditions
		? [self _weatherImageFromObject:aConditions->objectForKey(kReportSummaryCode)] : nil];
	[_genaralTemperature setStringValue:_valueOrDashes(aConditions
		? Utils::temperatureDescription(aConditions->objectForKey(kReportCurrentTemperature)) : 0)];
	[_genaralPressure setStringValue:_valueOrDashes(aConditions
		? Utils::pressureDescription(aConditions->objectForKey(kReportBarometricPressure)) : 0)];
	[_genaralVisibility setStringValue:_valueOrDashes(aConditions
		? Utils::distanceDescription(aConditions->objectForKey(kReportVisibility)) : 0)];
	int windDirection = aConditions ? UMapParsingUtils::loadIntFromObject(aConditions->objectForKey(kReportWindDir)) : 0;
	[_genaralWindDirection setStringValue:(NSString *)Utils::windShortDescription(windDirection)->cfString()];
}

// The city's local date and time.
- (void)_adjustTime
{
	if (!_entry) {
		[_genaralDayOfWeek setStringValue:@""];
		return;
	}
	LDate *date = LDate::date();
	date->setTimeZone(_entry->timeZone(0));
	[_genaralDateTime setStringValue:(NSString *)date->descriptionWithCalendarFormat(Preferences::uses24HourClock()
		? &_k24TimeDateFormat : &_k12TimeDateFormat)->cfString()];
	[_genaralDayOfWeek setStringValue:[[NSString stringReferenceFromLString:Utils::weekdayName(date->dayOfWeek(), 0)]
		uppercaseString]];
}

- (void)_refreshWithCityEntry:(PointEntry *)aEntry
{
	if (_entry != aEntry) {
		[self _unschedule];
		if (_entry) {
			_entry->release();
			_entry = 0;
		}
		if (aEntry) {
			aEntry->retain();
			_entry = aEntry;
			CWManager::instance()->resetWeatherInternetState();
			[self _checkProgressIndicatorAnimation];
			[self _schedule];
		}
	}
	[self _refresh];
}

// While the weather is missing the icon says it is being updated (or
// that the update failed).
- (void)_refresh
{
	[self _adjustTime];
	LMap *current = _entry ? _entry->dictionaryForReport(kReportTypeCurrent) : 0;
	[self _setCurrentConditions:current];
	LMap *expanded = _entry ? _entry->dictionaryForReport(kReportTypeExpanded) : 0;
	if ([self _isWeatherExpired] && _entry)
		[_genaralSummaryImageView setImage:[NSImage imageNamed:CWManager::instance()->isWeatherInternetStateOK()
			? @"weather_upd" : @"weather_upd_failed"]];
	LArray *days = expanded ? lobj_cast<LArray>(expanded->objectForKey(kReportSevenDaysForecasts)) : 0;
	[_weekdaysMatrix refreshWithDayForecasts:days];
	[_weekdaysMatrix setNeedsDisplay:YES];
	NSInteger column = [_weekdaysMatrix selectedColumn];
	if (column < 0) {
		NSInteger row = 0;
		column = 0;
		[_weekdaysMatrix getRow:&row column:&column ofCell:_weekdaysMatrixSelectedCell];
	}
	LMap *day = days && column < (NSInteger)days->count() ? lobj_cast<LMap>(days->objectAtIndex(column)) : 0;
	[self _setExtendedForecast:day];
}

- (id)_windImageForValue:(NSInteger)aValue
{
	LArray *images = CWManager::instance()->windImagesForIconAtIndex(Utils::windImageIndex(aValue));
	return [NSImage wgImageWithLImage:images && images->count() ? (LImage *)images->objectAtIndex(0) : 0];
}

@end
