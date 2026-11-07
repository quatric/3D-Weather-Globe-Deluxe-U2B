#import "CPExpandedForecastView.h"
#import "CityPropertyAccessibilityObject.h"
#import "NSImage+NSImageWGExtensions.h"
#include "AMOnline.h"

// The layout all expanded forecast views share, rebuilt when the palette
// font size or the days count changes. Each row: the date right-aligned in
// the left half; in the right half the high temperature, "/", the low
// temperature and the weather icon. Only the widths of the data rects are
// fixed; their heights and y are computed per row.
static BOOL _sRectsAreValid = NO;
static BOOL _sHighTempAttrsAreValid = NO;
static BOOL _sLowTempAttrsAreValid = NO;
static BOOL _sDefaultAttrsAreValid = NO;
static BOOL _sDateAttrsAreValid = NO;
static float _sRowHeight;
static NSSize _sSizeForBestFit;
static NSRect _sWeekdayRect;
static NSRect _sLowTempRect;
static NSRect _sHighTempRect;
static NSRect _sSummaryIconRect;
static NSRect _sSeparatorRect;
static NSRect _sRealImageRect;	// the icon images' size

// The first image of a CWManager image list.
static LImage *_firstImage(LArray *images)
{
	if (!images || images->count() == 0)
		return 0;
	return static_cast<LImage *>(images->objectAtIndex(0));
}

static NSDictionary *_attributes(NSString *fontName, NSColor *color, NSParagraphStyle *style)
{
	float size = Preferences::paletteFontSize();
	NSFont *font = [NSFont fontWithName:fontName size:size];
	if (!font)
		font = [NSFont systemFontOfSize:size];
	return [[NSDictionary alloc] initWithObjectsAndKeys:color, NSForegroundColorAttributeName, font,
		NSFontAttributeName, style, NSParagraphStyleAttributeName, nil];
}

// The day the report's forecasts start: its download date moved forward to
// the first forecast's weekday (1 = Sunday), or today without a report.
static NSCalendarDate *_firstForecastDate(LMap *report, LArray *days)
{
	if (!report)
		return [NSCalendarDate calendarDate];
	LValue *downloaded = lobj_cast<LValue>(report->objectForKey(kReportCommonDownloadDate));
	if (!downloaded)
		return [NSCalendarDate calendarDate];
	NSCalendarDate *date = [NSCalendarDate dateWithTimeIntervalSinceReferenceDate:downloaded->doubleValue()];
	LMap *first = days && days->count() ? lobj_cast<LMap>(days->objectAtIndex(0)) : 0;
	if (first) {
		unsigned int weekday = UMapParsingUtils::loadIntFromObject(first->objectForKey(kReportFCDayOfWeek)) - 1;
		if (weekday < 7)
			while ((unsigned int)[date dayOfWeek] != weekday)
				date = [date dateByAddingYears:0 months:0 days:1 hours:0 minutes:0 seconds:0];
	}
	return date;
}

static LArray *_forecastDays(LMap *report)
{
	return report ? lobj_cast<LArray>(report->objectForKey(kReportSevenDaysForecasts)) : 0;
}

static LMap *_forecastDay(LArray *days, int index)
{
	return days && index < (int)days->count() ? lobj_cast<LMap>(days->objectAtIndex(index)) : 0;
}

// A city's forecast for the next days in the city info palette.
@implementation CPExpandedForecastView

- (id)initWithCityEntry:(PointEntry *)aEntry
{
	self = [super initWithCityEntry:aEntry];
	if (self) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_viewFrameWasChanged)
			name:NSViewFrameDidChangeNotification object:self];
		[self setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

- (id)_highTemperatureAttribs
{
	static NSDictionary *attrs = nil;
	if (!_sHighTempAttrsAreValid) {
		[attrs release];
		attrs = nil;
	}
	if (!attrs) {
		attrs = _attributes(@"HelveticaNeue-Bold", [NSColor colorWithCalibratedRed:0.9647f green:0.3686f
			blue:0.4039f alpha:1.0f], nil);
		_sHighTempAttrsAreValid = YES;
	}
	return attrs;
}

- (id)_lowTemperatureAttribs
{
	static NSDictionary *attrs = nil;
	if (!_sLowTempAttrsAreValid) {
		[attrs release];
		attrs = nil;
	}
	if (!attrs) {
		attrs = _attributes(@"HelveticaNeue-Bold", [NSColor colorWithCalibratedRed:0.3059f green:0.5608f
			blue:0.749f alpha:1.0f], nil);
		_sLowTempAttrsAreValid = YES;
	}
	return attrs;
}

- (id)_defaultAttribs
{
	static NSDictionary *attrs = nil;
	if (!_sDefaultAttrsAreValid) {
		[attrs release];
		attrs = nil;
	}
	if (!attrs) {
		attrs = _attributes(@"HelveticaNeue", [NSColor colorWithCalibratedWhite:1.0f alpha:0.7f], nil);
		_sDefaultAttrsAreValid = YES;
	}
	return attrs;
}

- (id)_dateAttribs
{
	static NSDictionary *attrs = nil;
	if (!_sDateAttrsAreValid) {
		[attrs release];
		attrs = nil;
	}
	if (!attrs) {
		NSMutableParagraphStyle *style = [[[NSMutableParagraphStyle alloc] init] autorelease];
		[style setAlignment:NSTextAlignmentRight];
		attrs = _attributes(@"HelveticaNeue", [NSColor colorWithCalibratedWhite:1.0f alpha:0.7f], style);
		_sDateAttrsAreValid = YES;
	}
	return attrs;
}

// The widest date is measured on a day with the widest abbreviated month
// and weekday names.
- (void)_validateAllRects
{
	if (_sRectsAreValid)
		return;
	NSSize size = [@"/" sizeWithAttributes:[self _defaultAttribs]];
	_sSeparatorRect.size.width = (int)size.width + 1;
	_sSeparatorRect.size.height = size.height;
	_sSummaryIconRect.size.width = _sSummaryIconRect.size.height = (float)size.height * 1.5f;
	NSString *widest = [NSString stringReferenceFromLString:Utils::temperatureValueShortDescription(188.0f)];
	size = [widest sizeWithAttributes:[self _highTemperatureAttribs]];
	_sHighTempRect.size.width = (int)size.width + 1;
	_sHighTempRect.size.height = size.height;
	size = [widest sizeWithAttributes:[self _lowTemperatureAttribs]];
	_sLowTempRect.size.width = (int)size.width + 1;
	_sLowTempRect.size.height = size.height;
	NSCalendarDate *date = [NSCalendarDate dateWithYear:2222 month:[NSCalendarDate monthWithMaxAbbrDescriptionWidth]
		day:22 hour:22 minute:22 second:22 timeZone:nil];
	NSInteger weekday = [NSCalendarDate weekdayWithMaxAbbrDescriptionWidth];
	while ([date dayOfWeek] != weekday)
		date = [date dateByAddingYears:0 months:0 days:1 hours:0 minutes:0 seconds:0];
	_sWeekdayRect.size = [[date descriptionWithCalendarFormat:@"%a, %b %e"] sizeWithAttributes:[self _dateAttribs]];
	_sWeekdayRect.origin.x = 0.0;
	_sHighTempRect.origin.x = 5.0;
	float data = 20.0f + (float)_sSummaryIconRect.size.width + (float)_sHighTempRect.size.width
		+ (float)_sSeparatorRect.size.width + (float)_sLowTempRect.size.width;
	float dates = (float)_sWeekdayRect.size.width + 5.0f;
	_sSizeForBestFit.width = 2.0f * (data >= dates ? data : dates);
	_sSeparatorRect.origin.x = _sHighTempRect.size.width + 5.0 + 5.0;
	_sLowTempRect.origin.x = _sSeparatorRect.origin.x + _sSeparatorRect.size.width;
	_sSummaryIconRect.origin.x = _sLowTempRect.origin.x + _sLowTempRect.size.width + 5.0;
	float height = (float)_sSummaryIconRect.size.height + 2.0f;
	if ((float)_sSeparatorRect.size.height > height)
		height = _sSeparatorRect.size.height;
	_sRowHeight = floorf(height) + 1.0f;
	_sSizeForBestFit.height = _sRowHeight * Preferences::expandedForecastDaysCount();
	_sRectsAreValid = YES;
}

- (id)_summaryIconFromOneDayReport:(LMap *)aReport
{
	if (!aReport)
		return nil;
	int icon = UMapParsingUtils::loadIntFromObject(aReport->objectForKey(kReportSummaryCode));
	if (!icon)
		return nil;
	return [NSImage wgImageWithLImage:_firstImage(CWManager::instance()->weatherImagesForIconAtIndex(icon))];
}

- (id)_temperatureFromOneDayReport:(LMap *)aReport high:(BOOL)aHigh
{
	if (!aReport)
		return nil;
	LObject *temperature = aReport->objectForKey(aHigh ? kReportFCTemperatureHigh : kReportFCTemperatureLow);
	if (!temperature)
		return nil;
	return [NSString stringReferenceFromLString:Utils::temperatureShortDescription(temperature)];
}

- (id)_weatherDescriptionFromOneDayReport:(LMap *)aReport
{
	return [NSString stringReferenceFromLString:Utils::weatherDescription(aReport)];
}

// The icons tell the weather in their tool tips; the first day is the top
// row.
- (void)_checkToolTipRects
{
	if (_tooltipsRectsAreValid)
		return;
	[self removeAllToolTips];
	[self _validateAllRects];
	NSRect rect = _sSummaryIconRect;
	rect.origin.x += floorf([self bounds].size.width * 0.5f);
	rect.origin.y = 2.0;
	for (int day = Preferences::expandedForecastDaysCount() - 1; day >= 0; day--) {
		[self addToolTipRect:NSInsetRect(rect, 0.0, 1.0) owner:self userData:0];
		rect.origin.y += _sRowHeight;
	}
	_tooltipsRectsAreValid = YES;
}

+ (void)_applicationWillTerminate
{
	LNotificationCenter::center()->removeObserver(self);
}

+ (id)_menuItemWithTitle:(id)aTitle selector:(SEL)aSelector tag:(NSInteger)aTag
{
	NSMenuItem *item = [[NSMenuItem new] autorelease];
	[item setTitle:aTitle];
	[item setTarget:self];
	[item setAction:aSelector];
	[item setTag:aTag];
	return item;
}

+ (id)_submenu
{
	NSMenu *menu = [[NSMenu new] autorelease];
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *format = [bundle localizedStringForKey:@"DaysCountFormat" value:@"" table:nil];
	[menu addItem:[self _menuItemWithTitle:[bundle localizedStringForKey:@"None" value:@"" table:nil]
		selector:@selector(_enabledStateMenu:) tag:0]];
	[menu addItem:[NSMenuItem separatorItem]];
	for (int days = 3; days <= 7; days += 2)
		[menu addItem:[self _menuItemWithTitle:[NSString stringWithFormat:format, days]
			selector:@selector(_enabledStateMenu:) tag:days]];
	return menu;
}

+ (id)_menuItemsDescriptors
{
	NSString *title = [[NSBundle mainBundle] localizedStringForKey:@"Forecast" value:@"" table:nil];
	NSArray *keys = [NSArray arrayWithObjects:@"TITLE", @"SUBMENU", nil];
	NSArray *values = [NSArray arrayWithObjects:title, [self _submenu], nil];
	return [NSArray arrayWithObjects:[NSDictionary dictionaryWithObjects:values forKeys:keys], nil];
}

+ (BOOL)validateMenuItem:(id)aItem
{
	NSInteger tag = [aItem tag];
	if ([aItem action] == @selector(_enabledStateMenu:))
		[aItem setState:tag == Preferences::expandedForecastDaysCount()];
	return Preferences::doShowWeatherOnStartup() && Preferences::usesInternet();
}

+ (void)_enabledStateMenu:(id)aMenu
{
	_sRectsAreValid = NO;
	Preferences::setExpandedForecastDaysCount((TForecastDaysCount)[aMenu tag]);
}

+ (void)_acceptAdjustFontSize
{
	_sRectsAreValid = NO;
	_sHighTempAttrsAreValid = NO;
	_sLowTempAttrsAreValid = NO;
	_sDefaultAttrsAreValid = NO;
	_sDateAttrsAreValid = NO;
}

- (void)_viewFrameWasChanged
{
	_tooltipsRectsAreValid = NO;
	[_accessibilityObjects release];
	_accessibilityObjects = nil;
}

- (NSSize)sizeForBestFit
{
	[self _validateAllRects];
	return _sSizeForBestFit;
}

- (BOOL)enabled
{
	return Preferences::expandedForecastDaysCount() > 0 && [super enabled];
}

- (NSUInteger)_reportMask
{
	return 4;
}

- (NSUInteger)_reportType
{
	return kReportTypeExpanded;
}

- (id)accessibilityObjects
{
	if (![self enabled]) {
		[_accessibilityObjects release];
		_accessibilityObjects = nil;
		return nil;
	}
	if (_accessibilityObjects)
		return _accessibilityObjects;
	[self _validateAllRects];
	LMap *report = [self reportDictionary];
	NSMutableArray *objects = [NSMutableArray new];
	LArray *days = _forecastDays(report);
	NSCalendarDate *first = _firstForecastDate(report, days);
	float middle = floorf([self bounds].size.width * 0.5f);
	float y = 0.0f;
	for (int day = Preferences::expandedForecastDaysCount() - 1; day >= 0; day--) {
		LMap *forecast = _forecastDay(days, day);
		NSString *date = [[first dateByAddingYears:0 months:0 days:day hours:0 minutes:0 seconds:0]
			descriptionWithCalendarFormat:@"%a, %b %e"];
		[objects addObject:[[[CityPropertyAccessibilityObject alloc] initWithParentView:self
			frameRect:NSMakeRect(0.0, y, middle, _sRowHeight) description:date] autorelease]];
		NSString *text = [self _temperatureFromOneDayReport:forecast high:YES];
		if (text)
			[objects addObject:[[[CityPropertyAccessibilityObject alloc] initWithParentView:self
				frameRect:NSMakeRect(middle + _sHighTempRect.origin.x, y, _sHighTempRect.size.width, _sRowHeight)
				description:text] autorelease]];
		text = [self _temperatureFromOneDayReport:forecast high:NO];
		if (text)
			[objects addObject:[[[CityPropertyAccessibilityObject alloc] initWithParentView:self
				frameRect:NSMakeRect(middle + _sLowTempRect.origin.x, y, _sLowTempRect.size.width, _sRowHeight)
				description:text] autorelease]];
		if ([self _summaryIconFromOneDayReport:forecast])
			[objects addObject:[[[CityPropertyAccessibilityObject alloc] initWithParentView:self
				frameRect:NSMakeRect(middle + _sSummaryIconRect.origin.x, y, _sSummaryIconRect.size.width, _sRowHeight)
				description:[self _weatherDescriptionFromOneDayReport:forecast]] autorelease]];
		y += _sRowHeight;
	}
	_accessibilityObjects = objects;
	return _accessibilityObjects;
}

- (id)view:(id)aView stringForToolTip:(NSInteger)aTip point:(NSPoint)aPoint userData:(void *)aData
{
	[self _validateAllRects];
	NSRect rect = _sSummaryIconRect;
	rect.origin.x += floorf([self bounds].size.width * 0.5f);
	rect.origin.y = 2.0;
	for (int day = Preferences::expandedForecastDaysCount() - 1; day >= 0; day--) {
		if (NSPointInRect(aPoint, rect)) {
			LArray *days = _forecastDays([self reportDictionary]);
			if (!days)
				return nil;
			return [self _weatherDescriptionFromOneDayReport:_forecastDay(days, day)];
		}
		rect.origin.y += _sRowHeight;
	}
	return nil;
}

// Odd days get a darker band behind the data; a line separates the dates
// from the data and runs along the bottom.
- (void)drawRect:(NSRect)aRect
{
	[self _validateAllRects];
	[self _checkToolTipRects];
	LMap *report = [self reportDictionary];
	LArray *days = _forecastDays(report);
	NSCalendarDate *first = _firstForecastDate(report, days);
	NSRect bounds = [self bounds];
	float middle = floorf(bounds.size.width * 0.5f);
	_sWeekdayRect.origin.x = floorf(middle - 5.0f - (float)_sWeekdayRect.size.width);
	_sWeekdayRect.origin.y = floorf((_sRowHeight - (float)_sWeekdayRect.size.height) * 0.5f) + 0.0f;
	float y = 0.0f;
	for (int day = Preferences::expandedForecastDaysCount() - 1; day >= 0; day--) {
		LMap *forecast = _forecastDay(days, day);
		NSString *date = [[first dateByAddingYears:0 months:0 days:day hours:0 minutes:0 seconds:0]
			descriptionWithCalendarFormat:@"%a, %b %e"];
		[date drawInRect:_sWeekdayRect withAttributes:[self _dateAttribs]];
		_sWeekdayRect.origin.y += _sRowHeight;
		if (day & 1) {
			[[NSColor colorWithCalibratedWhite:0.2314f alpha:Preferences::paletteTransparencyLevel()] set];
			NSRectFill(NSMakeRect(middle, y, bounds.size.width - middle, _sRowHeight));
		}
		[[self _summaryIconFromOneDayReport:forecast] drawInRect:NSMakeRect(_sSummaryIconRect.origin.x + middle,
			y + 2.0f, _sSummaryIconRect.size.width, _sSummaryIconRect.size.height) fromRect:_sRealImageRect
			operation:NSCompositingOperationSourceOver fraction:1.0];
		NSString *low = [self _temperatureFromOneDayReport:forecast high:NO];
		float width = [low sizeWithAttributes:[self _lowTemperatureAttribs]].width;
		NSRect rect = _sLowTempRect;
		rect.origin.x = floorf((float)_sLowTempRect.size.width - width) + middle + (float)_sLowTempRect.origin.x;
		rect.origin.y = y + floorf((_sRowHeight - (float)_sLowTempRect.size.height) * 0.5f);
		[low drawInRect:rect withAttributes:[self _lowTemperatureAttribs]];
		NSString *high = [self _temperatureFromOneDayReport:forecast high:YES];
		width = [high sizeWithAttributes:[self _highTemperatureAttribs]].width;
		rect = _sHighTempRect;
		rect.origin.x = floorf((float)(_sHighTempRect.size.width + _sHighTempRect.origin.x) - width) + middle;
		rect.origin.y = y + floorf((_sRowHeight - (float)_sHighTempRect.size.height) * 0.5f);
		[high drawInRect:rect withAttributes:[self _highTemperatureAttribs]];
		if ([low length] || high) {
			rect = _sSeparatorRect;
			rect.origin.x = middle + _sSeparatorRect.origin.x;
			rect.origin.y = y + floorf((_sRowHeight - (float)_sSeparatorRect.size.height) * 0.5f);
			[@"/" drawInRect:rect withAttributes:[self _defaultAttribs]];
		}
		y += _sRowHeight;
	}
	[[NSColor colorWithCalibratedWhite:Preferences::separatorColorWhiteComponent()
		alpha:Preferences::paletteTransparencyLevel()] set];
	NSFrameRect(NSMakeRect(middle, 0.0, 1.0, bounds.size.height));
	NSFrameRect(NSMakeRect(0.0, 0.0, bounds.size.width, 1.0));
}

// The icons' size from the first one.
+ (void)initialize
{
	_sRealImageRect = NSZeroRect;
	LImage *image = _firstImage(CWManager::instance()->weatherImagesForIconAtIndex(1));
	if (image) {
		_sRealImageRect.size.height = image->height();
		_sRealImageRect.size.width = image->width();
	}
	LNotificationCenter::center()->addObserver(self, @selector(_acceptAdjustFontSize),
		Preferences::kCIPaletteFontSizeWasChanged, 0);
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_applicationWillTerminate)
		name:NSApplicationWillTerminateNotification object:nil];
}

@end
