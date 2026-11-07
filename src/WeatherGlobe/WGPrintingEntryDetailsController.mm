#import "WGPrintingEntryDetailsController.h"
#import "WGPrintingEntryDetailsInfoPaneController.h"

@implementation WGPrintingEntryDetailsController

- (id)panesControllersClasses
{
	return [NSArray arrayWithObjects:[WGPrintingEntryDetailsInfoPaneController class], nil];
}

- (id)paneContainer
{
	return paneContainer;
}

@end
