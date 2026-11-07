#import "WeatherGlobePrefix.h"
#import "WGPaletteBaseController.h"


@interface SnapshotEditorController : WGPaletteBaseController
{
    NSButton *_overrideWeatherButton;
    NSPopUpButton *_cloudsTypePopupButton;
    NSPopUpButton *_precipTypePopupButton;
    NSSlider *_windDirSlider;
    NSSlider *_windSpeedSlider;
    NSSlider *_precipInensitySlider;
    NSSlider *_mountainsHeightSlider;
}
- (void)controlAction:(id)aAction;
- (void)_saveFrame;
- (void)_loadFrame;
- (void)_updateControls;
- (void)_setupWeather;
- (WeatherTracer *)_weatherTracer;
- (AtlasPanelController *)_currentPanelController;
- (void)awakeFromNib;
- (id)autosaveName;
- (id)nibName;
@end

