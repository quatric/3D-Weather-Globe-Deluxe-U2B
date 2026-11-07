#import "NSCell+WGExtensions.h"
#import "WGCenteredTextAttachmentCell.h"

@implementation NSCell (WGExtensions)

// Cells without a line break mode get it in their attributed string.
- (void)wgFixLineBreakingTo:(NSInteger)aTo
{
	if (![self respondsToSelector:@selector(lineBreakMode)]) {
		[self setObjectValue:[[self attributedStringValue] wgSetLineBreak:aTo]];
	} else if ([self lineBreakMode] != aTo) {
		[self setLineBreakMode:(NSLineBreakMode)aTo];
	}
}

@end
