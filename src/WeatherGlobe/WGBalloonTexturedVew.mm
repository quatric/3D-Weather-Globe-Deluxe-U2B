#import "WGBalloonTexturedVew.h"

// A textured frame with a balloon tail on one of its sides (eight positions:
// 0-1 top, 2-3 right, 4-5 bottom, 6-7 left); the frame is inset by 7 points
// on that side and the edge texture leaves room for the tail image.
@implementation WGBalloonTexturedVew

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self) {
		_balloonPosition = 5;
		_isBalloonVisible = NO;
	}
	return self;
}

- (void)dealloc
{
	[_balloonImage release];
	[super dealloc];
}

- (id)balloonImage
{
	return _balloonImage;
}

- (void)setBalloonImage:(id)aImage
{
	if (_balloonImage != aImage) {
		[_balloonImage release];
		_balloonImage = [aImage retain];
		[self setNeedsDisplay:YES];
	}
}

- (NSInteger)balloonPosition
{
	return _balloonPosition;
}

- (void)setBalloonPosition:(NSInteger)aPosition
{
	if (_balloonPosition != aPosition) {
		_balloonPosition = (int)aPosition;
		[self setNeedsDisplay:YES];
	}
}

- (BOOL)isBalloonVisible
{
	return _isBalloonVisible;
}

- (void)setIsBalloonVisible:(BOOL)aVisible
{
	if (_isBalloonVisible != aVisible) {
		_isBalloonVisible = aVisible;
		[self setNeedsDisplay:YES];
	}
}

- (void)drawRect:(NSRect)aRect
{
	[super drawRect:aRect];
	if ([self isBalloonVisible]) {
		NSRect source = { NSZeroPoint, [_balloonImage size] };
		[_balloonImage drawInRect:[self _balloonFrame] fromRect:source
			operation:(NSCompositingOperation)_operation fraction:_fraction];
	}
}

- (NSRect)textureRect
{
	NSRect rect = [super textureRect];
	switch (_balloonPosition) {
	case 0: case 1:
		rect.size.height -= 7.0;
		break;
	case 2: case 3:
		rect.size.width -= 7.0;
		break;
	case 4: case 5:
		rect.origin.y += 7.0;
		rect.size.height -= 7.0;
		break;
	case 6: case 7:
		rect.origin.x += 7.0;
		rect.size.width -= 7.0;
		break;
	}
	return rect;
}

// The edge texture on the balloon's side gives up the tail's length.
- (NSRect)correctedDestinationRectForTexture:(NSInteger)aTexture destinationRect:(NSRect)aRect
{
	if (![self isBalloonVisible])
		return aRect;
	NSSize size = [_balloonImage size];
	switch (aTexture) {
	case 1:	// top
		if (_balloonPosition == 0)
			aRect.origin.x += size.width;
		if (_balloonPosition == 0 || _balloonPosition == 1)
			aRect.size.width -= size.width;
		break;
	case 3:	// left
		if (_balloonPosition == 6)
			aRect.origin.y += size.height;
		if (_balloonPosition == 6 || _balloonPosition == 7)
			aRect.size.height -= size.height;
		break;
	case 5:	// right
		if (_balloonPosition == 3)
			aRect.origin.y += size.height;
		if (_balloonPosition == 3 || _balloonPosition == 2)
			aRect.size.height -= size.height;
		break;
	case 7:	// bottom
		if (_balloonPosition == 5)
			aRect.origin.x += size.width;
		if (_balloonPosition == 5 || _balloonPosition == 4)
			aRect.size.width -= size.width;
		break;
	}
	return aRect;
}

// With the tail at the bottom, moves it to the other corner when the view
// would leave the main screen; the origin keeps the tail on the point.
- (NSPoint)balloonPositionForProposedPosition:(NSPoint)aPosition
{
	CGFloat balloonX = [self _balloonFrame].origin.x;
	NSRect bounds = [self bounds];
	NSRect visible = [[NSScreen mainScreen] visibleFrame];
	CGFloat x = aPosition.x;
	if (_balloonPosition == 4) {
		x = aPosition.x - balloonX;
		if (x < visible.origin.x) {
			[self setBalloonPosition:5];
			x += balloonX - [self _balloonFrame].origin.x;
		}
	} else if (_balloonPosition == 5) {
		x = aPosition.x - balloonX;
		if (visible.size.width + visible.origin.x < x + bounds.size.width) {
			[self setBalloonPosition:4];
			x -= [self _balloonFrame].origin.x - balloonX;
		}
	}
	return NSMakePoint(x, aPosition.y);
}

@end
