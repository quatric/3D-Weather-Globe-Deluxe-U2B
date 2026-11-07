#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface SFOneLocationController : WGNibOwnerController
{
    NSView *_dataView;
    NSTextField *_locationName;
    NSView *_sfPlaceholder;
    PointEntry *_location;
}
- (void)setLocation:(PointEntry *)aLocation;
- (id)dataView;
- (void)_layoutSFViewsFromControllers:(id)aControllers;
- (id)nibName;
- (void)dealloc;
@end

