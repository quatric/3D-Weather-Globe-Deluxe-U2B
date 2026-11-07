#import "CPWeatherDescriptionView.h"

// Shared by all the views; rebuilt when the palette font size changes.
static BOOL _sAttributesAreValid = NO;
static BOOL _sSizeIsValid = NO;
static NSSize _sSizeForBestFit;

// The current conditions' text, cut with "..." to fit.
@implementation CPWeatherDescriptionView

+ (void)initialize
{
	LNotificationCenter::center()->addObserver(self, @selector(_acceptAdjustFontSize),
		Preferences::kCIPaletteFontSizeWasChanged, 0);
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_applicationWillTerminate)
		name:NSApplicationWillTerminateNotification object:nil];
}

- (id)initWithCityEntry:(PointEntry *)aEntry
{
	self = [super initWithCityEntry:aEntry];
	if (self)
		[self setAutoresizingMask:NSViewWidthSizable | NSViewMaxYMargin];
	return self;
}

- (id)_attributes
{
	static NSDictionary *attrs = nil;
	if (!_sAttributesAreValid) {
		[attrs release];
		attrs = nil;
	}
	if (!attrs) {
		attrs = [[NSDictionary alloc] initWithObjectsAndKeys:[NSColor whiteColor], NSForegroundColorAttributeName,
			[NSFont systemFontOfSize:Preferences::paletteFontSize() - 2.0f], NSFontAttributeName, nil];
		_sAttributesAreValid = YES;
	}
	return attrs;
}

- (id)_weatherDescription
{
	return [NSString stringReferenceFromLString:Utils::weatherDescription([self reportDictionary])];
}

+ (void)_applicationWillTerminate
{
	LNotificationCenter::center()->removeObserver(self);
}

+ (void)_acceptAdjustFontSize
{
	_sSizeIsValid = NO;
	_sAttributesAreValid = NO;
}

- (void)drawRect:(NSRect)aRect
{
	NSRect bounds = [self bounds];
	CGFloat width = bounds.size.width - 8.0;
	NSString *text = [self _weatherDescription];
	while ([text length] >= 5 && [text sizeWithAttributes:[self _attributes]].width > width)
		text = [[text substringToIndex:[text length] - 4] stringByAppendingString:@"..."];
	[text drawInRect:NSMakeRect(bounds.origin.x + 4.0, bounds.origin.y + 2.0, width, bounds.size.height)
		withAttributes:[self _attributes]];
}

// One line of the font.
- (NSSize)sizeForBestFit
{
	if (!_sSizeIsValid) {
		NSSize size = [@"" sizeWithAttributes:[self _attributes]];
		_sSizeIsValid = YES;
		_sSizeForBestFit = NSMakeSize(size.width + 8.0, size.height + 2.0);
	}
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
	return 1;
}

- (id)accessibilityObjects
{
	if (![self enabled]) {
		[_accessibilityObjects release];
		_accessibilityObjects = nil;
		return nil;
	}
	if (!_accessibilityObjects) {
		NSString *description = [self _weatherDescription];
		if (description) {
			CityPropertyAccessibilityObject *object = [[[CityPropertyAccessibilityObject alloc]
				initWithParentView:self frameRect:[self bounds] description:description] autorelease];
			_accessibilityObjects = [[NSArray alloc] initWithObjects:object, nil];
		}
	}
	return _accessibilityObjects;
}

@end
