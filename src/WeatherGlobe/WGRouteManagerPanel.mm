#import "WGRouteManagerController.h"

@implementation WGRouteManagerPanel

// Undo and redo are the delegate's when it handles them.
- (BOOL)validateMenuItem:(id)aItem
{
	SEL action = [aItem action];
	if ((action == @selector(undo:) || action == @selector(redo:)) && [[self delegate] respondsToSelector:action]
	    && [[self delegate] respondsToSelector:@selector(validateMenuItem:)])
		return [(id)[self delegate] validateMenuItem:aItem];
	return [super validateMenuItem:aItem];
}

- (void)undo:(id)aUndo
{
	if ([[self delegate] respondsToSelector:_cmd])
		[(id)[self delegate] performSelector:_cmd withObject:aUndo];
}

- (void)redo:(id)aRedo
{
	if ([[self delegate] respondsToSelector:_cmd])
		[(id)[self delegate] performSelector:_cmd withObject:aRedo];
}

// Escape doesn't close the panel.
- (void)doCommandBySelector:(SEL)aSelector
{
	if (aSelector != @selector(cancel:))
		[super doCommandBySelector:aSelector];
}

@end
