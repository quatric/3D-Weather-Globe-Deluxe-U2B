#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface MainPrintDataSourceProvider : WGNibOwnerController <PrintDataSourceProviderProtocol>
{
    WeatherPaletteExtPrintDataSource *_locationWeatherPrintDS;
    NSPanel *_printDialog;
    NSPanel *_settingsPalette;
    NSTextField *_printTitle;
    NSButton *_printButton;
    NSButton *_printMainViewBtn;
    NSButton *_printCurrentRouteBtn;
    NSButton *_printLocationsBtn;
    NSButton *_printLocationsWeatherBtn;
    NSImageView *_printCurrentRouteImg;
    NSImageView *_printLocationsImg;
    NSImageView *_printCurrentRouteDisabledImg;
    NSImageView *_printLocationsDisabledImg;
    NSButton *_optionMapTerrain;
    NSButton *_optionSpaceAndSky;
    NSButton *_optionCountryNames;
    BOOL _doNotShowCustomDialog;
    NSString *_previousTitleFieldContent;
    SEL _prepearingEndSel;
    id _prepearingEndTarget;
}
- (void)locationWeatherDataSourceDidEndWeatherPrepearing;
- (void)printCustomOptionsCheckbox:(id)aCheckbox;
- (void)printOptionsCheckbox:(id)aCheckbox;
- (void)controlTextDidChange:(id)aChange;
- (void)_applyTitleTextUndo:(id)aUndo;
- (BOOL)_isTitleValid;
- (void)settingsHelpButton:(id)aButton;
- (void)helpButton:(id)aButton;
- (void)printButton:(id)aButton;
- (void)cancelButton:(id)aButton;
- (void)customizeButton:(id)aButton;
- (void)okSettingsButton:(id)aButton;
- (void)cancelSettingsButton:(id)aButton;
- (void)_closeSettingsPalette;
- (void)_openSettingsPalette;
- (void)endPrintDialogWithCode:(NSInteger)aCode;
- (void)setPrintTitle:(id)aTitle;
- (void)setDoShowCustomDialog:(BOOL)aDialog;
- (void)setLocationInfoBtnState:(NSInteger)aState;
- (void)setCurrentRouteBtnState:(NSInteger)aState;
- (void)setMainViewBtnState:(NSInteger)aState;
- (void)preparePrintingDidEndSelector:(SEL)aSelector target:(id)aTarget;
- (id)printJobTitle;
- (id)customPrintDialog;
- (id)hostWindowForPrintDialog;
- (id)printDataSources;
- (void)windowWillClose:(id)aClose;
- (id)nibName;
- (void)dealloc;
- (void)_validatePrintButton;
- (BOOL)_isNeedToPrint;
- (BOOL)_doPrintRoute;
- (BOOL)_doPrintBorders;
@end

