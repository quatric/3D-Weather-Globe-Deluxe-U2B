#import "WeatherGlobePrefix.h"


@interface WGCGImageCell : NSCell
{
    struct CGImage *_cgImage;
    BOOL _observerWasAdded;
}
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (id)copyWithZone:(NSZone *)aZone;
- (id)objectValue;
- (void)setObjectValue:(id)aValue;
- (void)dealloc;
- (void)_appWillTerminate;
@end

