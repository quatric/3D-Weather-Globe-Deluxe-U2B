#import "WGCustomToolbarButtonCell.h"
#import "WGCustomToolbarMatrix.h"
#import "WGBalloonPaletteController.h"

// A toolbar button that can carry a popup indicator (a small button inside
// it, at the right of the image) opening a balloon palette with the
// delegate's content view. For accessibility it is a group of two cells.
@implementation WGCustomToolbarButtonCell

- (id)init
{
	self = [super init];
	if (self)
		_showPopupIndicator = YES;
	return self;
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self)
		_showPopupIndicator = YES;
	return self;
}

- (id)initTextCell:(id)aCell
{
	self = [super initTextCell:aCell];
	if (self)
		_showPopupIndicator = YES;
	return self;
}

- (id)initImageCell:(id)aCell
{
	self = [super initImageCell:aCell];
	if (self)
		_showPopupIndicator = YES;
	return self;
}

- (void)dealloc
{
	[_balloonController release];
	[_popupIndicatorButtonCell release];
	[_buttonCell release];
	[super dealloc];
}

- (id)popupWindowContentViewDelegate
{
	return _popupWindowContentViewDelegate;
}

// The first delegate makes the indicator cell and the accessibility cell
// standing for the button itself.
- (void)setPopupWindowContentViewDelegate:(id)aDelegate
{
	if (aDelegate && !_popupWindowContentViewDelegate) {
		NSImage *image = [NSImage imageNamed:@"MWT_popup_button_norm"];
		NSSize size = [image size];
		_popupIndicatorImageRect = NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, size.width, size.height);
		NSString *title = [self title];
		NSBundle *bundle = [NSBundle mainBundle];
		if (![title isEqualToString:[bundle localizedStringForKey:@"Settings" value:@"" table:nil]])
			title = [NSString stringWithFormat:[bundle localizedStringForKey:@"Options Button" value:@"" table:nil], title];
		_popupIndicatorButtonCell = [[WGAccessibilityCustomToolbarOptionsButtonCell alloc] init];
		[_popupIndicatorButtonCell setButtonType:NSButtonTypeMomentaryChange];
		[_popupIndicatorButtonCell setControlSize:NSControlSizeSmall];
		[_popupIndicatorButtonCell setTitle:title];
		[_popupIndicatorButtonCell setHighlightsBy:NSPushInCellMask];
		[_popupIndicatorButtonCell setImagePosition:NSImageOnly];
		[_popupIndicatorButtonCell setBordered:NO];
		[_popupIndicatorButtonCell setBezeled:NO];
		[_popupIndicatorButtonCell setImage:image];
		[_popupIndicatorButtonCell setAlternateImage:[NSImage imageNamed:@"MWT_popup_button_press"]];
		[_popupIndicatorButtonCell setTarget:self];
		[_popupIndicatorButtonCell setAction:@selector(showPopupWindow:)];
		_buttonCell = [[WGAccessibilityCustomToolbarButtonCell alloc] init];
		[_buttonCell setButtonType:NSButtonTypeToggle];
		[_buttonCell setTitle:[self title]];
		[_buttonCell setTarget:self];
		[_buttonCell setAction:[self action]];
	}
	_popupWindowContentViewDelegate = aDelegate;
}

- (BOOL)showPopupIndicator
{
	return _showPopupIndicator;
}

- (void)setShowPopupIndicator:(BOOL)aIndicator
{
	_showPopupIndicator = aIndicator;
}

- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	[super drawInteriorWithFrame:aFrame inView:aView];
	_cellFrame = aFrame;
	if (_popupIndicatorButtonCell && [self showPopupIndicator])
		[_popupIndicatorButtonCell drawInteriorWithFrame:[self popupIndicatorImageRectForBounds:aFrame] inView:aView];
}

// The matrix's clicked cell is the indicator when it was hit.
static id _clickedCell(id view)
{
	return [view isKindOfClass:[WGCustomToolbarMatrix class]] ? [view clickedCell] : nil;
}

- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp
{
	id clicked = _clickedCell(aView);
	if (clicked == _popupIndicatorButtonCell)
		return [clicked trackMouse:aMouse inRect:[self popupIndicatorImageRectForBounds:aRect] ofView:aView untilMouseUp:aUp];
	return [super trackMouse:aMouse inRect:aRect ofView:aView untilMouseUp:aUp];
}

// Pressing the indicator of a button that is on leaves the button as is.
- (void)highlight:(BOOL)aHighlight withFrame:(NSRect)aFrame inView:(id)aView
{
	id clicked = _clickedCell(aView);
	if (clicked != _popupIndicatorButtonCell) {
		[super highlight:aHighlight withFrame:aFrame inView:aView];
		return;
	}
	if ([self state] != NSControlStateValueOn || (!aHighlight && [self isHighlighted]))
		[super highlight:aHighlight withFrame:aFrame inView:aView];
	[clicked highlight:aHighlight withFrame:[self popupIndicatorImageRectForBounds:aFrame] inView:aView];
}

- (id)realCellAtPoint:(NSPoint)aPoint inRect:(NSRect)aRect ofView:(id)aView
{
	if (_popupIndicatorButtonCell && [self showPopupIndicator]
		&& NSPointInRect(aPoint, [self popupIndicatorImageRectForBounds:aRect]))
		return _popupIndicatorButtonCell;
	return self;
}

// The cell's frame in its matrix.
- (NSRect)bounds
{
	NSInteger row = -1, column = -1;
	NSMatrix *matrix = (NSMatrix *)[self controlView];
	[matrix getRow:&row column:&column ofCell:self];
	return [matrix cellFrameAtRow:row column:column];
}

// Turns the button on (sending its action) and shows the balloon under
// the indicator.
- (void)showPopupWindow:(id)aWindow
{
	if (!_popupWindowContentViewDelegate)
		return;
	if ([self state] != NSControlStateValueOn) {
		NSControl *control = (NSControl *)[self controlView];
		if ([control isKindOfClass:[NSControl class]]) {
			[self setState:NSControlStateValueOn];
			[control sendAction:[control action] to:[control target]];
		}
	}
	NSView *content = [_popupWindowContentViewDelegate popupWindowContentViewForCell:self];
	if (!content)
		return;
	NSView *controlView = [self controlView];
	NSRect rect = [controlView convertRect:[self popupIndicatorImageRectForBounds:_cellFrame] toView:nil];
	NSPoint point = NSMakePoint(rect.origin.x + rect.size.width * 0.5, rect.origin.y + rect.size.height);
	point = [[controlView window] convertRectToScreen:NSMakeRect(point.x, point.y, 0, 0)].origin;
	if (!_balloonController) {
		BOOL autoHidden = [_popupWindowContentViewDelegate isPopupWindowAutoHiddenForCell:self];
		_balloonController = [[WGBalloonPaletteController alloc] init];
		[_balloonController setIsAutoHidden:autoHidden];
	}
	[_balloonController setPalettePosition:point contentView:content];
	[_balloonController showBalloonPalette];
}

- (id)accessibilityAttributeNames
{
	return [[super accessibilityAttributeNames] arrayByAddingObject:NSAccessibilityChildrenAttribute];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityChildrenAttribute])
		return [NSArray arrayWithObjects:_buttonCell, _popupIndicatorButtonCell, nil];
	return [super accessibilityAttributeValue:aValue];
}

- (BOOL)accessibilityIsIgnored
{
	return YES;
}

// The indicator's size, right of the image (1.3 of its width in from the
// image's right edge), centered vertically.
- (NSRect)popupIndicatorImageRectForBounds:(NSRect)aBounds
{
	NSRect image = [self imageRectForBounds:aBounds];
	NSSize size = _popupIndicatorImageRect.size;
	return NSMakeRect(image.origin.x + image.size.width + size.width * -1.3,
		(image.size.height - size.height) * 0.5 + image.origin.y, size.width, size.height);
}

@end
