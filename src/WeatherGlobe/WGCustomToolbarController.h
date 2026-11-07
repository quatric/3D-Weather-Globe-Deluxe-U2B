#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface WGCustomToolbarController : WGNibOwnerController
{
    NSMatrix *_toolbar;
    NSButton *_expandButton;
    WGTexturedView *_toolbarView;
    NSView *_spaceSettingsView;
    NSMatrix *_spaceSettingsMatrix;
    NSButton *_planetsNames;
    NSButton *_constellationsNames;
    NSView *_cloudsSettingsView;
    NSView *_cloudsPianinoPlaceView;
    NSButton *_clouds5DHistoryButton;
    NSButton *_cloudsPlayPauseButton;
    WGCustomSlider *_cloudsAnimSpeedSlider;
    WGCustomSlider *_cloudsBackInTimeSlider;
    BOOL _isBITSliderTracking;
    BOOL _isInPauseState;
    WGCloudsPianino *_cloudsPianino;
    NSView *_countriesSettingsView;
    NSMatrix *_countriesSettingsMatrix;
    NSButton *_highlightAlwaysButt;
    NSView *_settingsView;
    WGCustomSlider *_globeTransparency;
    WGCustomSlider *_soundVolume;
    WGCustomSlider *_spinSpeed;
    WGCustomSlider *_markerSize;
    NSButton *_locationNames;
    NSView *_locationsSettingsView;
    NSMatrix *_locationsSettingsMatrix;
    NSButton *_nearestPoint;
    BOOL _isDynamicIntercellWidth;
    NSMutableArray *_hiddenCells;
    NSMenu *_expandMenu;
}
- (BOOL)isPopupWindowAutoHiddenForCell:(id)aCell;
- (id)popupWindowContentViewForCell:(id)aCell;
- (void)locationsAction:(id)aAction;
- (void)settingsAction:(id)aAction;
- (void)countriesAction:(id)aAction;
- (void)cloudsAction:(id)aAction;
- (void)spaceAction:(id)aAction;
- (void)expandButtonAction:(id)aAction;
- (void)toolbarAction:(id)aAction;
- (void)showSettings;
- (id)toolbarView;
- (id)nibName;
- (void)_toolbarViewFrameWasChanged;
- (CGFloat)_requiredWidthForColumnsCount:(NSInteger)aCount;
- (id)_expandMenu;
- (void)_expandMenuAction:(id)aAction;
- (BOOL)validateMenuItem:(id)aItem;
- (id)pianino:(id)aPianino dateForKeyAtIndex:(NSInteger)aIndex;
- (BOOL)pianino:(id)aPianino isEnabledKeyAtIndex:(NSInteger)aIndex;
- (NSInteger)numberOfKeysInPianino:(id)aPianino;
- (void)sliderDidEndTracking:(id)aTracking;
- (void)sliderWillStartTracking:(id)aTracking;
- (void)dealloc;
- (void)awakeFromNib;
- (void)_setCellsTextColor:(id)aColor forMatrix:(id)aMatrix;
- (void)_setTextColor:(id)aColor forCell:(id)aCell;
- (void)_setupBackInTimeSlider;
- (void)_setupCustomSlider:(id)aSlider;
- (void)_validateToolbar;
- (void)_validateLocationsControlls;
- (void)_validateSettingsControlls;
- (void)_validateCountriesControlls;
- (void)_validateCloudsControlls;
- (void)_updateCloudsPianino;
- (void)_validateBITSliderValue;
- (void)_validateSpaceControlls;
- (void)_validateToolbarCell:(id)aCell;
- (void)_actionWithTag:(NSInteger)aTag;
- (id)_toolbarCelWithTag:(NSInteger)aTag;
- (CloudsController *)_cloudsController;
@end

