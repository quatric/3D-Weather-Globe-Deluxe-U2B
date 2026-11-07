#import "WGSplitView.h"

// A flipped split view with a settable divider thickness and fill color.
@implementation WGSplitView

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self)
		_dividerThickness = [super dividerThickness];
	return self;
}

- (BOOL)isFlipped
{
	return YES;
}

- (CGFloat)dividerThickness
{
	return _dividerThickness;
}

- (void)setDividerThickness:(CGFloat)aThickness
{
	_dividerThickness = aThickness;
	[self adjustSubviews];
}

- (void)drawDividerInRect:(NSRect)aRect
{
	[_dividerFillColor set];
	if (_dividerThickness <= 0 || !_dividerFillColor)
		[super drawDividerInRect:aRect];
	else
		NSRectFillUsingOperation(aRect, [self isOpaque] ? NSCompositingOperationCopy : NSCompositingOperationSourceOver);
}

- (id)dividerFillColor
{
	return _dividerFillColor;
}

- (void)setDividerFillColor:(id)aColor
{
	if (_dividerFillColor != aColor) {
		[_dividerFillColor release];
		_dividerFillColor = [aColor retain];
		[self setNeedsDisplay:YES];
	}
}

@end
