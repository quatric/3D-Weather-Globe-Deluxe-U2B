#import "WGTextFieldCell.h"

NSString *_kEditCellDidStartEditingNotification = @"EditCellDidStartEditingNotification";

@implementation WGTextFieldCell

// A copy of a text field cell.
- (id)initWithCell:(id)aCell
{
	NSTextFieldCell *source = aCell;
	self = [super init];
	if (self) {
		[self setType:[source type]];
		[self setFont:[source font]];
		[self setFormatter:[source formatter]];
		[self setEnabled:[source isEnabled]];
		[self setContinuous:[source isContinuous]];
		[self setEditable:[source isEditable]];
		[self setSelectable:[source isSelectable]];
		[self setBordered:[source isBordered]];
		[self setBezeled:[source isBezeled]];
		[self setScrollable:[source isScrollable]];
		[self setHighlighted:[source isHighlighted]];
		[self setAlignment:[source alignment]];
		[self setWraps:[source wraps]];
		[self setTarget:[source target]];
		[self setAction:[source action]];
		[self setTag:[source tag]];
		[self setShowsFirstResponder:[source showsFirstResponder]];
		[self setFocusRingType:[source focusRingType]];
		[self setObjectValue:[source objectValue]];
		[self setTextColor:[source textColor]];
	}
	return self;
}

- (void)selectWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate start:(NSInteger)aStart length:(NSInteger)aLength
{
	[super selectWithFrame:aFrame inView:aView editor:aEditor delegate:aDelegate start:aStart length:aLength];
	[[NSNotificationCenter defaultCenter] postNotificationName:_kEditCellDidStartEditingNotification object:aView];
}

- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView
{
	[aView setKeyboardFocusRingNeedsDisplayInRect:[aView bounds]];
	[super drawWithFrame:aFrame inView:aView];
}

@end
