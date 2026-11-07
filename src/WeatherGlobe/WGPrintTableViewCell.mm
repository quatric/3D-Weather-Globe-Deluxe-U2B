#import "RouteSetPrintDataSource.h"

@implementation WGPrintTableViewCell

// Centered vertically.
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView
{
	float height = [self cellSize].height;
	if (height < aFrame.size.height) {
		aFrame.origin.y += (aFrame.size.height - height) * 0.5f;
		aFrame.size.height = height;
	}
	[super drawWithFrame:aFrame inView:aView];
}

@end
