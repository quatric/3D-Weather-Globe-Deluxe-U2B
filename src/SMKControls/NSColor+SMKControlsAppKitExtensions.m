#import "NSColor+SMKControlsAppKitExtensions.h"

@implementation NSImage (SMKControlsAppKitExtensions)

// The image darkened where it is opaque.
- (id)smkHighlightedImage
{
	NSRect rect = NSZeroRect;
	rect.size = [self size];
	NSImage *image = [[NSImage alloc] initWithSize:rect.size];
	[image lockFocus];
	NSRectFillUsingOperation(rect, NSCompositingOperationClear);
	[self drawInRect:rect fromRect:rect operation:NSCompositingOperationSourceOver fraction:1.0];
	[[NSColor colorWithCalibratedWhite:0.0 alpha:0.16] set];
	NSRectFillUsingOperation(rect, NSCompositingOperationSourceAtop);
	[image unlockFocus];
	return [image autorelease];
}

- (id)smkDisabledImage
{
	return [[self retain] autorelease];
}

@end

@implementation NSColor (SMKControlsAppKitExtensions)

// NSColor's own value where it has one.
+ (NSInteger)smkCurrentControlTint
{
	if ([self respondsToSelector:@selector(currentControlTint)])
		return [self currentControlTint];
	return [_SMKBackgroundRenderer currentControlTint];
}

@end
