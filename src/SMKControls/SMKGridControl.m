// Original module: /Volumes/Private/Projects/X2/SMKReuseProjects/SMKControls/SMKControls_sources/Framework/GridControl/SMKGridControl.m
#import "SMKGridControl.h"

// A browser of items shown three ways: mode 0 a grid of thumbnails, mode 1
// one column of large thumbnails, mode 2 a list (a table) beside a preview
// of the selected items (the draw view in a box). The items come from the
// data source; the grid matrix holds one cell per item. Pasteboard, drop and
// delete handling goes to the delegate.
@implementation SMKGridControl

+ (Class)gridMatrixClass
{
	return [SMKGridMatrix class];
}

+ (Class)scrollViewClass
{
	return [SMKScrollView class];
}

// builds the scroll view with the grid matrix, the list table, and the
// preview box (added in mode 2)
- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self == nil) {
		return nil;
	}
	dataSource = nil;
	_mode = -1;
	_isDragEnabled = YES;
	_isDiscreetScrollingEnabled = NO;
	_isExtraDataProvidingAllowed = YES;
	_draggingLastProposedIndex = 0x7fffffff; // NSNotFound on i386
	_draggingLastDragOperation = NSDragOperationNone;
	_placeholderString = [[NSBundle bundleForClass:[SMKGridControl class]]
	                         localizedStringForKey:@"Grid Control Empty" value:@"" table:nil];
	[_placeholderString retain];
	_stringForEmptyTitle = @"??";
	[self setThumbnailSize:NSMakeSize(64.0, 64.0)];
	[self setThumbnailMaxTitleHeight:48.0];

	_scrollView = [[[[self class] scrollViewClass] alloc] initWithFrame:[self bounds]];
	_gridMatrix = [[[[self class] gridMatrixClass] alloc] initWithFrame:[self bounds]];
	_tableView = [[SMKGridTableView alloc] initWithFrame:[_scrollView bounds]];
	_drawView = [[SMKGridDrawView alloc] initWithFrame:[self bounds]];
	NSBox *box = [[NSBox alloc] initWithFrame:[self bounds]];
	[box setTitlePosition:NSBelowBottom];
	[box setBorderType:NSLineBorder];
	[box setBoxType:NSBoxOldStyle];
	[box setContentView:[_drawView autorelease]];
	[box setTitle:@""];
	[_drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	[_scrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	[_scrollView setContentView:[SMKGridClipView contentViewWithFrame:NSZeroRect]];
	[_scrollView setHasVerticalScroller:YES];
	[_scrollView setBorderType:NSGrooveBorder];
	[_scrollView setDocumentView:_gridMatrix];
	[self setScrollerControlSize:NSControlSizeRegular];
	[self setVertical:YES];
	[self addSubview:[_scrollView autorelease]];
	[self setMode:0];
	[_scrollView setVerticalLineScroll:[_gridMatrix cellSize].height];
	[_scrollView setVerticalPageScroll:0.0];
	[_gridMatrix sizeToCells];
	[_gridMatrix setOwningGridControl:self];
	if ([_gridMatrix respondsToSelector:@selector(setFocusRingType:)]) {
		[_gridMatrix setFocusRingType:NSFocusRingTypeNone];
	}

	NSTableColumn *column = [NSTableColumn new];
	[column setDataCell:[[SMKGridTextFieldCell new] autorelease]];
	[column setMinWidth:0.0];
	[_tableView setHeaderView:nil];
	[_tableView addTableColumn:column];
	[column release];
	[_tableView setDataSource:(id)_gridMatrix];
	[_tableView setDelegate:(id)self];
	[_tableView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	[_tableView sizeLastColumnToFit];
	[self setThreadedPreloadingEnabled:NO];
	return self;
}

- (void)awakeFromNib
{
	if (dataSource != nil) {
		[self reloadData];
	}
}

- (void)dealloc
{
	[_placeholderString release];
	[_gridMatrix enableThreadUpdater:NO];
	[_gridMatrix stopThreadedCellUpdating];
	[[_drawView superview] release];
	[_tableView setDelegate:nil];
	[_tableView release];
	[_gridMatrix setOwningGridControl:nil];
	[_gridMatrix release];
	dataSource = nil;
	[super dealloc];
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	_mode = (int)[aCoder decodeFloatForKey:@"mode"];
	_thumbnailSize = [aCoder decodeSizeForKey:@"thumbnailSize"];
	_thumbnailCellSize = [aCoder decodeSizeForKey:@"thumbnailCellSize"];
	_isThreadedPreloadingEnabled = [aCoder decodeBoolForKey:@"isThreadedPreloadingEnabled"];
	dataSource = [[aCoder decodeObjectForKey:@"dataSource"] retain];
	_scrollView = [[aCoder decodeObjectForKey:@"scrollView"] retain];
	_gridMatrix = [[aCoder decodeObjectForKey:@"gridMatrix"] retain];
	_tableView = [[aCoder decodeObjectForKey:@"tableView"] retain];
	_drawView = [[aCoder decodeObjectForKey:@"drawView"] retain];
	_placeholderString = [[aCoder decodeObjectForKey:@"placeholderString"] retain];
	_stringForEmptyTitle = [[aCoder decodeObjectForKey:@"stringForEmptyTitle"] retain];
	_isDragEnabled = [aCoder decodeBoolForKey:@"isDragEnabled"];
	_isDiscreetScrollingEnabled = [aCoder decodeBoolForKey:@"isDiscreetScrollingEnabled"];
	return self;
}

- (void)encodeWithCoder:(id)aCoder
{
	[aCoder encodeFloat:(float)_mode forKey:@"mode"];
	[aCoder encodeSize:_thumbnailSize forKey:@"thumbnailSize"];
	[aCoder encodeSize:_thumbnailCellSize forKey:@"thumbnailCellSize"];
	[aCoder encodeBool:_isThreadedPreloadingEnabled forKey:@"isThreadedPreloadingEnabled"];
	[aCoder encodeConditionalObject:dataSource forKey:@"dataSource"];
	[aCoder encodeObject:_scrollView forKey:@"scrollView"];
	[aCoder encodeObject:_gridMatrix forKey:@"gridMatrix"];
	[aCoder encodeObject:_tableView forKey:@"tableView"];
	[aCoder encodeObject:_drawView forKey:@"drawView"];
	[aCoder encodeObject:_placeholderString forKey:@"placeholderString"];
	[aCoder encodeObject:_stringForEmptyTitle forKey:@"stringForEmptyTitle"];
	[aCoder encodeBool:_isDragEnabled forKey:@"isDragEnabled"];
	[aCoder encodeBool:_isDiscreetScrollingEnabled forKey:@"isDiscreetScrollingEnabled"];
	[super encodeWithCoder:aCoder];
}

- (void)makeFirstResponder
{
	[[self window] makeFirstResponder:[_scrollView documentView]];
}

- (id)scrollView
{
	return _scrollView;
}

- (id)gridMatrix
{
	return _gridMatrix;
}

- (id)tableView
{
	return _tableView;
}

- (NSInteger)mode
{
	return _mode;
}

- (id)placeholderString
{
	return _placeholderString;
}

- (void)setPlaceholderString:(id)aString
{
	if (_placeholderString != aString) {
		[_placeholderString release];
		_placeholderString = [aString retain];
	}
}

// the placeholder in light gray 20 pt, centered in aRect
- (void)drawPlaceholderStringInRect:(NSRect)aRect
{
	NSString *placeholder = [self placeholderString];
	NSFont *font = [NSFont systemFontOfSize:20.0];
	SMKStringRenderer *renderer = [SMKStringRenderer sharedRenderer];
	[renderer setContainerWidth:aRect.size.width];
	[renderer setFont:font];
	[renderer setAlignment:NSTextAlignmentCenter];
	NSRect bounds = [renderer boundingRectOfString:placeholder];
	float offset = (float)ceil((double)((float)aRect.size.height - (float)bounds.size.height) * 0.5);
	float y = aRect.origin.y;
	if (offset > 0.0f) {
		y = (float)aRect.origin.y + offset;
		aRect.size.height = (float)aRect.size.height - offset;
	}
	NSString *fitted = [renderer stringByFittingString:placeholder toSize:aRect.size boundingRect:NULL];
	[renderer drawString:fitted atPoint:NSMakePoint(aRect.origin.x, y) withColor:[NSColor lightGrayColor]];
}

- (id)stringForEmptyTitle
{
	return _stringForEmptyTitle;
}

- (void)setStringForEmptyTitle:(id)aTitle
{
	if (_stringForEmptyTitle != aTitle) {
		[_stringForEmptyTitle release];
		_stringForEmptyTitle = [aTitle retain];
	}
}

- (void)copy:(id)aCopy
{
	[self writeSelectionToPasteboard:[NSPasteboard generalPasteboard] withImage:nil];
}

// the delegate takes a paste as a drop after the last item
- (void)paste:(id)aPaste
{
	NSUInteger count = [self count];
	if ([delegate respondsToSelector:@selector(gridControl:acceptDrop:itemIndex:)]) {
		[delegate gridControl:self acceptDrop:nil itemIndex:count];
	}
}

- (void)delete:(id)aDelete
{
	if ([delegate respondsToSelector:@selector(gridControl:deleteIndexes:)]) {
		[delegate gridControl:self deleteIndexes:[self selectedItemIndexes]];
	}
}

- (void)cut:(id)aCut
{
	[self copy:aCut];
	[self delete:aCut];
}

- (BOOL)canCopy
{
	if ([_gridMatrix selectedCell] != nil &&
	    [delegate respondsToSelector:@selector(canCopyToPasteboardFromGridControl:)]) {
		return [delegate canCopyToPasteboardFromGridControl:self];
	}
	return NO;
}

- (BOOL)canPaste
{
	if ([delegate respondsToSelector:@selector(gridControl:acceptDrop:itemIndex:)] &&
	    [delegate respondsToSelector:@selector(canPasteFromPasteboardToGridControl:)]) {
		return [delegate canPasteFromPasteboardToGridControl:self];
	}
	return NO;
}

- (BOOL)canDelete
{
	if ([delegate respondsToSelector:@selector(canDeleteFromGridControl:)]) {
		return [delegate canDeleteFromGridControl:self];
	}
	return NO;
}

// copy, paste, delete (with a selection) and cut (both) as the delegate
// allows; anything else is enabled
- (BOOL)validateUserInterfaceItem:(id)aItem
{
	if ([aItem action] == @selector(copy:)) {
		return [self canCopy];
	}
	if ([aItem action] == @selector(paste:)) {
		return [self canPaste];
	}
	if ([aItem action] == @selector(delete:)) {
		if ([self selectedItem] == nil) {
			return NO;
		}
		return [self canDelete];
	}
	if ([aItem action] != @selector(cut:)) {
		return YES;
	}
	if (![self canDelete]) {
		return NO;
	}
	return [self canCopy];
}

// The delegate writes the selection itself, or names the types to declare
// (plus, when allowed, TIFF for one item with an image and string for the
// titles, which this control provides lazily); its direct types are written
// at once.
- (BOOL)writeSelectionToPasteboard:(id)aPasteboard withImage:(id)aImage
{
	BOOL delegateWrites = [delegate respondsToSelector:@selector(gridControl:writeIndexes:toPasteboard:)];
	NSArray *types = nil;
	BOOL result = NO;
	if (delegateWrites) {
		result = [delegate gridControl:self writeIndexes:[self selectedItemIndexes] toPasteboard:aPasteboard];
	} else if ([delegate respondsToSelector:@selector(pasteboardTypesForGridControl:)]) {
		types = [delegate pasteboardTypesForGridControl:self];
	}
	if (_isExtraDataProvidingAllowed) {
		BOOL addImage = [[_gridMatrix selectedCells] count] == 1 && aImage != nil &&
		                ![types containsObject:NSTIFFPboardType];
		BOOL addTitles = NO;
		if ([[_gridMatrix selectedCells] count] >= 2 ||
		    ([[_gridMatrix selectedCell] title] != nil && [[[_gridMatrix selectedCell] title] length] != 0)) {
			addTitles = ![types containsObject:NSStringPboardType];
		}
		if (types == nil) {
			types = [NSArray array];
		}
		if (addImage) {
			types = [types arrayByAddingObject:NSTIFFPboardType];
		}
		if (addTitles) {
			types = [types arrayByAddingObject:NSStringPboardType];
		}
	}
	if (types != nil) {
		if (delegateWrites) {
			[aPasteboard addTypes:types owner:self];
		} else {
			[aPasteboard declareTypes:types owner:self];
		}
		result = YES;
	}
	NSArray *direct = [NSArray array];
	if (!delegateWrites && [delegate respondsToSelector:@selector(directPasteboardTypesForGridControl:)] &&
	    [delegate respondsToSelector:@selector(gridControl:pasteboard:provideDataForType:)]) {
		direct = [delegate directPasteboardTypesForGridControl:self];
	}
	if (direct != nil && [direct count] != 0) {
		NSEnumerator *enumerator = [direct objectEnumerator];
		id type;
		while ((type = [enumerator nextObject]) != nil) {
			[self pasteboard:aPasteboard provideDataForType:type];
		}
		result = YES;
	}
	return result;
}

// starts a drag from aSource; a single item drags at half opacity
- (void)dragImage:(id)aImage imageOrigin:(NSPoint)aOrigin source:(id)aSource event:(id)aEvent
{
	if (!_isDragEnabled || aImage == nil) {
		return;
	}
	NSImage *image = [[[NSImage alloc] initWithSize:[aImage size]] autorelease];
	NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
	if ([[_gridMatrix selectedCells] count] == 1) {
		NSSize size = [image size];
		[image setFlipped:[aImage isFlipped]];
		[image lockFocus];
		[aImage drawAtPoint:NSZeroPoint fromRect:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, size.width, size.height)
		          operation:NSCompositingOperationCopy fraction:0.5];
		[image unlockFocus];
	} else {
		image = aImage;
	}
	if ([self writeSelectionToPasteboard:pasteboard withImage:aImage]) {
		[aSource dragImage:image at:aOrigin offset:NSZeroSize event:aEvent pasteboard:pasteboard source:self slideBack:YES];
	}
}

- (void)draggedImage:(id)aImage endedAt:(NSPoint)aAt operation:(NSUInteger)aOperation
{
	if ([delegate respondsToSelector:@selector(gridControl:draggedImage:endedAt:operation:)]) {
		[delegate gridControl:self draggedImage:aImage endedAt:aAt operation:aOperation];
	}
}

// Lazily provides dragged data: the delegate's own types, else the selected
// item's image as TIFF or the selected titles, one per line.
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType
{
	NSArray *types = nil;
	if ([delegate respondsToSelector:@selector(pasteboardTypesForGridControl:)]) {
		types = [delegate pasteboardTypesForGridControl:self];
	}
	if ([delegate respondsToSelector:@selector(gridControl:pasteboard:provideDataForType:)] &&
	    [types containsObject:aType]) {
		[delegate gridControl:self pasteboard:aPasteboard provideDataForType:aType];
		return;
	}
	if ([aType isEqual:NSTIFFPboardType] && [[_gridMatrix selectedCells] count] == 1) {
		if ([[_gridMatrix selectedCell] image] == nil) {
			return;
		}
		[aPasteboard setData:[[[_gridMatrix selectedCell] image] TIFFRepresentation] forType:aType];
		return;
	}
	if (![aType isEqual:NSStringPboardType]) {
		return;
	}
	NSString *string;
	if ([[_gridMatrix selectedCells] count] == 1) {
		string = [[_gridMatrix selectedCell] title];
		if (string == nil || [string length] == 0) {
			return;
		}
	} else {
		NSMutableString *titles = [NSMutableString stringWithCapacity:0];
		NSEnumerator *enumerator = [[_gridMatrix selectedCells] objectEnumerator];
		id cell;
		while ((cell = [enumerator nextObject]) != nil) {
			[titles appendFormat:@"%@\n", [cell title]];
		}
		string = titles;
	}
	[aPasteboard setString:string forType:aType];
}

- (BOOL)isDragEnabled
{
	return _isDragEnabled;
}

- (void)setDragEnabled:(BOOL)aEnabled
{
	_isDragEnabled = aEnabled;
}

// accepts drops of the types the delegate registers
- (void)invalidateDraggedTypes
{
	[self unregisterDraggedTypes];
	if ([delegate respondsToSelector:@selector(registeredDraggedTypesForGridControl:)]) {
		[self registerForDraggedTypes:[delegate registeredDraggedTypesForGridControl:self]];
	}
}

// the item under the drop point, else the end of the items
- (NSUInteger)_dropIndexForDraggingInfo:(id)aInfo
{
	NSInteger row = 0, column = 0;
	NSUInteger index = [_gridMatrix count];
	NSPoint point = [_gridMatrix convertPoint:[aInfo draggingLocation] fromView:nil];
	if ([_gridMatrix getRow:&row column:&column forPoint:point]) {
		index = (int)[_gridMatrix numberOfColumns] * (int)row + (int)column;
	}
	return index;
}

// asks the delegate again only when the proposed index changes
- (NSUInteger)draggingEntered:(id)aEntered
{
	NSUInteger index = [self _dropIndexForDraggingInfo:aEntered];
	if (index != _draggingLastProposedIndex &&
	    [delegate respondsToSelector:@selector(gridControl:validateDrop:proposedIndex:)]) {
		_draggingLastProposedIndex = (unsigned int)index;
		_draggingLastDragOperation = (unsigned int)[delegate gridControl:self validateDrop:aEntered proposedIndex:index];
	}
	return _draggingLastDragOperation;
}

- (NSUInteger)draggingUpdated:(id)aUpdated
{
	return [self draggingEntered:aUpdated];
}

- (void)draggingExited:(id)aExited
{
	_draggingLastProposedIndex = 0x7fffffff; // NSNotFound on i386
	_draggingLastDragOperation = NSDragOperationNone;
}

- (BOOL)prepareForDragOperation:(id)aOperation
{
	return [self draggingUpdated:aOperation] != NSDragOperationNone;
}

- (BOOL)performDragOperation:(id)aOperation
{
	NSUInteger index = [self _dropIndexForDraggingInfo:aOperation];
	if ([delegate respondsToSelector:@selector(gridControl:acceptDrop:itemIndex:)]) {
		return [delegate gridControl:self acceptDrop:aOperation itemIndex:index];
	}
	return NO;
}

- (void)concludeDragOperation:(id)aOperation
{
	_draggingLastProposedIndex = 0x7fffffff; // NSNotFound on i386
	_draggingLastDragOperation = NSDragOperationNone;
}

- (void)selectionDidChange
{
	if (delegate != nil && [delegate respondsToSelector:@selector(selectionDidChangeInGridControl:)]) {
		[delegate selectionDidChangeInGridControl:self];
	}
}

// copies the list's selection to the grid and redraws the preview
- (void)tableViewSelectionDidChange:(id)aChange
{
	if ([_tableView selectedRow] < 0) {
		[_gridMatrix deselectAllCells];
		return;
	}
	[_gridMatrix deselectAllCells];
	NSIndexSet *rows = [_tableView selectedRowIndexes];
	for (NSUInteger row = [rows firstIndex]; row != NSNotFound; row = [rows indexGreaterThanIndex:row]) {
		[(SMKGridBaseCell *)[[_gridMatrix cells] objectAtIndex:row] updateContentIfNeeded];
		[_gridMatrix setSelectionFrom:row to:row anchor:row highlight:YES];
	}
	[_drawView setNeedsDisplay:YES];
}

- (void)viewWillStartLiveResize
{
}

- (void)viewDidEndLiveResize
{
}

// Copies the grid's selection to the list. As in the original, current
// systems select the rows one at a time without extending, so only the last
// selected item stays selected in the list.
- (void)_updateSelection
{
	if (_mode != 2) {
		return;
	}
	[_gridMatrix selectedCell];
	if ([_gridMatrix count] == 0) {
		return;
	}
	if (floor(NSAppKitVersionNumber) > 663.0) {
		NSEnumerator *enumerator = [[_gridMatrix selectedCells] objectEnumerator];
		id cell;
		while ((cell = [enumerator nextObject]) != nil) {
			NSUInteger row = [[_gridMatrix cells] indexOfObject:cell];
			[_tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
		}
	} else {
		NSMutableIndexSet *rows = [NSMutableIndexSet indexSet];
		NSEnumerator *enumerator = [[_gridMatrix selectedCells] objectEnumerator];
		id cell;
		while ((cell = [enumerator nextObject]) != nil) {
			[rows addIndex:[[_gridMatrix cells] indexOfObject:cell]];
		}
		[_tableView selectRowIndexes:rows byExtendingSelection:NO];
	}
}

- (NSSize)thumbnailSize
{
	if ([self mode] == 0) {
		return _thumbnailSize;
	}
	return [_gridMatrix cellContentSize];
}

- (void)setThumbnailSize:(NSSize)aSize
{
	[self setThumbnailSize:aSize andBorderOffset:[_gridMatrix cellBorderOffset]];
}

// The content area is the thumbnail plus the title height (and a border's
// worth of gap when there are titles); the cell adds the border offsets.
- (void)setThumbnailSize:(NSSize)aSize andBorderOffset:(NSSize)aOffset
{
	[_gridMatrix setCellBorderOffset:aOffset];
	NSSize offset = [_gridMatrix cellBorderOffset];
	_thumbnailSize = aSize;
	float height = (float)[_gridMatrix cellTitleAdditionalHeight] + (float)aSize.height;
	if (height > (float)_thumbnailSize.height) {
		float gap = [_gridMatrix cellBorderOffset].height;
		height = (float)((double)height + ((double)gap + (double)gap));
	}
	_thumbnailCellSize.height = (float)((double)height + ((double)(float)offset.height + (double)(float)offset.height));
	_thumbnailCellSize.width = (float)((double)(float)aSize.width + ((double)(float)offset.width + (double)(float)offset.width));
	[_gridMatrix setCellContentSize:NSMakeSize(aSize.width, height)];
	if ([self mode] == 0) {
		[_gridMatrix setCellSize:_thumbnailCellSize];
		[_gridMatrix sizeToCells];
		[_gridMatrix fitByWidth];
	}
}

- (void)setThumbnailMaxTitleHeight:(CGFloat)aHeight
{
	[_gridMatrix setThumbnailMaxTitleHeight:aHeight];
}

- (void)setThumbnailHyphenationEnabled:(BOOL)aEnabled
{
	[_gridMatrix setThumbnailHyphenationEnabled:aEnabled];
}

- (BOOL)isThumbnailCachingEnabled
{
	return [_gridMatrix isThumbnailCachingEnabled];
}

- (void)setThumbnailCachingEnabled:(BOOL)aEnabled
{
	[_gridMatrix setThumbnailCachingEnabled:aEnabled];
}

- (void)setColumnCount:(NSInteger)aCount
{
	[_gridMatrix setColumnCount:aCount];
}

- (void)setCellHeightToWidthProportion:(CGFloat)aProportion
{
	[_gridMatrix setCellHeightToWidthProportion:aProportion];
}

- (CGFloat)cellHeightToWidthProportion
{
	return [_gridMatrix cellHeightToWidthProportion];
}

- (BOOL)allowsMultipleSelection
{
	return [_gridMatrix allowsMultipleSelection];
}

- (void)setAllowsMultipleSelection:(BOOL)aSelection
{
	[_gridMatrix setAllowsMultipleSelection:aSelection];
	[_tableView setAllowsMultipleSelection:aSelection];
}

- (NSInteger)selectionStyle
{
	return [_gridMatrix selectionStyle];
}

- (void)setSelectionStyle:(NSInteger)aStyle
{
	[_gridMatrix setSelectionStyle:aStyle];
}

- (id)imageFrameColor
{
	return [_gridMatrix imageFrameColor];
}

- (void)setImageFrameColor:(id)aColor
{
	[_gridMatrix setImageFrameColor:aColor];
}

- (BOOL)isGraphicContentShadowEnabled
{
	return [_gridMatrix isGraphicContentShadowEnabled];
}

- (void)setGraphicContentShadowEnabled:(BOOL)aEnabled
{
	[_gridMatrix setGraphicContentShadowEnabled:aEnabled];
}

- (BOOL)isDiscreetScrollingEnabled
{
	return _isDiscreetScrollingEnabled;
}

- (void)setDiscreetScrollingEnabled:(BOOL)aEnabled
{
	_isDiscreetScrollingEnabled = aEnabled;
}

- (void)setEmulatingNoOwnScrollView:(BOOL)aView
{
	_isEmulatingNoOwnScrollView = aView;
}

- (BOOL)isEmulatingNoOwnScrollView
{
	return _isEmulatingNoOwnScrollView;
}

- (NSInteger)scrollerControlSize
{
	return (int)[[_scrollView verticalScroller] controlSize];
}

- (void)setScrollerControlSize:(NSInteger)aSize
{
	[[_scrollView verticalScroller] setControlSize:aSize];
}

- (void)setCellControlSize:(NSInteger)aSize
{
	[_gridMatrix setControlSize:aSize];
}

- (BOOL)isTitlePreloadingEnabled
{
	if (dataSource != nil && [dataSource respondsToSelector:@selector(areTitlesCanBePreloadedInGridControl:)]) {
		return [dataSource areTitlesCanBePreloadedInGridControl:self];
	}
	return NO;
}

- (BOOL)isThreadedPreloadingEnabled
{
	return _isThreadedPreloadingEnabled;
}

- (void)setThreadedPreloadingEnabled:(BOOL)aEnabled
{
	_isThreadedPreloadingEnabled = aEnabled;
	[_gridMatrix enableThreadUpdater:aEnabled];
}

- (void)setTableViewAutofitTextDuringLiveResize:(BOOL)aResize
{
	[_tableView setAutofitTextDuringLiveResize:aResize];
}

- (NSUInteger)count
{
	return [_gridMatrix count];
}

- (id)cellAtIndex:(NSUInteger)aIndex
{
	if (aIndex < [_gridMatrix count]) {
		return [[_gridMatrix cells] objectAtIndex:aIndex];
	}
	return nil;
}

- (NSUInteger)indexOfCell:(id)aCell
{
	NSUInteger index = [[_gridMatrix cells] indexOfObject:aCell];
	if (index >= [_gridMatrix count]) {
		index = NSNotFound;
	}
	return index;
}

- (NSUInteger)selectedItemIndex
{
	return [[_gridMatrix cells] indexOfObject:[_gridMatrix selectedCell]];
}

// As in the original, whether the item may be selected is read from the
// first cell.
- (void)selectItemAtIndex:(NSUInteger)aIndex byExtendingSelection:(BOOL)aSelection
{
	NSUInteger count = [_gridMatrix count];
	if (!aSelection || aIndex >= count) {
		[_gridMatrix deselectAllCells];
	}
	if (aIndex < count && aIndex != NSNotFound && [[[_gridMatrix cells] objectAtIndex:0] isEnabled]) {
		[_gridMatrix setSelectionFrom:aIndex to:aIndex anchor:aIndex highlight:YES];
	}
	[self _updateSelection];
}

- (id)selectedItem
{
	return [_gridMatrix selectedCell];
}

- (id)selectedItems
{
	return [_gridMatrix selectedCells];
}

// The selected indexes as NSNumbers. In the list (mode 2) the shipped loop
// starts after the first selected row, which this keeps.
- (id)selectedItemIndexes
{
	if (_mode == 2) {
		if (![_tableView respondsToSelector:@selector(selectedRowIndexes)]) {
			[[NSAssertionHandler currentHandler] handleFailureInMethod:_cmd object:self
			                                                      file:[NSString stringWithUTF8String:__FILE__]
			                                                lineNumber:__LINE__ description:@"Not implemented"];
			return nil;
		}
		NSIndexSet *rows = [_tableView selectedRowIndexes];
		NSUInteger row = [rows firstIndex];
		NSMutableArray *indexes = [NSMutableArray arrayWithCapacity:[rows count]];
		while ((row = [rows indexGreaterThanIndex:row]) != NSNotFound) {
			[indexes addObject:[NSNumber numberWithUnsignedInt:(unsigned int)row]];
		}
		return indexes;
	}
	NSArray *cells = [_gridMatrix selectedCells];
	NSEnumerator *enumerator = [cells objectEnumerator];
	NSMutableArray *indexes = [NSMutableArray arrayWithCapacity:[cells count]];
	id cell;
	while ((cell = [enumerator nextObject]) != nil) {
		NSUInteger index = [[_gridMatrix cells] indexOfObject:cell];
		if (index != NSNotFound) {
			[indexes addObject:[NSNumber numberWithUnsignedInt:(unsigned int)index]];
		}
	}
	return indexes;
}

- (void)setSelectedItem:(id)aItem byExtendingSelection:(BOOL)aSelection
{
	[self selectItemAtIndex:[[_gridMatrix cells] indexOfObject:aItem] byExtendingSelection:aSelection];
}

- (void)scrollToVisibleAtIndex:(NSUInteger)aIndex
{
	if ([self mode] == 2) {
		[_tableView scrollRowToVisible:aIndex];
	} else {
		[_gridMatrix scrollCellToVisibleAtIndex:aIndex];
	}
}

- (BOOL)isExtraPasteboardDataProvidingAllowed
{
	return _isExtraDataProvidingAllowed;
}

- (void)setExtraPasteboardDataProvidingAllowed:(BOOL)aAllowed
{
	_isExtraDataProvidingAllowed = aAllowed;
}

- (void)setAction:(SEL)aAction
{
	[_gridMatrix setAction:aAction];
	[_tableView setAction:aAction];
}

- (void)setDoubleAction:(SEL)aAction
{
	[_gridMatrix setDoubleAction:aAction];
	[_tableView setDoubleAction:aAction];
}

- (void)setTarget:(id)aTarget
{
	[_gridMatrix setTarget:aTarget];
	[_tableView setTarget:aTarget];
}

- (void)onMouseDown:(id)aDown
{
	if ([aDown clickCount] == 1) {
		[_gridMatrix sendAction];
		return;
	}
	[_gridMatrix sendDoubleAction];
}

// The delegate sees key equivalents first; then Delete or Forward Delete in
// the focused grid deletes the selection when allowed.
- (BOOL)performKeyEquivalent:(id)aEquivalent
{
	if (delegate != nil && [delegate respondsToSelector:@selector(gridControl:performKeyEquivalent:)]) {
		BOOL handled = [delegate gridControl:self performKeyEquivalent:aEquivalent];
		if (handled) {
			return handled;
		}
	}
	if ([[self window] firstResponder] == [self gridMatrix] && [(NSEvent *)aEquivalent type] == NSEventTypeKeyDown &&
	    ((short)[(NSEvent *)aEquivalent keyCode] == 0x75 || (short)[(NSEvent *)aEquivalent keyCode] == 0x33) && [self canDelete]) {
		[self delete:nil];
		return YES;
	}
	return NO;
}

- (id)imageForItemAt:(NSInteger)aAt
{
	if (dataSource != nil && (int)aAt < (int)[dataSource numberOfItemsInGridControl:self]) {
		return [dataSource gridControl:self imageForItemAt:aAt];
	}
	return nil;
}

- (id)titleForItemAt:(NSInteger)aAt
{
	if (dataSource != nil && (int)aAt < (int)[dataSource numberOfItemsInGridControl:self]) {
		return [dataSource gridControl:self titleForItemAt:aAt];
	}
	return nil;
}

- (id)additionalInfoForItemAt:(NSInteger)aAt
{
	if ([dataSource respondsToSelector:@selector(gridControl:additionalInfoForItemAt:)]) {
		return [dataSource gridControl:self additionalInfoForItemAt:aAt];
	}
	return nil;
}

- (id)dataSource
{
	return dataSource;
}

// not retained; a data source with a cellClass picks the matrix's cells
- (void)setDataSource:(id)aSource
{
	dataSource = aSource;
	if ([aSource respondsToSelector:@selector(cellClass)]) {
		[_gridMatrix setCellClass:[dataSource cellClass]];
	}
}

// Marks the cells in aRange for reloading from the data source; cells past
// the item count are disabled.
- (void)_reloadDataInRange:(NSRange)aRange
{
	if (dataSource == nil) {
		return;
	}
	[_gridMatrix stopThreadedCellUpdating];
	if (aRange.location < [_gridMatrix count]) {
		[_gridMatrix setCellTitleAdditionalHeight:0.0];
	}
	unsigned int count = (unsigned int)[_gridMatrix count];
	unsigned int end = (unsigned int)(aRange.location + aRange.length);
	for (unsigned int i = (unsigned int)aRange.location; i < end; i++) {
		SMKGridBaseCell *cell = (SMKGridBaseCell *)[[_gridMatrix cells] objectAtIndex:i];
		if (i < count) {
			[cell setNeedsUpdateAsItem:i];
		}
		if (floor(NSAppKitVersionNumber) - 743.0 > 0.001) {
			[cell setControlView:i < count ? _gridMatrix : nil];
		}
		[cell setEnabled:i < count];
	}
	if ([self mode] == 2) {
		[_tableView reloadData];
		[_drawView setNeedsDisplay:YES];
	} else {
		[_gridMatrix sizeToCells];
		[_gridMatrix startThreadedCellUpdating];
		[_scrollView reflectScrolledClipView:[_scrollView contentView]];
	}
	if (aRange.location == 0) {
		[_scrollView setNeedsDisplay:YES];
	}
}

// reloads every item; single selection starts on the first one
- (void)reloadData
{
	if (dataSource == nil) {
		return;
	}
	NSInteger count = [dataSource numberOfItemsInGridControl:self];
	[_gridMatrix setCount:count];
	[self _reloadDataInRange:NSMakeRange(0, [[_gridMatrix cells] count])];
	if (count != 0 && ![self allowsMultipleSelection]) {
		[self selectItemAtIndex:0 byExtendingSelection:NO];
	}
}

- (id)delegate
{
	return delegate;
}

- (void)setDelegate:(id)aDelegate
{
	delegate = aDelegate;
	[self invalidateDraggedTypes];
}

// the dragged image faded, with a count badge in its corner
- (id)imageForMultipleDraggedItemsForImage:(id)aImage
{
	int count;
	if ([self mode] == 2) {
		count = (int)[_tableView numberOfSelectedRows];
	} else {
		count = (int)[[_gridMatrix selectedCells] count];
	}
	NSBundle *bundle = [NSBundle bundleForClass:[SMKGridControl class]];
	long digits = lrint(log10((double)(unsigned int)count));
	if (digits < 2) {
		digits = 2;
	} else if (digits > 5) {
		digits = 5;
	}
	NSString *name = [NSString stringWithFormat:@"dragBadge_%i", (int)digits];
	NSImage *badge = [[[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:name ofType:@"tiff"]] autorelease];
	SMKStringRenderer *renderer = [SMKStringRenderer sharedRenderer];
	[renderer setAlignment:NSTextAlignmentCenter];
	[renderer setContainerWidth:[badge size].width];
	[renderer setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
	NSSize badgeSize = [badge size];
	NSSize imageSize = [aImage size];
	float width = (float)badgeSize.width < (float)imageSize.width ? (float)imageSize.width : (float)badgeSize.width;
	float height = (float)badgeSize.height < (float)imageSize.height ? (float)imageSize.height : (float)badgeSize.height;
	NSImage *result = [[[NSImage alloc] initWithSize:NSMakeSize(width, height)] autorelease];
	[result lockFocus];
	SMKCompositeToPoint(aImage, NSMakePoint(0.0, height - (float)[aImage size].height), NSCompositingOperationSourceOver, 0.5);
	float x = width - (float)[badge size].width;
	SMKCompositeToPoint(badge, NSMakePoint(x, 0.0), NSCompositingOperationSourceOver, 1.0);
	[renderer drawString:[[NSNumber numberWithInt:count] stringValue] atPoint:NSMakePoint(x, -1.0)
	           withColor:[NSColor whiteColor]];
	[result unlockFocus];
	return result;
}

// Switches between the grid (0), the single column (1) and the list with
// its preview (2).
- (void)setMode:(NSInteger)aMode
{
	if (_mode == aMode) {
		return;
	}
	if (_mode == 2) {
		[[_drawView superview] removeFromSuperview];
		[_scrollView setFrameSize:[self bounds].size];
		BOOL listWasFocused = [_tableView isEqual:[[self window] firstResponder]];
		[_scrollView setDocumentView:_gridMatrix];
		if (listWasFocused) {
			[[self window] makeFirstResponder:_gridMatrix];
		}
		[self adjustSubviews];
	}
	if (_mode == 0) {
		_thumbnailSize = [_gridMatrix cellContentSize];
		_thumbnailCellSize = [_gridMatrix cellSize];
		[_gridMatrix stopThreadedCellUpdating];
	}
	_mode = (int)aMode;
	if (aMode == 0) {
		[_gridMatrix setAutosizesCells:NO];
		[self setThumbnailSize:_thumbnailSize];
		[_gridMatrix setCellSize:_thumbnailCellSize];
		[_gridMatrix fitByWidth];
		[_gridMatrix startThreadedCellUpdating];
	}
	if (_mode == 1) {
		[_gridMatrix setColumnCount:1];
		[_gridMatrix setAutosizesCells:YES];
		[_gridMatrix resizeWithOldSuperviewSize:NSZeroSize];
		[_scrollView setVerticalLineScroll:[_gridMatrix cellSize].height];
		if ([[_gridMatrix cells] count] != 0) {
			NSUInteger index = [[_gridMatrix cells] indexOfObject:[_gridMatrix selectedCell]];
			if (index != 0) {
				[_gridMatrix scrollCellToVisibleAtRow:index column:0];
			}
		}
		[_gridMatrix setNeedsDisplay:YES];
	}
	if (_mode == 2) {
		NSRect bounds = [self bounds];
		[_scrollView setFrameSize:NSMakeSize((float)[self bounds].size.width * 0.33f, bounds.size.height)];
		BOOL gridWasFocused = [_gridMatrix isEqual:[[self window] firstResponder]];
		[_scrollView setDocumentView:_tableView];
		if (gridWasFocused) {
			[[self window] makeFirstResponder:_tableView];
		}
		[_tableView sizeLastColumnToFit];
		[self addSubview:[_drawView superview]];
		[self adjustSubviews];
		[self _updateSelection];
		if ([_tableView selectedRow] >= 0 && [_tableView selectedRow] < [_tableView numberOfRows]) {
			[_tableView scrollRowToVisible:[_tableView selectedRow]];
		}
	}
}

// grows the grid to the data source's count and loads the new items
- (void)notifyOfAppendedItems
{
	if (dataSource == nil) {
		return;
	}
	NSUInteger oldCount = [_gridMatrix count];
	NSInteger count = [dataSource numberOfItemsInGridControl:self];
	if ([_gridMatrix count] < (NSUInteger)count) {
		[_gridMatrix setCount:count];
	}
	NSInteger added = (int)count - (int)oldCount;
	[self _reloadDataInRange:NSMakeRange(oldCount, added > 0 ? (NSUInteger)added : 0)];
	if (count != 0 && oldCount == 0 && ![self allowsMultipleSelection]) {
		[self selectItemAtIndex:0 byExtendingSelection:NO];
	}
}

@end
