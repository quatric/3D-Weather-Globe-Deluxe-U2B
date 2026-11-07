#import "WeatherGlobePrefix.h"


@interface NSMutableDictionary (EXExtensions)
- (id)extraAppendDictionary:(id)aDictionary;
@end

@interface NSDictionary (EXDictionaryCreation)
+ (id)extraDictionaryNamed:(id)aNamed inBundle:(id)aBundle;
+ (id)extraDictionaryNamed:(id)aNamed;
- (id)extraObjectsForKeys:(id)aKeys omitAbsent:(BOOL)aAbsent;
- (BOOL)extraContainsDictionary:(id)aDictionary;
- (id)extraDictionaryByAppendingDictionary:(id)aDictionary;
@end

