#import "WGCustomTitleView.h"

// A textured title bar drawing the window's title centered, with an
// optional shadow; a double click miniaturizes like a real title bar.
@implementation WGCustomTitleView

- (void)dealloc
{
	[_titleCell release];
	[_frontColor release];
	[_shadowColor release];
	[super dealloc];
}

// Utility panels use the small font.
- (CGFloat)_titleFontSize
{
	NSWindow *window = [self window];
	NSUInteger mask = [window styleMask];
	if ([window isKindOfClass:[NSPanel class]] && (mask & NSWindowStyleMaskUtilityWindow))
		return [NSFont smallSystemFontSize];
	return [NSFont systemFontSize];
}

- (id)_titleFont
{
	return [NSFont systemFontOfSize:[self _titleFontSize]];
}

- (id)_titleCell
{
	if (!_titleCell) {
		_titleCell = [NSTextFieldCell new];
		[_titleCell setAlignment:NSTextAlignmentCenter];
		[_titleCell setFont:[self _titleFont]];
	}
	return _titleCell;
}

- (void)drawRect:(NSRect)aRect
{
	[super drawRect:aRect];
	if (!_drawsTitle)
		return;
	NSTextFieldCell *cell = [self _titleCell];
	[cell setStringValue:[[self window] title]];
	NSRect bounds = [self bounds];
	NSSize size = [_titleCell cellSize];
	NSRect rect = NSMakeRect(floorf((bounds.size.width - size.width) * 0.5f) + bounds.origin.x,
		ceilf((bounds.size.height - size.height) * 0.5f) + bounds.origin.y, size.width, size.height);
	if (!NSEqualSizes(_shadowOffset, NSZeroSize)) {
		NSRect shadow = NSOffsetRect(rect, _shadowOffset.width, _shadowOffset.height);
		[_titleCell setTextColor:[self titleShadowColor]];
		[_titleCell drawInteriorWithFrame:shadow inView:self];
	}
	[_titleCell setTextColor:[self titleFrontColor]];
	[_titleCell drawInteriorWithFrame:rect inView:self];
}

// Honors the "double-click to minimize" setting (on when unset).
- (void)mouseDown:(id)aDown
{
	NSWindow *window = [self window];
	if ([aDown clickCount] < 2 || [(NSEvent *)aDown type] != NSEventTypeLeftMouseDown
		|| !([window styleMask] & NSWindowStyleMaskMiniaturizable))
		return;
	id setting = [[NSUserDefaults standardUserDefaults] objectForKey:@"AppleMiniaturizeOnDoubleClick"];
	if (setting && ![setting boolValue])
		return;
	[window performMiniaturize:[window standardWindowButton:NSWindowMiniaturizeButton]];
}

- (BOOL)drawsTitle
{
	return _drawsTitle;
}

- (void)setDrawsTitle:(BOOL)aTitle
{
	if (_drawsTitle != aTitle) {
		_drawsTitle = aTitle;
		[self setNeedsDisplay:YES];
	}
}

- (NSSize)titleShadowOffset
{
	return _shadowOffset;
}

- (void)setTitleShadowOffset:(NSSize)aOffset
{
	if (!NSEqualSizes(aOffset, _shadowOffset)) {
		_shadowOffset = aOffset;
		[self setNeedsDisplay:YES];
	}
}

- (id)titleShadowColor
{
	return _shadowColor ? _shadowColor : [NSColor blackColor];
}

- (void)setTitleShadowColor:(id)aColor
{
	if (_shadowColor != aColor) {
		[_shadowColor release];
		_shadowColor = [aColor retain];
		[self setNeedsDisplay:YES];
	}
}

- (id)titleFrontColor
{
	return _frontColor ? _frontColor : [NSColor whiteColor];
}

- (void)setTitleFrontColor:(id)aColor
{
	if (_frontColor != aColor) {
		[_frontColor release];
		_frontColor = [aColor retain];
		[self setNeedsDisplay:YES];
	}
}

@end
