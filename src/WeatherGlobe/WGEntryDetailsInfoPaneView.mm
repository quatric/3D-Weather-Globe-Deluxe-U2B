#import "WGEntryDetailsInfoPaneView.h"
#import "NSView+WGExtensions.h"

@implementation WGEntryDetailsInfoPaneView

- (BOOL)isFlipped
{
	return YES;
}

- (void)dealloc
{
	[super dealloc];
}

- (void)awakeFromNib
{
	[self wgFixFlip];
}

@end
