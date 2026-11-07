#import "_SMKBackgroundRendererSmall.h"

@implementation _SMKBackgroundRendererSmall

- (id)loadImageForTag:(NSInteger)aTag
{
	NSBundle *bundle = [NSBundle bundleForClass:[self class]];
	NSString *path = [bundle pathForResource:[NSString stringWithFormat:@"segsmall_%i", (int)aTag] ofType:@"tiff"];
	return [[NSImage alloc] initWithContentsOfFile:path];
}

// The whole top inset goes to the bottom.
- (struct _SMKInset)defaultContentInset
{
	struct _SMKInset inset = [super defaultContentInset];
	inset.bottom += inset.top;
	inset.top = 0.0f;
	return inset;
}

@end
