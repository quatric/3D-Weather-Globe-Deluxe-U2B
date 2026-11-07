#import "WGSplitView.h"
#import "NSMutableArray+EXMutableArray.h"
#import "NSView+WGExtensions.h"

// Delegate hooks of the size-constrained split views.
@interface NSObject (WGSplitViewSizeDelegate)
- (NSSize)splitView:(NSSplitView *)aSplitView minSizeForView:(NSView *)aView;
- (NSSize)splitView:(NSSplitView *)aSplitView maxSizeForView:(NSView *)aView;
- (NSArray *)splitView:(NSSplitView *)aSplitView orderedArrayForViewsArray:(NSArray *)aViews;
@end

// Subviews may have a minimum and a maximum size (0 = unlimited), from the
// delegate or from the subview's own wgMinSize/wgMaxSize.
@implementation NSSplitView (WGSizeConstains)

// Along the split axis (width when vertical) and across it.
- (CGFloat)_wgMain:(NSSize)aSize
{
	return [self isVertical] ? aSize.width : aSize.height;
}

- (CGFloat)_wgMainOrigin:(NSRect)aRect
{
	return [self isVertical] ? aRect.origin.x : aRect.origin.y;
}

- (BOOL)wgCanAdjustViews:(id)aViews failedOnMax:(BOOL *)aMax
{
	CGFloat sumMin = 0.0, sumMax = 0.0;
	BOOL unlimited = NO;
	for (NSView *view in aViews) {
		sumMin += [self _wgMain:[self wgMinSizeForView:view]];
		CGFloat max = [self _wgMain:[self wgMaxSizeForView:view]];
		if (max != 0.0)
			sumMax += max;
		else
			unlimited = YES;
	}
	CGFloat size = [self _wgMain:[self bounds].size];
	BOOL fitsMax = unlimited || size < sumMax;
	if (aMax)
		*aMax = fitsMax;
	return sumMin < size && fitsMax;
}

// The leading subviews whose minimums fit; empty when even those fail on
// the minimum.
- (id)wgFittingSubviewsInContainer
{
	NSMutableArray *views = [[[self subviews] mutableCopy] autorelease];
	BOOL failedOnMax = NO;
	while ([views count] && ![self wgCanAdjustViews:views failedOnMax:&failedOnMax]) {
		if (failedOnMax)
			return views;
		[views removeLastObject];
	}
	return failedOnMax ? (id)views : (id)[NSArray array];
}

// Clamps every subview to its limits, then spreads the remaining space over
// them (in the delegate's order if it gives one).
- (void)wgCustomAdjustSubviews:(BOOL)aSubviews
{
	NSArray *views = [self wgFittingSubviewsInContainer];
	NSUInteger count = [views count];
	if (aSubviews)
		[self adjustSubviews];
	BOOL vertical = [self isVertical];
	NSSize bounds = [self bounds].size;
	CGFloat total = 0.0;
	for (NSUInteger i = 0; i < count; i++) {
		NSView *view = [views objectAtIndex:i];
		NSRect frame = [view frame];
		CGFloat *main = vertical ? &frame.size.width : &frame.size.height;
		if (vertical)
			frame.size.height = bounds.height;
		else
			frame.size.width = bounds.width;
		CGFloat min = [self _wgMain:[self wgMinSizeForView:view]];
		if (*main < min)
			*main = min;
		CGFloat max = [self _wgMain:[self wgMaxSizeForView:view]];
		if (max > 0.0 && max < *main)
			*main = max;
		[view setFrame:frame];
		total += *main;
	}
	// The original subtracted a single divider thickness.
	CGFloat avail = [self _wgMain:bounds] - ([self dividerThickness] + total);
	if (avail != 0.0) {
		NSArray *ordered = views;
		id delegate = [self delegate];
		if ([delegate respondsToSelector:@selector(splitView:orderedArrayForViewsArray:)]) {
			NSArray *array = [delegate splitView:self orderedArrayForViewsArray:views];
			if (array && [array count] == count)
				ordered = array;
		}
		for (NSUInteger i = 0; i < count && avail != 0.0; i++) {
			NSView *view = [ordered objectAtIndex:i];
			NSRect frame = [view frame];
			CGFloat *main = vertical ? &frame.size.width : &frame.size.height;
			CGFloat delta = 0.0;
			if (avail > 0.0) {
				CGFloat max = [self _wgMain:[self wgMaxSizeForView:view]];
				if (max == 0.0 || *main < max) {
					delta = avail;
					if (max != 0.0 && max - *main <= avail)
						delta = max - *main;
				}
			} else {
				CGFloat min = [self _wgMain:[self wgMinSizeForView:view]];
				if (min < *main) {
					delta = avail;
					if (avail <= min - *main)
						delta = min - *main;
				}
			}
			*main += delta;
			[view setFrame:frame];
			avail -= delta;
		}
	}
	if (!aSubviews) {
		CGFloat pos = [self _wgMainOrigin:[self bounds]];
		for (NSUInteger i = 0; i < count; i++) {
			NSView *view = [views objectAtIndex:i];
			NSRect frame = [view frame];
			CGFloat *origin = vertical ? &frame.origin.x : &frame.origin.y;
			if (*origin != pos) {
				*origin = pos;
				[view setFrame:frame];
			}
			pos = *origin + [self _wgMain:frame.size];
		}
		[self adjustSubviews];
	}
}

// For the delegate's splitView:constrainMinCoordinate:ofSubviewAt:.
- (CGFloat)wgConstrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	NSView *prev = [[self subviews] extraObjectAtIndex:aAt];
	NSView *next = [[self subviews] extraObjectAtIndex:aAt + 1];
	CGFloat result = aCoordinate;
	if (prev) {
		CGFloat min = [self _wgMain:[self wgMinSizeForView:prev]];
		CGFloat limit = [self _wgMainOrigin:[prev frame]] + min;
		if (min != 0.0 && result <= limit)
			result = limit;
	}
	if (next) {
		CGFloat max = [self _wgMain:[self wgMaxSizeForView:next]];
		if (max != 0.0) {
			NSRect frame = [prev frame];
			CGFloat limit = [self _wgMainOrigin:frame] + [self _wgMain:frame.size]
				- (max - [self _wgMain:[next frame].size]);
			if (result <= limit)
				result = limit;
		}
	}
	return result;
}

// For the delegate's splitView:constrainMaxCoordinate:ofSubviewAt:.
- (CGFloat)wgConstrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	NSView *prev = [[self subviews] extraObjectAtIndex:aAt];
	NSView *next = [[self subviews] extraObjectAtIndex:aAt + 1];
	CGFloat result = aCoordinate;
	if (prev) {
		CGFloat max = [self _wgMain:[self wgMaxSizeForView:prev]];
		CGFloat limit = [self _wgMainOrigin:[prev frame]] + max;
		if (max != 0.0 && limit <= aCoordinate)
			result = limit;
	}
	if (next) {
		CGFloat min = [self _wgMain:[self wgMinSizeForView:next]];
		if (min != 0.0 && aCoordinate - min <= result)
			result = aCoordinate - min;
	}
	return result;
}

// Sum along the split axis, the largest across it.
- (NSSize)wgMinSize
{
	NSSize size = NSZeroSize;
	BOOL vertical = [self isVertical];
	for (NSView *view in [self wgFittingSubviewsInContainer]) {
		NSSize min = [self wgMinSizeForView:view];
		if (vertical) {
			size.width += min.width;
			size.height = MAX(size.height, min.height);
		} else {
			size.height += min.height;
			size.width = MAX(size.width, min.width);
		}
	}
	return size;
}

// Sum along the split axis, the smallest limit across it (0 = unlimited).
- (NSSize)wgMaxSize
{
	NSSize size = NSZeroSize;
	BOOL vertical = [self isVertical];
	for (NSView *view in [self wgFittingSubviewsInContainer]) {
		NSSize max = [self wgMaxSizeForView:view];
		CGFloat *along = vertical ? &size.width : &size.height;
		CGFloat *across = vertical ? &size.height : &size.width;
		CGFloat viewAlong = vertical ? max.width : max.height;
		CGFloat viewAcross = vertical ? max.height : max.width;
		*along += viewAlong;
		if (viewAcross != 0.0 && (*across == 0.0 || viewAcross < *across))
			*across = viewAcross;
	}
	return size;
}

- (NSSize)wgMinSizeForView:(id)aView
{
	id delegate = [self delegate];
	if ([delegate respondsToSelector:@selector(splitView:minSizeForView:)])
		return [delegate splitView:self minSizeForView:aView];
	return [aView wgMinSize];
}

- (NSSize)wgMaxSizeForView:(id)aView
{
	id delegate = [self delegate];
	if ([delegate respondsToSelector:@selector(splitView:maxSizeForView:)])
		return [delegate splitView:self maxSizeForView:aView];
	return [aView wgMaxSize];
}

@end
