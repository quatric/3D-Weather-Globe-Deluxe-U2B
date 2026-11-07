#import "WGCustomButton.h"

NSString *_kWGCustomButtonCellScaleKey = @"scale";
NSString *_kWGCustomButtonCellAlignKey = @"align";
NSString *_kWGCustomButtonCellAltImageKey = @"altImage";
NSString *_kWGCustomButtonCellImageKey = @"image";

// A button drawn from layered images (each with an alternate for the
// pressed/on state, an alignment and a scaling), darkened while pressed.
@implementation WGCustomButtonCell

- (void)dealloc
{
	[_data release];
	[super dealloc];
}

- (void)awakeFromNib
{
	[self setHighlighted:NO];
}

- (id)images
{
	return _data ? _data : [NSArray array];
}

- (void)setImages:(id)aImages
{
	if (_data != aImages) {
		[_data release];
		_data = [aImages retain];
	}
}

- (void)addImage:(id)aImage altImage:(id)aImage2 align:(NSInteger)aAlign scale:(NSInteger)aScale
{
	if (!aImage && !aImage2)
		return;
	NSMutableDictionary *layer = [NSMutableDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:(int)aAlign],
	                                                                             _kWGCustomButtonCellAlignKey,
	                                                                             [NSNumber numberWithInt:(int)aScale],
	                                                                             _kWGCustomButtonCellScaleKey, nil];
	if (aImage)
		[layer setObject:aImage forKey:_kWGCustomButtonCellImageKey];
	if (aImage2)
		[layer setObject:aImage2 forKey:_kWGCustomButtonCellAltImageKey];
	[self setImages:[[self images] arrayByAddingObject:layer]];
}

// Scaling 0 fits proportionally, 2 keeps the image's size, others fill.
// Alignment 0 centers, 8 goes right, others left.
- (void)drawImageAtIndex:(NSInteger)aIndex withFrame:(NSRect)aFrame
{
	NSDictionary *layer = [[self images] objectAtIndex:aIndex];
	NSString *key = [self isHighlighted] || [self state] == NSControlStateValueOn ? _kWGCustomButtonCellAltImageKey
	                                                                               : _kWGCustomButtonCellImageKey;
	NSImage *image = [layer objectForKey:key];
	if (!image)
		return;
	int align = [[layer objectForKey:_kWGCustomButtonCellAlignKey] intValue];
	int scale = [[layer objectForKey:_kWGCustomButtonCellScaleKey] intValue];
	NSSize size = [image size];
	NSSize drawn = aFrame.size;
	if (scale == 0) {
		float factor = MIN(aFrame.size.width / size.width, aFrame.size.height / size.height);
		drawn = NSMakeSize(truncf(size.width * factor), truncf(factor * size.height));
	} else if (scale == 2)
		drawn = size;
	if (align == 0) {
		aFrame.origin.x += (aFrame.size.width - drawn.width) * 0.5;
		aFrame.origin.y += (aFrame.size.height - drawn.height) * 0.5;
	} else if (align == 8)
		aFrame.origin.x += aFrame.size.width - drawn.width;
	[image drawInRect:NSMakeRect(aFrame.origin.x, aFrame.origin.y, drawn.width, drawn.height)
	         fromRect:NSMakeRect(0, 0, size.width, size.height)
	        operation:NSCompositingOperationSourceOver
	         fraction:1];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute])
		return [[NSBundle mainBundle] localizedStringForKey:@"Tab" value:@"" table:nil];
	if ([aValue isEqualToString:NSAccessibilityTitleAttribute])
		return [self title];
	return [super accessibilityAttributeValue:aValue];
}

// The last cell in the view reaches its right edge. The layers are drawn
// unflipped, then the title centered.
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	NSRect bounds = [aView bounds];
	CGFloat width = aFrame.size.width;
	if (NSMaxX(bounds) - NSMaxX(aFrame) <= 1.0)
		width = NSMaxX(bounds) - aFrame.origin.x;
	NSInteger count = [[self images] count];
	CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
	BOOL highlighted = [self isHighlighted];
	if (highlighted)
		CGContextBeginTransparencyLayer(context, NULL);
	BOOL flipped = [aView isFlipped];
	if (flipped) {
		[NSGraphicsContext saveGraphicsState];
		NSAffineTransform *transform = [NSAffineTransform transform];
		NSAffineTransformStruct flip = {1, 0, 0, -1, 0, aFrame.origin.y + aFrame.origin.y + aFrame.size.height};
		[transform setTransformStruct:flip];
		[transform concat];
	}
	for (NSInteger i = 0; i < count; i++)
		[self drawImageAtIndex:i withFrame:NSMakeRect(aFrame.origin.x, aFrame.origin.y, width, aFrame.size.height)];
	if (flipped)
		[NSGraphicsContext restoreGraphicsState];
	if (highlighted) {
		[[[NSColor blackColor] colorWithAlphaComponent:0.25] set];
		NSRectFillUsingOperation(NSMakeRect(aFrame.origin.x, aFrame.origin.y, width, aFrame.size.height),
		                         NSCompositingOperationSourceAtop);
		CGContextEndTransparencyLayer(context);
	}
	NSSize size = [self cellSize];
	[super drawInteriorWithFrame:NSMakeRect(ceilf((width - size.width) * 0.5) + aFrame.origin.x,
	                                        aFrame.origin.y + ceilf((aFrame.size.height - size.height) * 0.5), size.width,
	                                        size.height)
	                      inView:aView];
}

@end
