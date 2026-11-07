#import "SMKAngleSliderCell.h"

static NSImage *__sBackgroundImage;
static NSImage *__sIndicatorImage;

// Draws an image upright with its bottom-left corner at aPoint, as the
// removed -dissolveToPoint:fraction: did in flipped and unflipped views alike.
static void SMKDissolveImage(NSImage *aImage, NSPoint aPoint, CGFloat aFraction, BOOL aFlipped)
{
	NSRect to;
	to.size = [aImage size];
	to.origin = aPoint;
	if (aFlipped) {
		to.origin.y -= to.size.height;
	}
	[aImage drawInRect:to fromRect:NSZeroRect operation:NSCompositingOperationSourceOver
	          fraction:aFraction respectFlipped:YES hints:nil];
}

// A round knob whose indicator points at an angle. With zero on top (the
// default) 0 is at the top going clockwise; otherwise 0 is on the right going
// counterclockwise. The angle is the cell's object value, in degrees.
@implementation SMKAngleSliderCell

+ (void)setupImages
{
	NSBundle *bundle = [NSBundle bundleForClass:[SMKAngleSliderCell class]];
	NSString *name = @"angleSliderIndicator";
	if ([NSColor respondsToSelector:@selector(currentControlTint)] &&
	    [NSColor currentControlTint] == NSGraphiteControlTint) {
		name = @"angleSliderIndicator_G";
	}
	if (__sBackgroundImage == nil) {
		__sBackgroundImage = [[NSImage alloc] initWithContentsOfFile:
		    [bundle pathForResource:@"angleSliderBack" ofType:@"tiff"]];
	}
	[__sIndicatorImage autorelease];
	__sIndicatorImage = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:name ofType:@"tiff"]];
}

+ (void)controlTintDidChangeNotification:(id)aNotification
{
	[self setupImages];
}

+ (BOOL)prefersTrackingUntilMouseUp
{
	return YES;
}

+ (void)initialize
{
	[self setupImages];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(controlTintDidChangeNotification:)
	                                             name:NSControlTintDidChangeNotification object:nil];
}

// the indicator's position in the background image for an angle in degrees
- (NSPoint)pointFromAngleValue:(CGFloat)aValue
{
	NSSize size = [[self _backgroundImage] size];
	float radius = [self radius];
	double angle = ((double)(float)aValue * M_PI) / 180.0;
	if (!_isZeroOnTop) {
		return NSMakePoint((float)size.width * 0.5f + (float)cos(angle) * radius,
		                   (float)size.height * 0.5f + radius * (float)sin(angle));
	}
	return NSMakePoint((float)size.width * 0.5f + (float)sin(angle) * radius,
	                   (float)size.height * 0.5f + radius * (float)cos(angle));
}

- (CGFloat)angleValueAtPoint:(NSPoint)aPoint increment:(NSInteger)aIncrement
{
	NSSize size = [[self _backgroundImage] size];
	float dx = (float)aPoint.x + (float)size.width * -0.5f;
	float dy = (float)aPoint.y + (float)size.height * -0.5f;
	float radius = [self radius];
	double a = _isZeroOnTop ? acos((double)(dy / radius)) : acos((double)(dx / radius));
	float angle = (float)((a * 180.0) / M_PI);
	if ((dx < 0.0f && _isZeroOnTop) || (dy < 0.0f && !_isZeroOnTop)) {
		angle = 360.0f - angle;
	}
	if (aIncrement != 0) {
		angle = roundf(angle / (float)aIncrement) * (float)aIncrement;
	}
	return angle;
}

// projects a point onto the indicator's circle, snapping to 45 degrees with
// shift and 15 with option
- (NSPoint)calcIndicatorPoint:(NSPoint)aPoint mouseFlags:(NSInteger)aFlags
{
	NSSize size = [[self _backgroundImage] size];
	float dx = (float)aPoint.x + (float)size.width * -0.5f;
	float dy = (float)aPoint.y + (float)size.height * -0.5f;
	if (dx == 0.0f && dy == 0.0f) {
		return NSZeroPoint;
	}
	float scale = (float)[self radius] / sqrtf(dx * dx + dy * dy);
	NSPoint point = NSMakePoint(dx * scale + (float)size.width * 0.5f, dy * scale + (float)size.height * 0.5f);
	NSInteger increment;
	if (aFlags & NSEventModifierFlagShift) {
		increment = 45;
	} else if (aFlags & NSEventModifierFlagOption) {
		increment = 15;
	} else {
		return point;
	}
	return [self pointFromAngleValue:[self angleValueAtPoint:point increment:increment]];
}

- (NSUInteger)mouseFlags
{
	return [[NSApp currentEvent] modifierFlags];
}

- (CGFloat)radius
{
	NSSize size = [[self _backgroundImage] size];
	float side = (float)size.width < (float)size.height ? (float)size.width : (float)size.height;
	return side * 0.5f * _radiusMultiplier;
}

- (id)_indicatorImage
{
	return _indicatorImage != nil ? _indicatorImage : __sIndicatorImage;
}

- (id)_backgroundImage
{
	return _backgroundImage != nil ? _backgroundImage : __sBackgroundImage;
}

// the arrow keys: direction 0 turns clockwise, 1 counterclockwise
- (void)moveInDirection:(NSInteger)aDirection withFlags:(NSInteger)aFlags
{
	float step;
	if (aFlags & NSEventModifierFlagShift) {
		step = 45.0f;
	} else if (aFlags & NSEventModifierFlagOption) {
		step = 15.0f;
	} else {
		step = 1.0f;
	}
	float angle;
	if (aDirection == 0) {
		angle = roundf([self floatValue] / step) * step + step;
	} else {
		angle = roundf([self floatValue] / step) * step - step;
	}
	[self setFloatValue:angle];
	if ([[self controlView] respondsToSelector:@selector(sendAction:to:)]) {
		[(NSControl *)[self controlView] sendAction:[self action] to:[self target]];
	}
}

// a missing value becomes 0 (this call still returns nil)
- (id)objectValue
{
	id value = [super objectValue];
	if (value == nil) {
		[self setObjectValue:[NSNumber numberWithFloat:0.0f]];
	}
	return value;
}

- (void)setFloatValue:(float)aValue
{
	[self setObjectValue:[NSNumber numberWithFloat:aValue]];
}

- (float)floatValue
{
	return [[self objectValue] floatValue];
}

- (void)setIntValue:(int)aValue
{
	[self setFloatValue:(float)aValue];
}

// whole degrees, 360 reading as 0
- (int)intValue
{
	int value = (int)lroundf([self floatValue]);
	return value == 360 ? 0 : value;
}

- (void)setZeroOnTop:(BOOL)aTop
{
	if (_isZeroOnTop != aTop) {
		_isZeroOnTop = aTop;
		if ([[self controlView] respondsToSelector:@selector(updateCell:)]) {
			[[self controlView] performSelector:@selector(updateCell:) withObject:self];
		}
	}
}

- (BOOL)isZeroOnTop
{
	return _isZeroOnTop;
}

- (void)setRadiusMultiplierToSmallestBackgroundSize:(CGFloat)aSize
{
	_radiusMultiplier = aSize;
}

- (CGFloat)radiusMultiplierToSmallestBackgroundSize
{
	return _radiusMultiplier;
}

- (void)setRadiusDrawingEnabled:(BOOL)aEnabled
{
	_isRadiusDrawn = aEnabled;
}

- (BOOL)isRadiusDrawingEnabled
{
	return _isRadiusDrawn;
}

- (void)setIndicatorImage:(id)aImage
{
	if (_indicatorImage != aImage) {
		[_indicatorImage release];
		_indicatorImage = [aImage retain];
	}
}

- (void)setBackgroundImage:(id)aImage
{
	if (_backgroundImage != aImage) {
		[_backgroundImage release];
		_backgroundImage = [aImage retain];
	}
}

- (id)indicatorImage
{
	return _indicatorImage;
}

- (id)backgroundImage
{
	return _backgroundImage;
}

- (NSSize)contentSize
{
	return [[self _backgroundImage] size];
}

// moves the indicator to the mouse; YES if the point gave a valid angle
- (BOOL)_trackToPoint:(NSPoint)aPoint inView:(id)aView
{
	float y = aPoint.y;
	if (![aView isFlipped]) {
		y = (float)aPoint.y - ((float)_cellFrame.size.height - (float)[[self _backgroundImage] size].height);
	}
	if ([aView isFlipped]) {
		y = (float)[[self _backgroundImage] size].height - y;
	}
	NSPoint point = [self calcIndicatorPoint:NSMakePoint(aPoint.x, y) mouseFlags:[self mouseFlags]];
	float angle = [self angleValueAtPoint:point increment:0];
	if (angle < 0.0f || angle > 360.0f) {
		return NO;
	}
	[self setFloatValue:angle];
	[aView setNeedsDisplayInRect:_cellFrame];
	return YES;
}

- (BOOL)continueTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView
{
	[self _trackToPoint:aAt inView:aView];
	return [super continueTracking:aTracking at:aAt inView:aView];
}

- (BOOL)startTrackingAt:(NSPoint)aAt inView:(id)aView
{
	if (![self _trackToPoint:aAt inView:aView]) {
		return NO;
	}
	return [super startTrackingAt:aAt inView:aView];
}

- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp
{
	_cellFrame = aRect;
	_isHighlighted = YES;
	BOOL result = [super trackMouse:aMouse inRect:aRect ofView:aView untilMouseUp:aUp];
	_isHighlighted = NO;
	return result;
}

// The background sits in the top-left corner of the frame. As in the
// original, the radius line and the indicator are placed in the background's
// coordinates without the frame's origin, so the cell expects a frame at (0,0).
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	NSImage *indicator = [self _indicatorImage];
	NSImage *background = [self _backgroundImage];
	BOOL flipped = [aView isFlipped];
	float fraction;
	if ([aView respondsToSelector:@selector(isEnabled)] && ![(NSControl *)aView isEnabled]) {
		fraction = 0.5f;
	} else if (_isHighlighted) {
		fraction = 1.0f;
	} else {
		fraction = 0.85f;
	}
	NSSize backSize = [background size];
	NSPoint backOrigin;
	backOrigin.x = aFrame.origin.x;
	if (flipped) {
		backOrigin.y = (float)aFrame.origin.y + (float)backSize.height;
	} else {
		backOrigin.y = ((float)aFrame.size.height + (float)aFrame.origin.y) - (float)backSize.height;
	}
	SMKDissolveImage(background, backOrigin, fraction, flipped);

	NSPoint point = [self pointFromAngleValue:[self floatValue]];
	if (_isRadiusDrawn) {
		NSBezierPath *path = [NSBezierPath bezierPath];
		[path moveToPoint:NSMakePoint((float)(aFrame.origin.x + aFrame.size.width * 0.5),
		                              (float)(aFrame.origin.y + aFrame.size.height * 0.5))];
		[path lineToPoint:point];
		[[NSColor controlShadowColor] set];
		[path stroke];
	}

	NSSize indicatorSize = [indicator size];
	float y;
	if (flipped) {
		y = ((float)backSize.height - (float)point.y) + (float)indicatorSize.height;
	} else {
		y = (float)point.y + ((float)aFrame.size.height - (float)backSize.height);
	}
	y += (float)indicatorSize.height * -0.5f;
	SMKDissolveImage(indicator, NSMakePoint((float)indicatorSize.width * -0.5f + (float)point.x, y), fraction, flipped);

	if ([self showsFirstResponder]) {
		[NSGraphicsContext saveGraphicsState];
		[[NSBezierPath bezierPathWithRect:aFrame] addClip];
		NSSetFocusRingStyle(NSFocusRingOnly);
		NSRect ring;
		ring.origin.x = aFrame.origin.x;
		ring.origin.y = flipped ? aFrame.origin.y
		                        : ((float)aFrame.size.height - (float)backSize.height) + (float)aFrame.origin.y;
		ring.size = backSize;
		[[NSBezierPath bezierPathWithOvalInRect:NSInsetRect(ring, 3.0, 3.0)] fill];
		[NSGraphicsContext restoreGraphicsState];
	}
}

- (BOOL)accessibilityIsIgnored
{
	return NO;
}

- (id)accessibilityHitTest:(NSPoint)aTest
{
	return self;
}

- (id)accessibilityFocusedUIElement
{
	return self;
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityRoleAttribute]) {
		return NSAccessibilityValueIndicatorRole;
	}
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute]) {
		return [[NSBundle mainBundle] localizedStringForKey:@"Angle slider" value:@"" table:nil];
	}
	if ([aValue isEqualToString:NSAccessibilityTitleAttribute] || [aValue isEqualToString:NSAccessibilityHelpAttribute]) {
		return [NSString stringWithFormat:@"%d %@", [self intValue],
		        [[NSBundle mainBundle] localizedStringForKey:@"degrees" value:@"" table:nil]];
	}
	if ([aValue isEqualToString:NSAccessibilityValueAttribute]) {
		return [NSString stringWithFormat:@"%d", [self intValue]];
	}
	return [super accessibilityAttributeValue:aValue];
}

- (void)update
{
	id view = [self controlView];
	if ([view isKindOfClass:[NSControl class]]) {
		[view updateCell:self];
	}
}

- (void)controlTintDidChangeNotification:(id)aNotification
{
	[self update];
}

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	[aCoder encodeObject:_backgroundImage forKey:@"backgroundImage"];
	[aCoder encodeObject:_indicatorImage forKey:@"indicatorImage"];
	if (_radiusMultiplier != 0.68f) {
		[aCoder encodeFloat:_radiusMultiplier forKey:@"radiusMultiplier"];
	}
	[aCoder encodeBool:_isRadiusDrawn forKey:@"isRadiusDrawn"];
	[aCoder encodeBool:_isZeroOnTop forKey:@"isZeroOnTop"];
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	_backgroundImage = [[aCoder decodeObjectForKey:@"backgroundImage"] retain];
	_indicatorImage = [[aCoder decodeObjectForKey:@"indicatorImage"] retain];
	if ([aCoder containsValueForKey:@"radiusMultiplier"]) {
		_radiusMultiplier = [aCoder decodeFloatForKey:@"radiusMultiplier"];
	} else {
		_radiusMultiplier = 0.68f;
	}
	_isRadiusDrawn = [aCoder decodeBoolForKey:@"isRadiusDrawn"];
	_isZeroOnTop = [aCoder decodeBoolForKey:@"isZeroOnTop"];
	_cellFrame = NSZeroRect;
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(controlTintDidChangeNotification:)
	                                             name:NSControlTintDidChangeNotification object:nil];
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_backgroundImage release];
	[_indicatorImage release];
	[super dealloc];
}

- (id)init
{
	self = [super init];
	if (self == nil) {
		return nil;
	}
	_backgroundImage = nil;
	_indicatorImage = nil;
	_radiusMultiplier = 0.68f;
	_isHighlighted = NO;
	_cellFrame = NSZeroRect;
	_isRadiusDrawn = YES;
	_isZeroOnTop = YES;
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(controlTintDidChangeNotification:)
	                                             name:NSControlTintDidChangeNotification object:nil];
	return self;
}

@end
