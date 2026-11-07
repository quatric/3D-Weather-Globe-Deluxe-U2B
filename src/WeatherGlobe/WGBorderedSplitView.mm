#import "WGBorderedSplitView.h"

// A split view whose divider is framed in gray.
@implementation WGBorderedSplitView

- (void)drawDividerInRect:(NSRect)aRect
{
	[super drawDividerInRect:aRect];
	[[NSColor colorWithDeviceRed:0.65 green:0.65 blue:0.65 alpha:1.0] set];
	NSFrameRectWithWidth(aRect, 1.0);
}

@end
