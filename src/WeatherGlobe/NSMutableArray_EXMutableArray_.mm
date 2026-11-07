#import "NSMutableArray+EXMutableArray.h"

@implementation NSMutableArray (EXMutableArray)

- (void)extraInsertObjectsFromArray:(id)aArray atIndex:(NSInteger)aIndex
{
	NSEnumerator *objects = [aArray objectEnumerator];
	id object;
	while ((object = [objects nextObject]))
		[self insertObject:object atIndex:aIndex++];
}

- (void)extraRemoveObjectAtIndex:(NSUInteger)aIndex
{
	if (aIndex < [self count])
		[self removeObjectAtIndex:aIndex];
}

@end
