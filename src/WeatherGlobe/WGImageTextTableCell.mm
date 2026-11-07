#import "WGImageTextTableCell.h"

NSString *_kImageTextTableCellBackgroundColorKey = @"ImageTextTableCellBackgroundColorKey";
NSString *_kImageTextTableCellImageKey = @"ImageTextTableCellImageKey";
NSString *_kImageTextTableCellTextAttributesKey = @"ImageTextTableCellTextAttributesKey";
NSString *_kImageTextTableCellTextKey = @"ImageTextTableCellTextKey";

// The image is kept as the represented object and drawn in a square at the
// left.
@implementation WGImageTextTableCell

- (id)initTextCell:(id)aCell
{
	return [super initTextCell:aCell];
}

// NSTextFieldCell's initImageCell: is unavailable now; the image is only the
// represented object anyway.
- (id)initImageCell:(id)aCell
{
	self = [super initTextCell:@""];
	if (self) {
		[self setStringValue:@""];
		[self setRepresentedObject:aCell];
	}
	return self;
}

- (id)init
{
	return [super init];
}

// A dictionary gives the text (with attributes), image and background color.
- (void)setObjectValue:(id)aValue
{
	if (![aValue isKindOfClass:[NSDictionary class]]) {
		[super setObjectValue:aValue];
		[self setRepresentedObject:nil];
		// The original cleared the background color ivar directly.
		[self setBackgroundColor:nil];
		return;
	}
	NSString *text = [aValue objectForKey:_kImageTextTableCellTextKey];
	NSAttributedString *string = nil;
	if (text)
		string = [[[NSAttributedString alloc] initWithString:text
		                                          attributes:[aValue objectForKey:_kImageTextTableCellTextAttributesKey]] autorelease];
	[super setAttributedStringValue:string];
	[self setRepresentedObject:[aValue objectForKey:_kImageTextTableCellImageKey]];
	NSColor *background = [aValue objectForKey:_kImageTextTableCellBackgroundColorKey];
	if (background)
		[self setBackgroundColor:background];
	[self setDrawsBackground:background != nil];
}

// The image is scaled to fit an even-sided square of the frame's height,
// drawn unsmoothed (the view is flipped), and the text follows 3 points
// after it.
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	NSImage *image = [self representedObject];
	if (image) {
		if ([self drawsBackground] && ![self isHighlighted]) {
			[[self backgroundColor] set];
			NSRectFill(NSMakeRect(aFrame.origin.x, aFrame.origin.y, aFrame.size.height + 3.0, aFrame.size.height));
		}
		NSSize size = [image size];
		int height = (int)aFrame.size.height;
		float side = (float)((height - 1) + ((height & 1) == 0));
		float scale = MIN(side / size.width, side / size.height);
		float drawn = floorf(size.height * scale);
		NSAffineTransform *transform = [NSAffineTransform transform];
		float y = floorf((aFrame.size.height - drawn) * 0.5 + aFrame.origin.y) + drawn;
		float x = floorf((aFrame.size.height - floorf(scale * size.width)) * 0.5 + aFrame.origin.x);
		[transform translateXBy:x yBy:y];
		[transform scaleXBy:1 yBy:-1];
		NSImageInterpolation interpolation = [[NSGraphicsContext currentContext] imageInterpolation];
		[[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationNone];
		[NSGraphicsContext saveGraphicsState];
		[transform concat];
		[image drawInRect:NSMakeRect(0, 0, drawn, drawn)
		         fromRect:NSMakeRect(0, 0, size.width, size.height)
		        operation:NSCompositingOperationSourceOver
		         fraction:1];
		[NSGraphicsContext restoreGraphicsState];
		[[NSGraphicsContext currentContext] setImageInterpolation:interpolation];
		aFrame.size.width = aFrame.size.width - aFrame.size.height - 3.0;
		aFrame.origin.x = aFrame.origin.x + 3.0 + aFrame.size.height;
	}
	[super drawInteriorWithFrame:aFrame inView:aView];
}

@end
