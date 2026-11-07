#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface WGPaletteBaseController : WGNibOwnerController
{
    BOOL _paletteIsVisible;
    NSPanel *_palette;
}
- (BOOL)doShowOnStartup;
- (BOOL)isPaletteVisible;
- (void)togglePalette;
- (void)hidePalette;
- (void)showPalette;
- (void)windowWillClose:(id)aClose;
- (void)windowDidBecomeKey:(id)aKey;
- (void)_setShowOnStartup:(BOOL)aStartup;
- (id)_doShowOnStartupKey;
- (void)dealloc;
@end

