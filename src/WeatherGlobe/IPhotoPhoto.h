#import "WeatherGlobePrefix.h"


@interface IPhotoPhoto : NSObject
{
    NSDictionary *_info;
    BOOL _urlAccessTested;
    BOOL _urlAccessOk;
}
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType;
- (id)info;
- (BOOL)hasAccessibleURL;
- (id)displayName;
- (id)thumbURL;
- (id)URL;
- (void)dealloc;
- (id)initWithDictionaryRepresentation:(id)aRepresentation;
- (id)init;
@end

