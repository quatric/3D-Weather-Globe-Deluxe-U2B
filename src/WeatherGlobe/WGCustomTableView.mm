#import "WGCustomTableView.h"
#import "NSMenu+NSMenuWGExtensions.h"

@interface NSObject (WGCustomTableViewDelegate)
- (id)defaultInvisibleTableColumnIDsForForTableView:(id)aTableView;
- (id)orderedTableColumnIDsForTableView:(id)aTableView;
- (BOOL)tableView:(id)aTableView canHideTableColumn:(id)aColumn;
- (void)tableView:(id)aTableView willHideTableColumn:(id)aColumn;
@end

// A table view whose columns can be hidden from a menu on its corner view;
// the hidden column IDs are kept in the defaults under the autosave name.
@implementation WGCustomTableView

- (void)dealloc
{
	[_popupMenu release];
	[_invisibleColumns release];
	[super dealloc];
}

// Made on first use, hiding the saved columns (or the delegate's default
// ones when nothing is saved).
- (id)invisibleColumns
{
	if (_invisibleColumns)
		return _invisibleColumns;
	id delegate = [self delegate];
	NSString *name = [self autosaveName];
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[self setAutosaveTableColumns:NO];
	_invisibleColumns = [NSMutableArray new];
	NSArray *ids = name ? [defaults arrayForKey:name] : nil;
	if (!ids) {
		if (![delegate respondsToSelector:@selector(defaultInvisibleTableColumnIDsForForTableView:)])
			return _invisibleColumns;
		ids = [delegate defaultInvisibleTableColumnIDsForForTableView:self];
		if (!ids)
			return _invisibleColumns;
	}
	if ([ids count]) {
		NSEnumerator *enumerator = [ids objectEnumerator];
		id identifier;
		while ((identifier = [enumerator nextObject]))
			[self hideColumnWithIdentifier:identifier];
	}
	return _invisibleColumns;
}

- (id)invisibleColumnIDs
{
	NSMutableArray *ids = [NSMutableArray array];
	NSEnumerator *enumerator = [[self invisibleColumns] objectEnumerator];
	NSTableColumn *column;
	while ((column = [enumerator nextObject]))
		[ids addObject:[column identifier]];
	return ids;
}

- (id)invisibleColumnWithIdentifier:(id)aIdentifier
{
	NSArray *columns = [self invisibleColumns];
	for (NSUInteger i = 0; i < [columns count]; i++) {
		NSTableColumn *column = [columns objectAtIndex:i];
		if ([[column identifier] isEqualToString:aIdentifier])
			return column;
	}
	return nil;
}

// Where the column goes when shown again: its place in the delegate's
// column order, less the hidden columns before it.
- (NSInteger)positionOfInvisibleColumn:(id)aColumn
{
	id delegate = [self delegate];
	if (![delegate respondsToSelector:@selector(orderedTableColumnIDsForTableView:)])
		return NSIntegerMax;
	NSArray *ordered = [delegate orderedTableColumnIDsForTableView:self];
	if (!ordered || ![ordered count])
		return NSIntegerMax;
	NSInteger position = [ordered indexOfObject:[aColumn identifier]];
	NSArray *before = [NSArray arrayWithArray:[ordered subarrayWithRange:NSMakeRange(0, position)]];
	NSEnumerator *enumerator = [before reverseObjectEnumerator];
	id identifier;
	while ((identifier = [enumerator nextObject])) {
		if ([self isHiddenColumnWithIdentifier:identifier])
			position--;
	}
	return position;
}

// Hidden columns are still found by identifier.
- (id)tableColumnWithIdentifier:(id)aIdentifier
{
	id column = [super tableColumnWithIdentifier:aIdentifier];
	if (!column && _invisibleColumns)
		column = [self invisibleColumnWithIdentifier:aIdentifier];
	return column;
}

- (void)_saveInvisibleColumnIDs
{
	NSString *name = [self autosaveName];
	if (name)
		[[NSUserDefaults standardUserDefaults] setObject:[self invisibleColumnIDs] forKey:name];
}

// The position is clamped to the visible columns (the original passed it
// unchecked).
- (void)showColumnWithIdentifier:(id)aIdentifier
{
	NSTableColumn *column = [self tableColumnWithIdentifier:aIdentifier];
	NSInteger position = [self positionOfInvisibleColumn:column];
	[self addTableColumn:column];
	[[self invisibleColumns] removeObject:column];
	NSInteger last = [self numberOfColumns] - 1;
	if (position > last)
		position = last;
	[super moveColumn:[self columnWithIdentifier:aIdentifier] toColumn:position];
	[self sizeToFit];
	[self _saveInvisibleColumnIDs];
}

// Only columns the delegate lets go are hidden.
- (void)hideColumnWithIdentifier:(id)aIdentifier
{
	NSTableColumn *column = [self tableColumnWithIdentifier:aIdentifier];
	id delegate = [self delegate];
	if (![delegate respondsToSelector:@selector(tableView:canHideTableColumn:)]
		|| ![delegate tableView:self canHideTableColumn:column])
		return;
	[[self invisibleColumns] addObject:column];
	if ([delegate respondsToSelector:@selector(tableView:willHideTableColumn:)])
		[delegate tableView:self willHideTableColumn:column];
	[self removeTableColumn:column];
	[self sizeToFit];
	[self _saveInvisibleColumnIDs];
}

- (BOOL)isHiddenColumnWithIdentifier:(id)aIdentifier
{
	return [self invisibleColumnWithIdentifier:aIdentifier] != nil;
}

// An item per titled column, visible or not, that the delegate can hide.
- (id)_createMenu
{
	NSMenu *menu = [NSMenu new];
	NSArray *invisible = [self invisibleColumns];
	NSMutableArray *columns = [NSMutableArray arrayWithArray:[self tableColumns]];
	[columns addObjectsFromArray:invisible];
	[columns sortUsingSelector:@selector(comparisonFunc:)];
	NSEnumerator *enumerator = [columns objectEnumerator];
	NSTableColumn *column;
	while ((column = [enumerator nextObject])) {
		BOOL canHide = NO;
		id delegate = [self delegate];
		if ([delegate respondsToSelector:@selector(tableView:canHideTableColumn:)])
			canHide = [delegate tableView:self canHideTableColumn:column];
		NSString *title = [[column headerCell] title];
		if ([title length] && canHide) {
			NSMenuItem *item = [[[NSMenuItem alloc] initWithTitle:title action:@selector(menuAction:)
				keyEquivalent:@""] autorelease];
			[item setTarget:self];
			[item setRepresentedObject:[column identifier]];
			[menu addItem:item];
		}
	}
	return [menu autorelease];
}

// The button fills the corner view and pops up the columns menu.
- (void)createCornerMenuForButton:(id)aButton
{
	NSView *cornerView = [self cornerView];
	NSRect frame = [cornerView frame];
	frame.origin = NSZeroPoint;
	[aButton setFrame:frame];
	[cornerView addSubview:aButton];
	[aButton setTarget:self];
	[aButton setAction:@selector(onCornerViewButton:)];
	_popupMenu = [[self _createMenu] retain];
}

- (id)cornerViewPopupMenu
{
	return _popupMenu;
}

- (void)onCornerViewButton:(id)aButton
{
	[NSMenu wgPopUpContextMenu:_popupMenu withEvent:[[aButton window] currentEvent] forView:aButton];
}

// Visible columns are checked.
- (BOOL)validateMenuItem:(id)aItem
{
	if ([_popupMenu indexOfItem:aItem] != -1)
		[aItem setState:![self isHiddenColumnWithIdentifier:[aItem representedObject]]];
	return YES;
}

- (void)menuAction:(id)aAction
{
	if ([aAction state] == NSControlStateValueOff)
		[self showColumnWithIdentifier:[aAction representedObject]];
	else
		[self hideColumnWithIdentifier:[aAction representedObject]];
}

@end
