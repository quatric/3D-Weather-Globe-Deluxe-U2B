#import "WGCustomSlider.h"

// A linear slider drawn with images: a knob (disabled and pressed
// variants) and a bar of two edges and a stretched middle (disabled
// variants), placed across the track as the tick marks are; tick marks in
// their own color. Images are drawn as the original's compositeToPoint
// placed them, with the flipped offset left to respectFlipped.
@implementation WGCustomSliderCell

- (void)dealloc
{
	[_knobImage release];
	[_knobDisabledImage release];
	[_knobAlternateImage release];
	[_barFirstEdgeImage release];
	[_barLastEdgeImage release];
	[_barMiddleImage release];
	[_barDisabledFirstEdgeImage release];
	[_barDisabledLastEdgeImage release];
	[_barDisabledMiddleImage release];
	[_tickMarksColor release];
	[super dealloc];
}

- (BOOL)_isSupported
{
	return [self sliderType] == NSSliderTypeLinear;
}

- (id)_currentKnobImage
{
	if (![self isEnabled] && _knobDisabledImage)
		return _knobDisabledImage;
	if (_mouseIsTracking && _knobAlternateImage)
		return _knobAlternateImage;
	return _knobImage;
}

- (id)_currentBarFirstEdgeImage
{
	return [self isEnabled] ? _barFirstEdgeImage : _barDisabledFirstEdgeImage;
}

- (id)_currentBarLastEdgeImage
{
	return [self isEnabled] ? _barLastEdgeImage : _barDisabledLastEdgeImage;
}

- (id)_currentBarMiddleImage
{
	return [self isEnabled] ? _barMiddleImage : _barDisabledMiddleImage;
}

- (BOOL)_drawsCustomKnob
{
	return [self _isSupported] && [self _currentKnobImage];
}

- (BOOL)_drawsCustomBar
{
	return [self _isSupported] && ([self _currentBarFirstEdgeImage] || [self _currentBarLastEdgeImage]
		|| [self _currentBarMiddleImage]);
}

- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp
{
	_mouseIsTracking = YES;
	BOOL result = [super trackMouse:aMouse inRect:aRect ofView:aView untilMouseUp:aUp];
	_mouseIsTracking = NO;
	return result;
}

// The bar, then the knob (with its focus ring when first responder).
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView
{
	if (![self _drawsCustomBar]) {
		[super drawWithFrame:aFrame inView:aView];
		return;
	}
	[self drawBarInside:aFrame flipped:[aView isFlipped]];
	if ([self showsFirstResponder]) {
		[NSGraphicsContext saveGraphicsState];
		NSSetFocusRingStyle(NSFocusRingAbove);
		[self drawKnob];
		[NSGraphicsContext restoreGraphicsState];
	} else {
		[self drawKnob];
	}
}

static void _drawImage(NSImage *image, NSPoint point)
{
	NSSize size = [image size];
	[image drawInRect:NSMakeRect(point.x, point.y, size.width, size.height) fromRect:NSZeroRect
		operation:NSCompositingOperationSourceOver fraction:1.0 respectFlipped:YES hints:nil];
}

- (void)drawKnob:(NSRect)aKnob
{
	if (![self _drawsCustomKnob]) {
		[super drawKnob:aKnob];
		return;
	}
	_drawImage([self _currentKnobImage], aKnob.origin);
}

- (void)_setImage:(NSImage **)aImage to:(id)aNew
{
	if (*aImage == aNew)
		return;
	[*aImage release];
	*aImage = [aNew retain];
	[[self controlView] setNeedsDisplay:YES];
}

- (void)setKnobImage:(id)aImage
{
	[self _setImage:&_knobImage to:aImage];
}

- (void)setKnobDisabledImage:(id)aImage
{
	[self _setImage:&_knobDisabledImage to:aImage];
}

- (void)setKnobAlternateImage:(id)aImage
{
	[self _setImage:&_knobAlternateImage to:aImage];
}

- (void)setBarFirstEdgeImage:(id)aImage
{
	[self _setImage:&_barFirstEdgeImage to:aImage];
}

- (void)setBarLastEdgeImage:(id)aImage
{
	[self _setImage:&_barLastEdgeImage to:aImage];
}

- (void)setBarMiddleImage:(id)aImage
{
	[self _setImage:&_barMiddleImage to:aImage];
}

- (void)setBarDisabledFirstEdgeImage:(id)aImage
{
	[self _setImage:&_barDisabledFirstEdgeImage to:aImage];
}

- (void)setBarDisabledLastEdgeImage:(id)aImage
{
	[self _setImage:&_barDisabledLastEdgeImage to:aImage];
}

- (void)setBarDisabledMiddleImage:(id)aImage
{
	[self _setImage:&_barDisabledMiddleImage to:aImage];
}

- (void)setDrawsTickMarks:(BOOL)aMarks
{
	_dontDrawsTickMarks = !aMarks;
	[[self controlView] setNeedsDisplay:YES];
}

- (void)setTickMarksColor:(id)aColor
{
	[aColor retain];
	[_tickMarksColor release];
	_tickMarksColor = aColor;
	[[self controlView] setNeedsDisplay:YES];
}

// The edges at the ends, the middle stretched between; across the track
// they sit at 0.6 of the free space, or 0.2 / 0.9 with tick marks below /
// above.
- (void)drawBarInside:(NSRect)aInside flipped:(BOOL)aFlipped
{
	BOOL custom = [self _drawsCustomBar];
	if (custom) {
		BOOL vertical = [self isVertical];
		float position = 0.6f;
		if ([self numberOfTickMarks])
			position = [self tickMarkPosition] == 0 ? 0.2f : 0.9f;
		NSRect rect = aInside;
		NSImage *image = [self _currentBarFirstEdgeImage];
		if (image) {
			NSSize size = [image size];
			NSPoint point = aInside.origin;
			if (vertical) {
				point.x += floorf((aInside.size.width - size.width) * position);
				rect = NSMakeRect(aInside.origin.x, aInside.origin.y + size.height, aInside.size.width,
					aInside.size.height - size.height);
			} else {
				point.y += floorf((aInside.size.height - size.height) * position);
				rect = NSMakeRect(aInside.origin.x + size.width, aInside.origin.y, aInside.size.width - size.width,
					aInside.size.height);
			}
			_drawImage(image, point);
		}
		image = [self _currentBarLastEdgeImage];
		if (image) {
			NSSize size = [image size];
			NSPoint point;
			if (vertical) {
				point.y = rect.origin.y + rect.size.height - size.height;
				point.x = rect.origin.x + floorf((rect.size.width - size.width) * position);
				rect.size.height -= size.height;
			} else {
				point.y = rect.origin.y + floorf((rect.size.height - size.height) * position);
				point.x = rect.size.width + rect.origin.x - size.width;
				rect.size.width -= size.width;
			}
			_drawImage(image, point);
		}
		image = [self _currentBarMiddleImage];
		if (image) {
			BOOL flipped = [image isFlipped];
			NSSize size = [image size];
			if (vertical) {
				rect.origin.x += floorf((rect.size.width - size.width) * position);
				rect.size.width = size.width;
			} else {
				rect.origin.y += floorf((rect.size.height - size.height) * position);
				rect.size.height = size.height;
			}
			[image setFlipped:aFlipped];
			[image drawInRect:rect fromRect:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, size.width, size.height)
				operation:NSCompositingOperationSourceOver fraction:1.0];
			[image setFlipped:flipped];
		}
	}
	if (![self numberOfTickMarks] || _dontDrawsTickMarks)
		return;
	NSColor *color = _tickMarksColor;
	if (!custom && !color)
		return;
	if (!color)
		color = [NSColor grayColor];
	if (![self isEnabled])
		color = [color colorWithAlphaComponent:0.5];
	[color set];
	NSInteger count = [self numberOfTickMarks];
	for (NSInteger i = 0; i < count; i++)
		NSRectFillUsingOperation([self rectOfTickMarkAtIndex:i], NSCompositingOperationSourceOver);
}

// The knob image at the value along the track and across it as the bar.
// (The original took the bounds of the focused view; that is the control
// view while drawing.)
- (NSRect)knobRectFlipped:(BOOL)aFlipped
{
	if (![self _drawsCustomKnob])
		return [super knobRectFlipped:aFlipped];
	BOOL vertical = [self isVertical];
	float value = (float)(([self doubleValue] - [self minValue]) / ([self maxValue] - [self minValue]));
	float across = 0.5f;
	if ([self numberOfTickMarks])
		across = [self tickMarkPosition] ? 1.0f : 0.0f;
	float xFactor = across, yFactor = value;
	if (!vertical) {
		xFactor = value;
		yFactor = across;
	}
	NSRect rect = [self drawingRectForBounds:[[self controlView] bounds]];
	NSSize size = [[self _currentKnobImage] size];
	return NSMakeRect(floorf((rect.size.width - size.width) * xFactor) + rect.origin.x,
		floorf(yFactor * (rect.size.height - size.height)) + rect.origin.y, size.width, size.height);
}

@end
