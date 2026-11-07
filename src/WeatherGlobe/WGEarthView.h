#import "WeatherGlobePrefix.h"


@interface WGEarthView : NSView
{
    RoutePreviewController *_routePreviewController;
    NSButton *_zoomIn;
    NSButton *_zoomOut;
}
- (void)_needsDisplay:(id)aDisplay;
- (void)scrollWheel:(id)aWheel;
- (void)mouseDragged:(id)aDragged;
- (void)mouseDown:(id)aDown;
- (void)scaleToFit:(id)aFit;
- (void)zoomOut:(id)aOut;
- (void)zoomIn:(id)aIn;
- (BOOL)canZoomOut;
- (BOOL)canZoomIn;
- (void)setSelectedRoute:(GenericRouteNode *)aRoute;
- (void)drawRect:(NSRect)aRect;
- (void)_drawRoute;
- (void)_drawImages;
- (CGFloat)_fixFloat:(CGFloat)aFloat;
- (void)awakeFromNib;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
- (id)initWithCoder:(id)aCoder;
@end

