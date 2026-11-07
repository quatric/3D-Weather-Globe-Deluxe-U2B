#import "WGBezierPathMatrix.h"

// Whether the matrix's cells are bezier path cells.
static BOOL _hasBezierCells(NSMatrix *matrix)
{
	return [[matrix cellClass] conformsToProtocol:@protocol(WGBezierPathCellProtocol)] ||
	       [[matrix prototype] conformsToProtocol:@protocol(WGBezierPathCellProtocol)];
}

@implementation WGBezierPathMatrix

// Bezier path cells may overlap: the selected one is drawn last.
- (void)drawRect:(NSRect)aRect
{
	if (!_hasBezierCells(self)) {
		[super drawRect:aRect];
		return;
	}
	NSInteger selectedRow = [self selectedRow];
	NSInteger selectedColumn = [self selectedColumn];
	NSInteger rows = [self numberOfRows];
	NSInteger columns = [self numberOfColumns];
	for (NSInteger row = 0; row < rows; row++) {
		for (NSInteger column = columns - 1; column >= 0; column--) {
			if (row == selectedRow && column == selectedColumn)
				continue;
			if (NSIntersectsRect(aRect, [self cellFrameAtRow:row column:column]))
				[[self cellAtRow:row column:column] drawWithFrame:[self cellFrameAtRow:row column:column] inView:self];
		}
	}
	if (NSIntersectsRect(aRect, [self cellFrameAtRow:selectedRow column:selectedColumn]))
		[[self selectedCell] drawWithFrame:[self cellFrameAtRow:selectedRow column:selectedColumn] inView:self];
}

// Clicks select by the cells' paths; dragging out of all cells or back to
// the first one returns to it, and the action is sent on releasing over a
// new cell.
- (void)mouseDown:(id)aDown
{
	if (!_hasBezierCells(self)) {
		[super mouseDown:aDown];
		return;
	}
	NSEvent *event = aDown;
	NSInteger selectedColumn = [self selectedColumn];
	NSRect frame = [self cellFrameAtRow:[self selectedRow] column:selectedColumn];
	if ([[self selectedCell] containsPoint:[event locationInWindow] inRect:frame ofView:self])
		return;
	int originalRow = (int)[self selectedRow];
	int originalColumn = (int)[self selectedColumn];
	struct _WGMatrixPosition current = [self cellPositionByPoint:[event locationInWindow]];
	if (current.row < 0 || current.column < 0)
		return;
	[self selectCellAtRow:current.row column:current.column];
	BOOL inside = YES;
	for (;;) {
		event = [[self window] nextEventMatchingMask:NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged];
		if ([event type] == NSEventTypeLeftMouseDragged) {
			struct _WGMatrixPosition position = [self cellPositionByPoint:[event locationInWindow]];
			if ((position.row != originalRow || position.column != originalColumn) &&
			    (position.row != -1 || position.column != -1)) {
				[self selectCellAtRow:position.row column:position.column];
				current = position;
				inside = YES;
			} else {
				inside = NO;
				[self selectCellAtRow:originalRow column:originalColumn];
			}
		} else if ([event type] == NSEventTypeLeftMouseUp) {
			if (inside) {
				[self selectCellAtRow:current.row column:current.column];
				[self sendAction];
			}
			return;
		}
	}
}

// The cell whose path contains the point, bottom rows first.
- (struct _WGMatrixPosition)cellPositionByPoint:(NSPoint)aPoint
{
	NSInteger rows = [self numberOfRows];
	NSInteger columns = [self numberOfColumns];
	for (NSInteger row = rows - 1; row >= 0; row--) {
		for (NSInteger column = 0; column < columns; column++) {
			id cell = [self cellAtRow:row column:column];
			if ([cell containsPoint:aPoint inRect:[self cellFrameAtRow:row column:column] ofView:self]) {
				struct _WGMatrixPosition position = {(int)row, (int)column};
				return position;
			}
		}
	}
	struct _WGMatrixPosition none = {-1, -1};
	return none;
}

@end
