#import "WeatherPaletteShortPrintDataSource.h"
#import "SFOneLocationController.h"
#import "WGFilledView.h"
#import "NSView+WGExtensions.h"

// The short forecast print: one SFOneLocationController row per location
// stacked in the placeholder, cut into pages between rows.
@implementation WeatherPaletteShortPrintDataSource

- (id)nibName
{
	return @"ShortWeatherPrintLayout";
}

- (void)awakeFromNib
{
	NSColor *gray = [NSColor colorWithCalibratedWhite:0.82f alpha:1];
	[_topView setFillColor:gray];
	[_bottomView setFillColor:gray];
	NSMutableString *title = [[[_7dayForecastTitle stringValue] mutableCopy] autorelease];
	[title appendFormat:@"%@", Preferences::usesMetricSystem() ? @"C" : @"F"];
	[_7dayForecastTitle setStringValue:title];
}

// The row containing the point.
- (id)_forecastViewForPoint:(NSPoint)aPoint
{
	for (NSView *view in [_placeholderView subviews])
		if (NSPointInRect(aPoint, [view frame]))
			return view;
	return nil;
}

// The rows go top to bottom; the placeholder is sized to them.
- (void)_layoutSFViewsFromControllers:(id)aControllers
{
	[[_placeholderView subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
	CGFloat y = 0;
	NSUInteger count = [aControllers count];
	for (NSUInteger i = 0; i < count; i++) {
		NSView *view = [[aControllers objectAtIndex:i] dataView];
		[view setFrameOrigin:NSMakePoint(0, y)];
		y += [view frame].size.height;
		[_placeholderView addSubview:view];
	}
	NSRect last = [[[aControllers lastObject] dataView] frame];
	[_placeholderView setFrameSize:NSMakeSize([_placeholderView frame].size.width, last.origin.y + last.size.height)];
}

- (void)dataSourceWasChanged
{
	_tmpLastPrintViewOrigin = 0;
}

- (id)printViewName
{
	return [[NSBundle mainBundle] localizedStringForKey:@"Short Weather Forecast" value:@"" table:nil];
}

// The print view is laid out once at the page width; the page count rounds
// up, ignoring a last page under 7% full.
- (NSInteger)pagesCountForSize:(NSSize)aSize
{
	[self loadNibIfNeeded];
	_tmpLastPrintViewOrigin = 0;
	if (_printViewAlreadySetupped)
		return _pagesCount;
	[_printView wgFixFlip];
	_neededPrintViewSize = aSize;
	int count = _entries ? (int)_entries->count() : 0;
	NSMutableArray *controllers = [NSMutableArray arrayWithCapacity:count];
	for (int i = 0; i < count; i++) {
		SFOneLocationController *controller = [[SFOneLocationController new] autorelease];
		[controllers addObject:controller];
		[controller setLocation:_entries->objectAtIndex(i)];
	}
	[self _layoutSFViewsFromControllers:controllers];
	[_printView setFrameSize:aSize];
	[_topView setAutoresizingMask:NSViewNotSizable];
	[_placeholderView setAutoresizingMask:NSViewNotSizable];
	[_bottomView setAutoresizingMask:NSViewNotSizable];
	NSRect placeholder = [_placeholderView frame];
	[_bottomView setFrameOrigin:NSMakePoint([_bottomView frame].origin.x, placeholder.origin.y + placeholder.size.height)];
	[_printView setFrameSize:NSMakeSize([_printView frame].size.width, [_bottomView frame].origin.y + 15.0)];
	_printViewAlreadySetupped = YES;
	float pages = [_printView frame].size.height / aSize.height;
	int whole = (int)pages;
	if (pages - (float)whole >= 0.93f)
		whole = (int)(pages + 0.09f);
	_pagesCount = whole + 1;
	return _pagesCount;
}

// Pages are taken from the bottom of the print view up. A row cut by the
// page's lower edge moves to the next page.
- (id)pageViewAtIndex:(NSInteger)aIndex
{
	if (_tmpLastPrintViewOrigin < 0)
		_tmpLastPrintViewOrigin = 0;
	if (_tmpLastPrintViewOrigin == 0) {
		NSRect frame = [_printView frame];
		_tmpLastPrintViewOrigin = frame.size.height + frame.origin.y;
	}
	NSView *container = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, _neededPrintViewSize.width, _neededPrintViewSize.height)];
	[container addSubview:_printView];
	float height = _neededPrintViewSize.height;
	float bottom = _tmpLastPrintViewOrigin - height;
	_tmpLastPrintViewOrigin = bottom;
	NSPoint point = [container convertPoint:NSMakePoint(0, bottom) toView:_placeholderView];
	NSView *row = [self _forecastViewForPoint:NSMakePoint(0, point.y)];
	if (row) {
		NSRect rect = [container convertRect:[row frame] fromView:_placeholderView];
		float top = rect.origin.y + rect.size.height;
		height = (height - (top - bottom)) + 1.0f;
		bottom = top - 1.0f;
		_tmpLastPrintViewOrigin = top;
	}
	NSRect frame = [container frame];
	NSView *page = [[NSView alloc] initWithFrame:NSMakeRect(0, frame.size.height - height, frame.size.width, height)];
	[page addSubview:_printView];
	[page setBoundsOrigin:NSMakePoint(0, bottom)];
	[container release];
	return [page autorelease];
}

@end
