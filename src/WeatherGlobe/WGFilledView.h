#import "WeatherGlobePrefix.h"


@interface WGFilledView : NSView
{
    NSColor *_fillColor;
}
- (void)setFillColor:(id)aColor;
- (id)fillColor;
- (void)drawRect:(NSRect)aRect;
- (void)dealloc;
@end

