#import "SMKSearchField.h"

// A search field for systems before NSSearchField; on newer ones it can hand
// out a real NSSearchField.
@implementation SMKSearchField

+ (void)initialize
{
	[self setCellClass:[SMKSearchFieldCell class]];
}

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		return;
	}
	[aCoder encodeValueOfObjCType:"I" at:&_reserved1];
	[aCoder encodeValueOfObjCType:"I" at:&_reserved2];
	[aCoder encodeValueOfObjCType:"I" at:&_reserved3];
	[aCoder encodeValueOfObjCType:"I" at:&_reserved4];
}

// rounded fields get the standard height for their control size
- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	if (![aCoder respondsToSelector:@selector(allowsKeyedCoding)] || ![aCoder allowsKeyedCoding]) {
		[aCoder decodeValueOfObjCType:"I" at:&_reserved1];
		[aCoder decodeValueOfObjCType:"I" at:&_reserved2];
		[aCoder decodeValueOfObjCType:"I" at:&_reserved3];
		[aCoder decodeValueOfObjCType:"I" at:&_reserved4];
	}
	id cell = [self cell];
	if (![cell respondsToSelector:@selector(bezelStyle)] || [(NSTextFieldCell *)cell bezelStyle] != NSTextFieldRoundedBezel) {
		return self;
	}
	NSSize size = [self frame].size;
	NSControlSize controlSize = [cell controlSize];
	float height;
	if (NSAppKitVersionNumber > 663.6) {
		if (controlSize == NSControlSizeSmall) {
			height = 19.0f;
		}
		else if (controlSize == NSControlSizeMini) {
			height = 15.0f;
		}
		else {
			height = 22.0f;
			if (controlSize != NSControlSizeRegular) {
				[[self cell] setControlSize:controlSize];
			}
		}
	}
	else {
		height = 20.0f;
		if (controlSize != NSControlSizeRegular) {
			[[self cell] setControlSize:controlSize];
		}
	}
	if (!NSEqualSizes(size, NSMakeSize(size.width, height))) {
		[self setFrameSize:NSMakeSize(size.width, height)];
	}
	return self;
}

// before 10.3.x the cell tracks clicks outside the text area itself
- (void)mouseDown:(id)aDown
{
	if (NSAppKitVersionNumber <= 663.6) {
		NSPoint p = [self convertPoint:[aDown locationInWindow] fromView:nil];
		NSRect bounds = [self bounds];
		if ([self mouse:p inRect:bounds]
			&& ![self mouse:p inRect:[[self cell] searchTextRectForBounds:bounds]]) {
			[[self cell] trackMouse:aDown inRect:bounds ofView:self untilMouseUp:YES];
			return;
		}
	}
	[super mouseDown:aDown];
}

- (id)nsSearchField
{
	id cell = [[self cell] nsSearchFieldCell];
	Class searchFieldClass = NSClassFromString(@"NSSearchField");
	if (cell == nil || searchFieldClass == Nil) {
		return nil;
	}
	NSTextField *field = [[searchFieldClass alloc] initWithFrame:[self frame]];
	[field setAutoresizingMask:[self autoresizingMask]];
	[field setCell:cell];
	return [field autorelease];
}

- (void)setRecentSearches:(id)aSearches
{
	[[self cell] setRecentSearches:aSearches];
}

- (id)recentSearches
{
	return [[self cell] recentSearches];
}

- (void)setSearchMenuTemplate:(id)aTemplate
{
	[[self cell] setSearchMenuTemplate:aTemplate];
}

- (id)searchMenuTemplate
{
	return [[self cell] searchMenuTemplate];
}

@end
