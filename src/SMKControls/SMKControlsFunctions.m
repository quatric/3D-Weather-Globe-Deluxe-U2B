#import "SMKControlsPrefix.h"

const struct _SMKInset SMKZeroInset = { 0.0f, 0.0f, 0.0f, 0.0f };

float smk_floorf(float aValue)
{
	return floorf(aValue);
}

float smk_ceilf(float aValue)
{
	return ceilf(aValue);
}

float smk_roundf(float aValue)
{
	return roundf(aValue);
}

void SMKCompositeToPoint(NSImage *image, NSPoint point, NSCompositingOperation operation, CGFloat fraction)
{
	NSSize size = [image size];
	NSRect rect = NSMakeRect(point.x, point.y, size.width, size.height);
	if ([[NSView focusView] isFlipped])
		rect.origin.y -= size.height;
	[image drawInRect:rect fromRect:NSZeroRect operation:operation fraction:fraction respectFlipped:YES hints:nil];
}
// size: 0 regular, 1 small, 2 mini (others mean regular)
static NSImage *_searchFieldImage(NSString *aKind, NSString *aVariant, NSImage **aCache, NSUInteger aSize)
{
	if (aSize > 2) {
		aSize = 0;
	}
	if (aCache[aSize] == nil) {
		NSString *suffix = aSize == 0 ? @"" : (aSize == 1 ? @"_sm" : @"_mini");
		NSString *name = [NSString stringWithFormat:@"sfld_%@%@%@", aKind, aVariant, suffix];
		NSString *path = [[NSBundle bundleForClass:[SMKSearchFieldCell class]] pathForResource:name ofType:@"tiff"];
		aCache[aSize] = [[NSImage alloc] initWithContentsOfFile:path];
	}
	return aCache[aSize];
}

NSImage *SearchImageForSize(NSUInteger aSize, BOOL aPopup)
{
	static NSImage *sSearchImages[3];
	static NSImage *sPopupImages[3];
	return aPopup ? _searchFieldImage(@"search", @"_popup", sPopupImages, aSize)
		: _searchFieldImage(@"search", @"", sSearchImages, aSize);
}

NSImage *CancelImageForSize(NSUInteger aSize, BOOL aAlternate)
{
	static NSImage *sCancelImages[3];
	static NSImage *sCancelAltImages[3];
	return aAlternate ? _searchFieldImage(@"cancel", @"_alt", sCancelAltImages, aSize)
		: _searchFieldImage(@"cancel", @"", sCancelImages, aSize);
}

