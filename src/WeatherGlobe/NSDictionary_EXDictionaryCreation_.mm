#import "NSMutableDictionary+EXExtensions.h"

static NSDictionary *_dictionaryAtPath(NSString *path)
{
	return path ? [NSDictionary dictionaryWithContentsOfFile:path] : nil;
}

@implementation NSDictionary (EXDictionaryCreation)

+ (id)extraDictionaryNamed:(id)aNamed
{
	return [self extraDictionaryNamed:aNamed inBundle:[NSBundle mainBundle]];
}

// The name's own extension first, then .list, .plist, .toolbar, .strings,
// then the bare name.
+ (id)extraDictionaryNamed:(id)aNamed inBundle:(id)aBundle
{
	NSString *extension = [aNamed pathExtension];
	NSDictionary *dictionary;
	if ([extension length]
	    && (dictionary = _dictionaryAtPath([aBundle pathForResource:[aNamed stringByDeletingPathExtension] ofType:extension])))
		return dictionary;
	for (NSString *type in @[ @"list", @"plist", @"toolbar", @"strings" ])
		if ((dictionary = _dictionaryAtPath([aBundle pathForResource:aNamed ofType:type])))
			return dictionary;
	return _dictionaryAtPath([aBundle pathForResource:aNamed ofType:nil]);
}

- (id)extraDictionaryByAppendingDictionary:(id)aDictionary
{
	NSMutableDictionary *result = [NSMutableDictionary dictionaryWithDictionary:self];
	[result extraAppendDictionary:aDictionary];
	return [NSDictionary dictionaryWithDictionary:result];
}

// Every key of the other dictionary is here with an equal value; an empty one
// isn't contained.
- (BOOL)extraContainsDictionary:(id)aDictionary
{
	if (![aDictionary count])
		return NO;
	for (id key in [aDictionary keyEnumerator]) {
		id value = [self objectForKey:key];
		if (!value || ![[aDictionary objectForKey:key] isEqual:value])
			return NO;
	}
	return YES;
}

// Absent keys give NSNull unless omitted.
- (id)extraObjectsForKeys:(id)aKeys omitAbsent:(BOOL)aAbsent
{
	NSMutableArray *objects = [NSMutableArray arrayWithCapacity:[aKeys count]];
	for (id key in aKeys) {
		id object = [self objectForKey:key];
		if (!object && !aAbsent)
			object = [NSNull null];
		if (object)
			[objects addObject:object];
	}
	return objects;
}

@end
