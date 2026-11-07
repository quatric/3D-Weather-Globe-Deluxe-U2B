#import "CitiesListController.h"
#import "DragSourceTableView.h"
#import "WGCGImageCell.h"
#import "WGCenteredTextAttachmentCell.h"
#import "NSCell+WGExtensions.h"

NSString *_kIconColumnName = @"icon";
NSString *_kNameColumnIdentifier = @"Name";
NSString *_kPopulationColumnIdentifier = @"Population";
NSString *_kTypeColumnIdentifier = @"Type";
NSString *_kRegionColumnIdentifier = @"Region";
NSString *_kMarkColumnIdentifier = @"Mark";

// Rows dragged at once.
static const NSUInteger kMaxDraggedRows = 1000;

@implementation EntriesListController

- (void)awakeFromNib
{
	NSArray *columns = [_entriesTableView tableColumns];
	int count = (int)[columns count];
	for (int i = 0; i < count; i++)
		[[[columns objectAtIndex:i] headerCell] wgFixLineBreakingTo:NSLineBreakByTruncatingTail];
	NSTableColumn *nameColumn = [_entriesTableView tableColumnWithIdentifier:_kNameColumnIdentifier];
	[_entriesTableView setHighlightedTableColumn:nameColumn];
	[_entriesTableView setIndicatorImage:[self _indicatorImageForSortingMode:_entriesDataSource->_sortingMode]
		inTableColumn:nameColumn];
	[[_entriesTableView tableColumnWithIdentifier:_kIconColumnName] setDataCell:[[[WGCGImageCell alloc] init] autorelease]];
	[[_filterTextField cell] setSendsWholeSearchString:YES];
	LNotificationCenter::center()->addObserver(self, @selector(entriesWereChanged),
		EntriesListDataSource::kEntriesWereChangedNotification, _entriesDataSource);
}

- (id)init
{
	self = [super init];
	if (self) {
		LClass *dataSourceClass = [self _entriesListDataSourceClass];
		if (dataSourceClass) {
			_entriesDataSource = (EntriesListDataSource *)dataSourceClass->alloc()->init();
			_entriesDataSource->setGeotypeFilter([self initialGeotypeFilter]);
			_entriesDataSource->setSearchString([self initialSearchTerm]);
		}
	}
	return self;
}

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	if (_entriesDataSource) {
		_entriesDataSource->invalidate();
		_entriesDataSource = 0;
	}
	if (_selectedEntryUID) {
		_selectedEntryUID->release();
		_selectedEntryUID = 0;
	}
	if (_draggedEntries) {
		_draggedEntries->release();
		_draggedEntries = 0;
	}
	[_searchesCountMap release];
	[super dealloc];
}

- (LClass *)_entriesListDataSourceClass
{
	return EntriesListDataSource::classObject();
}

- (BOOL)isDragSupported
{
	return NO;
}

- (id)entriesTableView
{
	return _entriesTableView;
}

- (NSInteger)searchedEntriesCount
{
	return _entriesDataSource->searchedEntriesCount();
}

- (void)reloadTableData
{
	_isReloading = YES;
	[[self entriesTableView] deselectAll:nil];
	[[self entriesTableView] reloadData];
	_isReloading = NO;
}

- (unsigned long long)geotypeFilter
{
	return _entriesDataSource->_geotypeFilter;
}

- (void)setGeotypeFilter:(unsigned long long)aFilter
{
	_entriesDataSource->setGeotypeFilter(aFilter);
}

- (GenericEntry *)selectedItem
{
	NSInteger row = [[self entriesTableView] selectedRow];
	if (row < 0)
		return 0;
	return [self searchedEntryAtIndex:row createIfNeeded:YES];
}

- (BOOL)singleItemIsSelected
{
	return [[self entriesTableView] numberOfSelectedRows] == 1;
}

- (GenericEntry *)searchedEntryAtIndex:(NSInteger)aIndex createIfNeeded:(BOOL)aNeeded
{
	return _entriesDataSource->searchedEntryAtIndex((int)aIndex, aNeeded);
}

- (LString *)lastSelectedEntryUID
{
	return _selectedEntryUID;
}

- (void)selectEntryWithUID:(LString *)aUID
{
	NSInteger index = [self indexOfSerachedEntryWithUID:aUID];
	if (index < 0)
		return;
	[[self entriesTableView] selectRowIndexes:[NSIndexSet indexSetWithIndex:index] byExtendingSelection:NO];
	[[self entriesTableView] scrollRowToVisible:index];
}

- (NSInteger)indexOfSerachedEntryWithUID:(LString *)aUID
{
	return _entriesDataSource->indexOfSearchedEntryWithUID(aUID);
}

- (unsigned long long)initialGeotypeFilter
{
	return ~0ULL;
}

- (LString *)initialSearchTerm
{
	return 0;
}

- (void)entriesWereChanged
{
	[self reloadTableData];
	[self selectEntryWithUID:[self lastSelectedEntryUID]];
}

// Even modes sort ascending, odd ones descending.
- (id)_indicatorImageForSortingMode:(NSInteger)aMode
{
	if (aMode < 0 || aMode > kEntriesSortByPopulationDescending)
		return nil;
	return [NSImage imageNamed:(aMode & 1) ? @"NSDescendingSortIndicator" : @"NSAscendingSortIndicator"];
}

- (NSInteger)numberOfRowsInTableView:(id)aView
{
	return _entriesDataSource->searchedEntriesCount();
}

- (id)tableView:(id)aView objectValueForTableColumn:(id)aColumn row:(NSInteger)aRow
{
	GenericEntry *entry = [self searchedEntryAtIndex:aRow createIfNeeded:YES];
	if (!entry)
		return nil;
	id identifier = [aColumn identifier];
	if ([identifier isEqual:_kIconColumnName]) {
		LImage *icon = entry->previewIcon(0);
		return icon ? (id)icon->CGImage() : nil;
	}

	LString *value;
	if ([identifier isEqual:_kNameColumnIdentifier])
		value = entry->ownName(0);
	else if ([identifier isEqual:_kRegionColumnIdentifier])
		value = entry->regionName(0);
	else if ([identifier isEqual:_kTypeColumnIdentifier])
		value = entry->kind(0);
	else if ([identifier isEqual:_kPopulationColumnIdentifier]) {
		int population = entry->population(0);
		if (population < 0)
			return nil;
		return [NSString stringReferenceFromLString:Utils::commaSeparatedNumericValue((double)population, 0, (LString *)0)];
	}
	else
		return nil;

	NSString *string = [NSString stringReferenceFromLString:value];
	NSAttributedString *attributed = string ? [[[NSAttributedString alloc] initWithString:string] autorelease] : nil;
	return [attributed wgSetLineBreak:NSLineBreakByTruncatingTail];
}

- (void)tableView:(id)aView willHideTableColumn:(id)aColumn
{
	NSTableColumn *highlighted = [aView highlightedTableColumn];
	if (highlighted == aColumn) {
		[aView setIndicatorImage:nil inTableColumn:highlighted];
		[aView setHighlightedTableColumn:nil];
	}
}

- (void)tableViewSelectionDidChange:(id)aChange
{
	if (_isReloading)
		return;
	GenericEntry *entry = [self selectedItem];
	[self setLastSelectedEntryUID:entry ? entry->uid(0) : 0];
}

- (BOOL)tableView:(id)aView canHideTableColumn:(id)aColumn
{
	NSString *identifier = [aColumn identifier];
	return [identifier isEqualToString:_kTypeColumnIdentifier]
		|| [identifier isEqualToString:_kPopulationColumnIdentifier];
}

- (id)orderedTableColumnIDsForTableView:(id)aView
{
	return [NSArray arrayWithObjects:_kMarkColumnIdentifier, _kNameColumnIdentifier, _kRegionColumnIdentifier,
		_kTypeColumnIdentifier, _kPopulationColumnIdentifier, nil];
}

- (id)defaultInvisibleTableColumnIDsForForTableView:(id)aView
{
	return [NSArray arrayWithObject:_kPopulationColumnIdentifier];
}

- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType
{
	[self provideEntriesDataForType:aType toPasteboard:aPasteboard];
}

- (TLArray<GenericEntry*> *)draggedEntries
{
	return _draggedEntries;
}

- (void)controlTextDidBeginEditing:(id)aEditing
{
}

// Drops the autocompleted tail the user didn't accept.
- (void)controlTextDidEndEditing:(id)aEditing
{
	NSString *string = [_filterTextField stringValue];
	NSText *editor = [[_filterTextField window] fieldEditor:NO forObject:_filterTextField];
	NSUInteger location = [editor selectedRange].location;
	if (location != 0 && location != NSNotFound)
		[_filterTextField setStringValue:[string substringToIndex:location]];
}

// Clicking the sorted column again reverses the order.
- (void)tableView:(id)aView didClickTableColumn:(id)aColumn
{
	int mode = _entriesDataSource->_sortingMode;
	id identifier = [aColumn identifier];
	if ([identifier isEqual:_kNameColumnIdentifier])
		mode = mode == kEntriesSortByName ? kEntriesSortByNameDescending : kEntriesSortByName;
	else if ([identifier isEqual:_kRegionColumnIdentifier])
		mode = mode == kEntriesSortByRegion ? kEntriesSortByRegionDescending : kEntriesSortByRegion;
	else if ([identifier isEqual:_kTypeColumnIdentifier])
		mode = mode == kEntriesSortByKind ? kEntriesSortByKindDescending : kEntriesSortByKind;
	else if ([identifier isEqual:_kPopulationColumnIdentifier])
		mode = mode == kEntriesSortByPopulation ? kEntriesSortByPopulationDescending : kEntriesSortByPopulation;
	if (mode == (int)_entriesDataSource->_sortingMode)
		return;
	[aView setIndicatorImage:nil inTableColumn:[aView highlightedTableColumn]];
	[aView setHighlightedTableColumn:aColumn];
	[aView setIndicatorImage:[self _indicatorImageForSortingMode:mode] inTableColumn:aColumn];
	_entriesDataSource->setSortingMode((TEntriesSortingMode)mode);
}

- (void)setDraggedEntries:(TLArray<GenericEntry*> *)aEntries
{
	if (aEntries)
		aEntries->retain();
	if (_draggedEntries) {
		_draggedEntries->release();
		_draggedEntries = 0;
	}
	_draggedEntries = aEntries;
}

- (void)setLastSelectedEntryUID:(LString *)aUID
{
	if (aUID)
		aUID->retain();
	if (_selectedEntryUID) {
		_selectedEntryUID->release();
		_selectedEntryUID = 0;
	}
	_selectedEntryUID = aUID;
}

- (void)setSearchTerm:(LString *)aTerm andRefreshResults:(BOOL)aResults
{
	if (!aTerm)
		aTerm = LString::string();
	[_filterTextField setStringValue:(NSString *)aTerm->cfString()];
	if (aResults)
		[self controlTextDidChange:nil];
}

// Searches as the user types, completing the string from the results.
- (void)controlTextDidChange:(id)aChange
{
	NSText *editor = [[_filterTextField window] fieldEditor:NO forObject:_filterTextField];
	EntriesListDataSource::SAutoCompleteResult result;
	result.string = 0;
	_entriesDataSource->deferredSearchingWithString(LString::stringWithCFString((CFStringRef)[_filterTextField stringValue]), &result);
	if (editor && result.string) {
		[_filterTextField setStringValue:(NSString *)result.string->cfString()];
		[editor setSelectedRange:NSMakeRange(result.selection.location, result.selection.length)];
	}
}

// Return or Enter in the field goes on to the window's default button.
- (void)searchFieldAction:(id)aAction
{
	NSEvent *event = [[_filterTextField window] currentEvent];
	NSString *string = [_filterTextField stringValue];
	[[_filterTextField window] makeFirstResponder:_filterTextField];
	NSText *editor = [[_filterTextField window] fieldEditor:NO forObject:_filterTextField];
	[editor setSelectedRange:NSMakeRange(0, [string length])];
	if (([event type] == NSEventTypeKeyDown || [event type] == NSEventTypeKeyUp)
		&& [[event characters] rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"\x03"]].location != NSNotFound) {
		[[_filterTextField window] performKeyEquivalent:event];
		return;
	}
	_entriesDataSource->setSearchString(LString::stringWithCFString((CFStringRef)string));
}

- (TLArray<GenericEntry*> *)selectedItems
{
	NSIndexSet *rows = [[self entriesTableView] selectedRowIndexes];
	TLArray<GenericEntry*> *items = (TLArray<GenericEntry*> *)LArray::array();
	for (NSUInteger row = [rows firstIndex]; row != NSNotFound; row = [rows indexGreaterThanIndex:row]) {
		if ((NSInteger)row < 0)
			continue;
		GenericEntry *entry = [self searchedEntryAtIndex:row createIfNeeded:YES];
		if (entry)
			items->addObject(entry);
	}
	return items;
}

- (TLArray<GenericEntry*> *)selectedItemsWithLimit:(NSUInteger)aLimit createIfNeeded:(BOOL)aNeeded
{
	if (!aLimit)
		return 0;
	NSInteger selectedCount = [[self entriesTableView] numberOfSelectedRows];
	if (selectedCount <= 0)
		return 0;
	NSUInteger count = MIN((NSUInteger)selectedCount, aLimit);
	TLArray<GenericEntry*> *items = (TLArray<GenericEntry*> *)LArray::arrayWithCapacity((unsigned int)count);
	NSUInteger *indexes = (NSUInteger *)malloc(count * sizeof(NSUInteger));
	NSUInteger got = [[[self entriesTableView] selectedRowIndexes] getIndexes:indexes maxCount:count inIndexRange:NULL];
	for (NSUInteger i = 0; i < got; i++) {
		GenericEntry *entry = [self searchedEntryAtIndex:indexes[i] createIfNeeded:aNeeded];
		if (entry)
			items->addObject(entry);
	}
	free(indexes);
	return items;
}

- (BOOL)tableView:(id)aView writeRows:(id)aRows toPasteboard:(id)aPasteboard
{
	if (![self isDragSupported])
		return NO;
	NSUInteger count = [aRows count];
	if (count - 1 >= kMaxDraggedRows)
		return NO;
	TLArray<GenericEntry*> *entries = (TLArray<GenericEntry*> *)LArray::arrayWithCapacity((unsigned int)count);
	for (NSUInteger i = 0; i < count; i++) {
		GenericEntry *entry = [self searchedEntryAtIndex:[[aRows objectAtIndex:i] intValue] createIfNeeded:YES];
		if (entry)
			entries->addObject(entry);
	}
	if (!entries->count())
		return NO;
	[self declareEntries:entries inPasteboard:aPasteboard
		withAdditionTypes:[NSArray arrayWithObjects:_kEntriesListPasteboardType, _kPointsPasteboardType, nil]];
	return YES;
}

@end
