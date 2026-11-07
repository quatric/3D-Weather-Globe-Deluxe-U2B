#import "WGAccessibilityTableImageCell.h"

@implementation WGAccessibilityTableImageCell

- (id)accessibilityActionNames
{
	return [self wgTableCellAccessibilityActionNames];
}

- (id)accessibilityActionDescription:(id)aDescription
{
	return [self wgTableCellAccessibilityActionDescription:aDescription];
}

- (void)accessibilityPerformAction:(id)aAction
{
	[self wgTableCellAccessibilityPerformAction:aAction];
}

@end
