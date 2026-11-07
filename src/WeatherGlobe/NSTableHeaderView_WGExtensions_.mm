#import "NSTableHeaderView+WGExtensions.h"

@implementation NSTableHeaderView (WGExtensions)

// The table's menu.
- (id)menuForEvent:(id)aEvent
{
	return [[self tableView] menuForEvent:aEvent];
}

@end
