#import "WGBezierPathCell.h"

// A cell drawn as a filled, stroked bezier path given in its default
// bounds, scaled to the cell frame; it is hit only inside the path.
@implementation WGBezierPathCell

- (id)init
{
	return [super init];
}

- (void)dealloc
{
	[_bezierPath release];
	[_frameColor release];
	[_fillColor release];
	[_highlightColor release];
	[super dealloc];
}

- (void)setBezierPath:(id)aPath
{
	if (_bezierPath != aPath) {
		[_bezierPath release];
		_bezierPath = [aPath retain];
	}
}

- (id)frameColor
{
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
	return _fillColor;
}

- (void)setFillColor:(id)aColor
{
	if (_fillColor != aColor) {
		[_fillColor release];
		_fillColor = [aColor retain];
	}
}

- (id)highlightColor
{
	return _highlightColor;
}

- (void)setHighlightColor:(id)aColor
{
	if (_highlightColor != aColor) {
		[_highlightColor release];
		_highlightColor = [aColor retain];
	}
}

- (void)setDefaultCellBounds:(NSRect)aBounds
{
	_defaultBounds = aBounds;
}

- (NSRect)defaultBounds
{
	return _defaultBounds;
}

// Maps the default bounds onto the frame (flipped for flipped views).
- (id)transformForFrame:(NSRect)aFrame view:(id)aView
{
	NSAffineTransform *transform = [NSAffineTransform transform];
	NSRect bounds = [self defaultBounds];
	[transform translateXBy:aFrame.origin.x - bounds.origin.x yBy:-(aFrame.origin.y - bounds.origin.y)];
	[transform scaleXBy:aFrame.size.width / bounds.size.width yBy:aFrame.size.height / bounds.size.height];
	if ([aView isFlipped])
		[transform appendTransform:[self flippedTransformWithBounds:aFrame]];
	return transform;
}

- (BOOL)containsPoint:(NSPoint)aPoint inRect:(NSRect)aRect ofView:(id)aView
{
	if (!_bezierPath)
		return NO;
	NSPoint point = [aView convertPoint:aPoint fromView:nil];
	if (!NSPointInRect(point, aRect))
		return NO;
	NSAffineTransform *transform = [self transformForFrame:aRect view:aView];
	[transform invert];
	return [_bezierPath containsPoint:[transform transformPoint:point]];
}

// Filled with the highlight color when on.
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	if (!_bezierPath)
		return;
	NSAffineTransform *transform = [self transformForFrame:aFrame view:aView];
	[NSGraphicsContext saveGraphicsState];
	[transform concat];
	[[self state] ? _highlightColor : _fillColor set];
	[_bezierPath fill];
	[_frameColor set];
	[_bezierPath stroke];
	[NSGraphicsContext restoreGraphicsState];
}

- (BOOL)allowsMixedState
{
	return NO;
}

- (id)flippedTransformWithBounds:(NSRect)aBounds
{
	NSAffineTransform *transform = [NSAffineTransform transform];
	[transform translateXBy:0.0 yBy:aBounds.size.height];
	[transform scaleXBy:1.0 yBy:-1.0];
	return transform;
}

@end
