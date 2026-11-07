#import "SMKGridAttributedCell.h"

// A grid cell with small attribute images (the data source's
// SMKGridAttributeImages info) in a row under its graphic.
@implementation SMKGridAttributedCell

- (void)dealloc
{
	[_attributeImagesLock release];
	[_attributeImages release];
	[super dealloc];
}

- (void)updateContentIfNeeded
{
	NSLock *lock = [self lock];
	[lock lock];
	BOOL needed = _isUpdateNeeded;
	[lock unlock];
	if (!needed)
		return;
	[super updateContentIfNeeded];
	id info = [[[self controlView] owningGridControl] additionalInfoForItemAt:_dataNo];
	id images = [info objectForKey:@"SMKGridAttributeImages"];
	if (images == nil || [images isKindOfClass:[NSArray class]])
		[self setAttributeImages:images];
}

- (void)setAttributeImages:(id)aImages
{
	NSLock *lock = [self attributeImagesLock];
	[lock lock];
	if (_attributeImages != aImages) {
		[_attributeImages autorelease];
		_attributeImages = [aImages retain];
	}
	[lock unlock];
}

// The images' row, centred in the frame with a point between them.
- (NSRect)attributeImagesFrameForFrame:(NSRect)aFrame
{
	NSLock *lock = [self attributeImagesLock];
	[lock lock];
	NSEnumerator *images = [_attributeImages objectEnumerator];
	[lock unlock];
	float width = 1.0f;
	NSImage *image;
	while ((image = [images nextObject]) != nil)
		width += (float)[image size].width + 1.0f;
	return NSMakeRect(aFrame.origin.x + (aFrame.size.width - width) * 0.5f, aFrame.origin.y, width, aFrame.size.height);
}

// Only with threaded cell updating.
- (id)attributeImagesLock
{
	if (_attributeImagesLock == nil && [[self controlView] isThreadedCellUpdatingEnabled])
		_attributeImagesLock = (NSLock *)[NSRecursiveLock new];
	return _attributeImagesLock;
}

// The images right to left from the row's right end, along the graphic's
// bottom, as long as they fit in the frame.
- (void)drawGraphicContentInFrame:(NSRect)aFrame
{
	[super drawGraphicContentInFrame:aFrame];
	NSRect row = [self attributeImagesFrameForFrame:aFrame];
	NSRect graphic = [self cachedImageRectForBounds:aFrame];
	NSLock *lock = [self attributeImagesLock];
	[lock lock];
	NSEnumerator *images = [_attributeImages reverseObjectEnumerator];
	[lock unlock];
	float x = (float)row.origin.x;
	float offset = (float)row.size.width;
	if (row.size.width < graphic.size.width)
		x += (float)(graphic.size.width - row.size.width) * 0.5f;
	NSImage *image;
	while ((image = [images nextObject]) != nil) {
		float width = (float)[image size].width;
		offset -= width + 1.0f;
		if (x - 1.0f < aFrame.origin.x)
			break;
		float px = x + offset;
		if (width + px + 1.0f <= NSMaxX(aFrame))
			SMKCompositeToPoint(image, NSMakePoint(px, floorf((float)NSMaxY(graphic)) - 1.0f),
			                    NSCompositingOperationSourceOver, 1.0f);
	}
}

@end
