#import "MainWindowController.h"
#import "NSView+WGExtensions.h"

@implementation WGMainWindow

// Never smaller than the minimum size; growing the height keeps the top.
- (NSRect)constrainFrameRect:(NSRect)aRect toScreen:(id)aScreen
{
	NSSize minSize = [self minSize];
	NSRect rect = [super constrainFrameRect:aRect toScreen:aScreen];
	if (rect.size.height < minSize.height) {
		rect.origin.y -= minSize.height - rect.size.height;
		rect.size.height = minSize.height;
	}
	rect.size.width = MAX(minSize.width, rect.size.width);
	return rect;
}

// Command-Option-F goes to the search field.
- (void)sendEvent:(id)aEvent
{
	NSEvent *event = aEvent;
	if ([event type] == NSEventTypeKeyDown) {
		NSString *characters = [event charactersIgnoringModifiers];
		if (([characters isEqualToString:@"f"] || [characters isEqualToString:@"F"])
		    && ([event modifierFlags] & (NSEventModifierFlagCommand | NSEventModifierFlagOption))
		           == (NSEventModifierFlagCommand | NSEventModifierFlagOption)) {
			id field = [[self contentView] wgDescendantSearchField];
			if (field) {
				[self makeFirstResponder:field];
				return;
			}
		}
	}
	[super sendEvent:aEvent];
}

- (void)redo:(id)aRedo
{
	LUndoManager::instance()->redo();
}

- (void)undo:(id)aUndo
{
	LUndoManager::instance()->undo();
}

static NSString *_actionTitle(NSString *verb, LString *name)
{
	return [NSString stringWithFormat:@"%@ %@", [[NSBundle mainBundle] localizedStringForKey:verb value:@"" table:nil],
	                                  name ? (NSString *)name->cfString() : @""];
}

// Undo and redo (titled with the action) follow the model's undo manager
// unless a sheet is up.
- (BOOL)validateMenuItem:(id)aItem
{
	SEL action = [aItem action];
	if (![self attachedSheet]) {
		LUndoManager *manager = LUndoManager::instance();
		if (action == @selector(undo:)) {
			[aItem setTitle:_actionTitle(@"Undo", manager->undoActionName())];
			return manager->canUndo();
		}
		if (action == @selector(redo:)) {
			[aItem setTitle:_actionTitle(@"Redo", manager->redoActionName())];
			return manager->canRedo();
		}
	}
	if ([NSWindow instancesRespondToSelector:@selector(validateMenuItem:)])
		return [super validateMenuItem:aItem];
	return YES;
}

@end
