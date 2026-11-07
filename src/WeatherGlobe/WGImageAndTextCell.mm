#import "WGImageAndTextCell.h"

// A text cell with an image at the left (5 points of gap).
@implementation WGImageAndTextCell

- (void)dealloc
{
	[_leftImage release];
	[super dealloc];
}

- (id)copyWithZone:(NSZone *)aZone
{
	WGImageAndTextCell *copy = [super copyWithZone:aZone];
	copy->_leftImage = [_leftImage retain];
	return copy;
}

- (void)editWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate event:(id)aEvent
{
	NSRect imageRect, textRect;
	NSDivideRect(aFrame, &imageRect, &textRect, [_leftImage size].width + 5.0, NSMinXEdge);
	[super editWithFrame:textRect inView:aView editor:aEditor delegate:aDelegate event:aEvent];
}

- (void)selectWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate start:(NSInteger)aStart length:(NSInteger)aLength
{
	NSRect imageRect, textRect;
	NSDivideRect(aFrame, &imageRect, &textRect, [_leftImage size].width + 5.0, NSMinXEdge);
	[super selectWithFrame:textRect inView:aView editor:aEditor delegate:aDelegate start:aStart length:aLength];
}

- (void)setImage:(id)aImage
{
	if (_leftImage != aImage) {
		[_leftImage release];
		_leftImage = [aImage retain];
	}
}

- (id)image
{
	return _leftImage;
}

// The image is centered vertically (compositeToPoint: in the original); the
// text is centered when shorter than the frame.
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView
{
	if (_leftImage) {
		NSSize size = [_leftImage size];
		NSRect imageFrame;
		NSDivideRect(aFrame, &imageFrame, &aFrame, size.width + 5.0, NSMinXEdge);
		if ([self drawsBackground]) {
			[[self backgroundColor] set];
			NSRectFill(imageFrame);
		}
		NSRect imageRect = NSMakeRect(imageFrame.origin.x + 5.0, imageFrame.origin.y + ceil((aFrame.size.height - size.height) * 0.5),
		                              size.width, size.height);
		[_leftImage drawInRect:imageRect
		              fromRect:NSZeroRect
		             operation:NSCompositingOperationSourceOver
		              fraction:1
		        respectFlipped:YES
		                 hints:nil];
	}
	NSSize cellSize = [self cellSize];
	if (cellSize.height < aFrame.size.height) {
		aFrame.origin.y += (aFrame.size.height - cellSize.height) * 0.5;
		aFrame.size.height = cellSize.height;
	}
	[super drawWithFrame:aFrame inView:aView];
}

- (NSSize)cellSize
{
	NSSize size = [super cellSize];
	if (_leftImage)
		size.width += [_leftImage size].width + 5.0;
	return size;
}

@end
