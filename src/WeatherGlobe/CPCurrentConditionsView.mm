#import "CPCurrentConditionsView.h"
#import "CityPropertyAccessibilityObject.h"
#import "NSImage+NSImageWGExtensions.h"
#include "AMOnline.h"

// The layout all current conditions views share, rebuilt when the palette
// font size changes.
static BOOL _sRectsAreValid = NO;
static BOOL _sTempAttribsAreValid = NO;
static BOOL _sMetricAttribsAreValid = NO;
static NSRect _sMetricRect;
static NSRect _sSummaryIconRect;
static NSRect _sSummaryIconRealRect;	// the icon images' size
static NSRect _sTemperatureRect;
static NSSize _sSizeForBestFit;

// The first image of a CWManager image list.
static LImage *_firstImage(LArray *images)
{
	if (!images || images->count() == 0)
		return 0;
	return static_cast<LImage *>(images->objectAtIndex(0));
}

// A city's current conditions in the city info palette: the temperature
// in big type (its unit under it, right-aligned) and the weather icon.
@implementation CPCurrentConditionsView

- (id)initWithCityEntry:(PointEntry *)aEntry
{
	self = [super initWithCityEntry:aEntry];
	if (self) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_viewFrameWasChanged)
			name:NSViewFrameDidChangeNotification object:self];
		[self setAutoresizingMask:NSViewMinXMargin | NSViewMinYMargin];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

static NSDictionary *_whiteArialAttributes(float size)
{
	NSFont *font = [NSFont fontWithName:@"Arial" size:size];
	if (!font)
		font = [NSFont systemFontOfSize:size];
	return [[NSDictionary alloc] initWithObjectsAndKeys:[NSColor whiteColor], NSForegroundColorAttributeName,
		font, NSFontAttributeName, nil];
}

// Arial at 2.5 times the palette font size.
- (id)_temperatureAttribs
{
	static NSDictionary *attrs = nil;
	if (!_sTempAttribsAreValid) {
		[attrs release];
		attrs = nil;
	}
	if (!attrs) {
		attrs = _whiteArialAttributes(Preferences::paletteFontSize() * 2.5f);
		_sTempAttribsAreValid = YES;
	}
	return attrs;
}

- (id)_metricAttribs
{
	static NSDictionary *attrs = nil;
	if (!_sMetricAttribsAreValid) {
		[attrs release];
		attrs = nil;
	}
	if (!attrs) {
		attrs = _whiteArialAttributes(Preferences::paletteFontSize());
		_sMetricAttribsAreValid = YES;
	}
	return attrs;
}

- (id)_summaryIconFromReport:(LMap *)aReport
{
	if (!aReport)
		return nil;
	int icon = UMapParsingUtils::loadIntFromObject(aReport->objectForKey(kReportSummaryCode));
	if (!icon)
		return nil;
	return [NSImage wgImageWithLImage:_firstImage(CWManager::instance()->weatherImagesForIconAtIndex(icon))];
}

- (id)_temperatureFromReport:(LMap *)aReport
{
	if (!aReport)
		return nil;
	LObject *temperature = aReport->objectForKey(kReportCurrentTemperature);
	if (!temperature)
		return nil;
	return [NSString stringReferenceFromLString:Utils::temperatureShortDescription(temperature)];
}

- (id)_weatherDescriptionFromReport:(LMap *)aReport
{
	return [NSString stringReferenceFromLString:Utils::weatherDescription(aReport)];
}

// The icon tells the weather in its tool tip.
- (void)_checkToolTipRect
{
	if (_tooltipRectIsValid)
		return;
	[self removeAllToolTips];
	[self _validateAllRects];
	[self addToolTipRect:_sSummaryIconRect owner:self userData:0];
	_tooltipRectIsValid = YES;
}

+ (void)_applicationWillTerminate
{
	LNotificationCenter::center()->removeObserver(self);
}

+ (id)_menuItemsDescriptors
{
	return nil;
}

+ (BOOL)validateMenuItem:(id)aItem
{
	if ([aItem action] == @selector(_enabledStateMenu:))
		[aItem setState:Preferences::currentConditionsEnabled()];
	return Preferences::doShowWeatherOnStartup() && Preferences::usesInternet();
}

+ (void)_enabledStateMenu:(id)aMenu
{
	Preferences::setCurrentConditionsEnabled(!Preferences::currentConditionsEnabled());
}

+ (void)_acceptAdjustFontSize
{
	_sRectsAreValid = NO;
	_sTempAttribsAreValid = NO;
	_sMetricAttribsAreValid = NO;
}

- (void)_viewFrameWasChanged
{
	_tooltipRectIsValid = NO;
	[_accessibilityObjects release];
	_accessibilityObjects = nil;
}

- (void)drawRect:(NSRect)aRect
{
	[self _validateAllRects];
	[self _checkToolTipRect];
	NSString *text = [self _temperatureFromReport:[self reportDictionary]];
	NSRect rect = _sTemperatureRect;
	rect.origin.x += rect.size.width - [text sizeWithAttributes:[self _temperatureAttribs]].width;
	[text drawInRect:rect withAttributes:[self _temperatureAttribs]];
	if ([text length])
		text = [[NSBundle mainBundle] localizedStringForKey:Preferences::usesMetricSystem() ? @"Celcius" : @"Fahrenheit"
			value:@"" table:nil];
	[text drawInRect:_sMetricRect withAttributes:[self _metricAttribs]];
	[[self _summaryIconFromReport:[self reportDictionary]] drawInRect:_sSummaryIconRect fromRect:_sSummaryIconRealRect
		operation:NSCompositingOperationSourceOver fraction:1.0];
}

- (id)view:(id)aView stringForToolTip:(NSInteger)aTip point:(NSPoint)aPoint userData:(void *)aData
{
	return [self _weatherDescriptionFromReport:[self reportDictionary]];
}

- (NSSize)sizeForBestFit
{
	[self _validateAllRects];
	return _sSizeForBestFit;
}

- (BOOL)enabled
{
	return Preferences::currentConditionsEnabled() && [super enabled];
}

- (NSUInteger)_reportMask
{
	return 1;
}

- (NSUInteger)_reportType
{
	return kReportTypeCurrent;
}

- (id)accessibilityObjects
{
	if (![self enabled]) {
		[_accessibilityObjects release];
		_accessibilityObjects = nil;
		return nil;
	}
	if (!_accessibilityObjects) {
		[self _validateAllRects];
		NSMutableArray *objects = [NSMutableArray new];
		NSString *temperature = [self _temperatureFromReport:[self reportDictionary]];
		if (temperature)
			[objects addObject:[[[CityPropertyAccessibilityObject alloc] initWithParentView:self
				frameRect:_sTemperatureRect description:temperature] autorelease]];
		if ([self _summaryIconFromReport:[self reportDictionary]])
			[objects addObject:[[[CityPropertyAccessibilityObject alloc] initWithParentView:self
				frameRect:_sSummaryIconRect description:[self _weatherDescriptionFromReport:[self reportDictionary]]]
				autorelease]];
		_accessibilityObjects = objects;
	}
	return _accessibilityObjects;
}

// The widest temperature, 5 points, the icon scaled with the font, 5
// points; both centered in the row. The unit sits right-aligned at 12% of
// the temperature's height.
- (void)_validateAllRects
{
	if (_sRectsAreValid)
		return;
	_sSizeForBestFit = NSZeroSize;
	NSSize size = [[NSString stringReferenceFromLString:Utils::temperatureValueShortDescription(188.0f)]
		sizeWithAttributes:[self _temperatureAttribs]];
	_sTemperatureRect.size.width = floorf(size.width) + 1.0f;
	_sTemperatureRect.size.height = floorf(size.height) + 1.0f;
	float scale = Preferences::paletteFontSize() / 13.0f;
	_sSummaryIconRect.size.height = scale * _sSummaryIconRealRect.size.height;
	_sSummaryIconRect.size.width = scale * _sSummaryIconRealRect.size.width;
	CGFloat iconX = _sTemperatureRect.size.width + 5.0;
	CGFloat width = iconX + _sSummaryIconRect.size.width + 5.0;
	if (width > _sSizeForBestFit.width)
		_sSizeForBestFit.width = width;
	CGFloat height = _sTemperatureRect.size.height <= _sSummaryIconRect.size.height
		? _sSummaryIconRect.size.height : _sTemperatureRect.size.height;
	_sTemperatureRect.origin.x = 0.0;
	_sTemperatureRect.origin.y = floorf((height - _sTemperatureRect.size.height) * 0.5f);
	_sSizeForBestFit.height += height;
	_sSummaryIconRect.origin.y = floorf((height - _sSummaryIconRect.size.height) * 0.5f);
	_sSummaryIconRect.origin.x = iconX;
	size = [@"C" sizeWithAttributes:[self _metricAttribs]];
	_sMetricRect.size.width = floorf(size.width) + 1.0f;
	_sMetricRect.size.height = floorf(size.height) + 1.0f;
	_sMetricRect.origin.x = _sTemperatureRect.origin.x + _sTemperatureRect.size.width - _sMetricRect.size.width;
	_sMetricRect.origin.y = floorf(_sTemperatureRect.size.height * 0.12f + _sTemperatureRect.origin.y);
	_sRectsAreValid = YES;
}

// The icons' size from the first one.
+ (void)initialize
{
	_sSummaryIconRealRect = NSZeroRect;
	LImage *image = _firstImage(CWManager::instance()->weatherImagesForIconAtIndex(1));
	if (image) {
		_sSummaryIconRealRect.size.height = image->height();
		_sSummaryIconRealRect.size.width = image->width();
	}
	LNotificationCenter::center()->addObserver(self, @selector(_acceptAdjustFontSize),
		Preferences::kCIPaletteFontSizeWasChanged, 0);
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_applicationWillTerminate)
		name:NSApplicationWillTerminateNotification object:nil];
}

@end
