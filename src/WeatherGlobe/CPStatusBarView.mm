#import "CPStatusBarView.h"
#include "AMOnline.h"

// The text attributes and the size all status bars share.
static NSDictionary *_sAttrs = nil;
static NSSize _sSizeForBestFit;

// _state
enum {
	kStateIdle = 0,
	kStateUpdating = 1,
	kStateFailed = 2,
};

// The status line under a city's weather in the city info palette: while
// expired reports update, a spinner and "Updating Weather..." (or
// "Updating failed"), with a refresh button at the right.
@implementation CPStatusBarView

+ (id)_progressIndicator
{
	NSProgressIndicator *indicator = [[NSProgressIndicator new] autorelease];
	[indicator setStyle:NSProgressIndicatorStyleSpinning];
	[indicator setControlSize:NSControlSizeSmall];
	[indicator setDisplayedWhenStopped:NO];
	[indicator sizeToFit];
	return indicator;
}

+ (id)_updateButton
{
	NSButton *button = [[NSButton new] autorelease];
	[button setImage:[NSImage imageNamed:@"WeatherPlt_Refresh"]];
	[button setAlternateImage:[NSImage imageNamed:@"WeatherPlt_Refresh_pressed"]];
	[button setBordered:NO];
	[button setImagePosition:NSImageOnly];
	[button setButtonType:NSButtonTypeMomentaryChange];
	[button sizeToFit];
	return button;
}

// The spinner and the longest text, 4 points apart, with 6 points after.
+ (void)_calculateSizeForBestFit
{
	_sSizeForBestFit = [[self _progressIndicator] frame].size;
	NSSize text = [[[NSBundle mainBundle] localizedStringForKey:@"Updating Weather" value:@"" table:nil]
		sizeWithAttributes:_sAttrs];
	_sSizeForBestFit.width = text.width + 4.0 + _sSizeForBestFit.width + 6.0;
	_sSizeForBestFit.height = (text.height <= _sSizeForBestFit.height ? _sSizeForBestFit.height : text.height) + 4.0;
}

+ (void)initialize
{
	if (_sAttrs)
		return;
	_sAttrs = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
		[NSColor colorWithCalibratedWhite:1.0 alpha:0.5], NSForegroundColorAttributeName,
		[NSFont systemFontOfSize:9.0], NSFontAttributeName, nil];
	[self _calculateSizeForBestFit];
}

+ (id)_menuItemsDescriptors
{
	return nil;
}

- (id)initWithCityEntry:(PointEntry *)aEntry
{
	self = [super initWithCityEntry:aEntry];
	if (self) {
		_progressIndicator = [[[self class] _progressIndicator] retain];
		[_progressIndicator setFrameOrigin:NSMakePoint(3.0,
			floorf((_sSizeForBestFit.height - [_progressIndicator frame].size.height) * 0.5f))];
		_updateButton = [[[self class] _updateButton] retain];
		[_updateButton setAction:@selector(_updateWeather)];
		[_updateButton setTarget:self];
		[self addSubview:_updateButton];
		[self setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
		[self setAutoresizesSubviews:YES];
		LNotificationCenter::center()->addObserver(self, @selector(_reportsWereUpdated:),
			PointEntry::kPointEntryWasUpdatedViaOnlineNotification, _entry);
		LNotificationCenter::center()->addObserver(self, @selector(_reportsWereExpired:),
			PointEntry::kPointEntryWasExpiredViaOnlineNotification, _entry);
		NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
		[center addObserver:self selector:@selector(_startAnimations) name:@"NeedStartAnimationsNotification" object:nil];
		[center addObserver:self selector:@selector(_stopAnimations) name:@"NeedStopAnimationsNotification" object:nil];
		LNotificationCenter::center()->addObserver(self, @selector(_weatherInternetStateWasChanged:),
			CWManager::kWeatherInternetStateWasChangedNotification, 0);
		if (DownloadManager::isInternetAvailable() && [self _isWeatherExpired])
			[self _setState:CWManager::instance()->isWeatherInternetStateOK() ? kStateUpdating : kStateFailed];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[_statusText release];
	[_accessibilityObjects release];
	[_progressIndicator release];
	_progressIndicator = nil;
	[_updateButton release];
	[super dealloc];
}

- (void)_startAnimations
{
	if ([self enabled] && [_progressIndicator superview] && !_isAnimating) {
		[_progressIndicator startAnimation:self];
		_isAnimating = YES;
	}
}

- (void)_stopAnimations
{
	[_progressIndicator stopAnimation:self];
	_isAnimating = NO;
}

- (void)_reportsWereUpdated:(id)aUpdated
{
	if (![self _isWeatherExpired])
		[self _setState:kStateIdle];
}

- (void)_weatherInternetStateWasChanged:(id)aChanged
{
	if ([self _isWeatherExpired])
		[self _reportsWereExpired:aChanged];
	else
		[self _setState:kStateIdle];
}

- (void)_updateWeather
{
	_entry->manualUpdateOfScheduledReports();
}

- (void)setMenu:(id)aMenu
{
	[super setMenu:aMenu];
	[_progressIndicator setMenu:aMenu];
}

- (NSSize)sizeForBestFit
{
	return _sSizeForBestFit;
}

// Shown while there is weather to show and a connection.
- (BOOL)enabled
{
	if (!Preferences::internetState())
		return NO;
	return Preferences::currentConditionsEnabled() || Preferences::expandedForecastDaysCount() >= 1;
}

- (id)accessibilityObjects
{
	if (![self enabled]) {
		[_accessibilityObjects release];
		_accessibilityObjects = nil;
		return nil;
	}
	if (!_accessibilityObjects && [self _isWeatherExpired]) {
		id text = [[[CityPropertyAccessibilityObject alloc] initWithParentView:self frameRect:_lastTextRect
			description:_statusText] autorelease];
		_accessibilityObjects = [[NSArray alloc] initWithObjects:text,
			_state == kStateUpdating ? _progressIndicator : nil, nil];
	}
	return _accessibilityObjects;
}

- (void)_setState:(NSInteger)aState
{
	if (_state == aState)
		return;
	_state = (int)aState;
	[_statusText release];
	_statusText = nil;
	[_accessibilityObjects release];
	_accessibilityObjects = nil;
	if (aState == kStateUpdating) {
		_statusText = [[[NSBundle mainBundle] localizedStringForKey:@"Updating Weather" value:@"" table:nil] retain];
		[self addSubview:_progressIndicator];
		[self _startAnimations];
	} else if (aState == kStateFailed || aState == kStateIdle) {
		if (aState == kStateFailed)
			_statusText = [[[NSBundle mainBundle] localizedStringForKey:@"Updating failed" value:@"" table:nil] retain];
		[self _stopAnimations];
		[_progressIndicator removeFromSuperview];
	}
	[self display];
}

// Expired: the current conditions shown and expired (or missing), else the
// expanded forecast shown and expired.
- (BOOL)_isWeatherExpired
{
	LMap *report = lobj_cast<LMap>(_entry->dictionaryForReport(kReportTypeCurrent));
	if (Preferences::currentConditionsEnabled()
		&& (!report || UMapParsingUtils::loadIntFromObject(report->objectForKey(kReportCommonDataExpired))))
		return YES;
	report = lobj_cast<LMap>(_entry->dictionaryForReport(kReportTypeExpanded));
	if (Preferences::expandedForecastDaysCount() < 1)
		return NO;
	return !report || UMapParsingUtils::loadIntFromObject(report->objectForKey(kReportCommonDataExpired));
}

// The text right of the spinner, centered vertically.
- (void)drawRect:(NSRect)aRect
{
	if (!_statusText)
		return;
	_lastTextRect.size = [_statusText sizeWithAttributes:_sAttrs];
	_lastTextRect.origin.y = floorf(([self bounds].size.height - _lastTextRect.size.height) * 0.5f) + 1.0f;
	_lastTextRect.origin.x = [_progressIndicator frame].size.width + 3.0 + 4.0;
	[_statusText drawInRect:_lastTextRect withAttributes:_sAttrs];
}

// The button 3 points from the right, centered vertically.
- (void)resizeSubviewsWithOldSize:(NSSize)aSize
{
	[super resizeSubviewsWithOldSize:aSize];
	NSRect bounds = [self bounds];
	NSRect frame = [_updateButton frame];
	[_updateButton setFrameOrigin:NSMakePoint(bounds.origin.x + bounds.size.width - 3.0 - frame.size.width,
		bounds.origin.y + floorf((bounds.size.height - frame.size.height) * 0.5f))];
}

// Updating while the weather server answers, else failed.
- (void)_reportsWereExpired:(id)aExpired
{
	NSInteger state = kStateIdle;
	if (DownloadManager::isInternetAvailable())
		state = CWManager::instance()->isWeatherInternetStateOK() ? kStateUpdating : kStateFailed;
	[self _setState:state];
}

@end
