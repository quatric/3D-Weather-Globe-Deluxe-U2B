#import "WeatherGlobePrefix.h"


@interface CIPaletteTitleView : NSView
{
    BOOL _vertical;
    float _topOffset;
    NSArray *_viewComponents;
    NSSize _sizeForBestFit;
    NSImage *_horizontalTitleImage;
    NSImage *_verticalTitleImage;
    int _trackingRectTag;
    NSRect _trackingRect;
}
- (void)setVerticalTitleImage:(id)aImage;
- (void)setHorizontalTitleImage:(id)aImage;
- (id)verticalTitleImage;
- (id)horizontalTitleImage;
- (void)setViewComponents:(id)aComponents;
- (id)viewComponents;
- (void)setTopOffset:(CGFloat)aOffset;
- (BOOL)isVertical;
- (void)setVertical:(BOOL)aVertical;
- (NSSize)sizeForBestFit;
- (void)drawRect:(NSRect)aRect;
- (void)keyDown:(id)aDown;
- (void)mouseDragged:(id)aDragged;
- (void)mouseDown:(id)aDown;
- (void)mouseExited:(id)aExited;
- (void)mouseEntered:(id)aEntered;
- (BOOL)isFlipped;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
- (void)_frameWereChanged:(id)aChanged;
- (void)_stopLightsAllButtons;
- (void)_setLightsAllButtons;
- (void)_stopLightsButton:(id)aButton;
- (void)_setLightsButton:(id)aButton;
- (void)_reconstruct;
@end

