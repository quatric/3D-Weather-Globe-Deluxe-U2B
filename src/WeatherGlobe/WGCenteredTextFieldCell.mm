#import "WGCenteredTextFieldCell.h"

// The text's size, centered vertically in the bounds.
static NSRect _centeredRect(NSCell *cell, NSRect bounds)
{
	NSSize size = [cell cellSizeForBounds:bounds];
	return NSMakeRect(bounds.origin.x, bounds.origin.y + (bounds.size.height - size.height) * 0.5f, size.width, size.height);
}

@implementation WGCenteredTextFieldCell

- (NSRect)drawingRectForBounds:(NSRect)aBounds
{
	return _centeredRect(self, aBounds);
}

- (NSRect)titleRectForBounds:(NSRect)aBounds
{
	return _centeredRect(self, aBounds);
}

- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView
{
	[self drawInteriorWithFrame:aFrame inView:aView];
}

- (void)editWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate event:(id)aEvent
{
	[super editWithFrame:[self titleRectForBounds:aFrame] inView:aView editor:aEditor delegate:aDelegate event:aEvent];
}

@end
