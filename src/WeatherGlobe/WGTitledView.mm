#import "WGTitledView.h"
#import "WGSplitter.h"
#import "WGImageTextCell.h"

// The keys of the attributes a content view comes with.
NSString *_kWGViewOrderWeightKey = @"orderWeight";
NSString *_kWGViewTitleViewKey = @"titleView";
NSString *_kWGViewImageKey = @"image";
NSString *_kWGViewTitleKey = @"title";
NSString *_kWGViewMaxSizeKey = @"maxSize";
NSString *_kWGViewMinSizeKey = @"minSize";

@interface NSObject (WGTitledViewDelegate)
- (void)removeSubview:(id)aSubview;
@end

// A view under a title bar (the background cell's image and title, or a
// title view) with a splitter at its right end: dragging the bar resizes
// the view, double-clicking it asks the delegate to remove the view.
@implementation WGTitledView

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self) {
		[self setAutoresizesSubviews:YES];
		[self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		_backgroundCell = [WGImageTextCell new];
		_splitter = [[WGSplitter alloc] initWithFrame:aFrame];
		[self setupSplitter];
		[self addSubview:_splitter];
		[self initBackgroundCell];
	}
	return self;
}

- (void)dealloc
{
	[_splitter release];
	[_backgroundCell release];
	[super dealloc];
}

- (void)setFrameSize:(NSSize)aSize
{
	[super setFrameSize:aSize];
	[[self window] invalidateCursorRectsForView:self];
}

- (void)drawRect:(NSRect)aRect
{
	[_backgroundCell drawWithFrame:[self titleRect] inView:self];
}

- (void)viewDidMoveToSuperview
{
	if ([self superview])
		[self setupSplitter];
	[super viewDidMoveToSuperview];
}

// The view goes when its content does.
- (void)willRemoveSubview:(id)aSubview
{
	if (_contentView == aSubview) {
		_contentView = nil;
		[self removeFromSuperview];
	}
	[super willRemoveSubview:aSubview];
}

- (void)mouseDown:(id)aDown
{
	NSPoint point = [self convertPoint:[aDown locationInWindow] fromView:nil];
	if (!NSPointInRect(point, [self titleRect]))
		return;
	if ([aDown clickCount] < 2)
		[_splitter mouseDown:aDown];
	else if ([delegate respondsToSelector:@selector(removeSubview:)])
		[delegate removeSubview:self];
}

- (void)resetCursorRects
{
	[self addCursorRect:[self titleRect] cursor:[NSCursor resizeUpDownCursor]];
}

// The content's sizes with the title bar.
- (NSSize)wgMinSize
{
	return NSMakeSize(_minContentViewSize.width, _minContentViewSize.height + [self titleRect].size.height);
}

- (NSSize)wgMaxSize
{
	CGFloat height = _maxContentViewSize.height;
	if (height != 0.0)
		height += [self titleRect].size.height;
	return NSMakeSize(_maxContentViewSize.width, height);
}

- (NSUInteger)orderWeight
{
	return _orderWeight;
}

- (id)contentView
{
	return _contentView;
}

- (id)splitter
{
	return _splitter;
}

- (id)backgroundCell
{
	return _backgroundCell;
}

- (id)titleView
{
	return _titleView;
}

- (void)setDelegate:(id)aDelegate
{
	delegate = aDelegate;
}

// The view takes the content's width and its height plus the title bar's.
- (void)setContentView:(id)aView attributes:(id)aAttributes
{
	if (_contentView == aView)
		return;
	NSString *image = [aAttributes objectForKey:_kWGViewImageKey];
	[_backgroundCell setImage:image ? [NSImage imageNamed:image] : nil];
	[_backgroundCell setTitle:[[NSBundle mainBundle] localizedStringForKey:[aAttributes objectForKey:_kWGViewTitleKey]
		value:@"" table:nil]];
	NSString *size = [aAttributes objectForKey:_kWGViewMinSizeKey];
	_minContentViewSize = size ? NSSizeFromString(size) : NSZeroSize;
	size = [aAttributes objectForKey:_kWGViewMaxSizeKey];
	_maxContentViewSize = size ? NSSizeFromString(size) : NSZeroSize;
	_orderWeight = [[aAttributes objectForKey:_kWGViewOrderWeightKey] intValue];
	[self setTitleView:[aAttributes objectForKey:_kWGViewTitleViewKey]];
	NSSize contentSize = [aView frame].size;
	[aView setFrame:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, contentSize.width, contentSize.height)];
	[self setFrame:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, contentSize.width,
		contentSize.height + [self titleRect].size.height)];
	[aView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable | NSViewMinYMargin];
	if (_contentView)
		[self replaceSubview:_contentView with:aView];
	else
		[self addSubview:aView];
	_contentView = aView;
}

// The title view sits at the top left, under the splitter.
- (void)setTitleView:(id)aView
{
	if (aView && [[self subviews] containsObject:aView])
		return;
	[_titleView removeFromSuperview];
	_titleView = aView;
	NSRect frame = [aView frame];
	[_titleView setFrameOrigin:NSMakePoint(0.0, [self bounds].size.height - frame.size.height)];
	if (_titleView)
		[self addSubview:_titleView positioned:NSWindowBelow relativeTo:_splitter];
}

// The bar at the top: as high as the image, or the title view if higher.
- (NSRect)titleRect
{
	CGFloat height = NSZeroRect.size.height;
	if ([_backgroundCell image])
		height = [[_backgroundCell image] size].height;
	if (_titleView && height <= [_titleView frame].size.height)
		height = [_titleView frame].size.height;
	NSRect bounds = [self bounds];
	return NSMakeRect(NSZeroRect.origin.x, bounds.size.height - height, bounds.size.width, height);
}

- (void)initBackgroundCell
{
	[_backgroundCell setControlSize:NSControlSizeSmall];
	[_backgroundCell setImageScaling:NSImageScaleAxesIndependently];
	[_backgroundCell setFont:[NSFont boldSystemFontOfSize:11.0]];
	[_backgroundCell setTextColor:[NSColor whiteColor]];
	[_backgroundCell setSecondaryTextColor:[[NSColor blackColor] colorWithAlphaComponent:0.5]];
	[_backgroundCell setEngraveOffset:-1.0];
	[_backgroundCell setWraps:YES];
	[_backgroundCell setAlignment:NSTextAlignmentCenter];
	if ([_backgroundCell respondsToSelector:@selector(setLineBreakMode:)])
		[_backgroundCell setLineBreakMode:NSLineBreakByTruncatingTail];
}

// The splitter is 2 points from the right, centered in the title bar.
- (void)setupSplitter
{
	[_splitter setSplitterImage:[NSImage imageNamed:[[_splitter enclosingSplitView] isVertical] ? @"sp" : @"sp-h"]];
	[_splitter sizeToFit];
	[_splitter setAutoresizingMask:NSViewMinXMargin | NSViewMinYMargin];
	NSRect frame = [_splitter frame];
	NSRect bounds = [self bounds];
	CGFloat x = bounds.size.width - frame.size.width - 2.0;
	CGFloat y = (bounds.size.height - frame.size.height) + ([self titleRect].size.height - frame.size.height) * -0.5;
	[_splitter setFrameOrigin:NSMakePoint(x, y)];
}

@end
