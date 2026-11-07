#import "WeatherGlobePrefix.h"


@interface WGCGImageRep : NSImageRep
{
    struct CGImage *_image;
}
+ (id)imageRepWithCGImage:(struct CGImage *)aImage;
- (BOOL)draw;
- (void)dealloc;
- (id)initWithCGImage:(struct CGImage *)aImage;
@end

