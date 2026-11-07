#import "WGGridControl.h"

@implementation WGGridControl

// Tells an autoresize delegate about size changes.
- (void)setFrameSize:(NSSize)aSize
{
	NSSize oldSize = [self frame].size;
	if (aSize.width != oldSize.width || aSize.height != oldSize.height) {
		[super setFrameSize:aSize];
		if ([delegate conformsToProtocol:@protocol(WGAutoresizeDelegate)])
			[delegate view:self didResizeWithOldSize:oldSize];
	}
}

@end
