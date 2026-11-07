#import "WGTexturedButton.h"

// A button whose image and alternate image are composed from three pieces
// (left cap, stretched middle, right cap) at the button's width.
@implementation WGTexturedButton

- (void)dealloc
{
	for (int i = 0; i < 3; i++) {
		[_images[i] release];
		[_altImages[i] release];
	}
	[super dealloc];
}

- (id)imageForPosition:(NSInteger)aPosition
{
	return (NSUInteger)aPosition < 3 ? _images[aPosition] : nil;
}

- (void)setImage:(id)aImage forPosition:(NSInteger)aPosition
{
	if ((NSUInteger)aPosition >= 3 || _images[aPosition] == aImage)
		return;
	[_images[aPosition] release];
	_needRefreshIcon = YES;
	_images[aPosition] = [aImage retain];
	[self setNeedsDisplay:YES];
}

- (id)alternateImageForPosition:(NSInteger)aPosition
{
	return (NSUInteger)aPosition < 3 ? _altImages[aPosition] : nil;
}

- (void)setAlternateImage:(id)aImage forPosition:(NSInteger)aPosition
{
	if ((NSUInteger)aPosition >= 3 || _altImages[aPosition] == aImage)
		return;
	[_altImages[aPosition] release];
	_needRefreshAltIcon = YES;
	_altImages[aPosition] = [aImage retain];
	[self setNeedsDisplay:YES];
}

// Recomposed when the width or a piece changes.
- (void)_refreshIcons
{
	float width = [self bounds].size.width;
	BOOL resized = width != _lastWidth;
	_lastWidth = width;
	if (resized || _needRefreshIcon) {
		NSImage *image = [self _imageForType:0];
		if (image)
			[self setImage:image];
		_needRefreshIcon = NO;
	}
	if (resized || _needRefreshAltIcon) {
		NSImage *image = [self _imageForType:1];
		if (image)
			[self setAlternateImage:image];
		_needRefreshAltIcon = NO;
	}
}

- (void)drawRect:(NSRect)aRect
{
	[self _refreshIcons];
	[super drawRect:aRect];
}

// Type 0 is the normal image, 1 the alternate; nil when there are no
// pieces.
- (id)_imageForType:(NSInteger)aType
{
	NSImage **pieces = aType == 0 ? _images : _altImages;
	NSImage *left = pieces[0], *middle = pieces[1], *right = pieces[2];
	CGFloat width = [self bounds].size.width;
	NSSize leftSize = left ? [left size] : NSZeroSize;
	NSSize middleSize = middle ? [middle size] : NSZeroSize;
	NSSize rightSize = right ? [right size] : NSZeroSize;
	CGFloat height = MAX(rightSize.height, MAX(middleSize.height, leftSize.height));
	if (height == 0)
		return nil;
	NSImage *image = [[[NSImage alloc] initWithSize:NSMakeSize(width, height)] autorelease];
	[image lockFocus];
	if (left)
		[left drawAtPoint:NSMakePoint(0, floor((height - leftSize.height) * 0.5)) fromRect:NSZeroRect
		        operation:NSCompositingOperationSourceOver fraction:1];
	if (middle)
		[middle drawInRect:NSMakeRect(leftSize.width, floor((height - middleSize.height) * 0.5),
		                              width - leftSize.width - rightSize.width, middleSize.height)
		          fromRect:NSMakeRect(0, 0, middleSize.width, middleSize.height)
		         operation:NSCompositingOperationSourceOver
		          fraction:1];
	if (right)
		[right drawAtPoint:NSMakePoint(width - rightSize.width, floor((height - rightSize.height) * 0.5)) fromRect:NSZeroRect
		         operation:NSCompositingOperationSourceOver fraction:1];
	[image unlockFocus];
	return image;
}

@end
