#import "NSWindow+NSWindowWGExt.h"
#import "NSView+WGExtensions.h"

@implementation NSWindow (NSWindowWGExt)

// AppKit's recalculateKeyViewLoop, else a loop through the content view's
// subviews by position.
- (void)wgRecalculateKeyViewLoop
{
	if ([self respondsToSelector:@selector(recalculateKeyViewLoop)]) {
		[self performSelector:@selector(recalculateKeyViewLoop)];
		return;
	}
	NSArray *views = [[self contentView] subviewsSortedByPosition];
	int count = (int)[views count];
	for (int i = 0; i < count; i++) {
		int next = i + 1 < count ? i + 1 : 0;
		[[views objectAtIndex:i] setNextKeyView:[views objectAtIndex:next]];
	}
}

@end
