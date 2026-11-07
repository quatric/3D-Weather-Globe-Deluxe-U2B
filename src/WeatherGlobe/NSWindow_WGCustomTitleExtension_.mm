#import "WGCustomTitleView.h"

@implementation NSWindow (WGCustomTitleExtension)

// The title view in the frame view above the content view; made (of the
// delegate's customTitleViewClass, if it has one) when there's none.
- (id)customTitleView
{
	NSView *contentView = [self contentView];
	NSView *frameView = [contentView superview];
	NSArray *subviews = [frameView subviews];
	int count = (int)[subviews count];
	for (int i = 0; i < count; i++) {
		id view = [subviews objectAtIndex:i];
		if ([view isKindOfClass:[WGCustomTitleView class]])
			return view;
	}
	NSRect bounds = [frameView bounds];
	NSRect frame = [contentView frame];
	frame.origin.y = NSMaxY(frame);
	frame.size.height = NSMaxY(bounds) - frame.origin.y;
	Class titleClass = Nil;
	id delegate = [self delegate];
	if ([delegate respondsToSelector:@selector(customTitleViewClass)]) {
		titleClass = [delegate performSelector:@selector(customTitleViewClass)];
		if (![titleClass isSubclassOfClass:[WGCustomTitleView class]])
			titleClass = Nil;
	}
	if (!titleClass)
		titleClass = [WGCustomTitleView class];
	WGCustomTitleView *titleView = [[[titleClass alloc] initWithFrame:frame] autorelease];
	[titleView setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
	[titleView setCompositeOperation:NSCompositingOperationCopy];
	// The frame view is layer-backed now: below its other subviews (where the
	// original put it) the title view is never drawn, so give it a layer, above
	// the content view and under the window buttons. It draws the title
	// itself; the system's would show through the buttons.
	[titleView setWantsLayer:YES];
	[frameView addSubview:titleView positioned:NSWindowAbove relativeTo:contentView];
	[self setTitleVisibility:NSWindowTitleHidden];
	return titleView;
}

@end
