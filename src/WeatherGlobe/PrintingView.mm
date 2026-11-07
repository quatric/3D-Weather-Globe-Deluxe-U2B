#import "PrintingView.h"
#import "PrintController.h"

// Prints the controller's pages on white.
@implementation PrintingView

- (id)printController
{
	return _controller;
}

- (void)setPrintController:(id)aController
{
	_controller = aController;
}

- (BOOL)isOpaque
{
	return YES;
}

- (id)printJobTitle
{
	NSString *title = [_controller printJobTitle];
	return [title length] ? title : [super printJobTitle];
}

- (BOOL)knowsPageRange:(NSRange *)aRange
{
	aRange->location = 1;
	aRange->length = [_controller pagesCount];
	return YES;
}

- (NSRect)rectForPage:(NSInteger)aPage
{
	return [_controller preparePage:aPage];
}

- (void)drawRect:(NSRect)aRect
{
	[[NSColor whiteColor] set];
	NSRectFill(aRect);
	[super drawRect:aRect];
}

@end
