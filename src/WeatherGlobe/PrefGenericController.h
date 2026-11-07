#import "WeatherGlobePrefix.h"


@interface PrefGenericController : NSObject
{
    NSView *_mainView;
    id _bindingController;
}
- (id)toolbarImage;
- (id)toolbarLabel;
- (void)preferencesWillTerminate;
- (void)primaryAction;
- (void)viewWillBeUninstalled;
- (void)viewWillBeInstalled;
- (id)mainView;
- (id)preferencesWrapper;
- (void)_loadNibIfNeeded;
- (id)_nibName;
- (void)dealloc;
@end

