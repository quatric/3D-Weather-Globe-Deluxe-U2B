#import "PrefCloudsController.h"

@implementation PrefCloudsController

- (id)_nibName
{
	return @"PrefClouds";
}

- (id)toolbarLabel
{
	return [[NSBundle mainBundle] localizedStringForKey:@"WeatherPrefTitle" value:@"" table:nil];
}

- (id)toolbarImage
{
	return [NSImage imageNamed:@"pref_clouds"];
}

// The extra high clouds detail (tag 3) is always offered: the live cloud map
// has the detail for it.
- (void)viewWillBeInstalled
{
	[super viewWillBeInstalled];
	if ([_cloudsDetailsButt indexOfItemWithTag:3] < 0) {
		[_cloudsDetailsButt insertItemWithTitle:[[NSBundle mainBundle] localizedStringForKey:@"Extra High" value:@"" table:nil]
		                                atIndex:0];
		[[_cloudsDetailsButt itemAtIndex:0] setTag:3];
	}
}

@end
