#import "WBTableView.h"
#import "WBTableRowsLayout.h"
#import "NSEvent+WBAdditions.h"

@interface NSObject (WBTableViewDelegate)
- (BOOL)tableView:(id)aView shouldProcessKeyDown:(id)aEvent;
- (BOOL)tableView:(id)aView shouldProcessClick:(id)aEvent atRow:(NSInteger)aRow;
@end

// A table whose rows can have their own heights (laid out by a
// WBTableRowsLayout), Enter and the accessibility press sending the double
// action, Home and End scrolling, and a name for accessibility.
@implementation WBTableView

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	[self setVariableRowHeight:YES];
	return self;
}

- (void)dealloc
{
	[_name release];
	[_layout release];
	[super dealloc];
}

- (void)viewDidMoveToWindow
{
	[super viewDidMoveToWindow];
	if ([self window] && _variableRowHeight && !_layout)
		_layout = [[WBTableRowsLayout alloc] initWithTableView:self];
}

- (void)lockFocus
{
	if (_layout)
		[_layout displayVisibleRows];
	[super lockFocus];
}

- (void)reloadData
{
	[super reloadData];
	if (_layout)
		[_layout resetLayout];
}

// Scrolled again once the rows there are laid out.
- (void)scrollRowToVisible:(NSInteger)aVisible
{
	[super scrollRowToVisible:aVisible];
	if (_layout)
		[_layout displayVisibleRows];
	[super scrollRowToVisible:aVisible];
}

- (BOOL)isVariableRowHeight
{
	return _variableRowHeight;
}

- (void)setVariableRowHeight:(BOOL)aHeight
{
	if (_variableRowHeight == aHeight)
		return;
	_variableRowHeight = aHeight;
	if (aHeight) {
		[_layout resetLayout];
	} else {
		[_layout release];
		_layout = nil;
	}
}

- (NSRect)rectOfColumn:(NSInteger)aColumn
{
	NSRect rect = [super rectOfColumn:aColumn];
	if (_layout)
		rect = [_layout correctRect:rect ofColumn:aColumn];
	return rect;
}

- (NSRect)rectOfRow:(NSInteger)aRow
{
	NSRect rect = [super rectOfRow:aRow];
	if (_layout)
		rect = [_layout correctRect:rect ofRow:aRow];
	return rect;
}

- (NSInteger)rowAtPoint:(NSPoint)aPoint
{
	NSInteger row = [super rowAtPoint:aPoint];
	if (_layout)
		row = [_layout correctRow:row atPoint:aPoint];
	return row;
}

- (NSRect)frameOfCellAtColumn:(NSInteger)aColumn row:(NSInteger)aRow
{
	NSRect frame = [super frameOfCellAtColumn:aColumn row:aRow];
	if (_layout)
		frame = [_layout correctFrame:frame ofCellAtColumn:aColumn row:aRow];
	return frame;
}

- (void)drawBackgroundInClipRect:(NSRect)aRect
{
	if (_layout)
		[_layout drawBackgroundInClipRect:aRect];
}

- (void)drawGridInClipRect:(NSRect)aRect
{
	if (_layout)
		[_layout drawGridInClipRect:aRect];
	else
		[super drawGridInClipRect:aRect];
}

- (void)keyDown:(id)aDown
{
	id delegate = [self delegate];
	if (delegate && [delegate respondsToSelector:@selector(tableView:shouldProcessKeyDown:)]
		&& ![delegate tableView:self shouldProcessKeyDown:aDown])
		return;
	SEL action = [self doubleAction];
	if ([aDown isEnterDown] && action) {
		[[self target] performSelector:action withObject:self];
		return;
	}
	NSString *characters = [aDown characters];
	unichar key = [characters length] ? [characters characterAtIndex:0] : 0;
	if (key == NSHomeFunctionKey || key == NSEndFunctionKey)
		[self scrollRowToVisible:key == NSHomeFunctionKey ? 0 : [self numberOfRows] - 1];
	else
		[super keyDown:aDown];
}

- (void)mouseDown:(id)aDown
{
	id delegate = [self delegate];
	NSInteger row = [self rowAtPoint:[self convertPoint:[aDown locationInWindow] fromView:nil]];
	if (delegate && [delegate respondsToSelector:@selector(tableView:shouldProcessClick:atRow:)]
		&& ![delegate tableView:self shouldProcessClick:aDown atRow:row])
		return;
	[super mouseDown:aDown];
}

- (id)name
{
	return _name;
}

- (void)setName:(id)aName
{
	[aName retain];
	[_name release];
	_name = aName;
}

- (id)accessibilityAttributeNames
{
	return [[super accessibilityAttributeNames] arrayByAddingObjectsFromArray:
		[NSArray arrayWithObjects:NSAccessibilityTitleAttribute, nil]];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityTitleAttribute])
		return [self name];
	return [super accessibilityAttributeValue:aValue];
}

- (BOOL)accessibilityIsAttributeSettable:(id)aSettable
{
	if ([aSettable isEqualToString:NSAccessibilityTitleAttribute])
		return NO;
	return [super accessibilityIsAttributeSettable:aSettable];
}

- (id)accessibilityActionNames
{
	return [[super accessibilityActionNames] arrayByAddingObject:NSAccessibilityPressAction];
}

- (id)accessibilityActionDescription:(id)aDescription
{
	if ([aDescription isEqualToString:NSAccessibilityPressAction])
		return [[NSBundle mainBundle] localizedStringForKey:@"Open List Item Action" value:@"" table:nil];
	return [super accessibilityActionDescription:aDescription];
}

- (void)accessibilityPerformAction:(id)aAction
{
	if (![aAction isEqualToString:NSAccessibilityPressAction]) {
		[super accessibilityPerformAction:aAction];
		return;
	}
	SEL action = [self doubleAction];
	if (action)
		[[self target] performSelector:action withObject:self];
}

@end
