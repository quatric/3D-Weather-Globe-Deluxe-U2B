#import "SMKSplittableScrollView.h"

// A scroll view with a splitter handle in the corner between its scrollers.
@implementation SMKSplittableScrollView

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		[aCoder encodeObject:_splitterView forKey:@"smk_splitterView"];
	}
	else {
		int version = 0;
		[aCoder encodeValueOfObjCType:"i" at:&version];
		[aCoder encodeObject:_splitterView];
	}
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		_splitterView = [[aCoder decodeObjectForKey:@"smk_splitterView"] retain];
	}
	else {
		int version;
		[aCoder decodeValueOfObjCType:"i" at:&version];
		_splitterView = [[aCoder decodeObject] retain];
	}
	return self;
}

// puts the splitter in the top right corner and shortens the scrollers
- (void)tile
{
	[super tile];
	if (![self splitterViewIsVisible]) {
		return;
	}
	NSRect bounds = [self bounds];
	NSRect frame = [[self splitterView] frame];
	NSPoint origin;
	origin.x = (float)(bounds.size.width + bounds.origin.x) - (float)frame.size.width;
	origin.y = (float)(bounds.size.height + bounds.origin.y) - (float)frame.size.height;
	if ([self borderType] != NSNoBorder) {
		origin.x = (float)origin.x - 1.0f;
		origin.y = (float)origin.y - 1.0f;
	}
	[[self splitterView] setFrameOrigin:origin];
	NSScroller *horizontal = [self hasHorizontalScroller] ? [self horizontalScroller] : nil;
	NSScroller *vertical = [self hasVerticalScroller] ? [self verticalScroller] : nil;
	if (horizontal != nil) {
		NSRect r = [horizontal frame];
		[horizontal setFrameSize:NSMakeSize((float)origin.x - (float)r.origin.x, r.size.height)];
	}
	if (vertical != nil) {
		NSRect r = [vertical frame];
		[vertical setFrameSize:NSMakeSize(r.size.width, (float)origin.y - (float)r.origin.y)];
	}
}

- (void)setSplitterViewIsVisible:(BOOL)aVisible
{
	if ([self splitterViewIsVisible] == (aVisible != NO)) {
		return;
	}
	if (aVisible) {
		[self addSubview:[self splitterView]];
	}
	else {
		[[self splitterView] removeFromSuperview];
	}
	[self tile];
}

- (BOOL)splitterViewIsVisible
{
	return [[self splitterView] superview] == self;
}

// the original only checks its argument
- (void)setSplitterView:(id)aView
{
	NSParameterAssert(!aView || ([aView superview] == nil) || ([aView superview] == self));
}

- (id)splitterView
{
	if (_splitterView == nil) {
		_splitterView = [[SMKSplitter alloc] initWithFrame:NSZeroRect];
		[(id)_splitterView sizeToFit];
	}
	return _splitterView;
}

- (void)dealloc
{
	[_splitterView release];
	_splitterView = nil;
	[super dealloc];
}

@end
