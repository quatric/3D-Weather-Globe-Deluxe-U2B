#import "CPMainPropertyView.h"

// The cell all main property views draw with.
static NSTextFieldCell *_sDrawCell = nil;
// Cleared to rebuild the shared format and date.
static BOOL _sCalendarFormatIsValid = NO;
static BOOL _sCalendarDateIsValid = NO;

// Bits of Preferences::mainPropertiesDetailViewFlags().
enum {
	kDetailCountry = 1,
	kDetailDate = 2,
	kDetailWeekday = 4,
	kDetailGMTOffset = 8,
	kDetailLocalTime = 0x20,
};

// The first line of a city in the city info palette: its local time (and
// the details chosen in the options menu) above its name.
@implementation CPMainPropertyView

- (id)initWithCityEntry:(PointEntry *)aEntry
{
	self = [super initWithCityEntry:aEntry];
	if (self) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_acceptAdjustTime)
			name:@"AdjustTimeNotification" object:nil];
		LNotificationCenter::center()->addObserver(self, @selector(_acceptAdjustFontSize),
			Preferences::kCIPaletteFontSizeWasChanged, 0);
		LNotificationCenter::center()->addObserver(self, @selector(_acceptDetailsChanges),
			Preferences::kMainPropertiesStateWasChanged, 0);
		LNotificationCenter::center()->addObserver(self, @selector(_clocksMeasureWereChanged),
			Preferences::kClockFormatWasChanged, 0);
		[self setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[_accessibilityObjects release];
	[_formatedTime release];
	[_title release];
	[super dealloc];
}

// "Weekday Time, Date" as chosen.
- (id)_calendarFormat
{
	static NSString *format = nil;
	if (!_sCalendarFormatIsValid) {
		[format release];
		format = nil;
	}
	if (!format) {
		int flags = Preferences::mainPropertiesDetailViewFlags();
		NSMutableArray *parts = [NSMutableArray array];
		if (flags & kDetailWeekday)
			[parts addObject:@"%a"];
		if (flags & kDetailLocalTime) {
			NSMutableString *time = [NSMutableString stringWithString:Preferences::uses24HourClock() ? @"%H:%M" : @"%I:%M %p"];
			if (flags & kDetailDate)
				[time appendString:@","];
			[parts addObject:time];
		}
		if (flags & kDetailDate)
			[parts addObject:@"%b %e"];
		format = [[parts componentsJoinedByString:@" "] retain];
		_sCalendarFormatIsValid = YES;
	}
	return format;
}

// The time shared by all the views until the time is adjusted.
- (id)_calendarDate
{
	static NSCalendarDate *date = nil;
	if (!_sCalendarDateIsValid) {
		[date release];
		date = nil;
	}
	if (!date) {
		date = [[NSCalendarDate calendarDate] retain];
		_sCalendarDateIsValid = YES;
	}
	return date;
}

// The title in bold over the time in the palette font size less 2, with
// 10 points between.
- (void)_calculateSizeIfNeeded
{
	if (_sizeIsValid)
		return;
	[self _validateAllData];
	float fontSize = Preferences::paletteFontSize();
	[_sDrawCell setFont:[NSFont boldSystemFontOfSize:fontSize]];
	[_sDrawCell setStringValue:_title];
	_sizeForBestFit = [_sDrawCell cellSize];
	_sizeForBestFit.width = floorf(_sizeForBestFit.width) + 1.0f;
	_sizeForBestFit.height = floorf(_sizeForBestFit.height) + 1.0f;
	[_sDrawCell setFont:[NSFont systemFontOfSize:fontSize - 2.0f]];
	[_sDrawCell setStringValue:_formatedTime];
	NSSize time = [_sDrawCell cellSize];
	float timeWidth = floorf(time.width) + 1.0f;
	_sizeForBestFit.height = floorf(time.height) + 1.0f + 0.0f + 10.0f + _sizeForBestFit.height;
	if (_sizeForBestFit.width < timeWidth)
		_sizeForBestFit.width = timeWidth;
	_sizeIsValid = YES;
	_sizeForBestFit.width += 0.0f;
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
	NSArray *titles = [NSArray arrayWithObjects:@"Country/Location", @"Local Time", @"Date", @"Weekday", @"GMT Offset", nil];
	NSArray *tags = [NSArray arrayWithObjects:[NSNumber numberWithInt:kDetailCountry], [NSNumber numberWithInt:kDetailLocalTime],
		[NSNumber numberWithInt:kDetailDate], [NSNumber numberWithInt:kDetailWeekday], [NSNumber numberWithInt:kDetailGMTOffset], nil];
	int count = (int)[titles count];
	for (int i = 0; i < count; i++) {
		NSString *title = [[NSBundle mainBundle] localizedStringForKey:[titles objectAtIndex:i] value:@"" table:nil];
		[menu addItem:[self _menuItemWithTitle:title selector:@selector(_detailViewMenu:) tag:[[tags objectAtIndex:i] intValue]]];
	}
	return menu;
}

+ (id)_menuItemsDescriptors
{
	NSArray *keys = [NSArray arrayWithObjects:@"TITLE", @"TAG", @"SELECTOR", nil];
	NSDictionary *offset = [NSDictionary dictionaryWithObjects:[NSArray arrayWithObjects:
		[[NSBundle mainBundle] localizedStringForKey:@"GMT Offset" value:@"" table:nil],
		[NSNumber numberWithInt:kDetailGMTOffset], @"_detailViewMenu:", nil] forKeys:keys];
	NSDictionary *time = [NSDictionary dictionaryWithObjects:[NSArray arrayWithObjects:
		[[NSBundle mainBundle] localizedStringForKey:@"Local Time" value:@"" table:nil],
		[NSNumber numberWithInt:kDetailLocalTime], @"_detailViewMenu:", nil] forKeys:keys];
	NSDictionary *country = [NSDictionary dictionaryWithObjects:[NSArray arrayWithObjects:
		[[NSBundle mainBundle] localizedStringForKey:@"Country/Location" value:@"" table:nil],
		[NSNumber numberWithInt:kDetailCountry], @"_detailViewMenu:", nil] forKeys:keys];
	return [NSArray arrayWithObjects:country, time, offset, nil];
}

// Checks the shown details. (The original enables the items as
// Preferences::doShowWeatherOnStartup() says.)
+ (BOOL)validateMenuItem:(id)aItem
{
	NSInteger tag = [aItem tag];
	if ([aItem action] == @selector(_detailViewMenu:))
		[aItem setState:(Preferences::mainPropertiesDetailViewFlags() & tag) != 0];
	return Preferences::doShowWeatherOnStartup();
}

+ (void)_detailViewMenu:(id)aMenu
{
	Preferences::setMainPropertiesDetailViewFlags(Preferences::mainPropertiesDetailViewFlags() ^ (int)[aMenu tag]);
}

- (void)_clocksMeasureWereChanged
{
	_sCalendarFormatIsValid = NO;
	[_formatedTime release];
	_formatedTime = nil;
	_sizeIsValid = NO;
}

- (void)_acceptAdjustFontSize
{
	_sizeIsValid = NO;
}

- (void)_acceptDetailsChanges
{
	_sCalendarFormatIsValid = NO;
	[_formatedTime release];
	_formatedTime = nil;
	[_title release];
	_title = nil;
	_sizeIsValid = NO;
}

- (void)_acceptAdjustTime
{
	_sCalendarDateIsValid = NO;
	_sCalendarFormatIsValid = NO;
	[_formatedTime release];
	_formatedTime = nil;
	_sizeIsValid = NO;
	[self setNeedsDisplay:YES];
}

// The time 5 points from the top, the name under it.
- (void)drawRect:(NSRect)aRect
{
	[self _validateAllData];
	float fontSize = Preferences::paletteFontSize();
	[_sDrawCell setFont:[NSFont systemFontOfSize:fontSize - 2.0f]];
	[_sDrawCell setStringValue:_formatedTime];
	NSSize size = [_sDrawCell cellSize];
	float width = floorf(size.width) + 1.0f;
	float height = floorf(size.height) + 1.0f;
	_lastDateRect = NSMakeRect(0.0, 5.0, width, height);
	[_sDrawCell drawInteriorWithFrame:_lastDateRect inView:self];
	float y = height + 0.0f + 5.0f;
	[_sDrawCell setFont:[NSFont boldSystemFontOfSize:fontSize]];
	[_sDrawCell setStringValue:_title];
	size = [_sDrawCell cellSize];
	_lastTitleRect = NSMakeRect(0.0, y, floorf(size.width) + 1.0f, floorf(size.height) + 1.0f);
	[_sDrawCell drawInteriorWithFrame:_lastTitleRect inView:self];
}

- (NSSize)sizeForBestFit
{
	[self _calculateSizeIfNeeded];
	return _sizeForBestFit;
}

- (BOOL)enabled
{
	return YES;
}

- (id)accessibilityObjects
{
	if (!_accessibilityObjects) {
		id title = [[[CityPropertyAccessibilityObject alloc] initWithParentView:self frameRect:_lastTitleRect
			description:_title] autorelease];
		id time = [[[CityPropertyAccessibilityObject alloc] initWithParentView:self frameRect:_lastDateRect
			description:_formatedTime] autorelease];
		_accessibilityObjects = [[NSArray alloc] initWithObjects:time, title, nil];
	}
	return _accessibilityObjects;
}

// Rebuilds the time (in the city's time zone, with its GMT offset if
// chosen) and the title ("Name, Region", at most 40 characters).
- (void)_validateAllData
{
	BOOL unchanged = YES;
	if (!_formatedTime) {
		LTimeZone *zone = _entry->timeZone(0);
		if (!zone)
			zone = LTimeZone::defaultTimeZone();
		NSTimeZone *timeZone = zone ? (NSTimeZone *)zone->_cf : nil;
		NSCalendarDate *date = [self _calendarDate];
		[date setTimeZone:timeZone];
		_formatedTime = [date descriptionWithCalendarFormat:[self _calendarFormat]];
		if (Preferences::mainPropertiesDetailViewFlags() & kDetailGMTOffset) {
			int seconds = (int)[timeZone secondsFromGMTForDate:date];
			NSMutableString *time = [NSMutableString stringWithString:_formatedTime];
			if ([time length])
				[time appendString:@" "];
			[time appendFormat:seconds < 1 ? @"%d:%02d" : @"+%d:%02d", seconds / 3600, abs((seconds / 60) % 60)];
			_formatedTime = time;
		}
		[_formatedTime retain];
		unchanged = NO;
	}
	if (!_title) {
		NSMutableArray *parts = [NSMutableArray array];
		LString *name = _entry->ownName(0);
		[parts addObject:name ? (NSString *)name->cfString() : @""];
		if (Preferences::mainPropertiesDetailViewFlags() & kDetailCountry) {
			LString *region = _entry->regionName(0);
			[parts addObject:region ? (NSString *)region->cfString() : @""];
		}
		_title = [parts componentsJoinedByString:@", "];
		if ([_title length] > 40)
			_title = [[_title substringToIndex:37] stringByAppendingString:@"..."];
		[_title retain];
		unchanged = NO;
	}
	if (!unchanged) {
		[_accessibilityObjects release];
		_accessibilityObjects = nil;
	}
	if (!_sDrawCell) {
		_sDrawCell = [NSTextFieldCell new];
		[_sDrawCell setAlignment:NSTextAlignmentCenter];
		[_sDrawCell setTextColor:[NSColor whiteColor]];
	}
}

@end
