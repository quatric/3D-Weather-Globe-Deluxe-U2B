#import "WeatherGlobePrefix.h"
#import "WGTexturedView.h"


@interface WGCustomTitleView : WGTexturedView
{
    NSTextFieldCell *_titleCell;
    NSColor *_frontColor;
    NSColor *_shadowColor;
    NSSize _shadowOffset;
    BOOL _drawsTitle;
}
- (void)setTitleFrontColor:(id)aColor;
- (id)titleFrontColor;
- (void)setTitleShadowColor:(id)aColor;
- (id)titleShadowColor;
- (void)setTitleShadowOffset:(NSSize)aOffset;
- (NSSize)titleShadowOffset;
- (void)setDrawsTitle:(BOOL)aTitle;
- (BOOL)drawsTitle;
- (void)mouseDown:(id)aDown;
- (void)drawRect:(NSRect)aRect;
- (id)_titleCell;
- (id)_titleFont;
- (CGFloat)_titleFontSize;
- (void)dealloc;
@end

@interface NSWindow (WGCustomTitleExtension)
- (id)customTitleView;
@end

