#import "NSMutableArray+EXMutableArray.h"

// Sorts by the index in the reference array; objects missing from it go
// after those in it, in their own array's order.
static NSInteger _EXCompareObjectsForReorderingArray(id first, id second, void *context)
{
	NSArray **arrays = (NSArray **)[(NSMutableData *)context mutableBytes];
	NSArray *own = arrays[0], *reference = arrays[1];
	NSUInteger a, b;
	if (reference) {
		a = [reference indexOfObject:first];
		b = [reference indexOfObject:second];
		if (a != NSNotFound) {
			if (b == NSNotFound)
				return NSOrderedAscending;
			return b <= a ? NSOrderedDescending : NSOrderedAscending;
		}
		if (b != NSNotFound)
			return NSOrderedDescending;
	}
	a = [own indexOfObject:first];
	b = [own indexOfObject:second];
	return b <= a ? NSOrderedDescending : NSOrderedAscending;
}

@implementation NSArray (EXArray)

- (BOOL)extraContainsObjectIdenticalTo:(id)aTo
{
	return [self indexOfObjectIdenticalTo:aTo] != NSNotFound;
}

- (id)extraFirstObject
{
	return [self extraObjectAtIndex:0];
}

- (id)extraObjectAtIndex:(NSUInteger)aIndex
{
	return aIndex < [self count] ? [self objectAtIndex:aIndex] : nil;
}

- (id)extraSortedArray
{
	return [self sortedArrayUsingSelector:@selector(compare:)];
}

// The same objects (identical), in any order.
- (BOOL)isExtraReorderedArray:(id)aArray
{
	if ([self count] != [aArray count])
		return NO;
	for (NSUInteger i = 0; i < [aArray count]; i++)
		if (![self extraContainsObjectIdenticalTo:[aArray objectAtIndex:i]])
			return NO;
	return YES;
}

- (id)extraReorderedArrayAgainstArray:(id)aArray
{
	if (self == aArray || ![aArray count])
		return self;
	NSArray *arrays[2] = {self, aArray};
	NSMutableData *context = [NSMutableData dataWithBytes:arrays length:sizeof(arrays)];
	return [self sortedArrayUsingFunction:_EXCompareObjectsForReorderingArray context:context];
}

- (BOOL)isExtraIdenticalToArray:(id)aArray
{
	if ([self count] != [aArray count])
		return NO;
	for (NSUInteger i = 0; i < [aArray count]; i++)
		if ([self objectAtIndex:i] != [aArray objectAtIndex:i])
			return NO;
	return YES;
}

@end
