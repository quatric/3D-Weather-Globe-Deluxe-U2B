#import "WGPlaceholderView.h"
#import "NSView+WGExtensions.h"

// A flipped view that draws a placeholder string (wgDrawPlaceholderStringInRect:).
@implementation WGPlaceholderView

- (BOOL)isFlipped
{
	return YES;
}

- (id)initWithFrame:(NSRect)aFrame
{
	return [super initWithFrame:aFrame];
}

- (void)dealloc
{
	[_string release];
	[_backgroundColor release];
	[super dealloc];
}

- (void)awakeFromNib
{
	[self wgFixFlip];
}

- (void)drawRect:(NSRect)aRect
{
	[self wgDrawPlaceholderStringInRect:aRect];
}

- (id)wgPlaceholderString
{
	return _string;
}

- (void)setWGPlaceholderString:(id)aString
{
	if (_string != aString) {
		[_string release];
		_string = [aString retain];
		[self setNeedsDisplay:YES];
	}
}

- (id)wgBackgroundColor
{
	return _backgroundColor;
}

- (void)setWGBackgroundColor:(id)aColor
{
	if (_backgroundColor != aColor) {
		[_backgroundColor release];
		_backgroundColor = [aColor retain];
		[self setNeedsDisplay:YES];
	}
}

@end
