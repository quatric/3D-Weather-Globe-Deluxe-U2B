#import "WeatherGlobePrefix.h"
#import "PrefGenericController.h"


@interface PrefMyLocationController : PrefGenericController
{
    NSTextField *_location;
    NSButton *_showInDock;
    CitiesListController *_citiesListController;
}
- (void)selectCitySheetDidEnd;
- (void)controlsAction:(id)aAction;
- (void)viewWillBeUninstalled;
- (void)viewWillBeInstalled;
- (id)toolbarImage;
- (id)toolbarLabel;
- (void)_setLocationName:(LString *)aName;
- (id)_nibName;
- (void)dealloc;
@end

