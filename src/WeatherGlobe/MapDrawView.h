#import "WeatherGlobePrefix.h"


@interface MapDrawView : NSView
{
    BaseDrawing *_drawer;
}
- (void)drawRect:(NSRect)aRect;
- (void)setDrawer:(BaseDrawing *)aDrawer;
- (BaseDrawing *)drawer;
- (void)dealloc;
@end

