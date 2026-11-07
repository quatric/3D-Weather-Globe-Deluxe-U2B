#import "NSOutlineView+NSOutlineViewWGExtensions.h"

@implementation NSOutlineView (NSOutlineViewWGExtensions)

- (id)selectedItem
{
	return [self itemAtRow:[self selectedRow]];
}

- (id)allSelectedItems
{
	NSMutableArray *items = [NSMutableArray array];
	NSIndexSet *rows = [self selectedRowIndexes];
	for (NSUInteger row = [rows firstIndex]; row != NSNotFound; row = [rows indexGreaterThanIndex:row]) {
		id item = [self itemAtRow:row];
		if (item)
			[items addObject:item];
	}
	return items;
}

@end
