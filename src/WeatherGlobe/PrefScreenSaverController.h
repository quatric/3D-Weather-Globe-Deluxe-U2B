#import "WeatherGlobePrefix.h"
#import "PrefGenericController.h"


@interface PrefScreenSaverController : PrefGenericController
{
    NSWindow *_tipsSheet;
    NSTextField *_tips;
    BOOL _tipsSet;
}
- (void)controlsAction:(id)aAction;
- (void)_closeTipsSheet;
- (void)sheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)_showTips;
- (id)toolbarImage;
- (id)toolbarLabel;
- (id)_nibName;
- (void)dealloc;
@end

