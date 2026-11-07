#import "PrefGenericController.h"
#import "WGPreferencesWrapper.h"

// A preferences pane loaded from _nibName; its object controller is bound
// to the shared preferences wrapper.
@implementation PrefGenericController

- (void)dealloc
{
	[_mainView release];
	[super dealloc];
}

- (id)_nibName
{
	return nil;
}

- (void)_loadNibIfNeeded
{
	if (!_mainView) {
		[NSBundle loadNibNamed:[self _nibName] owner:self];
		[_bindingController setContent:[self preferencesWrapper]];
	}
}

- (id)preferencesWrapper
{
	return [WGPreferencesWrapper sharedInstance];
}

- (id)mainView
{
	[self _loadNibIfNeeded];
	return _mainView;
}

- (void)viewWillBeInstalled
{
}

- (void)viewWillBeUninstalled
{
}

- (void)primaryAction
{
}

- (void)preferencesWillTerminate
{
	[(NSObjectController *)_bindingController setContent:nil];
	[_bindingController release];
	_bindingController = nil;
}

- (id)toolbarLabel
{
	return nil;
}

- (id)toolbarImage
{
	return nil;
}

@end
