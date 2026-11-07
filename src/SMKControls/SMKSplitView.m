#import "SMKSplitView.h"

// A split view with a settable divider thickness and image, and double
// clicks on dividers reported to the delegate.
@implementation SMKSplitView

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		[aCoder encodeFloat:_smkcDividerThickness forKey:@"smk_dividerThickness"];
		[aCoder encodeObject:_smkcDividerImage forKey:@"smk_dividerImage"];
		return;
	}
	int version = 0;
	[aCoder encodeValueOfObjCType:"i" at:&version];
	[aCoder encodeValueOfObjCType:"f" at:&_smkcDividerThickness];
	[aCoder encodeObject:_smkcDividerImage];
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		_smkcDividerThickness = [aCoder decodeFloatForKey:@"smk_dividerThickness"];
		_smkcDividerImage = [[aCoder decodeObjectForKey:@"smk_dividerImage"] retain];
	}
	else {
		int version;
		[aCoder decodeValueOfObjCType:"i" at:&version];
		[aCoder decodeValueOfObjCType:"f" at:&_smkcDividerThickness];
		_smkcDividerImage = [[aCoder decodeObject] retain];
	}
	return self;
}

- (void)mouseDown:(id)aDown
{
	if ([aDown clickCount] == 2 && [self smkcHandleDoubleClickInSplitterView:self withEvent:aDown]) {
		return;
	}
	[super mouseDown:aDown];
}

// the divider image centered in the divider
- (void)drawDividerInRect:(NSRect)aRect
{
	if (_smkcDividerImage == nil) {
		[super drawDividerInRect:aRect];
		return;
	}
	NSRect from = NSZeroRect;
	from.size = [_smkcDividerImage size];
	NSRect to = from;
	to.origin.x = floorf((float)NSMidX(aRect) - (float)NSMidX(from));
	to.origin.y = floorf((float)NSMidY(aRect) - (float)NSMidY(from));
	[_smkcDividerImage drawInRect:to fromRect:from operation:NSCompositingOperationSourceOver fraction:1.0];
}

- (void)setDividerImage:(id)aImage
{
	if (_smkcDividerImage != aImage) {
		[_smkcDividerImage release];
		_smkcDividerImage = [aImage retain];
		[self setNeedsDisplay:YES];
	}
}

- (id)dividerImage
{
	return _smkcDividerImage;
}

- (void)setDividerThickness:(CGFloat)aThickness
{
	_smkcDividerThickness = (float)aThickness;
}

- (CGFloat)dividerThickness
{
	return _smkcDividerThickness;
}

- (void)dealloc
{
	[super dealloc];
}

@end

// Lets an SMKSplitter inside a split view's subview drag a divider.
@implementation NSSplitView (SMKSplitterEnclosingSplitView)

- (void)trackSMKSplitterView:(id)aView withEvent:(id)aEvent
{
	if ([aEvent clickCount] == 2 && [self smkcHandleDoubleClickInSplitterView:aView withEvent:aEvent]) {
		return;
	}
	NSPoint start = [self convertPoint:[aEvent locationInWindow] fromView:nil];
	NSInteger index = [self smkcHitTestDividerIndexAtPoint:start isExactlyInDivider:NULL];
	if (index == -1) {
		return;
	}
	BOOL vertical = [self isVertical];
	id delegate = [self delegate];
	NSView *first = [[self subviews] objectAtIndex:index];
	NSView *second = [[self subviews] objectAtIndex:index + 1];
	NSRect firstFrame = [first frame];
	NSRect secondFrame = [second frame];
	float thickness = (float)[self dividerThickness];
	float minCoordinate = vertical ? (float)firstFrame.origin.x : (float)firstFrame.origin.y;
	float maxCoordinate = (vertical ? (float)NSMaxX(secondFrame) : (float)NSMaxY(secondFrame)) - thickness;
	float position = vertical ? (float)NSMaxX(firstFrame) : (float)NSMaxY(firstFrame);
	float end = vertical ? (float)NSMaxX(secondFrame) : (float)NSMaxY(secondFrame);
	float startCoordinate = vertical ? (float)start.x : (float)start.y;
	NSRect dirty = NSUnionRect(secondFrame, firstFrame);
	if ([delegate respondsToSelector:@selector(splitView:constrainMinCoordinate:ofSubviewAt:)]) {
		minCoordinate = MAX(minCoordinate, (float)[delegate splitView:self constrainMinCoordinate:minCoordinate ofSubviewAt:index]);
	}
	if ([delegate respondsToSelector:@selector(splitView:constrainMaxCoordinate:ofSubviewAt:)]) {
		maxCoordinate = MIN(maxCoordinate, (float)[delegate splitView:self constrainMaxCoordinate:maxCoordinate ofSubviewAt:index]);
	}
	BOOL constrains = [delegate respondsToSelector:@selector(splitView:constrainSplitPosition:ofSubviewAt:)];
	float initial = position;
	float last = position;
	NSEvent *event;
	do {
		event = [[self window] nextEventMatchingMask:NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged];
		NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
		float coordinate = vertical ? (float)p.x : (float)p.y;
		float pos = MIN(MAX(coordinate + initial - startCoordinate, minCoordinate), maxCoordinate);
		if (constrains) {
			pos = (float)[delegate splitView:self constrainSplitPosition:pos ofSubviewAt:index];
			pos = MIN(MAX(pos, minCoordinate), maxCoordinate);
		}
		if (last != pos) {
			[[NSNotificationCenter defaultCenter] postNotificationName:NSSplitViewWillResizeSubviewsNotification object:self];
			NSRect r = firstFrame;
			if (vertical) {
				r.size.width = pos - (float)firstFrame.origin.x;
			}
			else {
				r.size.height = pos - (float)firstFrame.origin.y;
			}
			[first setFrame:r];
			float secondStart = pos + thickness;
			r = secondFrame;
			if (vertical) {
				r.origin.x = secondStart;
				r.size.width = end - secondStart;
			}
			else {
				r.origin.y = secondStart;
				r.size.height = end - secondStart;
			}
			[second setFrame:r];
			[[NSNotificationCenter defaultCenter] postNotificationName:NSSplitViewDidResizeSubviewsNotification object:self];
			last = pos;
		}
		[self setNeedsDisplayInRect:dirty];
	} while ([event type] != NSEventTypeLeftMouseUp);
}

// double clicks on a divider (aView == self) or on a splitter
- (BOOL)smkcHandleDoubleClickInSplitterView:(id)aView withEvent:(id)aEvent
{
	NSPoint p = [self convertPoint:[aEvent locationInWindow] fromView:nil];
	BOOL exactly = NO;
	NSInteger index = [self smkcHitTestDividerIndexAtPoint:p isExactlyInDivider:&exactly];
	if ([aEvent clickCount] != 2) {
		return NO;
	}
	id delegate = [self delegate];
	if (aView == self) {
		if (![delegate respondsToSelector:@selector(splitView:didDoubleClickInDividerAtIndex:)] || index == -1 || !exactly) {
			return NO;
		}
		[delegate splitView:self didDoubleClickInDividerAtIndex:index];
		return YES;
	}
	if (![delegate respondsToSelector:@selector(splitView:didDoubleClickInSplitter:)]) {
		return NO;
	}
	[delegate splitView:self didDoubleClickInSplitter:aView];
	return YES;
}

// The divider at the point: in a vertical split view the divider before
// the subview whose left edge is past the point, or the one after the
// subview the point is in; in a horizontal one the divider before either
// (the original's rule). aDivider tells whether the point is in the divider
// itself.
- (NSInteger)smkcHitTestDividerIndexAtPoint:(NSPoint)aPoint isExactlyInDivider:(BOOL *)aDivider
{
	BOOL vertical = [self isVertical];
	NSArray *subviews = [self subviews];
	NSInteger count = (NSInteger)[subviews count];
	float coordinate = vertical ? (float)aPoint.x : (float)aPoint.y;
	NSInteger i;
	for (i = 0; i < count; i++) {
		NSRect frame = [[subviews objectAtIndex:i] frame];
		if (coordinate < (vertical ? (float)frame.origin.x : (float)frame.origin.y)) {
			i--;
			if (aDivider != NULL)
				*aDivider = YES;
			break;
		}
		if ((i < count - 1 || !vertical) && coordinate < (vertical ? (float)NSMaxX(frame) : (float)NSMaxY(frame))) {
			if (!vertical)
				i--;
			if (aDivider != NULL)
				*aDivider = NO;
			break;
		}
	}
	if (i == count)
		i = -1;
	if (aDivider != NULL && i == -1)
		*aDivider = (BOOL)-1;
	return i;
}

@end
