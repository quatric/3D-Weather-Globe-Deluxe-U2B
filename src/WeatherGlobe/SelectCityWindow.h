#import "WeatherGlobePrefix.h"
#import "CitiesListController.h"


@interface SelectCityWindow : NSWindow
{
}
- (BOOL)canBecomeKeyWindow;
- (id)initWithContentRect:(NSRect)aRect styleMask:(NSUInteger)aMask backing:(NSInteger)aBacking defer:(BOOL)aDefer;
@end

@interface SelectCityDialogController : CitiesListController
{
    NSWindow *_window;
    NSButton *_selectButton;
    int _dialogMode;
    id _modalDelegate;
}
+ (void)beginSheetModalForWindow:(id)aWindow modalDelegate:(id)aDelegate;
+ (void)runModalDialog;
+ (PointEntry *)_homeCity;
+ (void)_appWillTerminate;
+ (id)_sharedController;
- (void)tableViewSelectionDidChange:(id)aChange;
- (void)selectAction:(id)aAction;
- (void)sheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)_beginSheetWithDefaultCity:(PointEntry *)aCity modalForWindow:(id)aWindow modalDelegate:(id)aDelegate;
- (void)_runModalDialogWithDefaultCity:(PointEntry *)aCity;
- (void)_prepareWindowForCity:(PointEntry *)aCity;
- (LString *)_defaultCityName;
- (void)_doubleClickAction:(id)aAction;
- (id)nibName;
- (void)dealloc;
- (void)awakeFromNib;
@end

