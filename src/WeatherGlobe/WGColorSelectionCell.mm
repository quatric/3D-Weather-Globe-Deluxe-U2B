#import "WGColorSelectionCell.h"

NSString *_kWGColorSelectionCellImageKey = @"mask";
NSString *_kWGColorSelectionCellColorKey = @"color";

// The object value is a dictionary of the color and the mask image.
@implementation WGColorSelectionCell

- (id)color
{
	NSColor *color = [[self objectValue] objectForKey:_kWGColorSelectionCellColorKey];
	return color ? color : [NSColor clearColor];
}

- (id)maskImage
{
	return [[self objectValue] objectForKey:_kWGColorSelectionCellImageKey];
}

- (void)setColor:(id)aColor
{
	NSAssert(aColor != nil, @"nil should never be passed as color value");
	[self setObjectValue:[NSDictionary dictionaryWithObjectsAndKeys:aColor, _kWGColorSelectionCellColorKey,
	                                                                [self maskImage], _kWGColorSelectionCellImageKey, nil]];
}

- (void)setMaskImage:(id)aImage
{
	[self setObjectValue:[NSDictionary dictionaryWithObjectsAndKeys:[self color], _kWGColorSelectionCellColorKey, aImage,
	                                                                _kWGColorSelectionCellImageKey, nil]];
}

// A color oval (inset 1) under the mask, centered at the mask's size (the
// frame's height square without one).
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	BOOL flipped = [aView isFlipped];
	NSSize size = [self maskImage] ? [[self maskImage] size] : NSMakeSize(aFrame.size.height, aFrame.size.height);
	NSRect rect = NSMakeRect(aFrame.origin.x + (aFrame.size.width - size.width) * 0.5,
	                         aFrame.origin.y + (aFrame.size.height - size.height) * 0.5, size.width, size.height);
	if (flipped) {
		[NSGraphicsContext saveGraphicsState];
		NSAffineTransform *offset = [NSAffineTransform transform];
		[offset translateXBy:0 yBy:rect.origin.y];
		NSAffineTransform *flip = [NSAffineTransform transform];
		[flip translateXBy:0 yBy:rect.size.height];
		[flip scaleXBy:1 yBy:-1];
		[flip appendTransform:offset];
		[flip concat];
		rect.origin.y = 0;
	}
	[(NSColor *)[self color] set];
	[[NSBezierPath bezierPathWithOvalInRect:NSInsetRect(rect, 1, 1)] fill];
	[[self maskImage] drawInRect:rect fromRect:NSMakeRect(0, 0, size.width, size.height) operation:NSCompositingOperationSourceOver fraction:1];
	if (flipped)
		[NSGraphicsContext restoreGraphicsState];
}

@end
