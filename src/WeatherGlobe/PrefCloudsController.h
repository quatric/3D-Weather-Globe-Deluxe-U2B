#import "WeatherGlobePrefix.h"
#import "PrefGenericController.h"


@interface PrefCloudsController : PrefGenericController
{
    NSPopUpButton *_cloudsDetailsButt;
}
- (void)viewWillBeInstalled;
- (id)toolbarImage;
- (id)toolbarLabel;
- (id)_nibName;
@end

