#import "WGRouteManagerController.h"

@implementation WGRouteManagerOutlineView

- (void)dealloc
{
	[_accessibilityRoleDesc release];
	[super dealloc];
}

- (void)setAccessibilityRoleDesc:(id)aDesc
{
	if (_accessibilityRoleDesc != aDesc) {
		[_accessibilityRoleDesc release];
		_accessibilityRoleDesc = [aDesc retain];
	}
}

// Every other request (the flag toggles, a click clears it) edits; the
// others select the row above instead (the last row for the first two).
- (void)editColumn:(NSInteger)aColumn row:(NSInteger)aRow withEvent:(id)aEvent select:(BOOL)aSelect
{
	_editingFlag = !_editingFlag;
	if (_editingFlag) {
		[super editColumn:aColumn row:aRow withEvent:aEvent select:aSelect];
		return;
	}
	if (aRow < 2)
		aRow = [self numberOfRows];
	[self deselectAll:nil];
	[self selectRowIndexes:[NSIndexSet indexSetWithIndex:aRow - 1] byExtendingSelection:NO];
	[[self window] makeFirstResponder:self];
}

- (void)mouseDown:(id)aDown
{
	_editingFlag = NO;
	[super mouseDown:aDown];
}

- (void)setEditingFlag:(BOOL)aFlag
{
	_editingFlag = aFlag;
}

// Home/End scroll, the delete keys delete, Return edits the name.
- (void)keyDown:(id)aDown
{
	switch ([aDown keyCode]) {
	case 0x73:
		[self scrollRowToVisible:0];
		return;
	case 0x77:
		[self scrollRowToVisible:[self numberOfRows] - 1];
		return;
	case 0x75:
	case 0x33:
		[self deleteForward:nil];
		return;
	case 0x24:
	case 0x4c:
		if ([self selectedRow] != -1) {
			if ([[[self tableColumnWithIdentifier:@"OLVColumn"] dataCell] isEditable])
				[self editColumn:[self columnWithIdentifier:@"OLVColumn"] row:[self selectedRow] withEvent:nil select:YES];
			return;
		}
		break;
	}
	[super keyDown:aDown];
}

- (void)deleteForward:(id)aForward
{
	id delegate = [self delegate];
	if ([delegate respondsToSelector:@selector(deleteRoute:)])
		[delegate deleteRoute:aForward];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute])
		return _accessibilityRoleDesc;
	return [super accessibilityAttributeValue:aValue];
}

@end
