#import "WeatherGlobePrefix.h"


@interface WGBalloonPaletteController : NSObject
{
    BOOL _isAutoHidden;
    NSPanel *_balloonPalette;
    WGBalloonTexturedVew *_balloonBackground;
}
- (void)windowDidMove:(id)aMove;
- (void)windowDidResignKey:(id)aKey;
- (void)hideBalloonPalette;
- (void)showBalloonPalette;
- (void)setIsAutoHidden:(BOOL)aHidden;
- (void)setPalettePosition:(NSPoint)aPosition contentView:(id)aView;
- (void)dealloc;
@end

@interface _WGBalloonPanel : NSPanel
{
    BOOL _isClosable;
}
- (void)keyDown:(id)aDown;
- (BOOL)canBecomeKeyWindow;
- (void)performClose:(id)aClose;
- (BOOL)validateMenuItem:(id)aItem;
- (void)setIsClosable:(BOOL)aClosable;
@end

@interface WGBalloonPaletteController (Private)
- (void)_setupPalette;
- (void)_setupTexturedView:(id)aView;
@end

