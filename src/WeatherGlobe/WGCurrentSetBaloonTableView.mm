#import "WGCurrentSetBaloonTableView.h"
#import "WGCurrentSetController.h"

@implementation WGCurrentSetBaloonTableView

// Rows are drawn on a rounded balloon background, the selected one marked
// with a dot; route breaks are separators.
- (void)drawRow:(NSInteger)aRow clipRect:(NSRect)aRect
{
	if ([(id)[self delegate] wgTableView:self shouldDisplayBreakAtRow:aRow]) {
		[self drawSeparatorInRect:NSIntersectionRect(aRect, [self rectOfRow:aRow])];
		return;
	}
	NSImage *left = [NSImage imageNamed:@"tv_balloon_left"];
	NSImage *center = [NSImage imageNamed:@"tv_balloon_center"];
	NSImage *right = [NSImage imageNamed:@"tv_balloon_right"];
	NSSize leftSize = [left size];
	NSRect row = [self rectOfRow:aRow];
	float y = row.origin.y + (row.size.height - leftSize.height) * 0.5f;
	float x = row.origin.x;
	[left drawInRect:NSMakeRect(x, y, leftSize.width, leftSize.height) fromRect:NSMakeRect(0, 0, leftSize.width, leftSize.height)
	       operation:NSCompositingOperationSourceOver fraction:1.0];
	x += leftSize.width;
	float width = row.size.width - leftSize.width - [left size].width;
	NSSize centerSize = [center size];
	[center drawInRect:NSMakeRect(x, y, width, leftSize.height) fromRect:NSMakeRect(0, 0, centerSize.width, centerSize.height)
	         operation:NSCompositingOperationSourceOver fraction:1.0];
	// the right part has the left one's width
	NSSize rightSize = [left size];
	[right drawInRect:NSMakeRect(x + width, y, rightSize.width, leftSize.height)
	         fromRect:NSMakeRect(0, 0, rightSize.width, centerSize.height)
	        operation:NSCompositingOperationSourceOver fraction:1.0];
	[super drawRow:aRow clipRect:aRect];
	if ([[self selectedRowIndexes] containsIndex:aRow]) {
		NSRect cell = NSInsetRect([self frameOfCellAtColumn:0 row:aRow], 2.0, 0.0);
		NSRect dot = NSMakeRect(cell.origin.x, cell.origin.y + (cell.size.height - cell.size.width) * 0.5f,
		                        cell.size.width, cell.size.width);
		[[NSColor alternateSelectedControlColor] set];
		[[NSBezierPath bezierPathWithOvalInRect:dot] fill];
	}
}

- (id)_highlightColorForCell:(id)aCell
{
	return nil;
}

@end
