#import "WeatherGlobePrefix.h"


@interface NSImage (NSImageWGExtensions)
+ (id)wgImageWithCGImage:(struct CGImage *)aImage;
+ (id)wgImageWithLImage:(LImage *)aImage;
@end

