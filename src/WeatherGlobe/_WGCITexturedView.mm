#import "CityInfoController.h"

@implementation _WGCITexturedView

- (void)drawRect:(NSRect)aRect
{
	[self setFraction:Preferences::paletteTransparencyLevel()];
	[super drawRect:aRect];
}

@end
