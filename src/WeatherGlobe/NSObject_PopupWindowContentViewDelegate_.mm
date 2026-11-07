#import "WGCustomToolbarButtonCell.h"

@implementation NSObject (PopupWindowContentViewDelegate)

- (id)popupWindowContentViewForCell:(id)aCell
{
	return nil;
}

- (BOOL)isPopupWindowAutoHiddenForCell:(id)aCell
{
	return YES;
}

@end
