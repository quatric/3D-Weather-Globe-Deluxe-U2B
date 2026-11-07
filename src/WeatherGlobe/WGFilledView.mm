#import "WGFilledView.h"

@implementation WGFilledView

- (void)dealloc
{
	[_fillColor release];
	[super dealloc];
}

- (void)drawRect:(NSRect)aRect
{
	[_fillColor set];
	NSRectFill(aRect);
}

- (id)fillColor
{
	return _fillColor;
}

- (void)setFillColor:(id)aColor
{
	if (_fillColor != aColor) {
		[_fillColor release];
		_fillColor = [aColor retain];
		[self setNeedsDisplay:YES];
	}
}

@end
