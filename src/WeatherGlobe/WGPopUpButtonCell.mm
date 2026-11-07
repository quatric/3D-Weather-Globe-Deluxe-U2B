#import "WGPopUpButton.h"

@implementation WGPopUpButtonCell

- (void)dealloc
{
	[_attrs release];
	[super dealloc];
}

// NSPopUpButtonCell's private hook for the arrow area's width.
- (CGFloat)_extraWidthForCellHeight:(CGFloat)aHeight
{
	return _extraHeight;
}

- (void)setExtraWidthForCellHeight:(CGFloat)aHeight
{
	_extraHeight = aHeight;
}

- (void)setTitleWidth:(CGFloat)aWidth
{
	_titleWidth = aWidth;
}

// Every item's title gets the attributes.
- (void)setItemTitlesStringAttributes:(id)aAttributes
{
	[_attrs release];
	_attrs = [aAttributes retain];
	NSMenu *menu = [self menu];
	NSInteger count = [menu numberOfItems];
	for (NSInteger i = 0; i < count; i++) {
		NSMenuItem *item = [menu itemAtIndex:i];
		NSAttributedString *title = [[NSAttributedString alloc] initWithString:[item title] attributes:_attrs];
		[item setAttributedTitle:title];
		[title release];
	}
}

// A set title width is kept at the right end.
- (NSRect)titleRectForBounds:(NSRect)aBounds
{
	NSRect rect = [super titleRectForBounds:aBounds];
	if (_titleWidth > 0.0001) {
		rect.origin.x += rect.size.width - _titleWidth;
		rect.size.width = _titleWidth;
	}
	return rect;
}

@end
