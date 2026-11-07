#import "WeatherGlobePrefix.h"


@interface NSMutableString (NSStringWGExtensions)
- (void)wgRemoveCharactersInSet:(id)aSet;
@end

@interface NSString (NSStringWGExtensions)
+ (id)wgCreateUniqueFilename:(id)aFilename extension:(id)aExtension;
+ (id)wgStringWithFormat:(id)aFormat arguments:(id)aArguments;
+ (id)wgShortGeoCoordinatesDescription:(LPoint)aDescription;
+ (id)wgShortSingleCoordDescription:(CGFloat)aDescription coordTypeLatitude:(BOOL)aLatitude;
- (id)wgStringByRemovingCharactersInSet:(id)aSet;
@end

