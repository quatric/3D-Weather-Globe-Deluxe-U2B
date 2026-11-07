#import "_SMKBackgroundRendererRegular.h"

@implementation _SMKBackgroundRendererRegular

- (id)loadImageForTag:(NSInteger)aTag
{
	NSBundle *bundle = [NSBundle bundleForClass:[self class]];
	NSString *path = [bundle pathForResource:[NSString stringWithFormat:@"seg_%i", (int)aTag] ofType:@"tiff"];
	return [[NSImage alloc] initWithContentsOfFile:path];
}

- (struct _SMKInset)defaultContentInset
{
	struct _SMKInset inset = [super defaultContentInset];
	inset.top -= 1.0f;
	inset.bottom += 1.0f;
	return inset;
}

@end
