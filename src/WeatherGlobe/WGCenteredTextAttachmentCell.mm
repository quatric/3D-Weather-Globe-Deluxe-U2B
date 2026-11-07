#import "WGCenteredTextAttachmentCell.h"

@implementation WGCenteredTextAttachmentCell

- (NSPoint)cellBaselineOffset
{
	return _baselineOffset;
}

- (void)setBaselineOffset:(NSPoint)aOffset
{
	_baselineOffset = aOffset;
}

@end
