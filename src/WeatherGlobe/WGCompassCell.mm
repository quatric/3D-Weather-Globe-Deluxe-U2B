#import "WGCompassButton.h"

// The compass of the map: four corner buttons (zones 2-5: NW, NE, SW, SE)
// of which one is the current, and a center toggle (zone 1). The images are
// placed by their bottom-left corners in the flipped control view, as the
// original's compositeToPoint did.

static const NSPoint kImageOrigins[5] = {
	{13.0, 38.0}, {0.0, 25.0}, {26.0, 25.0}, {0.0, 51.0}, {26.0, 51.0}
};
static const NSPoint kCompassOrigin = {0.0, 53.0};

static void _drawImage(NSImage *image, NSPoint point)
{
	NSSize size = [image size];
	[image drawInRect:NSMakeRect(point.x, point.y - size.height, size.width, size.height) fromRect:NSZeroRect
		operation:NSCompositingOperationSourceOver fraction:1.0 respectFlipped:YES hints:nil];
}

@implementation WGCompassCell

- (id)init
{
	self = [super init];
	if (self) {
		[self setBordered:NO];
		[self setBezeled:NO];
		_isContinious = NO;
	}
	return self;
}

- (void)dealloc
{
	[_accessibilityObjects release];
	[super dealloc];
}

- (NSPoint)_segmentOriginForZone:(NSInteger)aZone
{
	if (aZone < 1 || aZone > 5)
		return NSZeroPoint;
	return kImageOrigins[aZone - 1];
}

- (id)_compassImage
{
	return [NSImage imageNamed:@"Compass_all"];
}

- (id)_segmentImageForZone:(NSInteger)aZone
{
	static NSString *const names[5] = {@"CoordPicker", @"NW_butt", @"NE_butt", @"SW_butt", @"SE_butt"};
	if (aZone < 1 || aZone > 5)
		return nil;
	return [NSImage imageNamed:names[aZone - 1]];
}

- (id)_segmentPressedImageForZone:(NSInteger)aZone
{
	static NSString *const names[5] = {@"CoordPicker_pressed", @"NW_butt_pressed", @"NE_butt_pressed",
		@"SW_butt_pressed", @"SE_butt_pressed"};
	if (aZone < 1 || aZone > 5)
		return nil;
	return [NSImage imageNamed:names[aZone - 1]];
}

// The zone's image rect, inset and nudged toward its corner.
- (NSRect)_tooltipRectForZone:(NSInteger)aZone
{
	NSRect rect;
	rect.size = [[self _segmentImageForZone:aZone] size];
	rect.origin = [self _segmentOriginForZone:aZone];
	rect.origin.y -= rect.size.height;
	switch (aZone) {
	case 1:
		return NSInsetRect(rect, 4.0, 4.0);
	case 2:
		return NSOffsetRect(NSInsetRect(rect, 3.0, 3.0), -2.0, -2.0);
	case 3:
		return NSOffsetRect(NSInsetRect(rect, 3.0, 3.0), 2.0, -2.0);
	case 4:
		return NSOffsetRect(NSInsetRect(rect, 3.0, 3.0), -2.0, 2.0);
	case 5:
		return NSOffsetRect(NSInsetRect(rect, 3.0, 3.0), 2.0, 2.0);
	default:
		return rect;
	}
}

- (id)_helpStringForZone:(NSInteger)aZone
{
	NSString *key;
	switch (aZone) {
	case 1:
		key = @"Center button";
		break;
	case 2:
		key = @"Northwest button";
		break;
	case 3:
		key = @"Northeast button";
		break;
	case 4:
		key = @"Southwest button";
		break;
	case 5:
		key = @"Southeast button";
		break;
	default:
		return nil;
	}
	return [[NSBundle mainBundle] localizedStringForKey:key value:@"" table:nil];
}

- (id)accessibilityObjects
{
	if (!_accessibilityObjects) {
		NSMutableArray *objects = [NSMutableArray new];
		for (NSInteger zone = 1; zone != 6; zone++) {
			NSPoint origin = [self _segmentOriginForZone:zone];
			NSSize size = [[self _segmentImageForZone:zone] size];
			[objects addObject:[[[CompassAccessibilityObject alloc] initWithCell:self
				frameRect:NSMakeRect(origin.x, origin.y, size.width, size.height) hotZone:zone] autorelease]];
		}
		_accessibilityObjects = objects;
	}
	return _accessibilityObjects;
}

- (BOOL)isOpaque
{
	return NO;
}

// The compass, the current corner, the pressed corner, then the center
// (pressed, or lit while active).
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	BOOL focus = [self showsFirstResponder];
	if (focus) {
		[NSGraphicsContext saveGraphicsState];
		NSSetFocusRingStyle(NSFocusRingAbove);
	}
	_drawImage([self _compassImage], kCompassOrigin);
	if ((unsigned)(_currentZone - 2) < 4)
		_drawImage([self _segmentImageForZone:_currentZone], [self _segmentOriginForZone:_currentZone]);
	if ((unsigned)(_zoneWithPressedState - 2) < 4)
		_drawImage([self _segmentPressedImageForZone:_zoneWithPressedState],
			[self _segmentOriginForZone:_zoneWithPressedState]);
	if (_zoneWithPressedState == 1)
		_drawImage([self _segmentPressedImageForZone:1], [self _segmentOriginForZone:1]);
	else if (_isCenterZoneActive)
		_drawImage([self _segmentImageForZone:1], [self _segmentOriginForZone:1]);
	if (focus)
		[NSGraphicsContext restoreGraphicsState];
}

// The center within 12.5 of (26.5, 27.5), else the quadrant.
- (NSInteger)_hitZoneForPoint:(NSPoint)aPoint
{
	float x = aPoint.x - 26.5f;
	float y = aPoint.y - 27.5f;
	if (sqrtf(x * x + y * y) <= 12.5f)
		return 1;
	if (x < 0.0f && y < 0.0f)
		return 2;
	if (x >= 0.0f && y < 0.0f)
		return 3;
	if (x < 0.0f && y >= 0.0f)
		return 4;
	if (x >= 0.0f && y >= 0.0f)
		return 5;
	return 0;
}

- (BOOL)startTrackingAt:(NSPoint)aAt inView:(id)aView
{
	if (!_isTrackingOverCenter) {
		_clickedZone = (int)[self _hitZoneForPoint:aAt];
		if (!_clickedZone)
			return NO;
		if (_clickedZone == 1)
			_isTrackingOverCenter = YES;
		_zoneWithPressedState = _clickedZone;
		[aView setNeedsDisplay:YES];
		if (_isContinious)
			[[self target] performSelector:[self action] withObject:self];
	}
	return YES;
}

// Pressing follows the mouse among the corners, or on and off the center.
- (BOOL)continueTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView
{
	int zone = (int)[self _hitZoneForPoint:aAt];
	if (zone != _clickedZone) {
		_clickedZone = zone;
		if (!_isTrackingOverCenter)
			_zoneWithPressedState = (unsigned)(zone - 2) <= 3 ? zone : _zoneWithPressedState;
		else
			_zoneWithPressedState = zone == 1;
		[aView setNeedsDisplay:YES];
		if (_isContinious)
			[[self target] performSelector:[self action] withObject:self];
	}
	return YES;
}

- (void)stopTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView mouseIsUp:(BOOL)aUp
{
	if (aUp) {
		int zone = (int)[self _hitZoneForPoint:aAt];
		if (zone) {
			if (!_isTrackingOverCenter) {
				if (zone != 1) {
					_clickedZone = zone;
					_currentZone = zone;
				}
			} else if (zone == 1) {
				_isCenterZoneActive = !_isCenterZoneActive;
			}
			_isTrackingOverCenter = NO;
		}
	}
	_zoneWithPressedState = 0;
	[aView setNeedsDisplay:YES];
}

- (int)intValue
{
	return _clickedZone;
}

- (void)addToolTipRectsToView:(id)aView
{
	for (NSInteger zone = 1; zone <= 5; zone++)
		[aView addToolTipRect:[self _tooltipRectForZone:zone] owner:[self _helpStringForZone:zone] userData:NULL];
}

- (NSInteger)currentZone
{
	return _currentZone;
}

- (void)setCurrentZone:(NSInteger)aZone
{
	if ((NSUInteger)aZone < 6 && aZone != _currentZone) {
		_clickedZone = (int)aZone;
		_currentZone = (int)aZone;
		[[self controlView] setNeedsDisplay:YES];
		[[self target] performSelector:[self action] withObject:self];
	}
}

- (BOOL)isCenterZoneActive
{
	return _isCenterZoneActive;
}

- (void)setCenterZoneActive:(BOOL)aActive
{
	if (_isCenterZoneActive != aActive) {
		_isCenterZoneActive = aActive;
		_clickedZone = 1;
		[[self controlView] setNeedsDisplay:YES];
		[[self target] performSelector:[self action] withObject:self];
	}
}

// A click on the zone the sender (an accessibility object) stands for.
- (void)performClick:(id)aClick
{
	_zoneWithPressedState = _clickedZone = [aClick intValue];
	[[self controlView] display];
	if (_clickedZone == 1)
		_isCenterZoneActive = !_isCenterZoneActive;
	else if (_clickedZone > 0 && _clickedZone < 6)
		_currentZone = _clickedZone;
	[[self target] performSelector:[self action] withObject:self];
	NSView *view = [self controlView];
	[view setKeyboardFocusRingNeedsDisplayInRect:[view bounds]];
	_zoneWithPressedState = 0;
	[[self controlView] setNeedsDisplay:YES];
}

- (id)accessibilityAttributeNames
{
	return [[super accessibilityAttributeNames]
		arrayByAddingObjectsFromArray:[NSArray arrayWithObjects:NSAccessibilityChildrenAttribute, nil]];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityChildrenAttribute])
		return [self accessibilityObjects];
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute])
		return [[NSBundle mainBundle] localizedStringForKey:@"Buttons" value:@"" table:nil];
	if ([aValue isEqualToString:NSAccessibilityTitleAttribute] || [aValue isEqualToString:NSAccessibilityHelpAttribute])
		return [[NSBundle mainBundle] localizedStringForKey:@"Compass" value:@"" table:nil];
	return [super accessibilityAttributeValue:aValue];
}

- (BOOL)accessibilityIsIgnored
{
	return NO;
}

- (BOOL)accessibilityIsAttributeSettable:(id)aSettable
{
	if ([aSettable isEqualToString:NSAccessibilityChildrenAttribute])
		return NO;
	return [super accessibilityIsAttributeSettable:aSettable];
}

- (id)accessibilityHitTest:(NSPoint)aTest
{
	NSView *view = [self controlView];
	NSRect screen = NSMakeRect(aTest.x, aTest.y, 0.0, 0.0);
	NSPoint point = [view convertPoint:[[view window] convertRectFromScreen:screen].origin fromView:nil];
	NSInteger zone = [self _hitZoneForPoint:point];
	if (zone)
		return [[[self accessibilityObjects] objectAtIndex:zone - 1] accessibilityHitTest:aTest];
	return NSAccessibilityUnignoredAncestor(self);
}

- (id)accessibilityFocusedUIElement
{
	NSView *view = [self controlView];
	if (view && [[view window] firstResponder] == view)
		return self;
	return [super accessibilityFocusedUIElement];
}

@end
