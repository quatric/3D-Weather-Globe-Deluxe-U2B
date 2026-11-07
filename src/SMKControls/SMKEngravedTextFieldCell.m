#import "SMKEngravedTextFieldCell.h"

// A text field cell that draws its text a second time, offset by the
// engrave offset (down for a positive one, the text itself up for a
// negative one), in a lighter color.
@implementation SMKEngravedTextFieldCell

- (void)dealloc
{
	[_secondaryTextColor release];
	_secondaryTextColor = nil;
	[super dealloc];
}

- (void)setSecondaryTextColor:(id)aColor
{
	[aColor retain];
	[_secondaryTextColor release];
	_secondaryTextColor = aColor;
	id view = [self controlView];
	if ([view isKindOfClass:[NSControl class]])
		[view updateCell:self];
}

- (id)secondaryTextColor
{
	return _secondaryTextColor;
}

- (id)_secondaryTextColor
{
	return _secondaryTextColor != nil ? _secondaryTextColor : [NSColor whiteColor];
}

- (CGFloat)engraveOffset
{
	return _engraveOffset;
}

- (void)setEngraveOffset:(CGFloat)aOffset
{
	_engraveOffset = (float)aOffset;
}

- (void)_acceptPropertiesFromTextFieldCell:(id)aCell
{
	[self setBezeled:[aCell isBezeled]];
	[self setEditable:[aCell isEditable]];
	[self setTarget:[aCell target]];
	[self setAction:[aCell action]];
	[self setControlSize:[aCell controlSize]];
	[self setFont:[aCell font]];
	[self setBackgroundColor:[aCell backgroundColor]];
	[self setDrawsBackground:[aCell drawsBackground]];
	[self setTextColor:[aCell textColor]];
	[self setTitle:[aCell title]];
	[self setAlignment:[aCell alignment]];
}

- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	NSColor *color = [[self textColor] retain];
	NSRect shadowFrame = aFrame, textFrame = aFrame;
	[self setTextColor:[self _secondaryTextColor]];
	float offset = (float)[self engraveOffset];
	if (offset <= 0.0f)
		textFrame.origin.y += [aView isFlipped] ? -offset : offset;
	else
		shadowFrame.origin.y += [aView isFlipped] ? offset : -offset;
	[super drawInteriorWithFrame:shadowFrame inView:aView];
	[self setTextColor:color];
	[super drawInteriorWithFrame:textFrame inView:aView];
	[color release];
}

- (id)init
{
	self = [super init];
	if (self != nil)
		_engraveOffset = 1.0f;
	return self;
}

- (id)initWithTextFieldCell:(id)aCell
{
	self = [self initTextCell:[aCell title]];
	if (self != nil)
		[self _acceptPropertiesFromTextFieldCell:aCell];
	return self;
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil)
		return nil;
	id color = nil;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		if ([aCoder containsValueForKey:@"smk_secondaryTextColor"])
			color = [aCoder decodeObjectForKey:@"smk_secondaryTextColor"];
		if ([aCoder containsValueForKey:@"smk_engraveOffset"])
			_engraveOffset = [aCoder decodeFloatForKey:@"smk_engraveOffset"];
	} else {
		int version;
		[aCoder decodeValueOfObjCType:"i" at:&version];
		color = [aCoder decodeObject];
		[aCoder decodeValueOfObjCType:"f" at:&_engraveOffset];
	}
	[self setSecondaryTextColor:color];
	return self;
}

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		[aCoder encodeObject:_secondaryTextColor forKey:@"smk_secondaryTextColor"];
		[aCoder encodeFloat:_engraveOffset forKey:@"smk_engraveOffset"];
	} else {
		int version = 0;
		[aCoder encodeValueOfObjCType:"i" at:&version];
		[aCoder encodeObject:_secondaryTextColor];
		[aCoder encodeValueOfObjCType:"f" at:&_engraveOffset];
	}
}

@end
