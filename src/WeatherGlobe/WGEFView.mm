#import "WGEFView.h"

// The weather forecast frame: two rounded panels (the top one filled with
// the frame color) over a shadow; the forecast matrix sits in the lower 84
// points.
@implementation WGEFView

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self) {
		_shadow = [NSShadow new];
		[_shadow setShadowBlurRadius:4.0];
		[_shadow setShadowOffset:NSMakeSize(0.0, -2.0)];
	}
	return self;
}

- (void)dealloc
{
	[_frameColor release];
	[_fillColor release];
	[_shadow release];
	[super dealloc];
}

- (void)drawRect:(NSRect)aRect
{
	NSBezierPath *top = [self topBezierPath];
	NSBezierPath *bottom = [self bottomBezierPath];
	NSBezierPath *both = [NSBezierPath bezierPath];
	[both appendBezierPath:top];
	[both appendBezierPath:bottom];
	[NSGraphicsContext saveGraphicsState];
	[_shadow set];
	[(NSColor *)[self fillColor] set];
	[both fill];
	[NSGraphicsContext restoreGraphicsState];
	[NSGraphicsContext saveGraphicsState];
	[(NSColor *)[self frameColor] set];
	[top fill];
	[top stroke];
	[bottom stroke];
	[NSGraphicsContext restoreGraphicsState];
	[super drawRect:aRect];
}

- (id)frameColor
{
	if (!_frameColor)
		_frameColor = [[NSColor colorWithDeviceRed:0.52f green:0.6f blue:0.69f alpha:1.0] retain];
	return _frameColor;
}

- (void)setFrameColor:(id)aColor
{
	if (_frameColor != aColor) {
		[_frameColor release];
		_frameColor = [aColor retain];
	}
}

- (id)fillColor
{
	if (!_fillColor)
		_fillColor = [[NSColor colorWithCalibratedRed:1.0 green:1.0 blue:1.0 alpha:1.0] retain];
	return _fillColor;
}

- (void)setFillColor:(id)aColor
{
	if (_fillColor != aColor) {
		[_fillColor release];
		_fillColor = [aColor retain];
	}
}

- (void)placeEFMatrix:(NSView *)aMatrix
{
	CGFloat height = [aMatrix bounds].size.height;
	NSRect bounds = [self bounds];
	[aMatrix setFrameOrigin:NSMakePoint(bounds.origin.x,
		bounds.size.height - 84.0 - height + bounds.origin.y + 2.0 + 1.0)];
	[aMatrix setAutoresizingMask:NSViewMinYMargin];
	[self addSubview:aMatrix];
}

// The lower panel, around the forecast matrix; its bottom corners are rounded.
- (id)bottomBezierPath
{
	NSBezierPath *path = [NSBezierPath bezierPath];
	NSRect bounds = [self bounds];
	CGFloat top = bounds.size.height - 84.0 - 0.5 + bounds.origin.y;
	CGFloat left = bounds.origin.x + 5.5;
	CGFloat right = bounds.origin.x + (bounds.size.width - 5.5);
	[path moveToPoint:NSMakePoint(left, top)];
	[path lineToPoint:NSMakePoint(right, top)];
	CGFloat cornerY = bounds.origin.y + 12.5;
	CGFloat bottomY = bounds.origin.y + 5.5;
	CGFloat cornerX = right - 7.0 - 0.5;
	[path lineToPoint:NSMakePoint(right, cornerY)];
	[path curveToPoint:NSMakePoint(cornerX, bottomY)
		controlPoint1:NSMakePoint((right - cornerX) / -6.0 + right, (cornerY - bottomY) * -0.5 + cornerY)
		controlPoint2:NSMakePoint((right - cornerX) * 0.5 + cornerX, (cornerY - bottomY) / 6.0 + bottomY)];
	CGFloat leftCornerX = left + 7.0 + 0.5;
	[path lineToPoint:NSMakePoint(leftCornerX, bottomY)];
	[path curveToPoint:NSMakePoint(left, cornerY)
		controlPoint1:NSMakePoint((leftCornerX - left) * -0.5 + leftCornerX, (cornerY - bottomY) / 6.0 + bottomY)
		controlPoint2:NSMakePoint((leftCornerX - left) / 6.0 + left, (cornerY - bottomY) * -0.5 + cornerY)];
	[path lineToPoint:NSMakePoint(left, top)];
	[path closePath];
	return path;
}

// The upper panel, with rounded top corners.
- (id)topBezierPath
{
	NSBezierPath *path = [NSBezierPath bezierPath];
	NSRect bounds = [self bounds];
	CGFloat bottom = bounds.size.height - 84.0 - 0.5 + bounds.origin.y;
	CGFloat left = bounds.origin.x + 5.5;
	CGFloat cornerY = bounds.origin.y + bounds.size.height - 5.5 - 7.0;
	CGFloat leftCornerX = left + 7.0;
	CGFloat topY = cornerY + 7.0;
	[path moveToPoint:NSMakePoint(left, bottom)];
	[path lineToPoint:NSMakePoint(left, cornerY)];
	[path curveToPoint:NSMakePoint(leftCornerX, topY)
		controlPoint1:NSMakePoint((leftCornerX - left) / 6.0 + left, (topY - cornerY) * 0.5 + cornerY)
		controlPoint2:NSMakePoint((leftCornerX - left) * -0.5 + leftCornerX, (topY - cornerY) / -6.0 + topY)];
	CGFloat rightCornerY = topY - 7.0 - 0.5;
	CGFloat rightCornerX = bounds.origin.x + bounds.size.width - 5.0 - 7.0 - 0.5;
	CGFloat right = rightCornerX + 7.0;
	[path lineToPoint:NSMakePoint(rightCornerX, topY)];
	[path curveToPoint:NSMakePoint(right, rightCornerY)
		controlPoint1:NSMakePoint((right - rightCornerX) * 0.5 + rightCornerX, (topY - rightCornerY) / -6.0 + topY)
		controlPoint2:NSMakePoint((right - rightCornerX) / -6.0 + right, (topY - rightCornerY) * 0.5 + rightCornerY)];
	[path lineToPoint:NSMakePoint(right, bottom)];
	[path lineToPoint:NSMakePoint(left, bottom)];
	[path closePath];
	return path;
}

@end
