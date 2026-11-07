#import "WGEntryDetailsWeatherDescriptionDataProvider.h"
#import "SFOneDayController.h"

@implementation WGEntryDetailsWeatherDescriptionDataProvider

// The days side by side; the first sizes the view.
- (void)_layoutSFViewsFromControllers:(id)aControllers
{
	int count = (int)[aControllers count];
	float x = 0.0f;
	for (int i = 0; i < count; i++) {
		NSView *view = [[aControllers objectAtIndex:i] dataView];
		if (NSEqualSizes([_descriptionView frame].size, NSZeroSize))
			[_descriptionView setFrameSize:NSMakeSize(count * [view frame].size.width, [view frame].size.height)];
		[view setFrameOrigin:NSMakePoint(x, [view frame].origin.y)];
		x += [view frame].size.width;
		[_descriptionView addSubview:view];
	}
}

- (void)updateDescription
{
}

- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	return [_descriptionView frame].size;
}

// The seven days of the report's forecast, from the first forecast's day
// of the week on or after the download date.
- (id)descriptionView
{
	if (_descriptionView)
		return _descriptionView;
	_descriptionView = [[NSView alloc] initWithFrame:NSZeroRect];
	LMap *report = lobj_cast<LMap>((LObject *)_object);
	NSCalendarDate *date = nil;
	LArray *forecasts = 0;
	if (report) {
		forecasts = lobj_cast<LArray>(report->objectForKey(kReportSevenDaysForecasts));
		LValue *downloaded = lobj_cast<LValue>(report->objectForKey(kReportCommonDownloadDate));
		if (downloaded) {
			date = [NSCalendarDate dateWithTimeIntervalSinceReferenceDate:downloaded->doubleValue()];
			LMap *first = forecasts && forecasts->count() ? lobj_cast<LMap>(forecasts->objectAtIndex(0)) : 0;
			if (first) {
				int weekday = loadIntFromObject(first->objectForKey(kReportFCDayOfWeek));
				if ((unsigned)(weekday - 1) < 7) {
					while ([date dayOfWeek] != weekday - 1)
						date = [date dateByAddingYears:0 months:0 days:1 hours:0 minutes:0 seconds:0];
				}
			}
		}
	}
	NSMutableArray *days = [NSMutableArray arrayWithCapacity:7];
	for (int i = 0; i != 7; i++) {
		SFOneDayController *day = [[SFOneDayController new] autorelease];
		[days addObject:day];
		NSCalendarDate *dayDate = [date dateByAddingYears:0 months:0 days:i hours:0 minutes:0 seconds:0];
		LMap *values = forecasts && (unsigned)i < forecasts->count() ? static_cast<LMap *>(forecasts->objectAtIndex(i)) : 0;
		[day setValues:values forDate:dayDate];
		[day setDrawsTopAndBottomBorders:YES];
		if (i == 6)
			[day setDrawsRightBorder:YES];
	}
	[self _layoutSFViewsFromControllers:days];
	return _descriptionView;
}

@end
