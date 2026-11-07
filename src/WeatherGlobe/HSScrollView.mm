#import "HSScrollView.h"

NSString *HSScrollViewReflectScrolledClipViewNotification = @"HSScrollViewReflectScrolledClipViewNotification";

// What the i386 conversion to int made INT_MIN of: NaN and out-of-range
// coordinates.
static bool _isBadCoordinate(CGFloat value)
{
	return !(value > -2147483648.0 && value < 2147483648.0);
}

@implementation HSScrollView

- (BOOL)mouseDownCanMoveWindow
{
	return NO;
}

// Scrolls a clip view with a broken origin back to zero, and tells about
// each reflection.
- (void)reflectScrolledClipView:(id)aView
{
	NSRect visible = [self documentVisibleRect];
	if (_isBadCoordinate(visible.origin.x) || _isBadCoordinate(visible.origin.y)) {
		BOOL posts = [aView postsBoundsChangedNotifications];
		[aView setPostsBoundsChangedNotifications:NO];
		[aView setBoundsOrigin:NSZeroPoint];
		[aView setPostsBoundsChangedNotifications:posts];
		[self setNeedsDisplay:YES];
		[self display];
	}
	[super reflectScrolledClipView:aView];
	[[NSNotificationCenter defaultCenter] postNotificationName:HSScrollViewReflectScrolledClipViewNotification object:self];
}

- (NSSize)frameSizeForContentSize:(NSSize)aSize
{
	return [[self class] frameSizeForContentSize:aSize hasHorizontalScroller:[self hasHorizontalScroller]
	                         hasVerticalScroller:[self hasVerticalScroller] borderType:[self borderType]];
}

@end
