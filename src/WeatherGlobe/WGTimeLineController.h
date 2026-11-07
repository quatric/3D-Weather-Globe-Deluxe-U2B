#import "WeatherGlobePrefix.h"
#import "WGCustomPaletteController.h"


@interface WGTimeLineController : WGCustomPaletteController
{
    SMKTintedView *_dateFieldsView;
    NSButton *_enableButton;
    WGTexturedButton *_currentButton;
    NSStepper *_dateStepper;
    NSButton *_forwardButton;
    NSButton *_backwardButton;
    NSButton *_playPauseButton;
    WGTimeLineControl *_timeLineControl;
    WGCustomSlider *_playingSpeedSlider;
    NSTextField *_monthField;
    NSTextField *_dayField;
    NSTextField *_yearField;
    NSTextField *_hoursField;
    NSTextField *_minutesField;
    NSTextField *_amPMField;
    NSTextField *_currentField;
    NSArray *_12timeFormatCaptions;
    NSArray *_24timeFormatCaptions;
    float _fieldsDeltaX;
    BOOL _isPlaying;
}
- (void)helpButton:(id)aButton;
- (void)controlsAction:(id)aAction;
- (id)autosaveName;
- (void)showPalette;
- (void)windowWillClose:(id)aClose;
- (void)timeDidEndEditing:(id)aEditing;
- (void)timeDidBeginEditing:(id)aEditing;
- (void)controlTextDidEndEditing:(id)aEditing;
- (void)controlTextDidChange:(id)aChange;
- (void)_textFieldDidStartEditing:(id)aEditing;
- (LDate *)_dateFromFields;
- (void)dealloc;
- (void)awakeFromNib;
- (id)nibName;
- (void)_setupCustomSlider:(id)aSlider;
- (void)_setupControls;
- (void)_setupCellForField:(id)aField;
- (void)_validateControls;
- (void)_validateTimeFormat;
- (void)_setupCustomTimeControls;
@end

