#import "SFOneLocationController.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "WGCurrentSetController.h"
#import "SFOneDayController.h"

@implementation SFOneLocationController

- (void)dealloc
{
	releaseAndClear(_location);
	[_dataView release];
	[super dealloc];
}

- (id)nibName
{
	return @"SFOneLocationView";
}

// The day views go left to right.
- (void)_layoutSFViewsFromControllers:(id)aControllers
{
	[[_sfPlaceholder subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
	CGFloat x = 0;
	NSUInteger count = [aControllers count];
	for (NSUInteger i = 0; i < count; i++) {
		NSView *view = [[aControllers objectAtIndex:i] dataView];
		[view setFrameOrigin:NSMakePoint(x, [view frame].origin.y)];
		x += [view frame].size.width;
		[_sfPlaceholder addSubview:view];
	}
}

- (id)dataView
{
	[self loadNibIfNeeded];
	return _dataView;
}

// Seven days from today in the location's time zone. The forecast days are
// matched up by weekday: the first forecast day's weekday (1–7) moves the
// start forward from the download date.
- (void)setLocation:(PointEntry *)aLocation
{
	[self loadNibIfNeeded];
	releaseAndClear(_location);
	_location = aLocation;
	LRetain(aLocation);
	[_locationName setStringValue:[[[[AtlasController atlasController] mainWindowController] currentSetController]
	                                  entryExtendedInfo:_location
	                                   withLineBreaking:0
	                                       ownEntryName:YES]];
	NSCalendarDate *today = [NSCalendarDate calendarDate];
	LTimeZone *zone = _location->timeZone(0);
	[today setTimeZone:zone ? (NSTimeZone *)zone->_cf : nil];

	LArray *days = 0;
	if (LMap *report = _location->dictionaryForReport(kReportTypeExpanded)) {
		days = lobj_cast<LArray>(report->objectForKey(kReportSevenDaysForecasts));
		LValue *downloaded = lobj_cast<LValue>(report->objectForKey(kReportCommonDownloadDate));
		if (downloaded) {
			NSCalendarDate *date = [NSCalendarDate dateWithTimeIntervalSinceReferenceDate:downloaded->doubleValue()];
			LMap *first = days && days->count() ? lobj_cast<LMap>(days->objectAtIndex(0)) : 0;
			if (first) {
				int weekday = loadIntFromObject(first->objectForKey(kReportFCDayOfWeek));
				if (weekday >= 1 && weekday <= 7)
					while ([date dayOfWeek] != weekday - 1)
						date = [date dateByAddingYears:0 months:0 days:1 hours:0 minutes:0 seconds:0];
			}
		}
	}

	NSMutableArray *controllers = [NSMutableArray arrayWithCapacity:7];
	for (int i = 0; i < 7; i++) {
		SFOneDayController *controller = [[SFOneDayController new] autorelease];
		[controllers addObject:controller];
		LMap *values = days && (unsigned)i < days->count() ? (LMap *)days->objectAtIndex(i) : 0;
		[controller setValues:values forDate:[today dateByAddingYears:0 months:0 days:i hours:0 minutes:0 seconds:0]];
	}
	[self _layoutSFViewsFromControllers:controllers];
}

@end
