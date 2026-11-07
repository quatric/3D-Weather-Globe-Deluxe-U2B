#import "PrefOnlineController.h"

@implementation PrefOnlineController

- (id)_nibName
{
	return @"PrefOnline";
}

- (id)toolbarLabel
{
	return [[NSBundle mainBundle] localizedStringForKey:@"Online" value:@"" table:nil];
}

- (id)toolbarImage
{
	return [NSImage imageNamed:@"pref_online"];
}

@end
