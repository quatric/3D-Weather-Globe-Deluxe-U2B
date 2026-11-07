#import "WGImageTextCell.h"

// An image under the engraved text: scaling 0 fits it proportionally, 2
// draws it at its size, others fill the frame.
@implementation WGImageTextCell

- (void)dealloc
{
	[_image release];
	[super dealloc];
}

- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	if (_image) {
		NSSize size = [_image size];
		NSRect rect = aFrame;
		if (_imageScaling == 0) {
			float scale = MIN(aFrame.size.width / size.width, aFrame.size.height / size.height);
			rect.size.width = truncf(size.width * scale);
			rect.size.height = truncf(scale * size.height);
			// The original centered by the image's size instead of the
			// frame's.
			rect.origin.x = truncf((aFrame.size.width - rect.size.width) * 0.5) + aFrame.origin.x;
			rect.origin.y = truncf((aFrame.size.height - rect.size.height) * 0.5) + aFrame.origin.y;
		} else if (_imageScaling == 2)
			rect.size = size;
		[_image drawInRect:rect
		          fromRect:NSMakeRect(0, 0, size.width, size.height)
		         operation:NSCompositingOperationSourceOver
		          fraction:1];
	}
	[super drawInteriorWithFrame:aFrame inView:aView];
}

- (id)image
{
	return _image;
}

- (NSInteger)imageScaling
{
	return _imageScaling;
}

- (void)setImage:(id)aImage
{
	if (_image != aImage) {
		[_image release];
		_image = [aImage retain];
	}
}

- (void)setImageScaling:(NSInteger)aScaling
{
	_imageScaling = (int)aScaling;
}

@end
