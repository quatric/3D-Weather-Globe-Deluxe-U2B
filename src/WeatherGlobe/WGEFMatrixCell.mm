#import "WGEFMatrixCell.h"

// The attributes of the temperatures: bold 13 pt in the color.
static NSDictionary *_temperatureAttributes(NSColor *color)
{
	NSFont *font = [NSFont fontWithName:@"HelveticaNeue-Bold" size:13.0];
	if (!font)
		font = [NSFont systemFontOfSize:13.0];
	return [[NSDictionary alloc] initWithObjectsAndKeys:color, NSForegroundColorAttributeName,
		font, NSFontAttributeName, nil];
}

// A day tab of the extended forecast: the weekday, the summary image and
// the high and low temperatures, stacked; selected, it draws its highlight
// path.
@implementation WGEFMatrixCell

- (void)dealloc
{
	[_highlightBezierPath release];
	[_weekdayAbbreviation release];
	[_summaryImage release];
	[_temperatureHigh release];
	[_temperatureLow release];
	[_weekdayAbbreviationAttributes release];
	[_temperatureHighAttributes release];
	[_temperatureLowAttributes release];
	[super dealloc];
}

- (void)setHighlightBezierPath:(id)aPath
{
	if (_highlightBezierPath != aPath) {
		[_highlightBezierPath release];
		_highlightBezierPath = [aPath retain];
	}
}

- (id)weekdayAbbreviation
{
	return _weekdayAbbreviation;
}

- (void)setWeekdayAbbreviation:(id)aAbbreviation
{
	[_weekdayAbbreviation release];
	_weekdayAbbreviation = [[NSAttributedString alloc] initWithString:aAbbreviation
		attributes:[self weekdayAbbreviationAttributes]];
}

- (id)summaryImage
{
	return _summaryImage;
}

- (void)setSummaryImage:(id)aImage
{
	if (_summaryImage != aImage) {
		[_summaryImage release];
		_summaryImage = [aImage retain];
	}
}

- (id)temperatureHigh
{
	return _temperatureHigh;
}

- (void)setTemperatureHigh:(id)aHigh
{
	[_temperatureHigh release];
	_temperatureHigh = [[NSAttributedString alloc] initWithString:aHigh attributes:[self temperatureHighAttributes]];
}

- (id)temperatureLow
{
	return _temperatureLow;
}

- (void)setTemperatureLow:(id)aLow
{
	[_temperatureLow release];
	_temperatureLow = [[NSAttributedString alloc] initWithString:aLow attributes:[self temperatureLowAttributes]];
}

// Selected, the cell is hit by its highlight path.
- (BOOL)containsPoint:(NSPoint)aPoint inRect:(NSRect)aRect ofView:(id)aView
{
	if ([self state] && _highlightBezierPath) {
		NSPoint point = [aView convertPoint:aPoint fromView:nil];
		if (NSPointInRect(point, aRect)) {
			NSAffineTransform *transform = [self transformForFrame:aRect view:aView];
			[transform invert];
			return [_highlightBezierPath containsPoint:[transform transformPoint:point]];
		}
	}
	return [super containsPoint:aPoint inRect:aRect ofView:aView];
}

- (id)weekdayAbbreviationAttributes
{
	if (!_weekdayAbbreviationAttributes) {
		NSFont *font = [NSFont fontWithName:@"HelveticaNeue-Bold" size:11.0];
		if (!font)
			font = [NSFont systemFontOfSize:11.0];
		_weekdayAbbreviationAttributes = [[NSDictionary alloc] initWithObjectsAndKeys:[self frameColor],
			NSForegroundColorAttributeName, font, NSFontAttributeName, nil];
	}
	return _weekdayAbbreviationAttributes;
}

- (id)temperatureHighAttributes
{
	if (!_temperatureHighAttributes)
		_temperatureHighAttributes = _temperatureAttributes([NSColor colorWithDeviceRed:0.47 green:0.11 blue:0.0 alpha:0.8]);
	return _temperatureHighAttributes;
}

- (id)temperatureLowAttributes
{
	if (!_temperatureLowAttributes)
		_temperatureLowAttributes = _temperatureAttributes([NSColor colorWithDeviceRed:0.11 green:0.2 blue:0.41 alpha:0.8]);
	return _temperatureLowAttributes;
}

// The contents are laid out top down in the cell's bounds (the matrix is
// flipped): the weekday, the image (compositeToPoint: in the original, at
// its bottom left), then the temperatures, each overlapping the one above
// by 4.
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	if (![self state] || !_highlightBezierPath) {
		[super drawInteriorWithFrame:aFrame inView:aView];
	} else {
		NSAffineTransform *transform = [self transformForFrame:aFrame view:aView];
		[NSGraphicsContext saveGraphicsState];
		[transform concat];
		[(NSColor *)[self highlightColor] set];
		[_highlightBezierPath fill];
		[(NSColor *)[self frameColor] set];
		[_highlightBezierPath stroke];
		[NSGraphicsContext restoreGraphicsState];
	}
	NSRect bounds = [self defaultBounds];
	NSAffineTransform *transform = [self transformForFrame:aFrame view:aView];
	[transform translateXBy:0.0 yBy:bounds.size.height];
	[transform scaleXBy:1.0 yBy:-1.0];
	[NSGraphicsContext saveGraphicsState];
	[transform concat];
	NSSize size = [_weekdayAbbreviation size];
	[_weekdayAbbreviation drawAtPoint:NSMakePoint((bounds.size.width - size.width) * 0.5 + bounds.origin.x, bounds.origin.y)];
	CGFloat y = bounds.origin.y + size.height;
	size = [_summaryImage size];
	[_summaryImage drawInRect:NSMakeRect((bounds.size.width - size.width) * 0.5 + bounds.origin.x, y, size.width, size.height)
		fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1.0 respectFlipped:YES hints:nil];
	y += size.height - 4.0;
	size = [_temperatureHigh size];
	[_temperatureHigh drawAtPoint:NSMakePoint((bounds.size.width - size.width) * 0.5 + bounds.origin.x, y)];
	y += size.height - 4.0;
	size = [_temperatureLow size];
	[_temperatureLow drawAtPoint:NSMakePoint(bounds.origin.x + (bounds.size.width - size.width) * 0.5, y)];
	[NSGraphicsContext restoreGraphicsState];
}

@end
