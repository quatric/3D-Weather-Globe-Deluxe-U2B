#import "CityInfoView.h"

@implementation CityInfoSelectionView

- (void)drawRect:(NSRect)aRect
{
	if ([[self window] isKeyWindow]) {
		[[NSColor colorWithCalibratedWhite:1.0 alpha:0.08] set];
		NSRectFillUsingOperation(aRect, NSCompositingOperationSourceOver);
	}
}

@end
