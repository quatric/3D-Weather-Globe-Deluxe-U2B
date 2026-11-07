// Original module: /Volumes/Private/Projects/X2/SMKReuseProjects/SMKControls/SMKControls_sources/Framework/GridControl/Views/SMKGridMatrix.m
#import "SMKGridMatrix.h"

static SMKShadow *__sDefaultShadow;
static NSMapTable *__sDefaultShadowCacheMap;

// The matrix inside a grid control. Each cell gets a content area of
// _cellContentSize inside a slot that is the border offset larger on every
// side. With threaded updating on, a background thread renders the thumbnails
// of the visible cells that asked for it (setNeedsUpdateCell:) and hands them
// back to the main thread.
@implementation SMKGridMatrix

- (void)_initVars
{
	_cellBorderOffset = NSMakeSize(3.0, 3.0);
	_useDefaultResize = NO;
	_extraCellCount = 0;
	_thumbnailMaxTitleHeight = 0.0f;
	_isThumbnailHyphenationEnabled = NO;
	_isGraphicContentShadowEnabled = YES;
	_cellTitleAdditionalHeight = 0.0f;
	_selectionRect = NSZeroRect;
	_controlSize = 0;
	_selectionStyle = 1;
	_isDrawingPermitted = YES;
	_isThumbnailCachingEnabled = YES;
	_numberOfColumns = 1;
	_cellHeightToWidthProportion = 1.0f;
	_isUpdaterRunning = NO;
	_isThreadedCellUpdatingStopped = YES;
	_isUpdaterThreadActive = NO;
	_owningGridControl = nil;
	_updateCellsLock = [[NSLock alloc] init];
	_cellTitleAdditionalHeightLock = [[NSLock alloc] init];
	_needsPanelToBecomeKey = !_acceptsFirstMouse;
	_updateCellsArray = [NSMutableArray new];
	_stringRenderer = [SMKStringRenderer new];
	[_stringRenderer setAlignment:NSTextAlignmentCenter];
	[_stringRenderer setHyphenationFactor:_isThumbnailHyphenationEnabled ? 1.0f : 0.0f];
}

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self != nil) {
		[self _initVars];
		[self setMode:NSRadioModeMatrix];
		[self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		[self setCellClass:[SMKGridBaseCell class]];
		[self setCellSize:[self cellContentSize]];
		[self setCellContentSize:NSMakeSize(150.0, 150.0)];
		[self setIntercellSpacing:NSZeroSize];
		[_stringRenderer setFont:[self textFont]];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_updateCellsArray removeAllObjects];
	[_updateCellsArray release];
	[_updateCellsLock release];
	[_loadingStatusImage release];
	[_stringRenderer release];
	[_cellTitleAdditionalHeightLock release];
	[_shadow release];
	if (_shadowCacheMap != NULL) {
		NSFreeMapTable(_shadowCacheMap);
	}
	[super dealloc];
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	[self _initVars];
	_cellTitleAdditionalHeight = [aCoder decodeFloatForKey:@"cellTitleAdditionalHeight"];
	_cellContentSize = [aCoder decodeSizeForKey:@"cellContentSize"];
	_extraCellCount = [aCoder decodeIntForKey:@"extraCellCount"];
	_useDefaultResize = [aCoder decodeBoolForKey:@"useDefaultResize"];
	_thumbnailMaxTitleHeight = [aCoder decodeFloatForKey:@"thumbnailMaxTitleHeight"];
	_isThumbnailHyphenationEnabled = [aCoder decodeBoolForKey:@"isThumbnailHyphenationEnabled"];
	_isGraphicContentShadowEnabled = [aCoder decodeBoolForKey:@"isGraphicContentShadowEnabled"];
	_cellHeightToWidthProportion = [aCoder decodeFloatForKey:@"cellHeightToWidthProportion"];
	_controlSize = [aCoder decodeIntForKey:@"controlSize"];
	_selectionStyle = [aCoder decodeIntForKey:@"selectionStyle"];
	_imageFrameColor = [aCoder decodeObjectForKey:@"imageFrameColor"];
	_isThumbnailCachingEnabled = [aCoder decodeBoolForKey:@"isThumbnailCachingEnabled"];
	[_stringRenderer setFont:[self textFont]];
	[_stringRenderer setHyphenationFactor:_isThumbnailHyphenationEnabled ? 1.0f : 0.0f];
	return self;
}

// As in the original, the proportion goes out as a Bool (its integer part).
- (void)encodeWithCoder:(id)aCoder
{
	[aCoder encodeFloat:_cellTitleAdditionalHeight forKey:@"cellTitleAdditionalHeight"];
	[aCoder encodeSize:_cellContentSize forKey:@"cellContentSize"];
	[aCoder encodeInt:_extraCellCount forKey:@"extraCellCount"];
	[aCoder encodeBool:_useDefaultResize forKey:@"useDefaultResize"];
	[aCoder encodeFloat:_thumbnailMaxTitleHeight forKey:@"thumbnailMaxTitleHeight"];
	[aCoder encodeBool:_isThumbnailHyphenationEnabled forKey:@"isThumbnailHyphenationEnabled"];
	[aCoder encodeInt:_controlSize forKey:@"controlSize"];
	[aCoder encodeInt:_selectionStyle forKey:@"selectionStyle"];
	[aCoder encodeObject:_imageFrameColor forKey:@"imageFrameColor"];
	[aCoder encodeBool:_isThumbnailCachingEnabled forKey:@"isThumbnailCachingEnabled"];
	[aCoder encodeBool:_isGraphicContentShadowEnabled forKey:@"isGraphicContentShadowEnabled"];
	[aCoder encodeBool:(BOOL)(int)_cellHeightToWidthProportion forKey:@"cellHeightToWidthProportion"];
	[super encodeWithCoder:aCoder];
}

- (BOOL)acceptsFirstMouse:(id)aMouse
{
	return _acceptsFirstMouse;
}

- (void)setAcceptsFirstMouse:(BOOL)aMouse
{
	_acceptsFirstMouse = aMouse;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)needsPanelToBecomeKey
{
	return _needsPanelToBecomeKey;
}

- (void)setNeedsPanelToBecomeKey:(BOOL)aKey
{
	_needsPanelToBecomeKey = aKey;
}

- (BOOL)becomeFirstResponder
{
	return YES;
}

- (void)viewDidMoveToWindow
{
	if ([self window] != nil) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onWindowBecomeResignKey:)
		                                             name:NSWindowDidBecomeKeyNotification object:[self window]];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onWindowBecomeResignKey:)
		                                             name:NSWindowDidResignKeyNotification object:[self window]];
	}
}

- (void)onWindowBecomeResignKey:(id)aKey
{
	[self _setNeedsDrawCells:[self selectedCells]];
}

- (NSUInteger)draggingSourceOperationMaskForLocal:(BOOL)aLocal
{
	return NSDragOperationCopy;
}

- (void)draggedImage:(id)aImage endedAt:(NSPoint)aAt operation:(NSUInteger)aOperation
{
	[_owningGridControl draggedImage:aImage endedAt:aAt operation:aOperation];
}

// Selects every item: the full rows, then the last row up to the item count.
- (void)selectAll:(id)aAll
{
	if ([self mode] != NSListModeMatrix) {
		return;
	}
	[self deselectAllCells];
	int lastRowStart = ((int)[self numberOfRows] - 1) * (int)[self numberOfColumns];
	if (lastRowStart > 0) {
		[self setSelectionFrom:0 to:lastRowStart - 1 anchor:0 highlight:YES];
	}
	[self setSelectionFrom:lastRowStart to:(int)[self count] - 1 anchor:lastRowStart highlight:YES];
}

- (BOOL)validateUserInterfaceItem:(id)aItem
{
	if ([aItem action] != @selector(selectAll:)) {
		return [_owningGridControl validateUserInterfaceItem:aItem];
	}
	return [self allowsMultipleSelection] && [self count] != 0;
}

// Single selection keeps the scroll position when selecting; multiple
// selection leaves modifier clicks to NSMatrix. A click outside an item's
// hit area clears the selection (when allowed). After the click the matrix
// waits for a drag and starts dragging the clicked item's thumbnail (or the
// grid control's stack image for several).
- (void)mouseDown:(id)aDown
{
	NSPoint point = [self convertPoint:[aDown locationInWindow] fromView:nil];
	NSInteger row, column;
	SMKGridBaseCell *cell = nil;
	if ([self getRow:&row column:&column forPoint:point]) {
		cell = (SMKGridBaseCell *)[self cellAtRow:row column:column];
	}
	if (![cell isEnabled] ||
	    ![cell hitTestPoint:point inFrame:[self cellFrameAtRow:row column:column] view:self]) {
		if ([self allowsEmptySelection]) {
			[self deselectAllCells];
		}
		_selectionStartPoint = point;
		return;
	}
	if ([self mode] == NSRadioModeMatrix) {
		if (![cell isEqual:[self selectedCell]]) {
			NSPoint origin = [[self enclosingScrollView] documentVisibleRect].origin;
			[self selectCellAtRow:row column:column];
			[[[self enclosingScrollView] contentView] scrollToPoint:origin];
		}
	} else {
		NSEventModifierFlags flags = [aDown modifierFlags];
		if ((flags & NSEventModifierFlagShift) || (flags & NSEventModifierFlagCommand) ||
		    (flags & NSEventModifierFlagOption)) {
			[super mouseDown:aDown];
			goto waitForDrag;
		}
		if (![[self selectedCells] containsObject:cell]) {
			[self selectCellAtRow:row column:column];
		}
	}
	if ([aDown clickCount] == 1) {
		[self sendAction];
	} else {
		[self sendDoubleAction];
	}
waitForDrag:;
	// The original passes 1 (NSLeftMouseDown, a type) as the mask, which
	// matches no event and would block for good; wait for a drag or the release.
	NSEvent *next = [[self window] nextEventMatchingMask:NSEventMaskLeftMouseDragged | NSEventMaskLeftMouseUp
	                                           untilDate:[NSDate distantFuture]
	                                              inMode:NSEventTrackingRunLoopMode
	                                             dequeue:YES];
	if (next == nil || [next type] != NSEventTypeLeftMouseDragged) {
		return;
	}
	NSRect frame = [self cellFrameAtRow:row column:column];
	NSRect interior = [cell interiorRectForFrame:frame inView:self];
	NSRect graphic = [cell graphicContentPlacementRectForBounds:interior];
	NSRect imageRect = [cell cachedImageRectForBounds:graphic];
	NSImage *image = [cell cachedImageWithSize:imageRect.size];
	if (image == nil) {
		image = [cell cacheTextImageForBounds:interior];
	}
	float height = imageRect.size.height;
	if ([[self selectedCells] count] > 1) {
		image = [[self owningGridControl] imageForMultipleDraggedItemsForImage:image];
		[image size];
		float stack = [image size].height;
		height = stack <= (float)imageRect.size.height ? (float)imageRect.size.height : stack;
	}
	[[self owningGridControl] dragImage:image imageOrigin:NSMakePoint(imageRect.origin.x, height + (float)imageRect.origin.y)
	                             source:self event:aDown];
}

// Rubber-band selection. Nothing in the shipped framework turns
// _isTrackingSelection on, so this stays idle as it did there.
- (void)mouseDragged:(id)aDragged
{
	if (!_isTrackingSelection) {
		return;
	}
	[self setNeedsDisplayInRect:_selectionRect];
	NSPoint point = [self convertPoint:[aDragged locationInWindow] fromView:nil];
	NSPoint start = _selectionStartPoint;
	_selectionRect.origin.x = point.x <= start.x ? point.x : start.x;
	_selectionRect.size.height = fabsf((float)start.y - (float)point.y);
	_selectionRect.size.width = fabsf((float)start.x - (float)point.x);
	_selectionRect.origin.y = point.y <= start.y ? point.y : start.y;
	[self setNeedsDisplayInRect:_selectionRect];
	NSInteger row1, column1, row2, column2;
	if (![self getRow:&row1 column:&column1
	         forPoint:NSMakePoint(_selectionRect.size.width + _selectionRect.origin.x, _selectionRect.origin.y)]) {
		return;
	}
	if (![self getRow:&row2 column:&column2
	         forPoint:NSMakePoint(_selectionRect.origin.x, _selectionRect.size.height + _selectionRect.origin.y)]) {
		return;
	}
	NSInteger columns = [self numberOfColumns];
	NSInteger first = MIN(column2, column1) + MIN(row2, row1) * columns;
	NSInteger last = MAX(column2, column1) + MAX(row2, row1) * columns;
	[self setSelectionFrom:first to:last anchor:first highlight:YES];
}

- (void)mouseUp:(id)aUp
{
	if (_isTrackingSelection) {
		[self mouseDragged:aUp];
		_isTrackingSelection = NO;
		[self setNeedsDisplayInRect:_selectionRect];
	}
}

- (id)owningGridControl
{
	return _owningGridControl;
}

- (void)setOwningGridControl:(id)aControl
{
	_owningGridControl = aControl;
}

- (BOOL)isCacheAllowed
{
	return [self superview] != nil;
}

- (BOOL)isThumbnailCachingEnabled
{
	return _isThumbnailCachingEnabled;
}

- (void)setThumbnailCachingEnabled:(BOOL)aEnabled
{
	_isThumbnailCachingEnabled = aEnabled;
}

- (BOOL)isThreadedCellUpdatingEnabled
{
	return [self isCacheAllowed] && _isUpdaterRunning;
}

- (BOOL)isTitlePreloading
{
	BOOL preloading = [[self owningGridControl] isTitlePreloadingEnabled];
	if ([self superview] != nil) {
		preloading = preloading & [self isCacheAllowed];
	}
	return preloading;
}

// read from the cells' drawing, possibly on the updater thread
- (BOOL)isDrawingPermitted
{
	[_cellTitleAdditionalHeightLock lock];
	BOOL permitted = _isDrawingPermitted;
	[_cellTitleAdditionalHeightLock unlock];
	return permitted;
}

- (BOOL)isGraphicContentShadowEnabled
{
	return _isGraphicContentShadowEnabled;
}

- (void)setGraphicContentShadowEnabled:(BOOL)aEnabled
{
	_isGraphicContentShadowEnabled = aEnabled;
}

- (void)setCellHeightToWidthProportion:(CGFloat)aProportion
{
	_cellHeightToWidthProportion = aProportion;
	if ([self autosizesCells]) {
		[self resizeWithOldSuperviewSize:NSZeroSize];
	}
}

- (CGFloat)cellHeightToWidthProportion
{
	return _cellHeightToWidthProportion;
}

// the rubber band: a translucent fill with a solid edge
- (void)drawSelectedRect:(NSRect)aRect
{
	[[[[NSColor selectedControlColor] colorUsingColorSpaceName:NSCalibratedRGBColorSpace] colorWithAlphaComponent:0.2] set];
	NSRectFillUsingOperation(aRect, NSCompositingOperationSourceOver);
	[[[NSColor selectedControlColor] colorUsingColorSpaceName:NSCalibratedRGBColorSpace] set];
	NSFrameRectWithWidth(aRect, 1.0);
}

// drawing queues cells; this wakes the updater to render them
- (void)drawRect:(NSRect)aRect
{
	_isDrawingPermitted = YES;
	[super drawRect:aRect];
	if (_isUpdaterRunning) {
		CFRunLoopSourceSignal(updaterUpdateSource);
		if (CFRunLoopIsWaiting(updaterRunLoop)) {
			CFRunLoopWakeUp(updaterRunLoop);
		}
	}
	if (_isTrackingSelection) {
		[self drawSelectedRect:_selectionRect];
	}
}

- (void)SMKGridMatrix_setNeedsDisplay
{
	[self setNeedsDisplay:YES];
}

// redraws the selected cells (aCells is ignored, as in the original)
- (void)_setNeedsDrawCells:(id)aCells
{
	NSEnumerator *enumerator = [[self selectedCells] objectEnumerator];
	id cell;
	while ((cell = [enumerator nextObject]) != nil) {
		[self _setNeedsDrawCell:cell];
	}
}

- (void)_setNeedsDrawAndMakeReadyCell:(id)aCell
{
	[self _setNeedsDrawCell:aCell];
}

- (void)_setNeedsDrawCell:(id)aCell
{
	NSInteger row, column;
	[self getRow:&row column:&column ofCell:aCell];
	NSSize size = [self cellSize];
	[self setNeedsDisplayInRect:NSMakeRect((float)size.width * (float)column, (float)size.height * (float)row,
	                                       size.width, size.height)];
}

// queues a cell for the updater thread
- (void)setNeedsUpdateCell:(id)aCell
{
	if (_isUpdaterRunning && !_isThreadedCellUpdatingStopped) {
		NSInteger row, column;
		[self getRow:&row column:&column ofCell:aCell];
		[_updateCellsLock lock];
		[_updateCellsArray addObject:aCell];
		[_updateCellsLock unlock];
	}
}

// Cells report selection changes; the grid control hears of them once per
// run loop pass, and not while the matrix is being re-laid out.
- (void)stateChangedForCell:(id)aCell
{
	if (!_isSelectionDidChangeNotificationQueued && _renewingRowsColumnsCounter == 0) {
		_isSelectionDidChangeNotificationQueued = YES;
		[self performSelectorOnMainThread:@selector(postSelectionDidChange) withObject:nil waitUntilDone:NO];
	}
}

- (void)postSelectionDidChange
{
	[_owningGridControl selectionDidChange];
	_isSelectionDidChangeNotificationQueued = NO;
}

- (NSUInteger)count
{
	return (int)[self numberOfRows] * (int)[self numberOfColumns] - _extraCellCount;
}

// lays out aCount items in the current (autosizing) or fitting number of
// columns, keeping the selection and the scroll position
- (void)setCount:(NSUInteger)aCount
{
	unsigned int count = (unsigned int)aCount;
	unsigned int columns = (unsigned int)([self autosizesCells] ? [self numberOfColumns] : [self fittingNumberOfColumns]);
	int rows = 0;
	if (columns != 0) {
		rows = (int)(double)(count / columns) + 1;
	}
	NSRect visible = NSZeroRect;
	NSEnumerator *selected = nil;
	if (columns == 0 || rows == 0) {
		_extraCellCount = 0;
	} else {
		_extraCellCount = rows * columns - count;
		if (_extraCellCount == columns) {
			rows--;
			_extraCellCount = 0;
		}
		selected = [[self selectedCells] objectEnumerator];
		if ([self superview] != nil) {
			visible = [[self enclosingScrollView] documentVisibleRect];
		}
	}
	[self renewRows:rows columns:columns];
	id cell;
	while ((cell = [selected nextObject]) != nil) {
		NSUInteger index = [[self cells] indexOfObject:cell];
		if (index != NSNotFound) {
			[self setSelectionFrom:index to:index anchor:index highlight:YES];
		}
	}
	if ([self superview] != nil) {
		[self scrollRectToVisible:visible];
	}
}

// keeps the content area at least the border offsets smaller than the cell
- (void)setCellSize:(NSSize)aSize
{
	NSSize offset = [self cellBorderOffset];
	if (NSEqualSizes([self cellSize], aSize)) {
		return;
	}
	float width = (float)aSize.width >= 0.0f ? (float)aSize.width : 0.0f;
	float height = (float)aSize.height >= 0.0f ? (float)aSize.height : 0.0f;
	[super setCellSize:NSMakeSize(width, height)];
	[[self enclosingScrollView] setLineScroll:[self cellSize].height];
	NSSize content = [self cellContentSize];
	float contentWidth = content.width;
	float borderWidth = (float)offset.width + (float)offset.width;
	if (width - contentWidth < borderWidth) {
		contentWidth = borderWidth < width ? width + (float)offset.width * -2.0f : width;
	}
	float contentHeight = content.height;
	float borderHeight = (float)offset.height + (float)offset.height;
	if (height - contentHeight < borderHeight) {
		contentHeight = borderHeight < height ? height + (float)offset.height * -2.0f : height;
	}
	[self setCellContentSize:NSMakeSize(contentWidth, contentHeight)];
}

- (NSControlSize)controlSize
{
	return _controlSize;
}

- (void)setControlSize:(NSControlSize)aSize
{
	_controlSize = (int)aSize;
	[_stringRenderer setFont:[self textFont]];
	[[self cells] makeObjectsPerformSelector:@selector(emptyTitleCache)];
}

- (void)setSelectionReflectsWindowState:(BOOL)aState
{
	_isSelectionReflectsWindowState = aState;
}

- (BOOL)isSelectionReflectsWindowState
{
	return _isSelectionReflectsWindowState;
}

- (id)textFont
{
	CGFloat size = _controlSize == NSControlSizeRegular ? [NSFont systemFontSize] : [NSFont smallSystemFontSize];
	return [NSFont systemFontOfSize:(float)size];
}

// the selection color, gray in an inactive window when the selection
// follows the window state
- (id)textFrameColor
{
	if ([self isSelectionReflectsWindowState] && ![[self window] isKeyWindow]) {
		return [[NSColor alternateSelectedControlColor] colorUsingColorSpaceName:NSCalibratedWhiteColorSpace];
	}
	return [[NSColor alternateSelectedControlColor] colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
}

// the set color, else one that suits the selection style
- (id)imageFrameColor
{
	NSColor *color = _imageFrameColor;
	if (color == nil) {
		NSInteger style = [self selectionStyle];
		if (style == 1 || style == 2) {
			color = [NSColor controlShadowColor];
		} else if (style == 3) {
			color = [NSColor alternateSelectedControlColor];
		}
	}
	if ([self isSelectionReflectsWindowState] && ![[self window] isKeyWindow]) {
		return [color colorUsingColorSpaceName:NSCalibratedWhiteColorSpace];
	}
	return [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
}

- (void)setImageFrameColor:(id)aColor
{
	if (_imageFrameColor != aColor) {
		[_imageFrameColor release];
		_imageFrameColor = [aColor retain];
	}
}

- (void)setCellBorderOffset:(NSSize)aOffset
{
	_cellBorderOffset = aOffset;
}

// at least the shadow's blur radius when the thumbnails have a shadow
- (NSSize)cellBorderOffset
{
	NSSize offset = _cellBorderOffset;
	if (_isGraphicContentShadowEnabled) {
		float blur = (float)[[self shadow] shadowBlurRadius];
		if (_isGraphicContentShadowEnabled) {
			if ((float)offset.height <= blur) {
				offset.height = blur;
			}
			if ((float)offset.width <= blur) {
				offset.width = blur;
			}
		}
	}
	return offset;
}

- (CGFloat)cellTitleAdditionalHeight
{
	[_cellTitleAdditionalHeightLock lock];
	float height = _cellTitleAdditionalHeight;
	[_cellTitleAdditionalHeightLock unlock];
	return height;
}

// cells call this while laying out their titles, possibly on the updater thread
- (void)setCellTitleAdditionalHeight:(CGFloat)aHeight
{
	[self performSelectorOnMainThread:@selector(setCellTitleAdditionalHeightValue:)
	                       withObject:[NSNumber numberWithFloat:(float)aHeight] waitUntilDone:YES];
}

// grows the cells to fit the tallest title; drawing waits for the relayout
- (void)setCellTitleAdditionalHeightValue:(id)aValue
{
	float height = aValue != nil ? [aValue floatValue] : 0.0f;
	[_cellTitleAdditionalHeightLock lock];
	float current = _cellTitleAdditionalHeight;
	[_cellTitleAdditionalHeightLock unlock];
	if (current < height) {
		NSSize content = [self cellContentSize];
		float gap = height <= 0.001f ? (float)NSZeroSize.height : (float)[self cellBorderOffset].height;
		[self setCellContentSize:NSMakeSize(content.width,
		                                    ((float)content.height - _cellTitleAdditionalHeight) + height + gap + gap)];
		[_cellTitleAdditionalHeightLock lock];
		_cellTitleAdditionalHeight = height;
		_isDrawingPermitted = NO;
		[_cellTitleAdditionalHeightLock unlock];
		[self sizeToCells];
		[self performSelectorOnMainThread:@selector(SMKGridMatrix_setNeedsDisplay) withObject:nil waitUntilDone:NO];
	}
}

- (NSSize)cellContentSize
{
	return _cellContentSize;
}

// keeps the cell at least the border offsets larger than the content area
- (void)setCellContentSize:(NSSize)aSize
{
	NSSize cellSize = [self cellSize];
	NSSize offset = [self cellBorderOffset];
	if (NSEqualSizes(_cellContentSize, aSize)) {
		return;
	}
	float width = (float)aSize.width >= 0.0f ? (float)aSize.width : 0.0f;
	float height = (float)aSize.height >= 0.0f ? (float)aSize.height : 0.0f;
	_cellContentSize.height = height;
	_cellContentSize.width = width;
	double borderWidth = (double)(float)offset.width + (double)(float)offset.width;
	if ((double)((float)cellSize.width - width) < borderWidth) {
		cellSize.width = (float)((double)width + borderWidth);
	}
	double borderHeight = (double)(float)offset.height + (double)(float)offset.height;
	if ((double)((float)cellSize.height - height) < borderHeight) {
		cellSize.height = (float)((double)height + borderHeight);
	}
	[self setCellSize:cellSize];
}

- (CGFloat)thumbnailMaxTitleHeight
{
	return _thumbnailMaxTitleHeight;
}

- (void)setThumbnailMaxTitleHeight:(CGFloat)aHeight
{
	_thumbnailMaxTitleHeight = aHeight;
}

- (void)setThumbnailHyphenationEnabled:(BOOL)aEnabled
{
	if (_isThumbnailHyphenationEnabled != aEnabled) {
		_isThumbnailHyphenationEnabled = aEnabled;
		[_stringRenderer setHyphenationFactor:aEnabled ? 1.0f : 0.0f];
		[[self cells] makeObjectsPerformSelector:@selector(emptyTitleCache)];
	}
}

- (id)stringRenderer
{
	return _stringRenderer;
}

// with autosizing: the visible width split between the columns; one column
// takes the visible height, several keep the height-to-width proportion
- (NSSize)_calcCellSize
{
	NSInteger columns = [self numberOfColumns];
	[self cellSize];
	double width = 0.0;
	if (columns != 0) {
		width = (double)((float)[[self enclosingScrollView] documentVisibleRect].size.width / (float)(int)columns);
	}
	float cellWidth = (float)floor(width);
	float cellHeight;
	if (columns == 1) {
		cellHeight = [[self enclosingScrollView] documentVisibleRect].size.height;
	} else {
		cellHeight = cellWidth * _cellHeightToWidthProportion;
	}
	return NSMakeSize(cellWidth, cellHeight);
}

// re-flows the same items into aCount columns, keeping the selection
- (void)setColumnCount:(NSUInteger)aCount
{
	_renewingRowsColumnsCounter++;
	unsigned int columns = (unsigned int)[self numberOfColumns];
	unsigned int rows = (unsigned int)[self numberOfRows];
	NSUInteger count = [self count];
	if ((NSUInteger)[self numberOfColumns] != aCount && aCount != 0 && aCount < 100) {
		unsigned int newColumns = (unsigned int)aCount;
		_numberOfColumns = newColumns;
		unsigned int newRows;
		NSEnumerator *selected = nil;
		if (count == 0) {
			_extraCellCount = 0;
			newRows = 0;
		} else {
			unsigned int items = columns * rows - _extraCellCount;
			unsigned int fullRows = items / newColumns;
			newRows = fullRows + 1;
			_extraCellCount = newColumns * newRows - items;
			if (_extraCellCount == newColumns) {
				_extraCellCount = 0;
				newRows = fullRows;
			}
			selected = [[self selectedCells] objectEnumerator];
		}
		[self renewRows:newRows columns:newColumns];
		[self sizeToCells];
		if (newRows != 0) {
			id cell;
			while ((cell = [selected nextObject]) != nil) {
				NSUInteger index = [[self cells] indexOfObject:cell];
				if (index != NSNotFound) {
					[self setSelectionFrom:index to:index anchor:index highlight:YES];
				}
			}
		}
	}
	_renewingRowsColumnsCounter--;
}

// autosizing cells are resized to the visible area first
- (void)renewRows:(NSInteger)aRows columns:(NSInteger)aColumns
{
	_renewingRowsColumnsCounter++;
	if ([self autosizesCells] && [self superview] != nil) {
		[self resizeWithOldSuperviewSize:[[self superview] bounds].size];
	}
	[super renewRows:aRows columns:aColumns];
	_renewingRowsColumnsCounter--;
}

// as many columns as fit, widening the cells to share out the leftover width
- (void)fitByWidth
{
	NSUInteger columns = [self fittingNumberOfColumns];
	double visibleWidth = (double)(float)[[self enclosingScrollView] documentVisibleRect].size.width;
	NSSize content = [self cellContentSize];
	NSSize offset = [self cellBorderOffset];
	float extra = (float)(visibleWidth - ((double)(float)offset.width + (double)(float)offset.width + (double)(float)content.width) *
	                                     (double)(unsigned int)columns);
	float width = [[self enclosingScrollView] documentVisibleRect].size.width;
	float count = (float)(unsigned int)columns;
	float slack = truncf((width - (float)[self cellSize].width * count) / count);
	if (extra >= 0.0f && slack != 0.0f) {
		float height = [self cellSize].height;
		content = [self cellContentSize];
		offset = [self cellBorderOffset];
		double base = (double)(float)offset.width + (double)(float)offset.width + (double)(float)content.width;
		[self setCellSize:NSMakeSize((float)((double)truncf(extra / count) + base), height)];
	}
	[self setColumnCount:columns];
	[self sizeToCells];
}

- (NSInteger)numberOfColumns
{
	if (![self autosizesCells]) {
		return [super numberOfColumns];
	}
	return _numberOfColumns;
}

- (NSUInteger)fittingNumberOfColumns
{
	NSRect visible = [[self enclosingScrollView] documentVisibleRect];
	NSSize content = [self cellContentSize];
	NSSize offset = [self cellBorderOffset];
	double fit = trunc((double)(float)visible.size.width /
	                   ((double)(float)offset.width + (double)(float)offset.width + (double)(float)content.width));
	unsigned int columns = fit >= 4294967295.0 ? 0xffffffffu : (fit <= 0.0 ? 0u : (unsigned int)fit);
	if (columns == 0) {
		columns = 1;
	}
	return columns;
}

- (void)setDefaultResize:(BOOL)aResize
{
	_useDefaultResize = aResize;
}

// cells always touch; the spacing is inside them
- (void)setIntercellSpacing:(NSSize)aSpacing
{
	[super setIntercellSpacing:NSZeroSize];
}

- (BOOL)allowsMultipleSelection
{
	return [self mode] == NSListModeMatrix;
}

- (void)setAllowsMultipleSelection:(BOOL)aSelection
{
	[self setMode:aSelection ? NSListModeMatrix : NSRadioModeMatrix];
	[self setAllowsEmptySelection:aSelection];
}

- (NSInteger)selectionStyle
{
	return _selectionStyle;
}

- (void)setSelectionStyle:(NSInteger)aStyle
{
	if (_selectionStyle != aStyle) {
		_selectionStyle = (int)aStyle;
		[self _setNeedsDrawCells:[self selectedCells]];
	}
}

- (BOOL)sendAction:(SEL)aAction to:(id)aTo
{
	return [super sendAction:aAction to:aTo];
}

// without a scroll view of its own the grid control scrolls the enclosing one
- (void)scrollWheel:(id)aWheel
{
	if ([[self owningGridControl] isEmulatingNoOwnScrollView]) {
		[[self enclosingScrollView] scrollWheel:aWheel];
	} else {
		[super scrollWheel:aWheel];
	}
}

- (void)scrollCellToVisibleAtRow:(NSInteger)aRow column:(NSInteger)aColumn
{
	if ([[self owningGridControl] isEmulatingNoOwnScrollView] && [[self window] firstResponder] == self &&
	    aRow >= 0 && aColumn >= 0) {
		NSRect frame = [self cellFrameAtRow:aRow column:aColumn];
		frame = [self convertRect:frame toView:[self owningGridControl]];
		[[self owningGridControl] scrollRectToVisible:frame];
		return;
	}
	[super scrollCellToVisibleAtRow:aRow column:aColumn];
}

- (void)scrollCellToVisibleAtIndex:(NSUInteger)aIndex
{
	if (aIndex < [self count]) {
		[self scrollCellToVisibleAtRow:aIndex / (NSUInteger)[self numberOfColumns]
		                        column:aIndex % (NSUInteger)[self numberOfColumns]];
	}
}

// The grid's list view is a table whose data source is this matrix; rows are
// the matrix's cells in order.
- (NSInteger)numberOfRowsInTableView:(id)aView
{
	return (int)[self numberOfRows] * (int)[self numberOfColumns] - _extraCellCount;
}

- (id)tableView:(id)aView objectValueForTableColumn:(id)aColumn row:(NSInteger)aRow
{
	SMKGridBaseCell *cell = (SMKGridBaseCell *)[[self cells] objectAtIndex:aRow];
	[cell updateTitleIfNeeded];
	if ([cell title] != nil && [[cell title] length] != 0) {
		return [[[cell title] copy] autorelease];
	}
	return [_owningGridControl stringForEmptyTitle];
}

- (BOOL)tableView:(id)aView writeRows:(id)aRows toPasteboard:(id)aPasteboard
{
	NSEnumerator *enumerator = [aRows objectEnumerator];
	NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];
	BOOL extend = NO;
	id row;
	while ((row = [enumerator nextObject]) != nil) {
		if (floor(NSAppKitVersionNumber) > 663.0) {
			[indexes addIndex:[row intValue]];
		} else {
			[aView selectRowIndexes:[NSIndexSet indexSetWithIndex:[row intValue]] byExtendingSelection:extend];
			extend = YES;
		}
	}
	// as in the original, current systems only collect the rows
	if (floor(NSAppKitVersionNumber) <= 663.0) {
		[aView selectRowIndexes:indexes byExtendingSelection:extend];
	}
	[[self owningGridControl] writeSelectionToPasteboard:aPasteboard withImage:nil];
	return YES;
}

- (id)loadingStatusImage
{
	return _loadingStatusImage;
}

// The shipped setter autoreleases and re-retains the old image and never
// stores the new one, so the cells always fall back to their own image.
- (void)setLoadingStatusImage:(id)aImage
{
	if (_loadingStatusImage != aImage) {
		[_loadingStatusImage autorelease];
		[_loadingStatusImage retain];
	}
}

- (void)enableThreadUpdater:(BOOL)aUpdater
{
	if (!aUpdater) {
		_isUpdaterRunning = NO;
		while (_isUpdaterThreadActive) {
			[NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.5]];
		}
	} else if (!_isUpdaterRunning) {
		[self _initCellUpdater];
	}
}

- (void)_initCellUpdater
{
	_isUpdaterRunning = YES;
	[NSThread detachNewThreadSelector:@selector(_updatingThread:) toTarget:self withObject:nil];
}

static void _updateCells(void *info)
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	[(SMKGridMatrix *)info _updateGraphicContentInCells];
	[pool release];
}

// the updater thread: a run loop whose source drawRect: signals
- (void)_updatingThread:(id)aThread
{
	_isUpdaterThreadActive = YES;
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	updaterRunLoop = CFRunLoopGetCurrent();
	CFRunLoopSourceContext context = { 0, self, NULL, NULL, NULL, NULL, NULL, NULL, NULL, _updateCells };
	updaterUpdateSource = CFRunLoopSourceCreate(NULL, 0, &context);
	CFRunLoopAddSource(updaterRunLoop, updaterUpdateSource, kCFRunLoopDefaultMode);
	while (_isUpdaterRunning) {
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, true);
	}
	[pool release];
	_isUpdaterThreadActive = NO;
}

- (void)stopThreadedCellUpdating
{
	if (!_isThreadedCellUpdatingStopped) {
		_isThreadedCellUpdatingStopped = YES;
	}
}

- (void)startThreadedCellUpdating
{
	if (_isThreadedCellUpdatingStopped) {
		_isThreadedCellUpdatingStopped = NO;
	}
}

// Runs on the updater thread: renders the queued cells that are in view and
// redraws them on the main thread. When updating gets stopped midway the
// rest of the queue is dropped.
- (void)_updateGraphicContentInCells
{
	if (_isThreadedCellUpdatingStopped) {
		return;
	}
	[_updateCellsLock lock];
	NSArray *cells = [NSArray arrayWithArray:_updateCellsArray];
	[_updateCellsArray removeAllObjects];
	[_updateCellsLock unlock];
	NSEnumerator *enumerator = [cells objectEnumerator];
	NSSize cellSize = [self cellSize];
	SMKGridBaseCell *cell;
	while ((cell = [enumerator nextObject]) != nil) {
		if (_isThreadedCellUpdatingStopped) {
			[_updateCellsLock lock];
			do {
				[_updateCellsArray addObject:cell];
			} while ((cell = [enumerator nextObject]) != nil);
			[_updateCellsArray objectEnumerator];
			[_updateCellsArray removeAllObjects];
			[_updateCellsLock unlock];
			return;
		}
		[cell updateContentSize];
		NSInteger row, column;
		[self getRow:&row column:&column ofCell:cell];
		NSRect frame = NSMakeRect((float)column * (float)cellSize.width, (float)row * (float)cellSize.height,
		                          cellSize.width, cellSize.height);
		if (NSIsEmptyRect(NSIntersectionRect([self visibleRect], frame))) {
			continue;
		}
		[cell updateContentIfNeeded];
		if ([self isCacheAllowed] && [self isThumbnailCachingEnabled] && [cell hasGraphicComponent] &&
		    [cell _cachedImage] == nil) {
			NSRect bounds = NSMakeRect(0.0, 0.0, _cellContentSize.width, _cellContentSize.height);
			[cell cacheGraphicContentWithSize:[cell graphicContentPlacementRectForBounds:bounds].size];
		}
		[self performSelectorOnMainThread:@selector(_setNeedsDrawAndMakeReadyCell:) withObject:cell waitUntilDone:NO];
	}
}

// Without autosizing the cells are refitted to the width; with it they
// share the visible width (unless the default resize is asked for) and
// the first visible item stays in view.
- (void)resizeWithOldSuperviewSize:(NSSize)aSize
{
	if (![self autosizesCells]) {
		[self fitByWidth];
		return;
	}
	if (_useDefaultResize) {
		[super resizeWithOldSuperviewSize:aSize];
		return;
	}
	NSRect visible = [[self enclosingScrollView] documentVisibleRect];
	NSInteger row, column;
	[self getRow:&row column:&column forPoint:visible.origin];
	NSSize size = [self _calcCellSize];
	NSSize offset = [self cellBorderOffset];
	[self setCellSize:size];
	[self setCellContentSize:NSMakeSize((float)((double)(float)size.width + (double)(float)offset.width * -2.0),
	                                    (float)((double)(float)size.height + (double)(float)offset.height * -2.0))];
	[self sizeToCells];
	[self scrollCellToVisibleAtRow:row column:column];
}

@end

@implementation SMKGridMatrix (Shadow)

- (void)setShadow:(id)aShadow
{
	[_shadow autorelease];
	_shadow = [aShadow retain];
	[self invalidateShadowCache];
}

- (void)invalidateShadowCache
{
	if (_shadowCacheMap != NULL) {
		NSResetMapTable(_shadowCacheMap);
	}
}

// A white rectangle of aSize drawn with the shadow, padded by the blur
// radius; cells composite it under their thumbnails. Cached per size.
- (id)shadowCacheForSize:(NSSize)aSize
{
	unsigned int width = (unsigned int)lroundf(aSize.width);
	unsigned int height = (unsigned int)lroundf(aSize.height);
	if (width > 0xfffe || height > 0xfffe) {
		[[NSAssertionHandler currentHandler] handleFailureInMethod:_cmd object:self
		                                                      file:[NSString stringWithUTF8String:__FILE__]
		                                                lineNumber:__LINE__
		                                               description:@"Shadow width or height exceeds maximum size"];
	}
	if (width == 0 || height == 0) {
		return nil;
	}
	NSMapTable *map = [self shadowCacheMap];
	uintptr_t key = width * 0x10000 + height;
	NSImage *image = (NSImage *)NSMapGet(map, (const void *)key);
	if (image == nil) {
		SMKShadow *shadow = [self shadow];
		float blur = (float)[shadow shadowBlurRadius];
		float w = (float)width;
		float h = (float)height;
		image = [[[NSImage alloc] initWithSize:NSMakeSize(ceilf(blur + blur + w), ceilf(h + blur + blur))] autorelease];
		[image setCachedSeparately:YES];
		[image setFlipped:YES];
		[image lockFocus];
		[shadow set];
		[[NSColor whiteColor] set];
		NSRectFill(NSMakeRect(blur, blur, w, h));
		[image unlockFocus];
		NSMapInsert(map, (const void *)key, image);
	}
	return image;
}

// matrices with the default shadow share one cache
- (NSMapTable *)shadowCacheMap
{
	if (_shadow == nil) {
		if (__sDefaultShadowCacheMap == NULL) {
			__sDefaultShadowCacheMap = NSCreateMapTable(NSIntegerMapKeyCallBacks, NSObjectMapValueCallBacks, 0);
		}
		return __sDefaultShadowCacheMap;
	}
	if (_shadowCacheMap == NULL) {
		_shadowCacheMap = NSCreateMapTable(NSIntegerMapKeyCallBacks, NSObjectMapValueCallBacks, 0);
	}
	return _shadowCacheMap;
}

// the set shadow, else a soft one 2 points below
- (id)shadow
{
	if (_shadow != nil) {
		return _shadow;
	}
	if (__sDefaultShadow == nil) {
		__sDefaultShadow = [SMKShadow new];
		[__sDefaultShadow setShadowOffset:NSMakeSize(0.0, -2.0)];
		[__sDefaultShadow setShadowBlurRadius:3.0];
		[__sDefaultShadow setShadowColor:[[NSColor blackColor] colorWithAlphaComponent:0.66]];
	}
	return __sDefaultShadow;
}

@end
