#import "WeatherGlobePrefix.h"


@interface NSMutableArray (EXMutableArray)
- (void)extraRemoveObjectAtIndex:(NSUInteger)aIndex;
- (void)extraInsertObjectsFromArray:(id)aArray atIndex:(NSInteger)aIndex;
@end

@interface NSArray (EXArray)
- (BOOL)isExtraIdenticalToArray:(id)aArray;
- (id)extraReorderedArrayAgainstArray:(id)aArray;
- (BOOL)isExtraReorderedArray:(id)aArray;
- (id)extraSortedArray;
- (id)extraObjectAtIndex:(NSUInteger)aIndex;
- (id)extraFirstObject;
- (BOOL)extraContainsObjectIdenticalTo:(id)aTo;
@end

