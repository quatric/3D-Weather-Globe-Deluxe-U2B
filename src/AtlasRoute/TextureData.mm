// TextureData
#import "AtlasRoute.h"

@implementation TextureData

@synthesize image = _image;
@synthesize srcRect = _srcRect;
@synthesize dstRect = _dstRect;

- (void)dealloc
{
	[_image release];
	[super dealloc];
}

@end
