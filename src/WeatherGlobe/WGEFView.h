#import "WeatherGlobePrefix.h"


@interface WGEFView : NSView
{
    NSColor *_frameColor;
    NSColor *_fillColor;
    NSShadow *_shadow;
}
- (id)bottomBezierPath;
- (id)topBezierPath;
- (void)placeEFMatrix:(id)aMatrix;
- (void)setFillColor:(id)aColor;
- (id)fillColor;
- (void)setFrameColor:(id)aColor;
- (id)frameColor;
- (void)drawRect:(NSRect)aRect;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
@end

