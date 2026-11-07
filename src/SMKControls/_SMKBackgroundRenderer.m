#import "_SMKBackgroundRenderer.h"

// One shared renderer for each control size (regular, small, mini).
static id sRenderers[3];

@implementation _SMKBackgroundRenderer

+ (id)newBackgroundRendererForControlSize:(NSInteger)aSize
{
	if ((NSUInteger)aSize > 2)
		aSize = 0;
	id renderer = [sRenderers[aSize] retain];
	if (renderer == nil) {
		Class cls = [_SMKBackgroundRendererRegular class];
		if (aSize == 1)
			cls = [_SMKBackgroundRendererSmall class];
		else if (aSize == 2)
			cls = [_SMKBackgroundRendererMini class];
		renderer = [cls new];
		sRenderers[aSize] = renderer;
	}
	return renderer;
}

- (void)dealloc
{
	for (int i = 0; i < 3; i++)
		if (sRenderers[i] == self)
			sRenderers[i] = nil;
	[super dealloc];
}

@end
