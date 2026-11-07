#import "SMKGridClipView.h"

// The grid control's clip view: shows the placeholder string while the grid
// is empty and, with discreet scrolling, scrolls by whole cells.
@implementation SMKGridClipView

- (id)owningGridControl
{
	return [[self documentView] owningGridControl];
}

- (BOOL)copiesOnScroll
{
	return NO;
}

+ (id)contentViewWithFrame:(NSRect)aFrame
{
	return [[[self alloc] initWithFrame:aFrame] autorelease];
}

- (NSPoint)constrainScrollPoint:(NSPoint)aPoint
{
	NSPoint p = [super constrainScrollPoint:aPoint];
	id matrix = [self documentView];
	if ([[matrix owningGridControl] isDiscreetScrollingEnabled]) {
		NSSize cell = [matrix cellSize];
		p.x = (float)cell.width * ceilf((float)p.x / (float)cell.width);
		p.y = (float)cell.height * ceilf((float)p.y / (float)cell.height);
	}
	return p;
}

- (void)drawRect:(NSRect)aRect
{
	[super drawRect:aRect];
	id grid = [self owningGridControl];
	if ([grid dataSource] != nil && [[grid dataSource] numberOfItemsInGridControl:grid] != 0)
		return;
	[grid drawPlaceholderStringInRect:[self bounds]];
}

@end
