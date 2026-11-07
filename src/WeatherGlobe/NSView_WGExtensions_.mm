#import "NSView+WGExtensions.h"

@interface NSView (WGPlaceholderProvider)
- (NSColor *)wgBackgroundColor;
- (NSAttributedString *)wgPlaceholderString;
@end

// Top to bottom, then left to right, in the parent's coordinates.
static NSInteger _wgCompareOrderOriginPos(__kindof NSView *first, __kindof NSView *second, void *context)
{
	NSPoint a = [first frame].origin;
	NSPoint b = [second frame].origin;
	BOOL flipped = [(NSView *)context isFlipped];
	if (a.y == b.y) {
		if (a.x < b.x)
			return NSOrderedAscending;
		return a.x > b.x ? NSOrderedDescending : NSOrderedSame;
	}
	if (a.y > b.y)
		return flipped ? NSOrderedDescending : NSOrderedAscending;
	return flipped ? NSOrderedAscending : NSOrderedDescending;
}

@implementation NSView (WGExtensions)

- (id)wgEnclosingSplitView
{
	NSView *superview = [self superview];
	if ([superview isKindOfClass:[NSSplitView class]])
		return superview;
	return [superview wgEnclosingSplitView];
}

- (NSSize)wgMinSize
{
	return NSZeroSize;
}

- (NSSize)wgMaxSize
{
	return NSZeroSize;
}

- (NSSize)wgPreferredSize
{
	return [self frame].size;
}

// Mirrors the subviews vertically: for views a nib laid out unflipped.
- (void)wgFixFlip
{
	NSArray *subviews = [self subviews];
	NSUInteger count = [subviews count];
	CGFloat height = [self bounds].size.height;
	for (NSUInteger i = 0; i < count; i++) {
		NSView *view = [subviews objectAtIndex:i];
		NSRect frame = [view frame];
		[view setFrameOrigin:NSMakePoint(frame.origin.x, height - (frame.origin.y + frame.size.height))];
	}
}

// The view's wgBackgroundColor and wgPlaceholderString (when it has them),
// the string fitted and centered vertically.
- (void)wgDrawPlaceholderStringInRect:(NSRect)aRect
{
	NSColor *color = [self respondsToSelector:@selector(wgBackgroundColor)] ? [self wgBackgroundColor] : nil;
	NSAttributedString *string = [self respondsToSelector:@selector(wgPlaceholderString)]
		? [self wgPlaceholderString] : nil;
	if (color) {
		[color set];
		NSRectFillUsingOperation(aRect, NSCompositingOperationSourceOver);
	}
	if (![string length])
		return;
	SMKAttributedStringRenderer *renderer = [SMKAttributedStringRenderer sharedRenderer];
	[renderer setContainerWidth:[self bounds].size.width];
	[renderer setHyphenationFactor:0.5];
	NSRect boundingRect = NSZeroRect;
	NSAttributedString *fitted = [renderer attributedStringByFittingAttributedString:string
		toSize:[self bounds].size boundingRect:&boundingRect];
	CGFloat y = boundingRect.origin.y + ceil(([self bounds].size.height - boundingRect.size.height) * 0.5);
	[renderer drawAttributedString:fitted atPoint:NSMakePoint(0.0, y)];
}

// Centered, word-wrapped.
- (id)wgAdjustedPlacholderStringAttributes:(id)aAttributes
{
	NSMutableDictionary *attributes = aAttributes ? [[aAttributes mutableCopy] autorelease]
		: [NSMutableDictionary dictionaryWithCapacity:1];
	NSParagraphStyle *style = [aAttributes objectForKey:NSParagraphStyleAttributeName];
	NSMutableParagraphStyle *paragraph = style ? [[style mutableCopy] autorelease]
		: [[NSMutableParagraphStyle new] autorelease];
	[paragraph setLineBreakMode:NSLineBreakByWordWrapping];
	[paragraph setAlignment:NSTextAlignmentCenter];
	[attributes setObject:paragraph forKey:NSParagraphStyleAttributeName];
	return [NSDictionary dictionaryWithDictionary:attributes];
}

- (void)wgRecurseSortSubviewsByPos:(id)aPos
{
	NSArray *sorted = [[self subviews] sortedArrayUsingFunction:_wgCompareOrderOriginPos
		context:self];
	for (NSView *view in sorted) {
		[aPos addObject:view];
		[view wgRecurseSortSubviewsByPos:aPos];
	}
}

// All the descendants in reading order.
- (id)subviewsSortedByPosition
{
	NSMutableArray *views = [NSMutableArray new];
	[self wgRecurseSortSubviewsByPos:views];
	return [views autorelease];
}

- (void)wgShiftSibviewsByXOnDelta:(CGFloat)aDelta
{
	NSArray *subviews = [self subviews];
	NSUInteger count = [subviews count];
	for (NSUInteger i = 0; i < count; i++) {
		NSView *view = [subviews objectAtIndex:i];
		NSPoint origin = [view frame].origin;
		[view setFrameOrigin:NSMakePoint(origin.x + aDelta, origin.y)];
	}
}

- (id)wgDescendantSearchField
{
	if ([self isKindOfClass:[NSSearchField class]])
		return self;
	NSArray *subviews = [self subviews];
	NSUInteger count = [subviews count];
	for (NSUInteger i = 0; i < count; i++) {
		id field = [[subviews objectAtIndex:i] wgDescendantSearchField];
		if (field)
			return field;
	}
	return nil;
}

@end
