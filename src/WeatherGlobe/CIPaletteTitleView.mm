#import "CIPaletteTitleView.h"
#import "CityInfoView.h"

// The title bar of the city info palette: a row (or column) of components,
// typically traffic-light buttons, laid out 2 points apart over a title
// image. The enabled buttons light up while the mouse is over them; ones
// tagged 1 at the end are pushed to the far edge.

static const float kDoubleOffset = 4.0f;

@implementation CIPaletteTitleView

- (void)_reconstruct
{
	NSSize size = NSZeroSize;
	int x = 2, y = 2;
	NSUInteger count = [_viewComponents count];
	for (NSUInteger i = 0; i < count; i++) {
		NSView *view = [_viewComponents objectAtIndex:i];
		NSRect frame = [view frame];
		NSPoint origin;
		if (!_vertical) {
			origin = NSMakePoint(x, y + _topOffset);
			size.width += frame.size.width;
			x += 2 + (int)frame.size.width;
			if (frame.size.height > size.height)
				size.height = frame.size.height;
		} else {
			origin = NSMakePoint(x + _topOffset, y);
			size.height += frame.size.height;
			y += 2 + (int)frame.size.height;
			if (frame.size.width > size.width)
				size.width = frame.size.width;
		}
		if (![view superview])
			[self addSubview:view];
		[view setFrameOrigin:origin];
	}
	int gaps = count ? (int)count * 2 - 2 : 0;
	size.width += kDoubleOffset + 1.0f;
	size.height += kDoubleOffset + 1.0f;
	if (!_vertical)
		size.width += gaps;
	else
		size.height += gaps;
	_sizeForBestFit = size;
}

- (void)_setLightsButton:(id)aButton
{
	if ([aButton alternateImage]) {
		[aButton setImage:[aButton alternateImage]];
		[aButton setAlternateImage:nil];
	}
}

// The colored image is kept as the alternate while the button shows gray.
- (void)_stopLightsButton:(id)aButton
{
	static NSImage *grayButtonImage = nil;
	if (!grayButtonImage)
		grayButtonImage = [[NSImage imageNamed:@"Gray.tif"] retain];
	if (![aButton alternateImage]) {
		[aButton setAlternateImage:[aButton image]];
		[aButton setImage:grayButtonImage];
	}
}

// (Both skip the last component, as the original did.)
- (void)_setLightsAllButtons
{
	NSInteger count = [_viewComponents count];
	for (NSInteger i = 0; i < count - 1; i++) {
		id view = [_viewComponents objectAtIndex:i];
		if ([view isEnabled] && [view isKindOfClass:[NSButton class]] && NSContainsRect(_trackingRect, [view frame]))
			[self _setLightsButton:view];
	}
}

- (void)_stopLightsAllButtons
{
	NSInteger count = [_viewComponents count];
	for (NSInteger i = 0; i < count - 1; i++) {
		id view = [_viewComponents objectAtIndex:i];
		if ([view isEnabled] && [view isKindOfClass:[NSButton class]] && NSContainsRect(_trackingRect, [view frame]))
			[self _stopLightsButton:view];
	}
}

// Moves the trailing tag-1 components to the far edge and tracks the mouse
// over the union of the enabled buttons. (The original kept "tracking rect
// added" in one static for all instances; this keeps it per view.)
- (void)_frameWereChanged:(id)aChanged
{
	if (_trackingRectTag)
		[self removeTrackingRect:_trackingRectTag];
	_trackingRect = NSZeroRect;
	BOOL trailing = YES;
	float offset = 0.0f;
	for (NSInteger i = (NSInteger)[_viewComponents count] - 1; i >= 0; i--) {
		id view = [_viewComponents objectAtIndex:i];
		NSRect frame = [view frame];
		if (trailing && [view tag] == 1) {
			NSPoint origin = frame.origin;
			if (!_vertical) {
				offset += frame.size.width + 2.0f;
				origin.x = [self frame].size.width - offset;
			} else {
				offset += frame.size.height + 2.0f;
				origin.y = [self frame].size.height - offset;
			}
			[view setFrameOrigin:origin];
			continue;
		}
		if ([view isEnabled] && [view isKindOfClass:[NSButton class]]) {
			if (NSIsEmptyRect(_trackingRect))
				_trackingRect = frame;
			else
				_trackingRect = NSUnionRect(_trackingRect, frame);
		}
		trailing = NO;
	}
	_trackingRectTag = (int)[self addTrackingRect:_trackingRect owner:self userData:NULL assumeInside:NO];
	[self _stopLightsAllButtons];
}

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self) {
		[self setPostsFrameChangedNotifications:YES];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_frameWereChanged:)
			name:NSViewFrameDidChangeNotification object:self];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_horizontalTitleImage release];
	[_verticalTitleImage release];
	[_viewComponents release];
	[super dealloc];
}

- (BOOL)isFlipped
{
	return YES;
}

- (void)mouseEntered:(id)aEntered
{
	[self _setLightsAllButtons];
}

- (void)mouseExited:(id)aExited
{
	[self _stopLightsAllButtons];
}

- (void)mouseDown:(id)aDown
{
}

- (void)mouseDragged:(id)aDragged
{
}

- (void)keyDown:(id)aDown
{
	[[self window] keyDown:aDown];
}

// The title image stretched along the bar, at the palette transparency.
- (void)drawRect:(NSRect)aRect
{
	NSImage *image = _vertical ? _verticalTitleImage : _horizontalTitleImage;
	if (!image)
		return;
	NSSize size = [image size];
	NSRect rect = aRect;
	if (!_vertical) {
		rect.origin.y = 0.0f;
		rect.size.height = [self bounds].size.height;
	} else {
		rect.origin.x = 0.0f;
		rect.size.width = [self bounds].size.width;
	}
	[image drawInRect:rect fromRect:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, size.width, size.height)
		operation:NSCompositingOperationCopy fraction:Preferences::paletteTransparencyLevel()];
}

- (NSSize)sizeForBestFit
{
	return _sizeForBestFit;
}

- (void)setVertical:(BOOL)aVertical
{
	if (_vertical != aVertical) {
		_vertical = aVertical;
		[self _reconstruct];
	}
}

- (BOOL)isVertical
{
	return _vertical;
}

- (void)setTopOffset:(CGFloat)aOffset
{
	if (aOffset != _topOffset) {
		_topOffset = aOffset;
		[self _reconstruct];
	}
}

- (id)viewComponents
{
	return _viewComponents;
}

- (void)setViewComponents:(id)aComponents
{
	[aComponents retain];
	[_viewComponents makeObjectsPerformSelector:@selector(removeFromSuperview)];
	[_viewComponents release];
	_viewComponents = aComponents;
	[self _reconstruct];
}

- (id)horizontalTitleImage
{
	return _horizontalTitleImage;
}

- (id)verticalTitleImage
{
	return _verticalTitleImage;
}

// A flipped copy, as the view is flipped.
- (void)setHorizontalTitleImage:(id)aImage
{
	[_horizontalTitleImage release];
	_horizontalTitleImage = [[NSImage alloc] initWithData:[aImage TIFFRepresentation]];
	[_horizontalTitleImage setFlipped:YES];
	[self setNeedsDisplay:YES];
}

- (void)setVerticalTitleImage:(id)aImage
{
	[aImage retain];
	[_verticalTitleImage release];
	_verticalTitleImage = aImage;
	[self setNeedsDisplay:YES];
}

@end
