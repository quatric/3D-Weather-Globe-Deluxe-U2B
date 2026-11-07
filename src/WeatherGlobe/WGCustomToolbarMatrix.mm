#import "WGCustomToolbarMatrix.h"
#import "WGCustomToolbarButtonCell.h"

@implementation WGCustomToolbarMatrix

// Remembers which part of a toolbar button cell is clicked while tracking.
- (void)mouseDown:(id)aDown
{
	NSPoint point = [self convertPoint:[aDown locationInWindow] fromView:nil];
	NSInteger row = 0, column = 0;
	if ([self getRow:&row column:&column forPoint:point]) {
		id cell = [self cellAtRow:row column:column];
		if ([cell isKindOfClass:[WGCustomToolbarButtonCell class]])
			_clickedCell = [cell realCellAtPoint:point inRect:[self cellFrameAtRow:row column:column] ofView:self];
	}
	[super mouseDown:aDown];
	_clickedCell = nil;
}

- (void)highlightCell:(BOOL)aCell atRow:(NSInteger)aRow column:(NSInteger)aColumn
{
	[[self cellAtRow:aRow column:aColumn] highlight:aCell withFrame:[self cellFrameAtRow:aRow column:aColumn] inView:self];
}

- (id)clickedCell
{
	return _clickedCell;
}

@end
