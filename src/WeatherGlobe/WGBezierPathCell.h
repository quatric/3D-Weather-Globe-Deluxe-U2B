#import "WeatherGlobePrefix.h"


@interface WGBezierPathCell : NSCell <WGBezierPathCellProtocol, NSObject>
{
    NSBezierPath *_bezierPath;
    NSColor *_frameColor;
    NSColor *_fillColor;
    NSColor *_highlightColor;
    NSRect _defaultBounds;
}
- (id)flippedTransformWithBounds:(NSRect)aBounds;
- (BOOL)allowsMixedState;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (BOOL)containsPoint:(NSPoint)aPoint inRect:(NSRect)aRect ofView:(id)aView;
- (id)transformForFrame:(NSRect)aFrame view:(id)aView;
- (NSRect)defaultBounds;
- (void)setDefaultCellBounds:(NSRect)aBounds;
- (void)setHighlightColor:(id)aColor;
- (id)highlightColor;
- (void)setFillColor:(id)aColor;
- (id)fillColor;
- (void)setFrameColor:(id)aColor;
- (id)frameColor;
- (void)setBezierPath:(id)aPath;
- (void)dealloc;
- (id)init;
@end

