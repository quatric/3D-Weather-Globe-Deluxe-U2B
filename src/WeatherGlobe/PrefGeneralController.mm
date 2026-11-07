#import "PrefGeneralController.h"

@implementation PrefGeneralController

- (id)_nibName
{
	return @"PrefGeneral";
}

- (id)toolbarLabel
{
	return [[NSBundle mainBundle] localizedStringForKey:@"General" value:@"" table:nil];
}

- (id)toolbarImage
{
	return [NSImage imageNamed:@"pref_general"];
}

@end
