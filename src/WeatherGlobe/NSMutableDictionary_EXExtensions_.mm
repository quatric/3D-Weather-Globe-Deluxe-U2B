#import "NSMutableDictionary+EXExtensions.h"

@implementation NSMutableDictionary (EXExtensions)

// Adds the dictionary's entries, merging dictionaries and concatenating
// arrays.
- (id)extraAppendDictionary:(id)aDictionary
{
	NSEnumerator *keys = [aDictionary keyEnumerator];
	id key;
	while ((key = [keys nextObject])) {
		id value = [aDictionary objectForKey:key];
		id existing = [self objectForKey:key];
		if ([value isKindOfClass:[NSDictionary class]]) {
			NSMutableDictionary *merged = existing ? [NSMutableDictionary dictionaryWithDictionary:existing]
			                                       : [NSMutableDictionary dictionary];
			value = [merged extraAppendDictionary:value];
		} else if ([value isKindOfClass:[NSArray class]]) {
			value = existing ? [existing arrayByAddingObjectsFromArray:value] : [NSArray arrayWithArray:value];
		}
		[self setObject:value forKey:key];
	}
	return self;
}

@end
