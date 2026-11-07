#import "PrefMyLocationController.h"
#import "CitiesListController.h"
#import "NSCell+WGExtensions.h"

// The "My Location" preferences pane: picks the home city.
@implementation PrefMyLocationController

- (void)dealloc
{
	[_citiesListController release];
	[super dealloc];
}

- (id)_nibName
{
	return @"PrefMyLocation";
}

- (void)_setLocationName:(LString *)aName
{
	[_location setStringValue:(NSString *)aName->cfString()];
	[[_location cell] wgFixLineBreakingTo:NSLineBreakByTruncatingTail];
}

- (id)toolbarLabel
{
	return [[NSBundle mainBundle] localizedStringForKey:@"My Location" value:@"" table:nil];
}

- (id)toolbarImage
{
	return [NSImage imageNamed:@"pref_home"];
}

- (void)viewWillBeInstalled
{
	[self _setLocationName:Preferences::homeCity()->shortCaption(0)];
}

- (void)viewWillBeUninstalled
{
}

- (void)selectCitySheetDidEnd
{
	[self _setLocationName:Preferences::homeCity()->shortCaption(0)];
}

// Tag 100 is "Set": the selected city becomes the home city.
- (void)controlsAction:(id)aAction
{
	if ([aAction tag] != 100)
		return;
	GenericEntry *entry = [_citiesListController selectedItem];
	if (entry && entry->getClass()->isSubclassOf(LF_CLASSOF(PointEntry))) {
		Preferences::setHomeCityUID(entry->uid(0));
		[self _setLocationName:entry->shortCaption(0)];
		return;
	}
	NSBeep();
}

@end
