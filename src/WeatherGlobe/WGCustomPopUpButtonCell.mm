#import "WGCustomPopUpButtonCell.h"
#import "WGCenteredTextAttachmentCell.h"

// A popup drawn from three images (left cap, stretched middle, right cap)
// with the selected title in white.
@implementation WGCustomPopUpButtonCell

- (id)initWithPopUpButtonCell:(id)aCell
{
	self = [super initTextCell:[aCell titleOfSelectedItem] pullsDown:[aCell pullsDown]];
	if (self) {
		[self setMenu:[aCell menu]];
		[self setAction:[aCell action]];
		[self setTarget:[aCell target]];
	}
	return self;
}

- (void)dealloc
{
	for (int i = 0; i < 3; i++)
		[_images[i] release];
	[_attribsDict release];
	[super dealloc];
}

- (id)attributesDict
{
	if (!_attribsDict)
		_attribsDict = [[NSDictionary alloc] initWithObjectsAndKeys:[NSColor whiteColor], NSForegroundColorAttributeName, nil];
	return _attribsDict;
}

- (id)imageForPosition:(NSInteger)aPosition
{
	return (NSUInteger)aPosition < 3 ? _images[aPosition] : nil;
}

// The original sent setNeedsDisplay: to the cell itself.
- (void)setImage:(id)aImage forPosition:(NSInteger)aPosition
{
	if ((NSUInteger)aPosition >= 3 || _images[aPosition] == aImage)
		return;
	[_images[aPosition] release];
	_images[aPosition] = [aImage retain];
	[[self controlView] setNeedsDisplay:YES];
}

- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView
{
	[self drawInteriorWithFrame:aFrame inView:aView];
}

// The caps and middle are composed into an image as tall as the tallest,
// drawn flipped, then the title 10 points in.
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	BOOL focused = [self showsFirstResponder];
	if (focused) {
		[NSGraphicsContext saveGraphicsState];
		[self setFocusRingType:NSFocusRingTypeDefault];
		NSSetFocusRingStyle(NSFocusRingBelow);
	}
	NSImage *left = _images[0], *middle = _images[1], *right = _images[2];
	NSSize leftSize = left ? [left size] : NSZeroSize;
	NSSize middleSize = middle ? [middle size] : NSZeroSize;
	NSSize rightSize = right ? [right size] : NSZeroSize;
	CGFloat height = MAX(rightSize.height, MAX(middleSize.height, leftSize.height));
	if (height == 0)
		return;
	NSImage *image = [[[NSImage alloc] initWithSize:NSMakeSize(aFrame.size.width, height)] autorelease];
	[image lockFocus];
	if (left)
		[left drawAtPoint:NSMakePoint(0, floor((height - leftSize.height) * 0.5)) fromRect:NSZeroRect
		        operation:NSCompositingOperationSourceOver fraction:1];
	if (middle)
		[middle drawInRect:NSMakeRect(leftSize.width, floor((height - middleSize.height) * 0.5),
		                              aFrame.size.width - leftSize.width - rightSize.width, middleSize.height)
		          fromRect:NSMakeRect(0, 0, middleSize.width, middleSize.height)
		         operation:NSCompositingOperationSourceOver
		          fraction:1];
	if (right)
		[right drawAtPoint:NSMakePoint(aFrame.size.width - rightSize.width, floor((height - rightSize.height) * 0.5))
		          fromRect:NSZeroRect
		         operation:NSCompositingOperationSourceOver
		          fraction:1];
	[image unlockFocus];
	NSGraphicsContext *context = [NSGraphicsContext currentContext];
	[context saveGraphicsState];
	NSAffineTransform *flip = [NSAffineTransform transform];
	[flip translateXBy:0 yBy:height];
	[flip scaleXBy:1 yBy:-1];
	[flip concat];
	[image drawAtPoint:NSZeroPoint fromRect:NSMakeRect(0, 0, aFrame.size.width, height)
	         operation:NSCompositingOperationSourceOver fraction:1];
	[context restoreGraphicsState];
	if (focused)
		[NSGraphicsContext restoreGraphicsState];

	NSString *title = [super titleOfSelectedItem] ? [super titleOfSelectedItem] : @"";
	NSAttributedString *string = [[[[NSAttributedString alloc] initWithString:title attributes:[self attributesDict]] autorelease]
	    wgSetLineBreak:NSLineBreakByTruncatingTail];
	[string drawInRect:NSMakeRect(10, 2, aFrame.size.width - 30.0, [string size].height)];
}

@end
