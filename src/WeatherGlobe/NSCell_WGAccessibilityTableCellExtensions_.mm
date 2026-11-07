#import "WGAccessibilityTableImageCell.h"
#import "DragSourceTableView.h"

@implementation NSCell (WGAccessibilityTableCellExtensions)

- (id)wgTableCellAccessibilityActionNames
{
	return [NSArray arrayWithObject:NSAccessibilityPressAction];
}

- (id)wgTableCellAccessibilityActionDescription:(id)aDescription
{
	return [aDescription isEqualToString:NSAccessibilityPressAction] ? @"Action" : nil;
}

// Pressing sends the table's accessibilityCellAction:.
- (void)wgTableCellAccessibilityPerformAction:(id)aAction
{
	if ([aAction isEqualToString:NSAccessibilityPressAction]) {
		id view = [self controlView];
		if ([view respondsToSelector:@selector(accessibilityCellAction:)])
			[NSApp sendAction:@selector(accessibilityCellAction:) to:view from:self];
	}
}

@end
