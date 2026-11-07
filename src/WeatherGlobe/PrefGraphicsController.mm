#import "PrefGraphicsController.h"

@implementation PrefGraphicsController

- (id)_nibName
{
	return @"PrefGraphics";
}

- (id)toolbarLabel
{
	return [[NSBundle mainBundle] localizedStringForKey:@"Graphics" value:@"" table:nil];
}

- (id)toolbarImage
{
	return [NSImage imageNamed:@"pref_graphics"];
}

@end
