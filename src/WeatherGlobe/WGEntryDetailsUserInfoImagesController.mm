#import "WGEntryDetailsUserInfoImagesController.h"
#import "WGEntryDetailsInfoPaneController.h"
#import "WGGridControl.h"
#import "SMKGridMatrix.h"

// Room around a picture in its thumbnail.
static const CGFloat kThumbnailInset = 5.0;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

@implementation WGEntryDetailsUserInfoImagesController

+ (id)pasteboardTypes
{
	return [[NSImage imagePasteboardTypes] arrayByAddingObject:NSFilenamesPboardType];
}

- (id)initWithOwner:(id)aOwner key:(LString *)aKey
{
	self = [super initWithOwner:aOwner key:aKey];
	if (self) {
		_userImagesControl = [WGGridControl new];
		[_userImagesControl setDelegate:self];
		[_userImagesControl setDataSource:self];
		[_userImagesControl setAllowsMultipleSelection:YES];
		[_userImagesControl setSelectionStyle:3];
		[_userImagesControl setAutoresizingMask:NSViewWidthSizable];
		[(NSView *)[_userImagesControl gridMatrix] setAutoresizingMask:NSViewNotSizable];
		[_userImagesControl setMode:0];
		[_userImagesControl setCellControlSize:NSControlSizeSmall];
		[_userImagesControl setThumbnailSize:[[self placeholderImage] size]];
		[_userImagesControl setThumbnailCachingEnabled:NO];
		[_userImagesControl setThumbnailHyphenationEnabled:YES];
		[_userImagesControl setExtraPasteboardDataProvidingAllowed:NO];
		[_userImagesControl setPlaceholderString:@""];
		[_userImagesControl setStringForEmptyTitle:@""];
		[_userImagesControl setDragEnabled:YES];
		[_userImagesControl setGraphicContentShadowEnabled:NO];
		[[_userImagesControl scrollView] setBorderType:NSNoBorder];
		[[_userImagesControl scrollView] setHasVerticalScroller:NO];
		[[_userImagesControl scrollView] setHasHorizontalScroller:NO];
		[_userImagesControl setEmulatingNoOwnScrollView:YES];
		[[_userImagesControl gridMatrix] setPostsFrameChangedNotifications:YES];
		[_userImagesControl setSelectedItem:nil byExtendingSelection:NO];
	}
	return self;
}

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_userImagesControl release];
	if (_thumbnailCacheMap)
		NSFreeMapTable(_thumbnailCacheMap);
	[super dealloc];
}

- (id)title
{
	return nil;
}

- (id)placeholderImage
{
	return [NSImage imageNamed:@"userinfo_img_back_txt"];
}

- (id)descriptionView
{
	return _userImagesControl;
}

- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	NSRect frame = [_userImagesControl frame];
	[_userImagesControl setFrameSize:NSMakeSize(aBounds.size.width - aOffset * 2.0, frame.size.height)];
	return [_userImagesControl frame].size;
}

- (void)update
{
	LNotificationCenter::center()->removeObserver(self, EntryInfo::kDidChangeItemNotification, 0);
	EntryInfo *info = [[self owner] entryInfo];
	if (info)
		LNotificationCenter::center()->addObserver(self, @selector(entryContentDidChange:),
			EntryInfo::kDidChangeItemNotification, info);
	[self validateEditing];
	if (_thumbnailCacheMap)
		NSFreeMapTable(_thumbnailCacheMap);
	_thumbnailCacheMap = 0;
	[_userImagesControl reloadData];
	[self adjustMatrixAndControl];
}

- (void)prepareCommit
{
}

- (void)validateEditing
{
	if (_isEditing == [[self owner] isEditing])
		return;
	_isEditing = !_isEditing;
	[_userImagesControl setSelectedItem:nil byExtendingSelection:NO];
	[_userImagesControl reloadData];
	[_userImagesControl invalidateDraggedTypes];
	[self adjustMatrixAndControl];
}

- (BOOL)tryStartDescriptionEditing
{
	NSInteger last = (NSInteger)[_userImagesControl count] - 1;
	[self selectImageAtIndex:last];
	[[_userImagesControl gridMatrix] scrollCellToVisibleAtIndex:last];
	return YES;
}

- (BOOL)isEditing
{
	return _isEditing;
}

- (void)didMountedToOwner:(id)aOwner
{
	[_userImagesControl setSelectedItem:nil byExtendingSelection:NO];
	[_userImagesControl reloadData];
	[self adjustMatrixAndControl];
}

- (BOOL)isScaleImageToCellSize
{
	return _isScaleImageToCellSize;
}

- (void)setScaleImageToCellSize:(BOOL)aSize
{
	_isScaleImageToCellSize = aSize;
}

- (void)selectImageAtIndex:(NSInteger)aIndex
{
	if ([self numberOfItemsInGridControl:_userImagesControl] <= 0)
		return;
	_doNotPostNotification = YES;
	[_userImagesControl selectItemAtIndex:aIndex byExtendingSelection:NO];
	[self _selectionDidChangeInGridControl:_userImagesControl];
	_doNotPostNotification = NO;
}

- (void)adjustMatrixCentering
{
	NSView *superview = [_userImagesControl superview];
	if (!superview)
		return;
	NSRect frame = [_userImagesControl frame];
	NSRect bounds = [superview bounds];
	CGFloat x = truncf((bounds.size.width - frame.size.width) * 0.5);
	if (x == frame.origin.x)
		return;
	[superview setNeedsDisplayInRect:frame];
	frame.origin.x = x;
	[_userImagesControl setFrameOrigin:frame.origin];
	[superview setNeedsDisplayInRect:frame];
}

- (NSSize)adjustMatrixWithWidth:(CGFloat)aWidth
{
	NSView *matrix = [_userImagesControl gridMatrix];
	NSRect frame = [matrix frame];
	if (aWidth != frame.size.width) {
		[matrix setFrameSize:NSMakeSize(aWidth, frame.size.height)];
		[(SMKGridMatrix *)matrix fitByWidth];
		frame = [matrix frame];
	}
	return frame.size;
}

// A picture fit into the thumbnail's back, or into the white cell when
// scaled to the cell.
- (id)thumbnailWithLBitmap:(LBitmapImage *)aBitmap ofSize:(NSSize)aSize
{
	NSImage *back = [NSImage imageNamed:@"userinfo_img_back"];
	NSImage *thumbnail = nil;
	NSSize size = aSize;
	if (!_isScaleImageToCellSize) {
		size = [back size];
		thumbnail = back;
	}
	if (!aBitmap)
		return thumbnail;

	thumbnail = [[[NSImage alloc] initWithSize:size] autorelease];
	LSize bitmapSize = aBitmap->size();
	CGFloat aspect = (float)bitmapSize.width / (float)bitmapSize.height;
	[thumbnail lockFocus];
	if (!_isScaleImageToCellSize)
		[back drawAtPoint:NSZeroPoint fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1.0];
	else {
		[[NSColor whiteColor] set];
		NSRectFill(NSMakeRect(0.0, 0.0, size.width, size.height));
	}
	CGFloat width, height;
	if (aspect <= 1.0) {
		height = size.height;
		width = aspect * size.height;
		if (size.width < width) {
			height = size.width / aspect;
			width = size.width;
		}
	}
	else {
		width = size.width;
		height = size.width / aspect;
		if (size.height < height) {
			height = size.height;
			width = aspect * size.height;
		}
	}
	NSRect rect = NSInsetRect(NSMakeRect(size.width * 0.5 - width * 0.5, size.height * 0.5 - height * 0.5, width, height),
		kThumbnailInset, kThumbnailInset);
	CGContextDrawImage([[NSGraphicsContext currentContext] CGContext], NSRectToCGRect(rect), aBitmap->CGImage());
	[thumbnail unlockFocus];
	return thumbnail;
}

- (NSInteger)numberOfItemsInGridControl:(id)aControl
{
	EntryInfoController *controller = [[self owner] entryInfoController];
	if (!controller)
		return 0;
	return controller->itemsCountForKey(kEntryInfoImagesKey, (TEntryKeySet)[[self owner] keySet],
		(TEntryKeyAccessMode)([self isEditing] != NO));
}

- (id)gridControl:(id)aControl titleForItemAt:(NSInteger)aAt
{
	return nil;
}

- (id)gridControl:(id)aControl imageForItemAt:(NSInteger)aAt
{
	return [self cellImageAtIndex:aAt andCellSize:[aControl thumbnailSize]];
}

- (void)gridControl:(id)aControl deleteIndexes:(id)aIndexes
{
	EntryInfo *info = [[self owner] entryInfo];
	EntryInfoController *controller = [[self owner] entryInfoController];
	TEntryKeySet keySet = (TEntryKeySet)[[self owner] keySet];
	if (![self isEditing] || !info)
		return;
	NSEnumerator *enumerator = [aIndexes reverseObjectEnumerator];
	EntryInfoController::undoManager()->beginUndoGrouping();
	int deleted = 0;
	id number;
	while ((number = [enumerator nextObject])) {
		int index = [number unsignedIntValue];
		LObject *item = info->itemForKeyAtIndex(kEntryInfoImagesKey, index, keySet, (TEntryKeyAccessMode)0);
		if (controller->removeItemForKeyAtIndex(kEntryInfoImagesKey, index, keySet)) {
			if (item && _thumbnailCacheMap)
				NSMapRemove(_thumbnailCacheMap, item);
			deleted++;
		}
	}
	EntryInfoController::undoManager()->endUndoGrouping();
	if (deleted > 0) {
		EntryInfoController::undoManager()->setActionName(
			LString::stringFromTable(deleted == 1 ? "Delete Image" : "Delete Images", "Localizable"));
		[_userImagesControl reloadData];
		[self adjustMatrixAndControl];
	}
}

// Shows where dropped pictures go: after the last one.
- (NSUInteger)gridControl:(id)aControl validateDrop:(id)aDrop proposedIndex:(NSUInteger)aIndex
{
	NSDragOperation mask = [aDrop draggingSourceOperationMask];
	if (![self isEditing] || !(mask & (NSDragOperationCopy | NSDragOperationMove)))
		return NSDragOperationNone;
	if ([aDrop draggingSource] == aControl)
		return NSDragOperationMove;
	NSPasteboard *pasteboard = aDrop ? [aDrop draggingPasteboard] : [NSPasteboard generalPasteboard];
	if (![NSImage canInitWithPasteboard:pasteboard])
		return NSDragOperationNone;
	NSUInteger last = [aControl count] - 1;
	[_userImagesControl selectItemAtIndex:last byExtendingSelection:NO];
	NSMatrix *matrix = [_userImagesControl gridMatrix];
	NSUInteger columns = [matrix numberOfColumns];
	NSRect cellFrame = [matrix cellFrameAtRow:last / columns column:last % columns];
	[_userImagesControl scrollRectToVisible:[matrix convertRect:cellFrame toView:_userImagesControl]];
	return NSDragOperationCopy;
}

- (void)gridControl:(id)aControl pasteboard:(id)aPasteboard provideDataForType:(id)aType
{
	if (![aType isEqual:NSTIFFPboardType])
		return;
	NSImage *image = [[[NSImage alloc] initWithContentsOfURL:[NSURL URLFromPasteboard:aPasteboard]] autorelease];
	[aPasteboard setData:[image TIFFRepresentation] forType:NSTIFFPboardType];
}

- (void)selectionDidChangeInGridControl:(id)aControl
{
	if (!_doNotPostNotification)
		[self _selectionDidChangeInGridControl:aControl];
}

- (id)registeredDraggedTypesForGridControl:(id)aControl
{
	return [self isEditing] ? [[self class] pasteboardTypes] : [NSArray array];
}

- (id)pasteboardTypesForGridControl:(id)aControl
{
	NSMutableArray *types = [NSMutableArray arrayWithArray:[NSImage imagePasteboardTypes]];
	[types addObject:NSFilenamesPboardType];
	return types;
}

- (BOOL)canCopyToPasteboardFromGridControl:(id)aControl
{
	return [aControl selectedItemIndex] != NSNotFound;
}

- (BOOL)canPasteFromPasteboardToGridControl:(id)aControl
{
	return [self isEditing];
}

- (BOOL)canDeleteFromGridControl:(id)aControl
{
	return [self isEditing];
}

- (NSUInteger)view:(id)aView validateDrop:(id)aDrop
{
	if (!([aDrop draggingSourceOperationMask] & NSDragOperationCopy) || [aDrop draggingSource] == _userImagesControl)
		return NSDragOperationNone;
	NSPasteboard *pasteboard = aDrop ? [aDrop draggingPasteboard] : [NSPasteboard generalPasteboard];
	return [NSImage canInitWithPasteboard:pasteboard] ? NSDragOperationCopy : NSDragOperationNone;
}

- (BOOL)view:(id)aView acceptDrop:(id)aDrop
{
	if (![[self owner] entryInfoController])
		return NO;
	return [self gridControl:_userImagesControl acceptDrop:aDrop itemIndex:0];
}

- (void)view:(id)aView didResizeWithOldSize:(NSSize)aSize
{
	NSRect frame = [aView frame];
	if (![aView superview])
		return;
	if (aSize.width != frame.size.width)
		[self adjustMatrixAndControl];
	if (aSize.height != frame.size.height)
		[[self owner] rebuildLayout];
}

- (void)entryContentDidChange:(id)aChange
{
	if ([self isEditing]) {
		LString *key = 0;
		LNotification *notification = lobj_cast<LNotification>([aChange getLObject]);
		if (notification && notification->userInfo())
			key = lobj_cast<LString>(notification->userInfo()->objectForKey(EntryInfo::kChangedKey));
		if (kEntryInfoImagesKey->isEqualTo(key))
			[self update];
	}
	// Keeps a selection past the end on the last picture.
	NSUInteger selected = [_userImagesControl selectedItemIndex];
	if (selected != NSNotFound && (NSInteger)selected > 0 && selected < [_userImagesControl count]) {
		NSUInteger last = [_userImagesControl count] - 1;
		[_userImagesControl selectItemAtIndex:MIN(selected, last) byExtendingSelection:NO];
	}
}

- (void)adjustMatrixAndControl
{
	NSRect frame = [_userImagesControl frame];
	NSSize size = [self adjustMatrixWithWidth:frame.size.width];
	if (frame.size.height != size.height)
		[_userImagesControl setFrameSize:NSMakeSize(frame.size.width, size.height)];
}

// The original files of the pictures; a single one also goes as its URL.
- (BOOL)gridControl:(id)aControl writeIndexes:(id)aIndexes toPasteboard:(id)aPasteboard
{
	EntryInfo *info = [[self owner] entryInfo];
	TEntryKeySet keySet = (TEntryKeySet)[[self owner] keySet];
	NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];
	NSUInteger count = [aIndexes count];
	for (NSUInteger i = 0; i < count; i++)
		[indexes addIndex:[[aIndexes objectAtIndex:i] unsignedIntValue]];
	count = [indexes count];
	if (!count || !info)
		return NO;

	NSArray *types = count < 2
		? [NSArray arrayWithObjects:NSFilenamesPboardType, NSURLPboardType, NSTIFFPboardType, nil]
		: [NSArray arrayWithObject:NSFilenamesPboardType];
	[aPasteboard declareTypes:types owner:self];
	NSMutableArray *paths = [NSMutableArray arrayWithCapacity:count];
	for (NSUInteger index = [indexes firstIndex]; index != NSNotFound; index = [indexes indexGreaterThanIndex:index]) {
		EntryInfoImageWrapper *image = lobj_cast<EntryInfoImageWrapper>(info->itemForKeyAtIndex(kEntryInfoImagesKey,
			(int)index, keySet, (TEntryKeyAccessMode)([self isEditing] != NO)));
		LString *path = image ? lobj_cast<LString>(image->originalDataPath()) : 0;
		if (path && path->length())
			[paths addObject:(NSString *)path->cfString()];
	}
	[aPasteboard setPropertyList:paths forType:NSFilenamesPboardType];
	if ([paths count] == 1)
		[[NSURL fileURLWithPath:[paths lastObject]] writeToPasteboard:aPasteboard];
	return [paths count] != 0;
}

- (id)cellImageAtIndex:(NSInteger)aIndex andCellSize:(NSSize)aSize
{
	EntryInfo *info = [[self owner] entryInfo];
	TEntryKeySet keySet = (TEntryKeySet)[[self owner] keySet];
	if (!info)
		return nil;
	TEntryKeyAccessMode mode = (TEntryKeyAccessMode)([self isEditing] != NO);
	EntryInfoImageWrapper *image = lobj_cast<EntryInfoImageWrapper>(info->itemForKeyAtIndex(kEntryInfoImagesKey,
		(int)aIndex, keySet, mode));
	if (!image)
		return [self isEditing] ? [self placeholderImage] : nil;

	if (!_thumbnailCacheMap)
		_thumbnailCacheMap = NSCreateMapTable(NSNonOwnedPointerOrNullMapKeyCallBacks, NSObjectMapValueCallBacks,
			info->itemsCountForKey(kEntryInfoImagesKey, keySet, mode));
	else {
		id cached = (id)NSMapGet(_thumbnailCacheMap, image);
		if (cached)
			return cached;
	}
	if (_isScaleImageToCellSize) {
		LString *path = image->originalDataPath();
		return [[[NSImage alloc] initWithContentsOfFile:path ? (NSString *)path->cfString() : nil] autorelease];
	}
	LString *path = image->thumbnailPath();
	LBitmapImage *bitmap = 0;
	if (path)
		bitmap = (LBitmapImage *)((LBitmapImage *)LBitmapImage::_alloc())->initWithContentsOfFile(path);
	id thumbnail = [self thumbnailWithLBitmap:bitmap ofSize:aSize];
	NSMapInsert(_thumbnailCacheMap, image, thumbnail);
	if (bitmap)
		bitmap->release();
	return thumbnail;
}

// Tells the panel which picture of the stored ones to show.
- (void)_selectionDidChangeInGridControl:(id)aControl
{
	NSUInteger selected = [aControl selectedItemIndex];
	TEntryKeySet keySet = (TEntryKeySet)[[self owner] keySet];
	LValue *index = 0;
	EntryInfo *info = [[self owner] entryInfo];
	if (selected != NSNotFound && info) {
		int stored = info->itemsCountForKey(kEntryInfoImagesKey, keySet, (TEntryKeyAccessMode)0);
		int position = (int)selected;
		if ([self isEditing])
			position += stored - info->itemsCountForKey(kEntryInfoImagesKey, keySet, (TEntryKeyAccessMode)1);
		if (position < stored)
			index = LValue::valueWithInt(position);
	}
	GenericEntry *entry = [[self owner] entry];
	LMap *userInfo = LMap::mapWithObjectsAndKeys(index, kEntryInfoImageIndexKey, entry, kEntryInfoGenericEntryKey, (LObject *)0);
	LNotificationCenter::center()->postNotification(kEntryInfoImageWasSelectedNotification, entry, userInfo);
}

// Moves the grid's own pictures, or adds dropped files, URLs and images
// after the last picture.
- (BOOL)gridControl:(id)aControl acceptDrop:(id)aDrop itemIndex:(NSUInteger)aIndex
{
	EntryInfo *info = [[self owner] entryInfo];
	EntryInfoController *controller = [[self owner] entryInfoController];
	TEntryKeySet keySet = (TEntryKeySet)[[self owner] keySet];
	if (!info)
		return NO;
	NSPasteboard *pasteboard = aDrop ? [aDrop draggingPasteboard] : [NSPasteboard generalPasteboard];
	id source = [aDrop draggingSource];
	BOOL accepted;

	if (source == aControl) {
		int to = (int)aIndex;
		EntryInfoController::undoManager()->beginUndoGrouping();
		NSArray *selected = [source selectedItemIndexes];
		int count = (int)[selected count];
		int moved = 0;
		int shifted = 0;
		for (int i = 0; i < count; i++) {
			int from = [[selected objectAtIndex:i] unsignedIntValue];
			int dest = to + moved;
			if (from < dest) {
				from -= shifted;
				shifted++;
			}
			if (controller->moveItemForKeyFromIndexToIndex(kEntryInfoImagesKey, from, dest, keySet)) {
				moved++;
				dest = to + moved;
			}
			to -= from < dest;
		}
		EntryInfoController::undoManager()->endUndoGrouping();
		if (moved < 1)
			return NO;
		[source reloadData];
		[source selectItemAtIndex:to byExtendingSelection:NO];
		for (int i = 1; i != moved; i++)
			[source selectItemAtIndex:to + i byExtendingSelection:YES];
		EntryInfoController::undoManager()->setActionName(
			LString::stringFromTable(moved == 1 ? "Move Image" : "Move Images", "Localizable"));
		accepted = YES;
	}
	else {
		if (![NSImage canInitWithPasteboard:pasteboard])
			return NO;
		int insertIndex = controller->itemsCountForKey(kEntryInfoImagesKey, (TEntryKeySet)[[self owner] keySet],
			(TEntryKeyAccessMode)1) - 1;
		int inserted = 0;
		if ([pasteboard availableTypeFromArray:[NSArray arrayWithObject:NSFilenamesPboardType]]) {
			NSArray *paths = [pasteboard propertyListForType:NSFilenamesPboardType];
			NSArray *imageTypes = [NSImage imageFileTypes];
			EntryInfoController::undoManager()->beginUndoGrouping();
			int count = (int)[paths count];
			for (int i = 0; i < count; i++) {
				NSString *path = [paths objectAtIndex:i];
				if (![imageTypes containsObject:[path pathExtension]])
					continue;
				LData *data = LData::dataWithContentsOfFile([path LString]);
				if (controller->insertImageWrapperWithData(data, [[path lastPathComponent] LString], insertIndex + inserted))
					inserted++;
			}
			EntryInfoController::undoManager()->endUndoGrouping();
		}
		else {
			LData *data = 0;
			LString *name = 0;
			if ([pasteboard availableTypeFromArray:[NSArray arrayWithObject:NSURLPboardType]]) {
				NSURL *url = [NSURL URLFromPasteboard:pasteboard];
				NSData *contents = [NSData dataWithContentsOfURL:url];
				if (contents) {
					data = LData::dataWithBytesNoCopy((void *)[contents bytes], (unsigned int)[contents length], false);
					name = [[[url path] lastPathComponent] LString];
				}
			}
			else {
				NSData *tiff = [[[[NSImage alloc] initWithPasteboard:pasteboard] autorelease] TIFFRepresentation];
				if (tiff) {
					data = LData::dataWithBytesNoCopy((void *)[tiff bytes], (unsigned int)[tiff length], false);
					name = EntryInfoImageWrapper::kImageDefaultName->stringByAppendingPathExtension(
						LString::stringWithCString("tiff"));
				}
			}
			// The original counted none here, and so refused a drop it had
			// taken.
			if (data && controller->insertImageWrapperWithData(data, name, insertIndex))
				inserted = 1;
		}
		[aControl reloadData];
		[aControl selectItemAtIndex:insertIndex byExtendingSelection:NO];
		for (int i = 1; i < inserted; i++)
			[aControl selectItemAtIndex:insertIndex + i byExtendingSelection:YES];
		accepted = inserted > 0;
		if (inserted > 1)
			EntryInfoController::undoManager()->setActionName(LString::stringFromTable("Insert Images", "Localizable"));
		if (!accepted)
			return NO;
	}
	[self adjustMatrixAndControl];
	if (![self isEditing])
		[[[self owner] owner] setIsEditing:YES];
	return accepted;
}

@end

#pragma clang diagnostic pop
