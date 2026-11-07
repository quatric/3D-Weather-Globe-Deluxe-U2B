#import "WeatherGlobePrefix.h"


@interface NSColor (WGExtensions)
+ (id)colorWithWGColor:(Color)aColor;
- (LArray *)wgColorComponents;
- (Color)wgColor;
@end

