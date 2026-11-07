#import "WGCommonEntryDetailsController.h"
#import "WGEntryDetailsWeatherPaneController.h"
#import "WGEntryDetailsInfoPaneController.h"
#import "WGEntryDetailsUserInfoImagesController.h"
#import "WGTitledView.h"

// The weather and info panes.
@implementation WGCommonEntryDetailsController

- (id)panesControllersClasses
{
	return [NSArray arrayWithObjects:[WGEntryDetailsInfoPaneController class], [WGEntryDetailsWeatherPaneController class], nil];
}

- (id)infoController
{
	return [self controllerForClass:[WGEntryDetailsInfoPaneController class]];
}

- (BOOL)isGotoLocationEnabled
{
	return YES;
}

// Mode 1 opens the weather paneController, mode 2 the info paneController on its first image.
- (void)setCurrentEntry:(GenericEntry *)aEntry openPane:(BOOL)aPane mode:(NSInteger)aMode
{
	[self setCurrentEntry:aEntry];
	if (!aPane)
		return;
	id paneController = nil;
	if (aMode == 1)
		paneController = [self controllerForClass:[WGEntryDetailsWeatherPaneController class]];
	else if (aMode == 2)
		paneController = [self controllerForClass:[WGEntryDetailsInfoPaneController class]];
	if ([paneController isEnabled]) {
		[self setCurrentPaneController:paneController];
		if (aMode == 2)
			[[[self infoController] imagesContainer] selectImageAtIndex:0];
	}
	[[self container] mountController:self];
}

// Editing happens in the info paneController.
- (void)setIsEditing:(BOOL)aEditing
{
	if (aEditing && ![self canEdit])
		[self setCurrentPaneController:[self controllerForClass:[WGEntryDetailsInfoPaneController class]]];
	[super setIsEditing:aEditing];
}

@end

NSInteger _compareOrderWeight(id first, id second, void *context)
{
	NSUInteger a = [(WGTitledView *)first orderWeight], b = [(WGTitledView *)second orderWeight];
	if (a < b)
		return NSOrderedAscending;
	return a > b ? NSOrderedDescending : NSOrderedSame;
}
