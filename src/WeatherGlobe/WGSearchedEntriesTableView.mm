#import "WGSearchedEntriesTableView.h"

static NSArray *_columnRow(NSInteger column, NSInteger row)
{
	return [NSArray arrayWithObjects:[NSNumber numberWithInt:(int)column], [NSNumber numberWithInt:(int)row], nil];
}

@implementation WGSearchedEntriesTableView

// A click that didn't move is reported to the delegate as
// tableView:didClickColumnRowIndices: [column, row].
- (void)mouseDown:(id)aDown
{
	NSPoint downPoint = [aDown locationInWindow];
	[super mouseDown:aDown];
	NSEvent *current = [[self window] currentEvent];
	if ([current type] != NSEventTypeLeftMouseUp)
		return;
	NSPoint upPoint = [current locationInWindow];
	if (!NSEqualPoints(downPoint, upPoint))
		return;
	id delegate = [self delegate];
	if (![delegate respondsToSelector:@selector(tableView:didClickColumnRowIndices:)])
		return;
	NSPoint point = [self convertPoint:downPoint fromView:nil];
	[delegate performSelector:@selector(tableView:didClickColumnRowIndices:) withObject:self
	               withObject:_columnRow([self columnAtPoint:point], [self rowAtPoint:point])];
}

// The data source gives a cursor per visible cell
// (tableView:requestedCursorForColumnRowIndices:).
- (void)resetCursorRects
{
	NSRect visible = [self visibleRect];
	NSRange columns = [self columnsInRect:visible];
	NSRange rows = [self rowsInRect:visible];
	id dataSource = [self dataSource];
	for (NSUInteger column = columns.location; column < NSMaxRange(columns); column++) {
		for (NSUInteger row = rows.location; row < NSMaxRange(rows); row++) {
			if (![dataSource respondsToSelector:@selector(tableView:requestedCursorForColumnRowIndices:)])
				continue;
			NSCursor *cursor = [dataSource performSelector:@selector(tableView:requestedCursorForColumnRowIndices:)
			                                    withObject:self
			                                    withObject:_columnRow(column, row)];
			if (cursor)
				[self addCursorRect:[self frameOfCellAtColumn:column row:row] cursor:cursor];
		}
	}
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute])
		return [[NSBundle mainBundle] localizedStringForKey:@"Smart list" value:@"" table:nil];
	return [super accessibilityAttributeValue:aValue];
}

// The action goes to the delegate as
// tableView:accessibilityActionOnColumnRowIndices: for the selected row's
// right edge.
- (void)accessibilityCellAction:(id)aAction
{
	NSRect rect = [self rectOfRow:[self selectedRow]];
	NSPoint point = NSMakePoint(rect.origin.x + rect.size.width, rect.origin.y + rect.size.height * 0.5);
	id delegate = [self delegate];
	if ([delegate respondsToSelector:@selector(tableView:accessibilityActionOnColumnRowIndices:)])
		[delegate performSelector:@selector(tableView:accessibilityActionOnColumnRowIndices:) withObject:self
		               withObject:_columnRow([self columnAtPoint:point], [self rowAtPoint:point])];
}

@end
