#import "WeatherGlobePrefix.h"
#import "WGPaletteBaseController.h"


@interface WGCustomPaletteController : WGPaletteBaseController
{
    WGTexturedView *_contentView;
}
- (void)closeButtonAction:(id)aAction;
- (void)dealloc;
- (void)awakeFromNib;
- (void)_setupPalette;
- (void)_setupContentView;
@end

@interface _WGBorderlessPanel : NSPanel
{
}
- (BOOL)canBecomeKeyWindow;
- (void)performClose:(id)aClose;
- (BOOL)validateMenuItem:(id)aItem;
@end

