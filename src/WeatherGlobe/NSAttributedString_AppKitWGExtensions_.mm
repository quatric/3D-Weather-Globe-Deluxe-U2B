#import "WGCenteredTextAttachmentCell.h"

@implementation NSAttributedString (AppKitWGExtensions)

// An image attachment for a path or an image resource of the main bundle
// or this class's, centered on the font's x-height.
+ (id)attributedStringWithImageAttachmentWithPath:(id)aPath forFont:(id)aFont
{
	NSFileWrapper *wrapper = [[NSFileWrapper alloc] initWithPath:aPath];
	if (!wrapper) {
		NSString *path = [[NSBundle mainBundle] pathForImageResource:aPath];
		wrapper = [[NSFileWrapper alloc] initWithPath:path];
		if (!wrapper) {
			wrapper = [[NSFileWrapper alloc] initWithPath:[[NSBundle bundleForClass:[self class]] pathForImageResource:path]];
			if (!wrapper)
				return nil;
		}
	}
	NSTextAttachment *attachment = [[[NSTextAttachment alloc] initWithFileWrapper:wrapper] autorelease];
	WGCenteredTextAttachmentCell *cell = [[WGCenteredTextAttachmentCell new] autorelease];
	[cell setImage:[(NSTextAttachmentCell *)[attachment attachmentCell] image]];
	float xHeight = [aFont xHeight];
	[cell setBaselineOffset:NSMakePoint(0.0, (xHeight - [cell cellSize].height) * 0.3f)];
	[attachment setAttachmentCell:cell];
	NSAttributedString *result = [NSAttributedString attributedStringWithAttachment:attachment];
	[wrapper release];
	return result;
}

// A copy with the line break mode in its paragraph style (the first
// character's, else the default).
- (id)wgSetLineBreak:(NSInteger)aBreak
{
	NSMutableAttributedString *copy = [self mutableCopy];
	if (copy && [copy length]) {
		NSParagraphStyle *style = [copy attribute:NSParagraphStyleAttributeName atIndex:0 effectiveRange:NULL];
		if (!style)
			style = [NSParagraphStyle defaultParagraphStyle];
		NSMutableParagraphStyle *newStyle = [style mutableCopy];
		[newStyle setLineBreakMode:(NSLineBreakMode)aBreak];
		[copy addAttribute:NSParagraphStyleAttributeName value:newStyle range:NSMakeRange(0, [copy length])];
		[newStyle release];
	}
	return [copy autorelease];
}

@end
