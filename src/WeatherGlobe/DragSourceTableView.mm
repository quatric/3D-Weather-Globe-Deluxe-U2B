#import "DragSourceTableView.h"
#import "CitiesListController.h"

NSString *_kEntriesListDraggingEndedNotification = @"EntriesListDraggingEndedNotification";

static NSArray *_selectedRows(NSTableView *view)
{
	NSMutableArray *rows = [NSMutableArray array];
	[[view selectedRowIndexes] enumerateIndexesUsingBlock:^(NSUInteger index, BOOL *stop) {
		[rows addObject:[NSNumber numberWithInt:(int)index]];
	}];
	return rows;
}

// The entries tables: rows drag out (copy), keys go to the data source
// (tableView:requestedToDeleteRows:, …ToMarkRows:, …ToEditRows: with the
// selected row numbers), and one selected entry offers its name as a
// service.
@implementation DragSourceTableView

- (NSUInteger)draggingSourceOperationMaskForLocal:(BOOL)aLocal
{
	return NSDragOperationCopy;
}

- (void)draggedImage:(id)aImage endedAt:(NSPoint)aAt operation:(NSUInteger)aOperation
{
	[[NSNotificationCenter defaultCenter] postNotificationName:_kEntriesListDraggingEndedNotification object:self];
}

- (void)keyDown:(id)aDown
{
	NSString *characters = [aDown charactersIgnoringModifiers];
	unichar c = [characters length] == 1 ? [characters characterAtIndex:0] : 0;
	id dataSource = [self dataSource];
	if (c == NSDeleteCharacter || c == NSDeleteFunctionKey) {
		[self deleteForward:nil];
		return;
	}
	if ([characters isEqualToString:@" "] && [dataSource respondsToSelector:@selector(tableView:requestedToMarkRows:)]) {
		[dataSource performSelector:@selector(tableView:requestedToMarkRows:) withObject:self withObject:_selectedRows(self)];
		return;
	}
	if (([aDown keyCode] == 0x24 || [aDown keyCode] == 0x4c)
	    && [dataSource respondsToSelector:@selector(tableView:requestedToEditRows:)]) {
		[dataSource performSelector:@selector(tableView:requestedToEditRows:) withObject:self withObject:_selectedRows(self)];
		return;
	}
	if (c == NSHomeFunctionKey) {
		[self scrollRowToVisible:0];
		return;
	}
	if (c == NSEndFunctionKey) {
		[self scrollRowToVisible:[self numberOfRows] - 1];
		return;
	}
	[super keyDown:aDown];
}

- (void)deleteForward:(id)aForward
{
	id dataSource = [self dataSource];
	if ([dataSource respondsToSelector:@selector(tableView:requestedToDeleteRows:)])
		[dataSource performSelector:@selector(tableView:requestedToDeleteRows:) withObject:self withObject:_selectedRows(self)];
}

// Posts a selection change so the list's owner hears that it got focus.
- (BOOL)becomeFirstResponder
{
	BOOL became = [super becomeFirstResponder];
	if (became)
		[[NSNotificationCenter defaultCenter]
		    postNotification:[NSNotification notificationWithName:NSTableViewSelectionDidChangeNotification object:self]];
	return became;
}

- (id)validRequestorForSendType:(id)aType returnType:(id)aType2
{
	if ([aType isEqual:NSPasteboardTypeString] && !aType2 && [[self dataSource] isKindOfClass:[EntriesListController class]]
	    && [self numberOfSelectedRows] == 1)
		return self;
	return [super validRequestorForSendType:aType returnType:aType2];
}

- (BOOL)writeSelectionToPasteboard:(id)aPasteboard types:(id)aTypes
{
	[aPasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
	GenericEntry *entry = [(EntriesListController *)[self dataSource] selectedItem];
	[aPasteboard setString:(NSString *)entry->ownName(0)->cfString() forType:NSPasteboardTypeString];
	return YES;
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute])
		return [[NSBundle mainBundle] localizedStringForKey:@"Locations list" value:@"" table:nil];
	return [super accessibilityAttributeValue:aValue];
}

// Opens the row's context menu at the selected row.
- (void)accessibilityCellAction:(id)aAction
{
	id delegate = [self delegate];
	if (![delegate respondsToSelector:@selector(tableView:menuForEvent:)])
		return;
	NSRect rect = [self rectOfRow:[self selectedRow]];
	NSEvent *event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
	                                    location:[self convertPoint:rect.origin toView:nil]
	                               modifierFlags:0
	                                   timestamp:0
	                                windowNumber:[[self window] windowNumber]
	                                     context:nil
	                                 eventNumber:0
	                                  clickCount:1
	                                    pressure:0];
	NSMenu *menu = [delegate performSelector:@selector(tableView:menuForEvent:) withObject:self withObject:event];
	[NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

@end
