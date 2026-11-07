#import "SMKGridDrawView.h"

static NSString *_kMultipleItemsSelectedPlaceholderString;

// The grid control's preview of the selected item: its graphic content
// centered, or a placeholder for none or several.
@implementation SMKGridDrawView

+ (void)initialize
{
	_kMultipleItemsSelectedPlaceholderString = [[NSBundle bundleForClass:[SMKGridDrawView class]]
		localizedStringForKey:@"Grid Control Multiselection" value:@"" table:nil];
}

- (void)mouseDown:(id)aDown
{
	[[self owningGridControl] onMouseDown:aDown];
}

// drags the single selected item's image
- (void)mouseDragged:(id)aDragged
{
	NSArray *cells = [self dataCells];
	if (cells == nil || [cells count] != 1) {
		return;
	}
	SMKGridBaseCell *cell = [cells lastObject];
	NSImage *image = [cell cachedImageWithSize:[cell cachedImageSizeForSize:[cell graphicContentSize] bounds:[self bounds].size]];
	NSRect r = [self graphicsContentRectInBounds:[self bounds] cell:cell];
	[[self owningGridControl] dragImage:image imageOrigin:NSMakePoint(r.origin.x, NSMaxY(r)) source:self event:aDragged];
}

- (NSUInteger)draggingSourceOperationMaskForLocal:(BOOL)aLocal
{
	return NSDragOperationCopy;
}

- (void)drawRect:(NSRect)aRect
{
	NSArray *cells = [self dataCells];
	if (cells == nil || [cells count] == 0) {
		SMKGridControl *control = [self owningGridControl];
		if ([control dataSource] != nil && [[control dataSource] numberOfItemsInGridControl:control] != 0) {
			return;
		}
		[control drawPlaceholderStringInRect:[self bounds]];
	}
	else if ([cells count] == 1) {
		NSEnumerator *e = [cells objectEnumerator];
		SMKGridBaseCell *cell;
		while ((cell = [e nextObject]) != nil) {
			[cell updateContentIfNeeded];
			[cell drawCachingGraphicContentInRect:[self graphicsContentRectInBounds:[self bounds] cell:cell] intoCache:nil];
		}
	}
	else {
		SMKGridControl *control = [self owningGridControl];
		NSString *placeholder = [[control placeholderString] retain];
		[control setPlaceholderString:_kMultipleItemsSelectedPlaceholderString];
		[control drawPlaceholderStringInRect:[self bounds]];
		[control setPlaceholderString:placeholder];
		[placeholder release];
	}
}

- (void)resizeWithOldSuperviewSize:(NSSize)aSize
{
	[super resizeWithOldSuperviewSize:aSize];
}

// the cell's image size, centered in the bounds
- (NSRect)graphicsContentRectInBounds:(NSRect)aBounds cell:(id)aCell
{
	NSSize size = [aCell cachedImageSizeForSize:[aCell graphicContentSize] bounds:aBounds.size];
	NSRect r;
	r.origin.x = (float)trunc((double)((float)aBounds.size.width - (float)size.width) * 0.5) + aBounds.origin.x;
	r.origin.y = (float)trunc((double)((float)aBounds.size.height - (float)size.height) * 0.5) + aBounds.origin.y;
	r.size = size;
	return r;
}

- (id)dataCells
{
	return [[self owningGridControl] selectedItems];
}

// the grid control is the superview's superview
- (id)owningGridControl
{
	NSView *view = [[self superview] superview];
	return [view isKindOfClass:[SMKGridControl class]] ? view : nil;
}

- (BOOL)isFlipped
{
	return YES;
}

- (BOOL)acceptsFirstMouse:(id)aMouse
{
	return YES;
}

- (void)dealloc
{
	[super dealloc];
}

- (id)initWithFrame:(NSRect)aFrame
{
	return [super initWithFrame:aFrame];
}

@end
