#import "WGSplitter.h"

@implementation WGSplitter

- (BOOL)acceptsFirstMouse:(id)aMouse
{
	return YES;
}

- (id)enclosingSplitView
{
	return splitView;
}

// Hidden while it has no split view.
- (void)setEnclosingSplitView:(id)aView
{
	if (splitView != aView) {
		[splitView release];
		splitView = [aView retain];
	}
	[self setHidden:splitView == nil];
}

// Dragging is a live resize of the split view.
- (void)mouseDown:(id)aDown
{
	if ([splitView respondsToSelector:@selector(_startLiveResize)])
		[splitView performSelector:@selector(_startLiveResize)];
	[super mouseDown:aDown];
	if ([splitView respondsToSelector:@selector(_endLiveResize)])
		[splitView performSelector:@selector(_endLiveResize)];
}

@end
