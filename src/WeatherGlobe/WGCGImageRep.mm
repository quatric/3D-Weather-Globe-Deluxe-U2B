#import "WGCGImageRep.h"

@implementation WGCGImageRep

+ (id)imageRepWithCGImage:(struct CGImage *)aImage
{
	return [[[[self class] alloc] initWithCGImage:aImage] autorelease];
}

- (id)initWithCGImage:(struct CGImage *)aImage
{
	if (!aImage) {
		[super dealloc];
		return nil;
	}
	self = [super init];
	if (self) {
		_image = CGImageRetain(aImage);
		size_t width = CGImageGetWidth(_image);
		size_t height = CGImageGetHeight(_image);
		CGImageAlphaInfo alpha = CGImageGetAlphaInfo(_image);
		[self setAlpha:!(alpha == kCGImageAlphaNone || alpha == kCGImageAlphaNoneSkipLast || alpha == kCGImageAlphaNoneSkipFirst)];
		[self setPixelsWide:width];
		[self setPixelsHigh:height];
		[self setSize:NSMakeSize(width, height)];
		[self setBitsPerSample:CGImageGetBitsPerComponent(_image)];
	}
	return self;
}

- (void)dealloc
{
	CGImageRelease(_image);
	[super dealloc];
}

- (BOOL)draw
{
	NSSize size = [self size];
	CGContextDrawImage([[NSGraphicsContext currentContext] CGContext], CGRectMake(0, 0, size.width, size.height), _image);
	return YES;
}

@end
