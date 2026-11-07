#import "SMKScrollView.h"

// Without scrollers it passes the wheel on (to an enclosing scroll view).
@implementation SMKScrollView

- (void)scrollWheel:(id)aWheel
{
	if (![self hasVerticalScroller] && ![self hasHorizontalScroller])
		[[self nextResponder] scrollWheel:aWheel];
	else
		[super scrollWheel:aWheel];
}

@end
