#import "WeatherPaletteExtPrintDataSource.h"
#import "EFOneDayController.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "NSImage+NSImageWGExtensions.h"
#import "NSMutableString+NSStringWGExtensions.h"

// The report value's description, or "".
static NSString *_description(LString *description)
{
	NSString *string = [NSString stringReferenceFromLString:description];
	return string ? string : @"";
}

// The extended forecast print: a page per location with its current
// conditions and seven forecast days. Expired reports are downloaded first
// behind a progress sheet.
@implementation WeatherPaletteExtPrintDataSource

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	[_downloadWeatherDialog release];
	[super dealloc];
}

- (id)nibName
{
	return @"ExtWeatherPrintLayout";
}

- (void)awakeFromNib
{
	_isPrintForecast = YES;
	_weatherDownloadDone = 0;
}

- (BOOL)_isCurrentConditionExpiredForEntry:(PointEntry *)aEntry
{
	LMap *report = aEntry ? aEntry->dictionaryForReport(kReportTypeCurrent) : 0;
	return !report || UMapParsingUtils::loadIntFromObject(report->objectForKey(kReportCommonDataExpired));
}

- (BOOL)_isExtendedForecastExpiredForEntry:(PointEntry *)aEntry
{
	LMap *report = aEntry ? aEntry->dictionaryForReport(kReportTypeExpanded) : 0;
	return !report || UMapParsingUtils::loadIntFromObject(report->objectForKey(kReportCommonDataExpired));
}

- (BOOL)_isWeatherExpiredForEntry:(PointEntry *)aEntry
{
	return [self _isCurrentConditionExpiredForEntry:aEntry] || [self _isExtendedForecastExpiredForEntry:aEntry];
}

// Closes the progress sheet, which ends the preparation.
- (void)_endDownloadSheet
{
	NSWindow *parent = [_downloadWeatherDialog sheetParent];
	[parent endSheet:_downloadWeatherDialog];
	[_downloadWeatherDialog close];
}

- (void)printWithoutWeather:(id)aWeather
{
	_isPrintForecast = NO;
	[self _endDownloadSheet];
}

// The day views go left to right.
- (void)_layoutEFViewsFromControllers:(id)aControllers
{
	[[_efPlaceholder subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
	CGFloat x = 0.0;
	int count = (int)[aControllers count];
	for (int i = 0; i < count; i++) {
		NSView *view = [[aControllers objectAtIndex:i] dataView];
		[view setFrameOrigin:NSMakePoint(x, [view frame].origin.y)];
		x += [view frame].size.width;
		[_efPlaceholder addSubview:view];
	}
}

- (void)dataSourceWasChanged
{
	_currentPageNum = 0;
}

- (id)printViewName
{
	return [[NSBundle mainBundle] localizedStringForKey:@"Extended Weather Forecast" value:@"" table:nil];
}

// Each downloaded report counts down; the sheet closes when all arrived.
- (void)_reportsWereChanged:(id)aChanged
{
	LNotification *notification = lobj_cast<LNotification>([aChanged getLObject]);
	PointEntry *entry = notification ? lobj_cast<PointEntry>(notification->object()) : 0;
	if (!entry)
		return;
	if (entry->dictionaryForReport(kReportTypeCurrent)) {
		entry->unscheduleReports(1);
		_weatherDownloadDone--;
	}
	if (entry->dictionaryForReport(kReportTypeExpanded)) {
		entry->unscheduleReports(4);
		_weatherDownloadDone--;
	}
	if (_weatherDownloadDone < 1)
		[self _endDownloadSheet];
}

// A page per location (none when printing without weather); the print
// view is scaled once to fit the page.
- (NSInteger)pagesCountForSize:(NSSize)aSize
{
	[self loadNibIfNeeded];
	_currentPageNum = 0;
	if (!_printViewAlreadySetupped) {
		NSRect bounds = [_printView bounds];
		CGFloat scale = aSize.width / bounds.size.width;
		if (aSize.height / bounds.size.height <= scale)
			scale = aSize.height / bounds.size.height;
		[_printView scaleUnitSquareToSize:NSMakeSize(scale, scale)];
		[_printView setFrameSize:aSize];
		_printViewAlreadySetupped = YES;
	}
	return _isPrintForecast ? _entries->count() : 0;
}

- (void)_downloadWeatherDialogDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	LNotificationCenter::center()->removeObserver(self);
	for (unsigned int i = 0; i < _entries->count(); i++)
		_entries->objectAtIndex(i)->unscheduleReports(5);
	[_endWeatherPrepareTarget performSelector:_endWeatherPrepareCallback];
}

// The pages go round the locations. The forecast days start at the first
// forecast day's weekday on or after the download date, else today.
- (id)pageViewAtIndex:(NSInteger)aIndex
{
	int index = _currentPageNum;
	if (index >= (int)_entries->count())
		index = 0;
	_currentPageNum = index + 1;
	PointEntry *entry = _entries->objectAtIndex(index);
	[_locationName setStringValue:_description(entry ? entry->ownName(0) : 0)];
	[_locationKind setStringValue:_description(entry ? entry->kind(0) : 0)];
	[_region setStringValue:_description(entry ? entry->fullRegionName(0) : 0)];
	[_locationIcon setImage:[NSImage wgImageWithLImage:entry->icon(0)]];
	[_coordinates setStringValue:[NSString wgShortGeoCoordinatesDescription:entry->geoPoint(0)]];

	NSCalendarDate *date = [NSCalendarDate calendarDate];
	LTimeZone *zone = entry->timeZone(0);
	NSTimeZone *timeZone = zone ? (NSTimeZone *)zone->_cf : nil;
	[date setTimeZone:timeZone];
	[_dateAndTime setStringValue:[date descriptionWithCalendarFormat:Preferences::uses24HourClock()
		? @"%A, %B %d, %H:%M" : @"%A, %B %d, %I:%M %p"]];
	int seconds = (int)[timeZone secondsFromGMTForDate:date];
	[_timeOffset setStringValue:[NSString stringWithFormat:seconds < 1 ? @"%d:%02d" : @"+%d:%02d",
		seconds / 3600, abs((seconds / 60) % 60)]];

	[self _setCurrentConditions:entry->dictionaryForReport(kReportTypeCurrent)];
	LArray *days = 0;
	if (LMap *report = entry->dictionaryForReport(kReportTypeExpanded)) {
		days = lobj_cast<LArray>(report->objectForKey(kReportSevenDaysForecasts));
		LValue *downloaded = lobj_cast<LValue>(report->objectForKey(kReportCommonDownloadDate));
		if (downloaded) {
			date = [NSCalendarDate dateWithTimeIntervalSinceReferenceDate:downloaded->doubleValue()];
			LMap *first = days && days->count() ? lobj_cast<LMap>(days->objectAtIndex(0)) : 0;
			if (first) {
				int weekday = UMapParsingUtils::loadIntFromObject(first->objectForKey(kReportFCDayOfWeek));
				if (weekday >= 1 && weekday <= 7)
					while ([date dayOfWeek] != weekday - 1)
						date = [date dateByAddingYears:0 months:0 days:1 hours:0 minutes:0 seconds:0];
			}
		}
	}

	NSMutableArray *controllers = [NSMutableArray arrayWithCapacity:7];
	for (int i = 0; i < 7; i++) {
		EFOneDayController *controller = [[EFOneDayController new] autorelease];
		[controllers addObject:controller];
		LMap *values = days && (unsigned)i < days->count() ? (LMap *)days->objectAtIndex(i) : 0;
		[controller setValues:values forDate:[date dateByAddingYears:0 months:0 days:i hours:0 minutes:0 seconds:0]];
	}
	[self _layoutEFViewsFromControllers:controllers];
	return _printView;
}

- (void)_setCurrentConditions:(LMap *)aConditions
{
	LMap *values = aConditions;
	[_temperature setStringValue:values ? _description(Utils::temperatureDescription(values->objectForKey(kReportCurrentTemperature))) : @""];
	[_feelsLike setStringValue:values ? _description(Utils::temperatureDescription(values->objectForKey(kReportComfortLevel))) : @""];
	[_windSpeed setStringValue:values ? _description(Utils::speedDescription(values->objectForKey(kReportWindSpeed))) : @""];
	[_humidity setStringValue:values ? _description(Utils::percentsDescription(values->objectForKey(kReportHumidity))) : @""];
	[_visibility setStringValue:values ? _description(Utils::distanceDescription(values->objectForKey(kReportVisibility))) : @""];
	[_devPoint setStringValue:values ? _description(Utils::temperatureDescription(values->objectForKey(kReportDewPoint))) : @""];
	[_pressure setStringValue:values ? _description(Utils::pressureDescription(values->objectForKey(kReportBarometricPressure))) : @""];
	NSImage *condition = nil;
	int icon = values ? UMapParsingUtils::loadIntFromObject(values->objectForKey(kReportSummaryCode)) : 0;
	if (icon)
		condition = [NSImage imageNamed:[NSString stringWithFormat:@"weather_icon%d", icon - 1]];
	[_conditinIcon setImage:condition];
	LImage *wind = 0;
	if (values) {
		int index = Utils::windImageIndex(UMapParsingUtils::loadIntFromObject(values->objectForKey(kReportWindDir)));
		if (index) {
			LArray *images = CWManager::instance()->windImagesForIconAtIndex(index);
			if (images && images->count())
				wind = static_cast<LImage *>(images->objectAtIndex(0));
		}
	}
	[_windDirectionIcon setImage:[NSImage wgImageWithLImage:wind]];
}

// Schedules the expired reports and shows the progress sheet; without the
// Internet, or with nothing to download, the target is called at once.
- (void)prepareWeatherDidEndSelector:(SEL)aSelector target:(id)aTarget
{
	if (!Preferences::usesInternet() || !CWManager::instance()->isWeatherInternetStateOK()) {
		[aTarget performSelector:aSelector];
		return;
	}
	[self loadNibIfNeeded];
	_endWeatherPrepareCallback = aSelector;
	_endWeatherPrepareTarget = aTarget;
	CWManager::instance()->resetWeatherInternetState();
	for (unsigned int i = 0; i < _entries->count(); i++) {
		PointEntry *entry = _entries->objectAtIndex(i);
		if (![self _isWeatherExpiredForEntry:entry])
			continue;
		_weatherDownloadDone += 2;
		LNotificationCenter::center()->addObserver(self, @selector(_reportsWereChanged:),
			PointEntry::kPointEntryWasUpdatedViaOnlineNotification, entry);
		LNotificationCenter::center()->addObserver(self, @selector(_reportsWereChanged:),
			PointEntry::kPointEntryWasExpiredViaOnlineNotification, entry);
		entry->scheduleReports(5);
	}
	if (!_weatherDownloadDone) {
		[_endWeatherPrepareTarget performSelector:_endWeatherPrepareCallback];
		return;
	}
	[_downloadProgress startAnimation:self];
	NSWindow *mainWindow = [[[AtlasController atlasController] mainWindowController] mainWindow];
	[mainWindow beginSheet:_downloadWeatherDialog completionHandler:^(NSModalResponse returnCode) {
		[self _downloadWeatherDialogDidEnd:_downloadWeatherDialog returnCode:returnCode contextInfo:0];
	}];
}

@end
