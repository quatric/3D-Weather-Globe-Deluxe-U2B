#import "WeatherGlobePrefix.h"
#import "WGCustomPaletteController.h"


@interface WGMovieController : WGCustomPaletteController
{
    NSTextField *_windowCaption;
    WGTexturedView *_boxTexturedView;
    NSTextField *_helperTextField;
    NSTextField *_fileNameField;
    NSTextField *_timeRecordingField;
    WGCustomSlider *_movieSlider;
    NSProgressIndicator *_exportingProgress;
    NSTextField *_currentPlayingTime;
    NSTextField *_movieLength;
    NSButton *_helpButton;
    WGTexturedButton *_cancelButton;
    WGTexturedButton *_exportButton;
    NSMatrix *_playControls;
    NSView *_exportMovieAccessory;
    NSPopUpButton *_exportMovieOutSizePopUp;
    NSButton *_openExportedMovieInQuickTime;
    NSTextField *_exportMovieWidthField;
    NSTextField *_exportMovieHeightField;
    NSButton *_advancedOptionsButton;
    NSMatrix *_targetMatrix;
    NSView *_containerView;
    NSView *_iPodSettingsView;
    NSView *_iPhoneSettingsView;
    NSView *_iMovieSettingsView;
    NSView *_customSettingsView;
    NSMatrix *_iPodMatrix;
    NSMatrix *_iPhoneMatrix;
    NSMatrix *_iMovieMatrix;
    NSView *_currentSettingsView;
    NSTimer *_moviePlayingTimer;
    NSTimer *_movieRecordingTimer;
    NSString *_moviesFolder;
    NSObjToLObjWrapper *_movieStopper;
    BOOL _isExportCanceled;
    BOOL _isSliderTracking;
    void *_usedPanelController;
    _NSModalSession *_exportSession;
    double _prevUpdateTime;
}
- (void)_onRecordTimer:(id)aTimer;
- (void)_onTimer:(id)aTimer;
- (void)_setMoviesFolder:(id)aFolder;
- (void)_validateCustomSize;
- (void)controlTextDidChange:(id)aChange;
- (CSceneCapturer *)_capturerFromMainView;
- (LSize)_movieSizeFromMenu;
- (id)_exportPathForFilename:(id)aFilename;
- (id)_iMovieExporterFromExportParams;
- (void)_setExportProgress:(CGFloat)aProgress;
- (void)_stop;
- (void)_stopPlaying:(AtlasPanelController *)aPlaying;
- (void)_pause:(AtlasPanelController *)aPause;
- (void)_play:(AtlasPanelController *)aPlay;
- (void)_stopRecording:(AtlasPanelController *)aRecording;
- (void)_record:(AtlasPanelController *)aRecord;
- (AtlasPanelController *)_currentPanelController;
- (void)exportMovieSizePopupChanged:(id)aChanged;
- (void)cancelExport:(id)aExport;
- (void)exportMovie:(id)aMovie;
- (void)iMovieMatrixAction:(id)aAction;
- (void)selectTarget:(id)aTarget;
- (void)_exportMovie:(id)aMovie;
- (void)_setFloatingPanel;
- (void)_setupMovieExporter:(MovieExporter *)aExporter;
- (void)_validateSettingsView;
- (id)_requiredFileType;
- (BOOL)wgShouldDoExportInWGExportPanel:(id)aPanel;
- (void)helpButton:(id)aButton;
- (void)showExportOptions:(id)aOptions;
- (void)sliderDidEndTracking:(id)aTracking;
- (void)movieSliderAction:(id)aAction;
- (void)sliderWillStartTracking:(id)aTracking;
- (void)controlsAction:(id)aAction;
- (id)defaultMoviesFolder;
- (void)hidePalette;
- (void)dealloc;
- (void)cleanUp:(id)aUp;
- (void)awakeFromNib;
- (id)autosaveName;
- (id)nibName;
- (void)_setupExportTargetCells;
@end

@interface NSString (TimeFromSecs)
+ (id)timeFromSecs:(CGFloat)aSecs;
@end

