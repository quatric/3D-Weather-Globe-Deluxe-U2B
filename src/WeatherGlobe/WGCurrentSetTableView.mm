#import "WGCurrentSetTableView.h"
#import "WGCurrentSetController.h"
#import "NSView+WGExtensions.h"

@interface NSObject (WGCurrentSetTableViewDelegate)
- (BOOL)wgValidateChangeItemColorInTableView:(id)aView;
@end

// The route set's table: a placeholder when empty (and no header), route
// breaks drawn as separators (with the subset's distance when printed),
// and the edit keys and menu items passed to the delegate.
@implementation WGCurrentSetTableView

- (void)dealloc
{
	[_placeholderString release];
	[_savedHeaderView release];
	[super dealloc];
}

- (id)wgPlaceholderString
{
	return _placeholderString;
}

- (void)setWGPlaceholderString:(id)aString
{
	if (_placeholderString != aString) {
		[_placeholderString release];
		_placeholderString = [aString retain];
	}
}

- (id)wgBackgroundColor
{
	return [self backgroundColor];
}

// The header is hidden while the table is empty.
- (void)toggleHeaderView
{
	if (![self numberOfRows] && [self headerView]) {
		_savedHeaderView = [[self headerView] retain];
		[self setHeaderView:nil];
	} else if ([self numberOfRows] > 0 && ![self headerView]) {
		[self setHeaderView:(NSTableHeaderView *)[_savedHeaderView autorelease]];
		_savedHeaderView = nil;
	}
}

- (void)reloadData
{
	[super reloadData];
	[self toggleHeaderView];
}

- (NSUInteger)draggingSourceOperationMaskForLocal:(BOOL)aLocal
{
	return NSDragOperationCopy | NSDragOperationMove;
}

// Lets the controller update for the focused table.
- (BOOL)becomeFirstResponder
{
	BOOL result = [super becomeFirstResponder];
	if (result)
		[[NSNotificationCenter defaultCenter] postNotification:
			[NSNotification notificationWithName:NSTableViewSelectionDidChangeNotification object:self]];
	return result;
}

- (void)drawSeparatorInRect:(NSRect)aRect
{
	NSImage *image = [NSImage imageNamed:@"route_separ"];
	NSSize size = [image size];
	[image drawInRect:NSMakeRect(aRect.origin.x, aRect.origin.y + ((aRect.size.height - size.height) * 0.5 - 5.0),
		aRect.size.width, size.height) fromRect:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, size.width, size.height)
		operation:NSCompositingOperationSourceOver fraction:1.0];
}

// The distance at the right of the row, the separator before it.
- (void)drawPrintSeparatorInRow:(NSInteger)aRow
{
	NSRect rect = [self rectOfRow:aRow];
	CGFloat y = rect.origin.y, height = rect.size.height;
	id delegate = [self delegate];
	NSString *distance = nil;
	if ([delegate respondsToSelector:@selector(wgTableView:distanceForSubset:)])
		distance = [delegate wgTableView:self distanceForSubset:aRow];
	NSDictionary *attributes = [NSDictionary dictionaryWithObject:[NSFont boldSystemFontOfSize:13.0]
		forKey:NSFontAttributeName];
	NSAttributedString *text = [[[NSAttributedString alloc] initWithString:distance ? distance : @""
		attributes:attributes] autorelease];
	NSSize size = [text size];
	if (size.height < rect.size.height) {
		y = rect.origin.y + (rect.size.height - size.height) * 0.5;
		height = size.height;
	}
	CGFloat x = rect.size.width - (size.width + 3.0);
	[text drawAtPoint:NSMakePoint(x, y)];
	NSImage *image = [NSImage imageNamed:@"route_separ"];
	NSSize imageSize = [image size];
	if (imageSize.height < height) {
		y += (height - imageSize.height) * 0.5;
		height = imageSize.height;
	}
	[image drawInRect:NSMakeRect(rect.origin.x + 7.0, y, floorf(x - 14.0), height)
		fromRect:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, imageSize.width, imageSize.height)
		operation:NSCompositingOperationSourceOver fraction:1.0];
}

// Break rows draw a separator in the break columns and the cells in the
// others.
- (void)drawRow:(NSInteger)aRow clipRect:(NSRect)aRect
{
	WGCurrentSetController *delegate = (WGCurrentSetController *)[self delegate];
	NSArray *columns = [self tableColumns];
	if (![delegate wgTableView:self shouldDisplayBreakAtRow:aRow]) {
		[super drawRow:aRow clipRect:aRect];
		return;
	}
	if ([delegate isWGTableViewPrinted:self]) {
		[self drawPrintSeparatorInRow:aRow];
		return;
	}
	int count = (int)[columns count];
	for (int i = 0; i < count; i++) {
		if ([delegate wgTableView:self shouldDisplayBreakAtColumn:[[self tableColumns] objectAtIndex:i]])
			[self drawSeparatorInRect:NSIntersectionRect([self rectOfColumn:i], [self rectOfRow:aRow])];
		else
			[super drawRow:aRow clipRect:[self frameOfCellAtColumn:i row:aRow]];
	}
}

- (void)drawRect:(NSRect)aRect
{
	if (![self numberOfRows])
		[self wgDrawPlaceholderStringInRect:aRect];
	else
		[super drawRect:aRect];
}

// The edit items are enabled as the delegate says. (The original checked
// for the undo validator before asking to validate redo.)
- (BOOL)validateMenuItem:(id)aItem
{
	SEL action = [aItem action];
	id delegate = [self delegate];
	if (action == @selector(delete:)) {
		if ([delegate respondsToSelector:@selector(wgDeleteItemsInTableView:)]
			&& [delegate respondsToSelector:@selector(wgValidateDeleteItemsInTableView:)])
			return [delegate wgValidateDeleteItemsInTableView:self];
	} else if (action == @selector(changeColor:)) {
		if ([delegate respondsToSelector:@selector(wgChangeItemColorInTableView:)]
			&& [delegate respondsToSelector:@selector(wgValidateChangeItemColorInTableView:)])
			return [delegate wgValidateChangeItemColorInTableView:self];
	} else if (action == @selector(undo:)) {
		if ([delegate respondsToSelector:@selector(wgPerformUndoInTableView:)]
			&& [delegate respondsToSelector:@selector(wgValidateUndo:inTableView:)])
			return [delegate wgValidateUndo:aItem inTableView:self];
	} else if (action == @selector(redo:)) {
		if ([delegate respondsToSelector:@selector(wgPerformRedoInTableView:)]
			&& [delegate respondsToSelector:@selector(wgValidateUndo:inTableView:)])
			return [delegate wgValidateRedo:aItem inTableView:self];
	} else {
		return [super validateMenuItem:aItem];
	}
	return NO;
}

// Deleting the last row selects the new last one.
- (void)delete:(id)aDelete
{
	NSInteger selected = [self selectedRow];
	NSInteger rows = [self numberOfRows];
	[(WGCurrentSetController *)[self delegate] wgDeleteItemsInTableView:self];
	if (selected == rows - 1)
		[self selectRowIndexes:[NSIndexSet indexSetWithIndex:[self numberOfRows] - 1] byExtendingSelection:NO];
}

- (void)changeColor:(id)aColor
{
	[(WGCurrentSetController *)[self delegate] wgChangeItemColorInTableView:self];
}

- (void)undo:(id)aUndo
{
	[(WGCurrentSetController *)[self delegate] wgPerformUndoInTableView:self];
}

- (void)redo:(id)aRedo
{
	[(WGCurrentSetController *)[self delegate] wgPerformRedoInTableView:self];
}

// Home and End scroll; Delete deletes; Return edits the delegate's column
// in the selected row.
- (void)keyDown:(id)aDown
{
	unsigned short key = [aDown keyCode];
	id delegate = [self delegate];
	if (key == 0x73) {
		[self scrollRowToVisible:0];
		return;
	}
	if (key == 0x77) {
		[self scrollRowToVisible:[self numberOfRows] - 1];
		return;
	}
	if ((key == 0x75 || key == 0x33) && [delegate respondsToSelector:@selector(wgDeleteItemsInTableView:)]
		&& [delegate respondsToSelector:@selector(wgValidateDeleteItemsInTableView:)]
		&& [delegate wgValidateDeleteItemsInTableView:self]) {
		[self delete:nil];
		return;
	}
	if ((key == 0x24 || key == 0x4c) && [self selectedRow] != -1) {
		if (![delegate respondsToSelector:@selector(wgColumnIDForEditingInTableView:)])
			return;
		id identifier = [delegate wgColumnIDForEditingInTableView:self];
		NSInteger row = [self selectedRow];
		NSInteger column = [self columnWithIdentifier:identifier];
		if ([delegate respondsToSelector:@selector(tableView:shouldEditTableColumn:row:)]
			&& ![delegate tableView:self shouldEditTableColumn:[self tableColumnWithIdentifier:identifier] row:row])
			return;
		[self editColumn:column row:[self selectedRow] withEvent:nil select:YES];
		return;
	}
	[super keyDown:aDown];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute])
		return [[NSBundle mainBundle] localizedStringForKey:@"Current route set" value:@"" table:nil];
	return [super accessibilityAttributeValue:aValue];
}

@end
