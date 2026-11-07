#import "PrefApearanceController.h"

@implementation PrefApearanceController

- (id)_nibName
{
	return @"PrefApearance";
}

- (id)toolbarLabel
{
	return [[NSBundle mainBundle] localizedStringForKey:@"ApearancePrefTitle" value:@"" table:nil];
}

- (id)toolbarImage
{
	return [NSImage imageNamed:@"pref_appearance"];
}

@end
