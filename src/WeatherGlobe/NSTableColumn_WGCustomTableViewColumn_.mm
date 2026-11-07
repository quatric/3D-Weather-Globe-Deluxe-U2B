#import "WGCustomTableView.h"

@implementation NSTableColumn (WGCustomTableViewColumn)

- (NSInteger)comparisonFunc:(id)aFunc
{
	return [[[self headerCell] title] compare:[[aFunc headerCell] title]];
}

@end
