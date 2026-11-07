#import "WGSerialNumberController.h"

@implementation SecurityTextView

- (id)menuForEvent:(id)aEvent
{
	return nil;
}

// Only the editing commands.
- (BOOL)validateMenuItem:(id)aItem
{
	SEL action = [aItem action];
	if (action == @selector(cut:) || action == @selector(copy:) || action == @selector(paste:) ||
	    action == @selector(delete:) || action == @selector(selectAll:))
		return [super validateMenuItem:aItem];
	return NO;
}

@end
