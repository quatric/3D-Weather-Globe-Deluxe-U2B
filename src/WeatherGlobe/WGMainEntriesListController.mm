#import "WGMainEntriesListController.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "CIPaletteController.h"
#import "WGCurrentSetController.h"
#import "WGPaletteBaseController.h"
#import "WGCustomTableView.h"
#import "WGFilledView.h"
#import "WGTexturedView.h"
#import "WGImageTextTableCell.h"
#import "NSImage+NSImageWGExtensions.h"
#import "NSMenu+NSMenuWGExtensions.h"
#import "NSString_LStringExtension.h"

static NSString *_kNameColumnName = @"public_name";
static NSString *_kTypeColumnName = @"public_type";

// The entries list's menu commands (menu item tags).
enum {
	kCommandDelete = 2,
	kCommandAddToPalette = 3,
	kCommandAddToRoute = 4,
	kCommandSwitchView = 5
};

@implementation WGMainEntriesListController

- (BOOL)_isAdvancedViewSelected
{
	return [[[_tabView selectedTabViewItem] identifier] isEqualToString:_kAdvancedEntriesTableViewID];
}

- (void)_validatePlusButton
{
	[_plusButton setState:[[[AtlasController atlasController] newLocationController] isPaletteVisible]];
}

- (void)_validateInterface
{
	NSInteger count = [self searchedEntriesCount];
	NSString *description;
	if (count == 1)
		description = [[NSBundle mainBundle] localizedStringForKey:@"MainEntriesListDescriptionFormatWith1Item"
			value:@"" table:nil];
	else
		description = [NSString stringWithFormat:[[NSBundle mainBundle]
			localizedStringForKey:@"MainEntriesListDescriptionFormat" value:@"" table:nil], count];
	[_descriptionField setStringValue:description];
}

- (BOOL)validateUserInterfaceItem:(id)aItem
{
	return NO;
}

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_entriesTableView setDoubleAction:NULL];
	[_entriesTableView setDataSource:nil];
	[_entriesTableView setDelegate:nil];
	[_advancedEntriesTableView setDoubleAction:NULL];
	[_advancedEntriesTableView setDataSource:nil];
	[_advancedEntriesTableView setDelegate:nil];
	if (_selectedPreviewEntryUID) {
		_selectedPreviewEntryUID->release();
		_selectedPreviewEntryUID = 0;
	}
	[_entriesListView release];
	[_resourceView release];
	[_foundByNameBackgroundColor release];
	[super dealloc];
}

- (id)nibName
{
	return @"MainEntriesListView";
}

+ (id)wgIdentifier
{
	return @"EntriesList";
}

- (BOOL)isDragSupported
{
	return YES;
}

- (id)pane
{
	[self loadNibIfNeeded];
	return _entriesListView;
}

- (BOOL)isActive
{
	id responder = [[_entriesListView window] firstResponder];
	return [responder isKindOfClass:[NSView class]] && [responder isDescendantOf:_entriesListView];
}

- (id)tabView
{
	return _tabView;
}

- (id)advancedEntriesTableView
{
	return _advancedEntriesTableView;
}

- (BOOL)addNewEntry:(GenericEntry *)aEntry
{
	[self setLastSelectedEntryUID:aEntry ? aEntry->uid(0) : 0];
	return _entriesDataSource->addNewEntry(aEntry);
}

// The geo types the filter tags stand for.
- (unsigned long long)flagsForTag:(NSInteger)aTag
{
	switch (aTag) {
	case 0:
		return ~0ULL;
	case 1:
		return 0x1;
	case 2:
		return 0x8;
	case 3:
		return 0x40000000;
	case 4:
		return 0x2800000;
	case 5:
		return 0x7e0;
	case 6:
		return 0x11ff000;
	case 7:
		return 0x1c000000;
	case 8:
		return 0x20000000;
	}
	return 0;
}

- (NSInteger)numberOfRowsInTableView:(id)aView
{
	if (aView == _advancedEntriesTableView)
		return _entriesDataSource->previewEntriesCount();
	if (!_isEntriesTableViewReady)
		return 0;
	return [super numberOfRowsInTableView:aView];
}

// The preview rows: clickable region rows indented in blue, entries found
// by another name on a lighter background.
- (id)tableView:(id)aView objectValueForTableColumn:(id)aColumn row:(NSInteger)aRow
{
	if (aView != _advancedEntriesTableView)
		return [super tableView:aView objectValueForTableColumn:aColumn row:aRow];
	SearchPreviewEntry *preview = _entriesDataSource->previewEntryAtIndex((int)aRow);
	NSMutableDictionary *value = [NSMutableDictionary dictionary];
	if (preview && !preview->isAlternative() && _foundByNameBackgroundColor)
		[value setObject:_foundByNameBackgroundColor forKey:_kImageTextTableCellBackgroundColorKey];

	id identifier = [aColumn identifier];
	NSMutableParagraphStyle *style;
	NSDictionary *attributes;
	if ([identifier isEqual:_kTypeColumnName]) {
		LString *type = preview ? preview->publicType() : 0;
		NSString *text = type ? [NSString stringWithLString:type] : nil;
		if (!text)
			return value;
		[value setObject:text forKey:_kImageTextTableCellTextKey];
		style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
		[style setAlignment:NSTextAlignmentRight];
		[style setLineBreakMode:NSLineBreakByTruncatingMiddle];
		NSColor *color = preview->emphasizeTypeString() ? [NSColor darkGrayColor] : [NSColor lightGrayColor];
		attributes = [NSDictionary dictionaryWithObjectsAndKeys:style, NSParagraphStyleAttributeName,
			[NSFont boldSystemFontOfSize:11.0], NSFontAttributeName, color, NSForegroundColorAttributeName, nil];
	}
	else if ([identifier isEqual:_kNameColumnName]) {
		GenericEntry *entry = [self searchedEntryAtIndex:aRow createIfNeeded:YES];
		NSImage *icon = [NSImage wgImageWithLImage:entry ? entry->previewIcon(0) : 0];
		if (icon)
			[value setObject:icon forKey:_kImageTextTableCellImageKey];
		LString *title = preview ? preview->publicTitle() : 0;
		NSString *text = title ? [NSString stringWithLString:title] : nil;
		if (!text)
			return value;
		[value setObject:text forKey:_kImageTextTableCellTextKey];
		BOOL clickable = preview->isClickable();
		style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
		[style setAlignment:NSTextAlignmentLeft];
		[style setLineBreakMode:NSLineBreakByTruncatingMiddle];
		if (clickable)
			[style setFirstLineHeadIndent:[_advancedEntriesTableView rowHeight]];
		NSColor *color = clickable ? [NSColor blueColor] : [NSColor blackColor];
		attributes = [NSDictionary dictionaryWithObjectsAndKeys:style, NSParagraphStyleAttributeName,
			[NSFont systemFontOfSize:11.0], NSFontAttributeName, color, NSForegroundColorAttributeName, nil];
	}
	else
		return value;
	[value setObject:attributes forKey:_kImageTextTableCellTextAttributesKey];
	[style release];
	return value;
}

// The header gets the columns menu; a row gets selected for the popup.
- (id)tableView:(id)aView menuForEvent:(id)aEvent
{
	NSPoint point = [[aView superview] convertPoint:[aEvent locationInWindow] fromView:nil];
	NSView *header = [aView headerView];
	if ([header hitTest:[aView convertPoint:point toView:header]])
		return [(WGCustomTableView *)_entriesTableView cornerViewPopupMenu];
	if ([aView hitTest:point] != aView)
		return nil;
	NSInteger row = [aView rowAtPoint:point];
	if (row != -1 && [self searchedEntryAtIndex:row createIfNeeded:YES]
		&& ![[aView selectedRowIndexes] containsIndex:row]) {
		if ([[aView window] firstResponder] != aView)
			[[aView window] makeFirstResponder:aView];
		[aView selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
	}
	return _popupMenu;
}

- (void)tableView:(id)aView willHideTableColumn:(id)aColumn
{
	if (_entriesTableView == aView)
		[super tableView:aView willHideTableColumn:aColumn];
}

- (void)tableView:(id)aView didClickTableColumn:(id)aColumn
{
	if (_entriesTableView == aView)
		[super tableView:aView didClickTableColumn:aColumn];
}

- (id)tableView:(id)aView toolTipForCell:(id)aCell rect:(NSRect *)aRect tableColumn:(id)aColumn row:(NSInteger)aRow mouseLocation:(NSPoint)aLocation
{
	NSString *title = [aCell title];
	return [title length] ? title : nil;
}

// The clickable preview row under the given column and row indices, if any.
- (SearchPreviewEntry *)_clickablePreviewAt:(id)aIndices inTableView:(id)aView
{
	if (aView != _advancedEntriesTableView)
		return 0;
	int column = [[aIndices objectAtIndex:0] intValue];
	if (![[[[aView tableColumns] objectAtIndex:column] identifier] isEqual:_kNameColumnName])
		return 0;
	int row = [[aIndices objectAtIndex:1] intValue];
	if (row < 0)
		return 0;
	return _entriesDataSource->previewEntryAtIndex(row);
}

- (void)tableView:(id)aView accessibilityActionOnColumnRowIndices:(id)aIndices
{
	SearchPreviewEntry *preview = [self _clickablePreviewAt:aIndices inTableView:aView];
	if (!preview)
		return;
	if (preview->isClickable()) {
		_entriesDataSource->previewEntryWasClicked(preview);
		return;
	}
	// Other rows show their popup menu.
	int row = [[aIndices objectAtIndex:1] intValue];
	NSRect rect = [aView rectOfRow:row];
	NSEvent *event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
		location:[aView convertPoint:rect.origin toView:nil] modifierFlags:0 timestamp:0.0
		windowNumber:[[aView window] windowNumber] context:nil eventNumber:0 clickCount:1 pressure:0.0f];
	[NSMenu popUpContextMenu:[self tableView:aView menuForEvent:event] withEvent:event forView:aView];
}

- (void)tableView:(id)aView didClickColumnRowIndices:(id)aIndices
{
	SearchPreviewEntry *preview = [self _clickablePreviewAt:aIndices inTableView:aView];
	if (preview && preview->isClickable())
		_entriesDataSource->previewEntryWasClicked(preview);
}

- (id)tableView:(id)aView requestedCursorForColumnRowIndices:(id)aIndices
{
	SearchPreviewEntry *preview = [self _clickablePreviewAt:aIndices inTableView:aView];
	if (preview && preview->isClickable())
		return [NSCursor pointingHandCursor];
	return nil;
}

// Only the preview rows of entries can be selected.
- (BOOL)tableView:(id)aView shouldSelectRow:(NSInteger)aRow
{
	if (_advancedEntriesTableView != aView)
		return YES;
	SearchPreviewEntry *preview = _entriesDataSource->previewEntryAtIndex((int)aRow);
	return preview && preview->geoEntry();
}

- (void)tableViewSelectionDidChange:(id)aChange
{
	if (_isReloading)
		return;
	[super tableViewSelectionDidChange:aChange];
	[delegate currentEntryDidChangeIn:self];
	[[[AtlasController atlasController] mainWindowController] gotoLocation:[self currentEntry] forced:NO];
}

- (void)entriesWereChanged
{
	[super entriesWereChanged];
	[self _validateInterface];
}

- (void)showPopupMenu:(id)aMenu
{
	[NSMenu wgPopUpContextMenu:_popupMenu withEvent:[[aMenu window] currentEvent] forView:aMenu];
}

- (void)onGearButton:(id)aButton
{
	[NSMenu wgPopUpContextMenu:_popupMenu withEvent:[[aButton window] currentEvent] forView:aButton];
}

- (void)entriesListCommand:(NSInteger)aCommand
{
	switch (aCommand) {
	case kCommandAddToPalette:
		[[[AtlasController atlasController] citiesPaletteController]
			addEntries:[self selectedItemsWithLimit:Preferences::citiesCountLimit() * 2 createIfNeeded:YES]];
		return;
	case kCommandDelete:
		_entriesDataSource->deleteEntries([self selectedItems]);
		return;
	case kCommandAddToRoute:
		[[[[AtlasController atlasController] mainWindowController] currentSetController] addEntriesToRoute:[self selectedItems]];
		return;
	case kCommandSwitchView:
		[self switchTabViewID:nil];
		return;
	}
}

- (LClass *)_entriesListDataSourceClass
{
	return AdvancedEntriesListDataSource::classObject();
}

- (id)entriesTableView
{
	if ([self _isAdvancedViewSelected])
		return _advancedEntriesTableView;
	return [super entriesTableView];
}

- (NSInteger)searchedEntriesCount
{
	if ([self _isAdvancedViewSelected])
		return _entriesDataSource->previewGeoEntriesCount();
	return [super searchedEntriesCount];
}

- (LString *)lastSelectedEntryUID
{
	if ([self _isAdvancedViewSelected])
		return _selectedPreviewEntryUID;
	return [super lastSelectedEntryUID];
}

- (GenericEntry *)searchedEntryAtIndex:(NSInteger)aIndex createIfNeeded:(BOOL)aNeeded
{
	if (![self _isAdvancedViewSelected])
		return [super searchedEntryAtIndex:aIndex createIfNeeded:aNeeded];
	SearchPreviewEntry *preview = _entriesDataSource->previewEntryAtIndex((int)aIndex);
	return preview ? preview->geoEntry() : 0;
}

- (NSInteger)indexOfSerachedEntryWithUID:(LString *)aUID
{
	if ([self _isAdvancedViewSelected])
		return _entriesDataSource->indexOfPreviewEntryWithUID(aUID);
	return [super indexOfSerachedEntryWithUID:aUID];
}

- (void)switchEntriesTableViewAction:(id)aAction
{
	[self switchTabViewID:nil];
}

// nil switches to the other view.
- (void)switchTabViewID:(id)aID
{
	GenericEntry *entry = [self currentEntry];
	if (!aID)
		aID = [[[_tabView selectedTabViewItem] identifier] isEqualToString:_kEntriesTableViewID]
			? _kAdvancedEntriesTableViewID : _kEntriesTableViewID;
	if (![[[_tabView selectedTabViewItem] identifier] isEqualToString:aID]) {
		BOOL advanced = [aID isEqualToString:_kAdvancedEntriesTableViewID];
		[advanced ? (NSTableView *)_advancedEntriesTableView : (NSTableView *)_entriesTableView reloadData];
		[self runTransitionToTabViewItemID:aID];
		[_switchEntriesTableViewButton setState:advanced];
	}
	if ([self currentEntry] && entry != [self currentEntry])
		[delegate currentEntryDidChangeIn:self];
}

// Rolls the list up into the search field (to the entries list) or down
// from it (to the preview list), in a borderless panel over the window.
- (void)runTransitionToTabViewItemID:(id)aID
{
	NSView *superview = [_tabView superview];
	NSUInteger autoresizingMask = [_tabView autoresizingMask];
	NSRect frame = [_tabView frame];
	BOOL rollUp = [aID isEqualToString:_kEntriesTableViewID];
	id responder = [[_tabView window] firstResponder];
	id tableView = [self entriesTableView];
	NSPoint origin = [superview convertPoint:frame.origin toView:nil];
	origin = [[[self pane] window] convertRectToScreen:NSMakeRect(origin.x, origin.y, 0.0, 0.0)].origin;

	CGFloat startY, startHeight, endY, endHeight;
	if (rollUp) {
		startY = origin.y;
		startHeight = frame.size.height - 1.0;
		endY = origin.y + frame.size.height - 2.0;
		endHeight = 1.0;
	}
	else {
		[_tabView selectTabViewItemWithIdentifier:aID];
		startY = origin.y + frame.size.height - 2.0;
		startHeight = 1.0;
		endY = origin.y;
		endHeight = frame.size.height - 1.0;
	}

	[_tabView setAutoresizingMask:NSViewNotSizable];
	[_tabView retain];
	[_tabView removeFromSuperviewWithoutNeedingDisplay];
	[_tabView setFrame:NSMakeRect(0.0, 0.0, frame.size.width, frame.size.height)];
	NSPanel *panel = [[NSPanel alloc] initWithContentRect:NSMakeRect(origin.x, startY, frame.size.width, startHeight)
		styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:YES screen:[[_tabView window] screen]];
	[panel setBecomesKeyOnlyIfNeeded:YES];
	[panel setFloatingPanel:YES];
	[panel setOpaque:NO];
	[[panel contentView] addSubview:_tabView];
	[panel orderFront:self];
	[panel displayIfNeeded];
	if (rollUp) {
		[_tabView removeFromSuperviewWithoutNeedingDisplay];
		[_tabView setAutoresizingMask:autoresizingMask];
		[_tabView setFrame:frame];
		[superview addSubview:_tabView];
		[_tabView selectTabViewItemWithIdentifier:aID];
		[superview display];
	}
	[panel setFrame:NSMakeRect(origin.x, endY, frame.size.width, endHeight + 1.0) display:!rollUp animate:YES];
	[panel setFrame:NSMakeRect(origin.x, endY, frame.size.width, endHeight) display:YES animate:YES];
	[panel orderBack:self];
	if (!rollUp) {
		[_tabView removeFromSuperviewWithoutNeedingDisplay];
		[_tabView setAutoresizingMask:autoresizingMask];
		[_tabView setFrame:frame];
		[superview addSubview:_tabView];
		[superview display];
	}
	[_tabView release];
	[panel close];
	[panel release];
	if (responder == tableView)
		[[_tabView window] makeFirstResponder:[self entriesTableView]];
	[self _validateInterface];
}

// The preview shows as many rows as fit.
- (void)rebuildLayout
{
	NSRect frame = [[_advancedEntriesTableView enclosingScrollView] frame];
	CGFloat rowHeight = [_advancedEntriesTableView rowHeight];
	CGFloat spacing = [_advancedEntriesTableView intercellSpacing].height;
	_entriesDataSource->setPreviewEntriesLimit((int)floorf(frame.size.height / (spacing + rowHeight)));
	if (![self _isAdvancedViewSelected])
		return;
	GenericEntry *entry = [self currentEntry];
	[_advancedEntriesTableView reloadData];
	if (entry != [self currentEntry])
		[delegate currentEntryDidChangeIn:self];
}

- (void)reloadGenericList
{
	[self entriesWereChanged];
}

- (GenericEntry *)currentEntry
{
	return [self selectedItem];
}

- (void)setLastSelectedEntryUID:(LString *)aUID
{
	if ([self _isAdvancedViewSelected]) {
		if (aUID)
			aUID->retain();
		if (_selectedPreviewEntryUID) {
			_selectedPreviewEntryUID->release();
			_selectedPreviewEntryUID = 0;
		}
		_selectedPreviewEntryUID = aUID;
	}
	[super setLastSelectedEntryUID:aUID];
}

- (void)awakeFromNib
{
	((AdvancedEntriesListDataSource *)_entriesDataSource)->_entriesListController = self;
	[super awakeFromNib];
	_foundByNameBackgroundColor = [[NSColor colorWithDeviceWhite:0.95f alpha:1.0f] retain];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(rebuildLayout)
		name:NSViewFrameDidChangeNotification object:_advancedEntriesTableView];
	[_advancedEntriesTableView setPostsFrameChangedNotifications:YES];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_validatePlusButton)
		name:_WGPaletteDidCloseNotification object:[[AtlasController atlasController] newLocationController]];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_validatePlusButton)
		name:_WGPaletteDidBecomeKey object:[[AtlasController atlasController] newLocationController]];
	WGImageTextTableCell *cell = [[WGImageTextTableCell new] autorelease];
	[[_advancedEntriesTableView tableColumnWithIdentifier:_kTypeColumnName] setDataCell:cell];
	[[_advancedEntriesTableView tableColumnWithIdentifier:_kNameColumnName] setDataCell:cell];
	[_statusBar setImage:[NSImage imageNamed:@"tools_background"] forTexture:1];
	[_entriesListView setFillColor:[NSColor colorWithCalibratedRed:0.953f green:0.953f blue:0.953f alpha:1.0f]];
	[_popupMenuButton sendActionOn:NSEventMaskLeftMouseDown];
	[_entriesTableView setDoubleAction:@selector(showDetails:)];
	[_advancedEntriesTableView setDoubleAction:@selector(showDetails:)];
	[(WGCustomTableView *)_entriesTableView createCornerMenuForButton:_columnSelectButton];
	[_switchEntriesTableViewButton setTarget:self];
	[_switchEntriesTableViewButton setAction:@selector(switchEntriesTableViewAction:)];
	[_tabView selectTabViewItemWithIdentifier:_kAdvancedEntriesTableViewID];
	[_switchEntriesTableViewButton setState:NSControlStateValueOn];
	[self _validateInterface];
	_isEntriesTableViewReady = YES;
	[self rebuildLayout];
}

- (BOOL)validateEntriesListCommand:(id)aCommand
{
	switch ([aCommand tag]) {
	case kCommandAddToPalette:
		return [[[AtlasController atlasController] citiesPaletteController]
			canAddEntries:[self selectedItemsWithLimit:Preferences::citiesCountLimit() * 2 createIfNeeded:YES]];
	case kCommandAddToRoute: {
		BOOL valid = [_entriesTableView numberOfSelectedRows] > 0;
		if (![self _isAdvancedViewSelected])
			return valid;
		// Every selected preview row must be an entry.
		valid = [[self advancedEntriesTableView] numberOfSelectedRows] > 0;
		NSIndexSet *rows = [[self advancedEntriesTableView] selectedRowIndexes];
		for (NSUInteger row = [rows firstIndex]; row != NSNotFound; row = [rows indexGreaterThanIndex:row])
			if ((NSInteger)row >= 0)
				valid = valid && [self searchedEntryAtIndex:row createIfNeeded:YES];
		return valid;
	}
	case kCommandDelete: {
		if ([[[[AtlasController atlasController] mainWindowController] entryDetailsController] isEditing])
			return NO;
		if (![self _isAdvancedViewSelected]) {
			NSIndexSet *rows = [[self entriesTableView] selectedRowIndexes];
			TArray<int> indexes;
			for (NSUInteger row = [rows firstIndex]; row != NSNotFound; row = [rows indexGreaterThanIndex:row])
				if ((NSInteger)row >= 0)
					indexes.addObject((int)row);
			return _entriesDataSource->canDeleteEntries(&indexes);
		}
		NSIndexSet *rows = [[self advancedEntriesTableView] selectedRowIndexes];
		TLArray<GenericEntry *> *entries = (TLArray<GenericEntry *> *)LArray::array();
		for (NSUInteger row = [rows firstIndex]; row != NSNotFound; row = [rows indexGreaterThanIndex:row]) {
			GenericEntry *entry = (NSInteger)row >= 0 ? [self searchedEntryAtIndex:row createIfNeeded:YES] : 0;
			if (entry)
				entries->addObject(entry);
		}
		return _entriesDataSource->canDeleteEntries(entries);
	}
	}
	return YES;
}

@end
