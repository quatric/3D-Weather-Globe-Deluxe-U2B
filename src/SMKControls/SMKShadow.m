#import "SMKShadow.h"

// A drop shadow for systems before NSShadow. Where NSShadow exists (always,
// now) init returns one instead; the original drew through private
// CoreGraphics calls, replaced here by NSShadow.
@implementation SMKShadow

+ (void)initialize
{
}

- (void)setShadowColor:(id)aColor
{
}

- (id)shadowColor
{
	return nil;
}

- (void)set
{
	NSShadow *shadow = [[NSShadow alloc] init];
	[shadow setShadowOffset:_shadowOffset];
	[shadow setShadowBlurRadius:_shadowBlurRadius];
	[shadow set];
	[shadow release];
}

- (void)setShadowBlurRadius:(CGFloat)aRadius
{
	_shadowBlurRadius = (float)aRadius;
}

- (CGFloat)shadowBlurRadius
{
	return _shadowBlurRadius;
}

- (void)setShadowOffset:(NSSize)aOffset
{
	_shadowOffset = aOffset;
}

- (NSSize)shadowOffset
{
	return _shadowOffset;
}

- (void)dealloc
{
	[super dealloc];
}

- (id)init
{
	Class shadowClass = NSClassFromString(@"NSShadow");
	if (shadowClass == Nil) {
		return [super init];
	}
	[super dealloc];
	return [[shadowClass alloc] init];
}

@end
