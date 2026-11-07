#import "CIBackgroundImageView.h"

@implementation CIBackgroundImageView

// The image at the palettes' transparency: above a separator line at the
// bottom (style 0) or across the whole width (style 1).
- (void)drawRect:(NSRect)aRect
{
	NSImage *image = [self image];
	if (!image)
		return;
	NSSize size = [image size];
	NSRect dest = aRect;
	if (_style == 0) {
		NSRect bounds = [self bounds];
		dest.origin.y = 1.0;
		dest.size.height = bounds.size.height - 1.0f;
	} else if (_style == 1) {
		NSRect bounds = [self bounds];
		dest.origin.x = 0.0;
		dest.size.width = bounds.size.width;
	}
	[image drawInRect:dest fromRect:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, size.width, size.height)
	        operation:NSCompositingOperationCopy fraction:Preferences::paletteTransparencyLevel()];
	if (_style == 0) {
		[[NSColor colorWithCalibratedWhite:Preferences::separatorColorWhiteComponent()
		                             alpha:Preferences::paletteTransparencyLevel()] set];
		NSFrameRect(NSMakeRect(0.0, 0.0, [self bounds].size.width, 1.0));
	}
}

- (void)setImageViewStyle:(NSInteger)aStyle
{
	if (_style != aStyle) {
		_style = (int)aStyle;
		[self setNeedsDisplay:YES];
	}
}

- (BOOL)acceptsFirstResponder
{
	return NO;
}

@end
