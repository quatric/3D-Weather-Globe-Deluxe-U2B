#import "WeatherGlobePrefix.h"


@interface WGCustomSlider : NSSlider
{
}
+ (void)initialize;
- (void)mouseDown:(id)aDown;
- (void)setTickMarksColor:(id)aColor;
- (void)setDrawsTickMarks:(BOOL)aMarks;
- (void)setBarDisabledMiddleImage:(id)aImage;
- (void)setBarDisabledLastEdgeImage:(id)aImage;
- (void)setBarDisabledFirstEdgeImage:(id)aImage;
- (void)setBarMiddleImage:(id)aImage;
- (void)setBarLastEdgeImage:(id)aImage;
- (void)setBarFirstEdgeImage:(id)aImage;
- (void)setKnobAlternateImage:(id)aImage;
- (void)setKnobDisabledImage:(id)aImage;
- (void)setKnobImage:(id)aImage;
- (id)initWithCoder:(id)aCoder;
@end

@interface WGCustomSliderCell : NSSliderCell
{
    NSImage *_knobImage;
    NSImage *_knobDisabledImage;
    NSImage *_knobAlternateImage;
    NSImage *_barFirstEdgeImage;
    NSImage *_barLastEdgeImage;
    NSImage *_barMiddleImage;
    NSImage *_barDisabledFirstEdgeImage;
    NSImage *_barDisabledLastEdgeImage;
    NSImage *_barDisabledMiddleImage;
    NSColor *_tickMarksColor;
    BOOL _mouseIsTracking;
    BOOL _dontDrawsTickMarks;
}
- (void)setTickMarksColor:(id)aColor;
- (void)setDrawsTickMarks:(BOOL)aMarks;
- (void)setBarDisabledMiddleImage:(id)aImage;
- (void)setBarDisabledLastEdgeImage:(id)aImage;
- (void)setBarDisabledFirstEdgeImage:(id)aImage;
- (void)setBarMiddleImage:(id)aImage;
- (void)setBarLastEdgeImage:(id)aImage;
- (void)setBarFirstEdgeImage:(id)aImage;
- (void)setKnobAlternateImage:(id)aImage;
- (void)setKnobDisabledImage:(id)aImage;
- (void)setKnobImage:(id)aImage;
- (void)drawKnob:(NSRect)aKnob;
- (void)drawBarInside:(NSRect)aInside flipped:(BOOL)aFlipped;
- (NSRect)knobRectFlipped:(BOOL)aFlipped;
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView;
- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp;
- (BOOL)_drawsCustomBar;
- (BOOL)_drawsCustomKnob;
- (id)_currentBarMiddleImage;
- (id)_currentBarLastEdgeImage;
- (id)_currentBarFirstEdgeImage;
- (id)_currentKnobImage;
- (BOOL)_isSupported;
- (void)dealloc;
@end

