#import "WeatherGlobePrefix.h"
#import "WGCustomPaletteController.h"


@interface WGSettingsPaletteController : WGCustomPaletteController
{
    WGCustomSlider *_globeTransparency;
    WGCustomSlider *_spinSpeed;
    WGCustomSlider *_markerSize;
    NSButton *_highlightBoundaries;
    NSButton *_showNearestPoint;
    NSButton *_locationNames;
    NSButton *_showRouteLines;
    NSButton *_showPushpins;
    NSMatrix *_labelOptions;
}
- (void)controlsAction:(id)aAction;
- (void)dealloc;
- (void)awakeFromNib;
- (id)autosaveName;
- (id)nibName;
- (void)_validateControls;
- (void)_setupCustomSlider:(id)aSlider;
- (void)_setTextColor:(id)aColor forCell:(id)aCell;
@end

