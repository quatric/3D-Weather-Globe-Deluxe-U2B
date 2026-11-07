#import "WGCustomImageView.h"

@implementation WGCustomImageView

// The image scaled to fit the bounds keeping its proportions (centered on
// the bounds' size), stretched to them, or at its size.
- (void)drawRect:(NSRect)aRect
{
	NSImage *image = [self image];
	NSSize size = [image size];
	NSRect source = {NSZeroPoint, size};
	NSRect dest;
	NSImageScaling scaling = [self imageScaling];
	if (scaling == NSImageScaleProportionallyDown) {
		NSRect bounds = [self bounds];
		float width = bounds.size.width, height = bounds.size.height;
		float ratio = size.width / size.height;
		if (ratio <= 1.0f)
			width = ratio * bounds.size.height;
		else
			height = bounds.size.width / ratio;
		if (height <= bounds.size.height) {
			if (bounds.size.width < width) {
				width = bounds.size.width;
				height = bounds.size.width / ratio;
			}
		} else {
			width = ratio * bounds.size.height;
			height = bounds.size.height;
		}
		dest = NSMakeRect(bounds.size.width * 0.5f - width * 0.5f, bounds.size.height * 0.5f - height * 0.5f, width, height);
	} else if (scaling == NSImageScaleAxesIndependently) {
		dest = [self bounds];
	} else {
		dest = source;
	}
	[image drawInRect:dest fromRect:source operation:NSCompositingOperationSourceOver fraction:1.0];
}

@end
