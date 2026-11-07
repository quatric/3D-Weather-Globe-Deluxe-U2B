#import "WBTableRowsLayout.h"
#import "HSScrollView.h"

// Variable row heights for a table (or outline) view. The rows are kept as
// runs of equal height in _layoutInfo, an array of _layout_info_t:
//	f0 first row, f1 row count, f2 origin of the first row,
//	f3 row height, f4 state, f5 padding.
// Runs start out estimated from the table's row height; the heights of the
// rows around the visible area are measured from the cells in short time
// slices, and a measured run is applied to the origins of the runs below it
// once it is displayed.

enum {
	STATE_ESTIMATED = 0,
	STATE_CALCULATED = 1,
	STATE_DISPLAYED = 2
};

typedef struct _layout_info_t layout_info_t;

@implementation WBTableRowsLayout

- (id)outlineView
{
	return [_tableView isKindOfClass:[NSOutlineView class]] ? _tableView : nil;
}

static int _defaultRowHeight(NSTableView *tableView)
{
	return (int)([tableView intercellSpacing].height + [tableView rowHeight]);
}

// One estimated run over all the rows.
- (void)_invalidateLayout
{
	NSInteger count = [_tableView numberOfRows];
	if (!_layoutInfo)
		_layoutInfo = [[NSMutableData alloc] initWithCapacity:16000];
	[_layoutInfo setLength:(NSUInteger)(count << 4) / 3];
	[_layoutInfo setLength:sizeof(layout_info_t)];
	layout_info_t *p = (layout_info_t *)[_layoutInfo mutableBytes];
	bzero(p, sizeof(layout_info_t));
	p->f1 = (int)count;
	p->f3 = (unsigned short)_defaultRowHeight(_tableView);
	_tableHeight = (float)(int)(p->f3 * p->f1);
}

// Marks a calculated run displayed, moving the runs below by what it grew.
- (void)_displayRowsAtIndex:(NSInteger)aIndex
{
	layout_info_t *p = (layout_info_t *)[_layoutInfo bytes];
	NSInteger count = [_layoutInfo length] / sizeof(layout_info_t);
	NSParameterAssert(aIndex >= 0 && aIndex < count);
	NSParameterAssert(p[aIndex].f4 == STATE_CALCULATED);
	p[aIndex].f4 = STATE_DISPLAYED;
	int height = _defaultRowHeight(_tableView);
	if (height < (int)p[aIndex].f3) {
		int delta = ((int)p[aIndex].f3 - height) * p[aIndex].f1;
		for (NSInteger i = aIndex + 1; i < count; i++)
			p[i].f2 += delta;
		_tableHeight = (float)delta + _tableHeight;
	}
}

// The run holding a row, or -1.
- (NSInteger)_indexAtRow:(NSInteger)aRow
{
	const layout_info_t *p = (const layout_info_t *)[_layoutInfo bytes];
	NSInteger count = [_layoutInfo length] / sizeof(layout_info_t);
	if (!count || aRow < 0 || aRow >= p[count - 1].f0 + p[count - 1].f1)
		return -1;
	NSInteger low = 0, high = count - 1, i = 0;
	while (low <= high) {
		i = (low + high) >> 1;
		if (aRow < p[i].f0)
			high = i - 1;
		else if (aRow < p[i].f0 + p[i].f1)
			break;
		else
			low = i + 1;
	}
	if (i >= count || i < 0)
		return -1;
	NSParameterAssert(aRow >= p[i].f0 && aRow < p[i].f0 + p[i].f1);
	return i;
}

// The run whose rows cover a y, or -1.
- (NSInteger)_indexAtOrigin:(NSInteger)aOrigin
{
	const layout_info_t *p = (const layout_info_t *)[_layoutInfo bytes];
	NSInteger count = [_layoutInfo length] / sizeof(layout_info_t);
	if (!count)
		return -1;
	NSInteger low = 0, high = count - 1, i = 0;
	while (low <= high) {
		i = (low + high) >> 1;
		if (aOrigin < p[i].f2)
			high = i - 1;
		else if (i >= count - 1 || aOrigin < p[i + 1].f2)
			break;
		else
			low = i + 1;
	}
	if (i < count && i >= 0)
		return i;
	return -1;
}

// Puts a calculated run in place of the rows it covers: joined to the run
// before when that has the same height, else splitting the run it falls in.
// Returns the index of the run that holds it.
- (NSInteger)_storeLayoutInfo:(struct _layout_info_t *)aInfo
{
	if (aInfo->f1 < 1)
		return -1;
	NSParameterAssert(aInfo->f4 == STATE_CALCULATED);
	layout_info_t *p = (layout_info_t *)[_layoutInfo mutableBytes];
	NSInteger count = [_layoutInfo length] / sizeof(layout_info_t);
	NSInteger last = count - 1;
	NSInteger i = last;
	if (aInfo->f0 < p[i].f0 || aInfo->f0 >= p[i].f0 + p[i].f1) {
		NSInteger low = 0, high = last;
		while (low <= high) {
			i = (low + high) >> 1;
			if (aInfo->f0 < p[i].f0)
				high = i - 1;
			else if (aInfo->f0 < p[i].f0 + p[i].f1)
				break;
			else
				low = i + 1;
		}
	}
	NSParameterAssert(i >= 0 && i < count);
	NSParameterAssert(p[i].f4 != STATE_DISPLAYED);
	NSParameterAssert(aInfo->f0 >= p[i].f0 && aInfo->f0 < p[i].f0 + p[i].f1);
	NSParameterAssert(aInfo->f0 + aInfo->f1 <= p[i].f0 + p[i].f1);
	if (aInfo->f0 == p[i].f0) {
		if (i > 0 && aInfo->f3 == p[i - 1].f3 && aInfo->f4 == p[i - 1].f4) {
			p[i].f1 -= aInfo->f1;
			p[i].f0 = aInfo->f0 + aInfo->f1;
			p[i - 1].f1 += aInfo->f1;
			p[i].f2 += p[i].f3 * aInfo->f1;
			if (!p[i].f1) {
				if (i < last)
					memmove(&p[i], &p[i + 1], (count - i - 1) * sizeof(layout_info_t));
				[_layoutInfo setLength:(count - 1) * sizeof(layout_info_t)];
			}
			return i - 1;
		}
		if (aInfo->f1 == p[i].f1) {
			p[i].f3 = aInfo->f3;
			p[i].f4 = aInfo->f4;
			return i;
		}
		aInfo->f2 = p[i].f2;
		p[i].f1 -= aInfo->f1;
		p[i].f0 = aInfo->f0 + aInfo->f1;
		p[i].f2 += p[i].f3 * aInfo->f1;
		[_layoutInfo setLength:(count + 1) * sizeof(layout_info_t)];
		p = (layout_info_t *)[_layoutInfo mutableBytes];
		memmove(&p[i + 1], &p[i], (count - i) * sizeof(layout_info_t));
		p[i] = *aInfo;
		return i;
	}
	// Split into the head of the run, the new run, and the rest if any.
	layout_info_t head = p[i];
	head.f1 = aInfo->f0 - p[i].f0;
	int origin = head.f1 * head.f3 + head.f2;
	p[i].f1 -= head.f1 + aInfo->f1;
	p[i].f0 = head.f0 + head.f1 + aInfo->f1;
	p[i].f2 = head.f3 * aInfo->f1 + origin;
	[_layoutInfo setLength:(count + 1) * sizeof(layout_info_t)];
	p = (layout_info_t *)[_layoutInfo mutableBytes];
	memmove(&p[i + 1], &p[i], (count - i) * sizeof(layout_info_t));
	p[i] = head;
	if (p[i + 1].f1 > 0) {
		[_layoutInfo setLength:(count + 2) * sizeof(layout_info_t)];
		p = (layout_info_t *)[_layoutInfo mutableBytes];
		memmove(&p[i + 2], &p[i + 1], (count - i) * sizeof(layout_info_t));
	}
	p[i + 1].f5 = aInfo->f5;
	p[i + 1].f4 = aInfo->f4;
	p[i + 1].f3 = aInfo->f3;
	p[i + 1].f0 = aInfo->f0;
	p[i + 1].f1 = aInfo->f1;
	p[i + 1].f2 = origin;
	return i + 1;
}

// Undisplayed runs still count at the table's row height.
- (BOOL)_getOrigin:(float *)aOrigin height:(float *)aHeight ofRow:(NSInteger)aRow
{
	const layout_info_t *p = (const layout_info_t *)[_layoutInfo bytes];
	NSInteger i = [self _indexAtRow:aRow];
	if (i < 0)
		return NO;
	p += i;
	unsigned int height = p->f4 == STATE_DISPLAYED ? p->f3 : (unsigned int)_defaultRowHeight(_tableView);
	if (aOrigin) {
		*aOrigin = (float)p->f2;
		if (p->f0 < aRow)
			*aOrigin = (float)p->f2 + (float)(int)(height * (aRow - p->f0));
	}
	if (aHeight)
		*aHeight = (float)(int)height;
	return YES;
}

- (NSInteger)_rowAtPoint:(NSPoint)aPoint
{
	const layout_info_t *p = (const layout_info_t *)[_layoutInfo bytes];
	int y = (int)ceilf(aPoint.y);
	NSInteger i = [self _indexAtOrigin:y];
	if (i < 0)
		return -1;
	p += i;
	int height = p->f4 == STATE_DISPLAYED ? p->f3 : _defaultRowHeight(_tableView);
	NSInteger row = (y - p->f2) / height + p->f0;
	if (row >= 0 && row < [_tableView numberOfRows])
		return row;
	return -1;
}

- (void)_postDisplayRows:(id)aRows
{
	if (aRows) {
		[_tableView scrollPoint:[aRows pointValue]];
		[aRows release];
	}
	[_tableView setNeedsDisplay:YES];
}

// Measures and displays the runs down through the rect, keeping the top
// visible row in place. Returns whether the layout changed.
- (BOOL)_displayRowsInRect:(NSRect)aRect
{
	static BOOL recursion = NO;
	BOOL changed = YES;
	if (recursion)
		return changed;
	recursion = YES;
	NSScrollView *scrollView = [_tableView enclosingScrollView];
	NSRect visible = [scrollView documentVisibleRect];
	NSInteger topRow = [_tableView rowAtPoint:visible.origin];
	NSRect topRect = [_tableView rectOfRow:topRow];
	if (_invalidateLayout) {
		[self _invalidateLayout];
		_invalidateLayout = NO;
	}
	const layout_info_t *bytes = (const layout_info_t *)[_layoutInfo bytes];
	NSInteger row = [self _rowAtPoint:aRect.origin];
	int bottom = (int)(aRect.size.height + aRect.origin.y);
	NSInteger count = [_tableView numberOfRows];
	changed = row < 0;
	while (row >= 0 && row < count) {
		NSInteger i = [self _indexAtRow:row];
		if (i < 0)
			break;
		const layout_info_t *p = &bytes[i];
		if (bottom < p->f2)
			break;
		if (p->f4 == STATE_ESTIMATED) {
			NSInteger first = p->f0;
			int rows = 2;
			int y = (int)(row - first) * p->f3 + p->f2;
			if (y < bottom)
				rows = (bottom - y) / p->f3 + 2;
			int half = rows / 2;
			row -= half;
			if (row < first)
				row = first;
			[self _calcHeightsInRange:NSMakeRange(row, rows + half) timeout:0.1 display:NO];
			bytes = (const layout_info_t *)[_layoutInfo bytes];
		} else if (p->f4 == STATE_CALCULATED) {
			[self _displayRowsAtIndex:i];
			row = p->f0 + p->f1;
			changed = YES;
		} else {
			row = p->f0 + p->f1;
		}
	}
	if (changed) {
		[_tableView tile];
		if (topRow >= 0) {
			NSRect rect = [_tableView rectOfRow:topRow];
			[[scrollView contentView] scrollToPoint:NSMakePoint(visible.origin.x,
				visible.origin.y - topRect.origin.y + rect.origin.y)];
		}
		if (count > 0)
			[[NSRunLoop currentRunLoop] performSelector:@selector(_postDisplayRows:) target:self argument:nil
				order:0 modes:[NSArray arrayWithObject:(NSString *)kCFRunLoopCommonModes]];
	}
	recursion = NO;
	return changed;
}

- (void)_columnDidResize:(id)aResize
{
	[self resetLayout];
	if ([_tableView window] && ![_tableView inLiveResize])
		[self displayVisibleRows];
}

- (void)_reflectScrolledView:(id)aView
{
	if ([_tableView window] && ![_tableView inLiveResize])
		[self displayVisibleRows];
}

- (void)_outlineViewItemDidExpand:(id)aExpand
{
	[self resetLayout];
}

- (void)_outlineViewItemDidCollapse:(id)aCollapse
{
	[self resetLayout];
}

- (void)_becomeObserver
{
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center addObserver:self selector:@selector(_columnDidResize:) name:NSTableViewColumnDidResizeNotification
		object:_tableView];
	[center addObserver:self selector:@selector(_columnDidResize:) name:NSOutlineViewColumnDidResizeNotification
		object:_tableView];
	[center addObserver:self selector:@selector(_outlineViewItemDidExpand:)
		name:NSOutlineViewItemDidExpandNotification object:_tableView];
	[center addObserver:self selector:@selector(_outlineViewItemDidCollapse:)
		name:NSOutlineViewItemDidCollapseNotification object:_tableView];
	NSScrollView *scrollView = [_tableView enclosingScrollView];
	if ([scrollView isKindOfClass:[HSScrollView class]])
		[center addObserver:self selector:@selector(_reflectScrolledView:)
			name:HSScrollViewReflectScrolledClipViewNotification object:scrollView];
}

- (void)_setColumnsWrap:(BOOL)aWrap
{
	NSArray *columns = [_tableView tableColumns];
	NSInteger count = [columns count];
	for (NSInteger i = 0; i < count; i++)
		[[[columns objectAtIndex:i] dataCell] setWraps:aWrap];
}

- (void)resetLayout
{
	_invalidateLayout = YES;
}

- (BOOL)displayVisibleRows
{
	NSScrollView *scrollView = [_tableView enclosingScrollView];
	NSRect rect = scrollView ? [scrollView documentVisibleRect] : [_tableView bounds];
	if (![_tableView window] || [_tableView inLiveResize])
		return NO;
	return [self _displayRowsInRect:rect];
}

// A visible height above and below the row.
- (void)displayRow:(NSInteger)aRow
{
	NSRect visible = [_tableView visibleRect];
	NSRect rect = [_tableView rectOfRow:aRow];
	float y = 0.0f;
	if (0.0f <= rect.origin.y - visible.size.height)
		y = rect.origin.y - visible.size.height;
	[self _displayRowsInRect:NSMakeRect(rect.origin.x, y, rect.size.width,
		visible.size.height + visible.size.height + rect.size.height)];
}

- (NSRect)correctRect:(NSRect)aRect ofColumn:(NSInteger)aColumn
{
	CGFloat height = aRect.size.height;
	if (0.0 < aRect.size.height && _tableHeight > aRect.size.height)
		height = _tableHeight;
	aRect.size.height = height;
	return aRect;
}

- (NSRect)correctRect:(NSRect)aRect ofRow:(NSInteger)aRow
{
	if (0.0 < aRect.size.height) {
		float origin = aRect.origin.y, height = aRect.size.height;
		[self _getOrigin:&origin height:&height ofRow:aRow];
		aRect.origin.y = origin;
		aRect.size.height = height;
	}
	return aRect;
}

- (NSInteger)correctRow:(NSInteger)aRow atPoint:(NSPoint)aPoint
{
	return [self _rowAtPoint:aPoint];
}

// The row's rect less the intercell spacing.
- (NSRect)correctFrame:(NSRect)aFrame ofCellAtColumn:(NSInteger)aColumn row:(NSInteger)aRow
{
	if (0.0 < aFrame.size.height) {
		float origin = aFrame.origin.y, height = aFrame.size.height;
		if ([self _getOrigin:&origin height:&height ofRow:aRow]) {
			float spacing = [_tableView intercellSpacing].height;
			origin = ceilf(spacing * 0.5f) + origin;
			height = height - ceilf(spacing);
		}
		aFrame.origin.y = origin;
		aFrame.size.height = height;
	}
	return aFrame;
}

- (id)initWithTableView:(id)aView
{
	self = [super init];
	if (self) {
		_invalidateLayout = YES;
		_tableView = aView;
		[self _becomeObserver];
		[self _setColumnsWrap:YES];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_layoutInfo release];
	[super dealloc];
}

// The grid lines along the laid-out rows: one at the top of each row and
// one at the right of each column but the last.
- (void)drawGridInClipRect:(NSRect)aRect
{
	if (![_tableView respondsToSelector:@selector(gridStyleMask)])
		return;
	NSTableViewGridLineStyle mask = [_tableView gridStyleMask];
	NSColor *color = [_tableView gridColor];
	if (mask & NSTableViewSolidHorizontalGridLineMask) {
		NSInteger row = [_tableView rowAtPoint:aRect.origin];
		NSInteger count = [_tableView numberOfRows];
		if (row >= 0) {
			for (; row < count; row++) {
				NSRect rect = [_tableView rectOfRow:row];
				if (aRect.origin.y + aRect.size.height < rect.origin.y)
					break;
				[color set];
				NSFrameRect(NSMakeRect(rect.origin.x, rect.origin.y, rect.size.width, 1.0));
			}
		}
	}
	if (mask & NSTableViewSolidVerticalGridLineMask) {
		NSInteger count = [[_tableView tableColumns] count];
		for (NSInteger i = 0; i < count - 1; i++) {
			NSRect rect = [_tableView rectOfColumn:i];
			CGFloat right = rect.size.width + rect.origin.x;
			if (aRect.origin.x <= right && right <= aRect.origin.x + aRect.size.width) {
				[color set];
				NSFrameRect(NSMakeRect(rect.origin.x + (rect.size.width - 1.0), rect.origin.y, 1.0, rect.size.height));
			}
		}
	}
}

// The alternating row colors along the laid-out rows (by level in an
// outline view).
- (void)drawBackgroundInClipRect:(NSRect)aRect
{
	[[NSColor controlBackgroundColor] set];
	NSRectFill(aRect);
	if (![NSColor respondsToSelector:@selector(controlAlternatingRowBackgroundColors)])
		return;
	NSOutlineView *outlineView = [self outlineView];
	NSArray *colors = [NSColor controlAlternatingRowBackgroundColors];
	NSInteger colorCount = [colors count];
	NSInteger row = [_tableView rowAtPoint:aRect.origin];
	NSInteger count = [_tableView numberOfRows];
	if (row < 0)
		return;
	for (; row < count; row++) {
		NSRect rect = [_tableView rectOfRow:row];
		if (aRect.size.height + aRect.origin.y < rect.origin.y)
			return;
		NSInteger index = outlineView ? [outlineView levelForRow:row] : row;
		[(NSColor *)[colors objectAtIndex:index % colorCount] set];
		NSRectFill(rect);
	}
}

// Measures the rows of the range from their cells (until the timeout),
// storing each run of equal heights; with display, the stored runs that are
// visible are displayed at once. Rows already measured are skipped.
- (void)_calcHeightsInRange:(NSRange)aRange timeout:(double)aTimeout display:(BOOL)aDisplay
{
	NSRect visible = [[_tableView enclosingScrollView] documentVisibleRect];
	id dataSource = [_tableView dataSource];
	NSInteger count = [_tableView numberOfRows];
	NSOutlineView *outlineView = [self outlineView];
	float spacing = [_tableView intercellSpacing].height;
	id delegate = [_tableView delegate];
	NSArray *columns = [_tableView tableColumns];
	NSInteger columnCount = [columns count];
	NSRect *columnRects = (NSRect *)malloc(columnCount * sizeof(NSRect));
	float rowHeight = [_tableView rowHeight];
	NSInteger end = MIN((NSInteger)(aRange.location + aRange.length), count);
	NSTableColumn *outlineColumn = nil;
	int indentation = 0;
	BOOL willDisplay;
	if (!outlineView) {
		willDisplay = [delegate respondsToSelector:@selector(tableView:willDisplayCell:forTableColumn:row:)];
	} else {
		outlineColumn = [outlineView outlineTableColumn];
		indentation = (int)[outlineView indentationPerLevel];
		willDisplay = [delegate respondsToSelector:@selector(outlineView:willDisplayCell:forTableColumn:item:)];
	}
	NSTimeInterval start = [NSDate timeIntervalSinceReferenceDate];
	layout_info_t info;
	bzero(&info, sizeof(info));
	for (NSInteger i = 0; i < columnCount; i++) {
		NSRect rect = [_tableView rectOfColumn:i];
		rect.size.height = 10000.0;
		columnRects[i] = rect;
	}
	int remaining = 1;
	for (NSInteger row = aRange.location; row < count && row < end; row++) {
		id item = outlineView ? [outlineView itemAtRow:row] : nil;
		float height = rowHeight;
		for (NSInteger c = 0; c < columnCount; c++) {
			NSTableColumn *column = [columns objectAtIndex:c];
			NSCell *cell = [column dataCellForRow:row];
			NSRect rect = columnRects[c];
			id value;
			if (!outlineView)
				value = [dataSource tableView:_tableView objectValueForTableColumn:column row:row];
			else
				value = [dataSource outlineView:outlineView objectValueForTableColumn:column byItem:item];
			[cell setObjectValue:value];
			if (willDisplay) {
				if (!outlineView)
					[delegate tableView:_tableView willDisplayCell:cell forTableColumn:column row:row];
				else
					[delegate outlineView:outlineView willDisplayCell:cell forTableColumn:column item:item];
			}
			if (outlineColumn == column)
				rect.size.width -= (float)(([outlineView levelForItem:item] + 1) * indentation);
			float cellHeight = [cell cellSizeForBounds:rect].height;
			if (cellHeight > height)
				height = cellHeight;
		}
		float rounded = ceilf(spacing + height);
		if (rounded != (float)info.f3 || remaining <= info.f1) {
			if (info.f1) {
				NSInteger stored = [self _storeLayoutInfo:&info];
				if (stored >= 0 && aDisplay) {
					const layout_info_t *p = (const layout_info_t *)[_layoutInfo mutableBytes] + stored;
					NSRect rect = NSMakeRect(visible.origin.x, (float)p->f2, visible.size.width,
						(float)(int)(p->f3 * p->f1));
					if (NSIntersectsRect(visible, rect))
						[self _displayRowsAtIndex:stored];
				}
			}
			const layout_info_t *bytes = (const layout_info_t *)[_layoutInfo mutableBytes];
			NSInteger index = [self _indexAtRow:row];
			if (index < 0)
				break;
			const layout_info_t *p = &bytes[index];
			remaining = p->f1 - ((int)row - p->f0);
			if (p->f4 == STATE_ESTIMATED) {
				info.f1 = 1;
				info.f4 = STATE_CALCULATED;
				info.f0 = (int)row;
				info.f3 = (unsigned short)(int)rounded;
			} else {
				bzero(&info, sizeof(info));
				row = p->f0 + p->f1 - 1;
			}
		} else {
			info.f1++;
		}
		if (start + aTimeout < [NSDate timeIntervalSinceReferenceDate])
			break;
	}
	if (info.f1)
		[self _storeLayoutInfo:&info];
	free(columnRects);
}

@end
