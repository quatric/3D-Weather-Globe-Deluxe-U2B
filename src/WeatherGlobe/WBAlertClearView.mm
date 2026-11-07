#import "WBAlert.h"

@implementation WBAlertClearView

- (BOOL)mouseDownCanMoveWindow
{
	return YES;
}

- (void)drawRect:(NSRect)aRect
{
	[[NSColor whiteColor] set];
	NSRectFill(aRect);
}

@end
