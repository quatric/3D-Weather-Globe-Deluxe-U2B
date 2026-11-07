#import "NSTableView+WGExtensions.h"

@implementation NSTableView (WGExtensions)

// The delegate's tableView:menuForEvent:, if it has one.
- (id)menuForEvent:(id)aEvent
{
	id delegate = [self delegate];
	if (delegate && [delegate respondsToSelector:@selector(tableView:menuForEvent:)])
		return [delegate performSelector:@selector(tableView:menuForEvent:) withObject:self withObject:aEvent];
	return [super menuForEvent:aEvent];
}

@end
