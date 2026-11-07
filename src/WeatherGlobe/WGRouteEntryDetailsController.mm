#import "WGRouteEntryDetailsController.h"
#import "WGEntryDetailsInfoPaneController.h"

@implementation WGRouteEntryDetailsController

- (id)panesControllersClasses
{
	return [NSArray arrayWithObjects:[WGEntryDetailsInfoPaneController class], nil];
}

- (id)paneContainer
{
	return paneContainer;
}

@end
