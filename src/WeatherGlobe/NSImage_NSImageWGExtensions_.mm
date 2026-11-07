#import "NSImage+NSImageWGExtensions.h"
#import "WGCGImageRep.h"

@implementation NSImage (NSImageWGExtensions)

// The LImage's NSImage, made once from its CGImage.
+ (id)wgImageWithLImage:(LImage *)aImage
{
	if (!aImage)
		return nil;
	id image = aImage->NSImage();
	if (!image) {
		image = [[self class] wgImageWithCGImage:aImage->CGImage()];
		aImage->setNSImage(image);
	}
	return image;
}

+ (id)wgImageWithCGImage:(struct CGImage *)aImage
{
	if (!aImage)
		return nil;
	WGCGImageRep *rep = [WGCGImageRep imageRepWithCGImage:aImage];
	NSImage *image = [[[NSImage alloc] initWithSize:[rep size]] autorelease];
	[image addRepresentation:rep];
	[image lockFocus];
	[image unlockFocus];
	return image;
}

@end
