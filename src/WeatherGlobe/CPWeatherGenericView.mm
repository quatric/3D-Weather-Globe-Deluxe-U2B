#import "CPWeatherGenericView.h"

// A city property drawn from one of the entry's online reports; the report
// is scheduled while the view is in a visible window.
@implementation CPWeatherGenericView

- (void)_schedule
{
	if (!_scheduled && [self enabled] && [[self window] isVisible]) {
		_entry->scheduleReports((unsigned int)[self _reportMask]);
		_scheduled = YES;
	}
}

- (void)_unschedule
{
	if (_scheduled) {
		_entry->unscheduleReports((unsigned int)[self _reportMask]);
		_scheduled = NO;
	}
}

- (id)initWithCityEntry:(PointEntry *)aEntry
{
	self = [super initWithCityEntry:aEntry];
	if (self) {
		LNotificationCenter *center = LNotificationCenter::center();
		center->addObserver(self, @selector(_reportsWereChanged:),
			PointEntry::kPointEntryWasUpdatedViaOnlineNotification, _entry);
		center->addObserver(self, @selector(_reportsWereChanged:),
			PointEntry::kPointEntryWasExpiredViaOnlineNotification, _entry);
		center->addObserver(self, @selector(_unitsOfMeasureWereChanged), Preferences::kUnitsOfMeasureWereChanged, 0);
		center->addObserver(self, @selector(_internetStateWasChanged), Preferences::kInternetStateWasChanged, 0);
	}
	return self;
}

- (void)dealloc
{
	[self _unschedule];
	LNotificationCenter::center()->removeObserver(self);
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	if (_report) {
		_report->release();
		_report = 0;
	}
	[_accessibilityObjects release];
	[super dealloc];
}

- (void)_unitsOfMeasureWereChanged
{
	[self setNeedsDisplay:YES];
}

- (void)_internetStateWasChanged
{
	if (Preferences::usesInternet())
		[self _schedule];
	else
		[self _unschedule];
}

// Scheduled while the window is key; unscheduled when it closes.
- (void)viewDidMoveToWindow
{
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	if ([self window]) {
		[self _schedule];
		[center addObserver:self selector:@selector(_schedule) name:NSWindowDidBecomeKeyNotification
			object:[self window]];
		[center addObserver:self selector:@selector(_unschedule) name:NSWindowWillCloseNotification
			object:[self window]];
		return;
	}
	[center removeObserver:self name:NSWindowDidBecomeKeyNotification object:nil];
	[center removeObserver:self name:NSWindowWillCloseNotification object:nil];
	[self _unschedule];
}

- (LMap *)reportDictionary
{
	if (!_report) {
		_report = _entry->dictionaryForReport((TReportType)[self _reportType]);
		if (_report)
			_report->retain();
	}
	return _report;
}

- (BOOL)enabled
{
	return Preferences::usesInternet();
}

- (NSUInteger)_reportMask
{
	return 0;
}

- (NSUInteger)_reportType
{
	return 0;
}

// A new or expired report of our type: dropped, to be fetched again.
- (void)_reportsWereChanged:(id)aChanged
{
	LNotification *notification = lobj_cast<LNotification>([aChanged getLObject]);
	if (!notification || !notification->userInfo())
		return;
	LObject *reportID = notification->userInfo()->objectForKey(CWOnlineCityData::kReportIDKey);
	if (!reportID || (NSUInteger)UMapParsingUtils::loadIntFromObject(reportID) != [self _reportType])
		return;
	if (_report) {
		_report->release();
		_report = 0;
	}
	[_accessibilityObjects release];
	_accessibilityObjects = nil;
	[self setNeedsDisplay:YES];
}

@end
