// WGCurrentSetController
#import "WGCurrentSetController.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "CIPaletteController.h"
#import "WGRouteManagerController.h"
#import "WGRouteImportExport.h"
#import "WGCurrentSetTableView.h"
#import "RouteSetPrintDataSource.h"
#import "WGColorSelectionCell.h"
#import "WGCGImageCell.h"
#import "WGTexturedView.h"
#import "WGCenteredTextAttachmentCell.h"
#import "NSCell+WGExtensions.h"
#import "NSColor+WGExtensions.h"
#import "NSMenu+NSMenuWGExtensions.h"
#import "NSPasteboard+NSPasteboardWGExtensions.h"
#import "NSMutableString+NSStringWGExtensions.h"
#import "NSString_LStringExtension.h"

NSString *_WGCurrentSetEntryIndexesPasteboardType = @"WGCurrentSetEntryIndexesPasteboardType";
NSString *_WGCurrentSetRouteWasChangedNotification = @"WGCurrentSetRouteWasChangedNotification";
NSString *_WGCurrentSetRouteWasAddedNotification = @"WGCurrentSetRouteWasAddedNotification";

static NSString *const _kLocationDistancePlaceholder = @"---";
static NSString *const _kLocationColumnDistance = @"interval";
static NSString *const _kFlagsColumnName = @"flags";
static NSString *const _kDescriptionColumnName = @"description";
static NSString *const _kCurrentSetFlagsMaskImageName = @"colormask";
static NSString *const _kLocationColumnName = @"location_name";

static LConstantString _kRouteRevisionKey("revision");

// The route commands' menu item tags.
enum {
	kRouteCommandAddToRouteManager = 0,
	kRouteCommandImport = 1,
	kRouteCommandExport = 2,
	kRouteCommandEmail = 3,
	kRouteCommandToggleRouteLines = 4,
	kRouteCommandAddBreak = 5,
	kRouteCommandChangeColor = 6,
	kRouteCommandAddToCitiesPalette = 7,
	kRouteCommandClear = 8,
	kRouteCommandDelete = 9,
	kRouteCommandFlyMeThere = 11,
	kRouteCommandAddToDatabase = 13
};

@implementation WGCurrentSetController

+ (id)wgIdentifier
{
	return @"CurrentSet";
}

- (id)init
{
	self = [super init];
	if (self) {
		_currentSetController = CurrentSetController::_alloc()->init();
		LNotificationCenter::center()->addObserver(self, @selector(routeChangedNotification:),
		                                           RouteObject::kRouteObjectAttributesDidChangeNotification,
		                                           _currentSetController->routeObject());
		LNotificationCenter::center()->addObserver(self, @selector(routeItemCountChangedNotification:),
		                                           RouteObject::kRouteObjectItemCountDidChangeNotification,
		                                           _currentSetController->routeObject());
	}
	return self;
}

// "Interval (km)" or "Interval (mi)".
static NSString *_intervalTitle()
{
	NSString *unit = NSLocalizedString(Preferences::usesMetricSystem() ? @"Kilometer" : @"Mile", @"");
	return [NSString stringWithFormat:NSLocalizedString(@"Interval", @""), unit];
}

- (void)awakeFromNib
{
	[[[listTableView tableColumnWithIdentifier:_kFlagsColumnName] headerCell] setImage:[NSImage imageNamed:@"flagged_header"]];
	NSCell *distanceHeader = [[listTableView tableColumnWithIdentifier:_kLocationColumnDistance] headerCell];
	[distanceHeader setTitle:_intervalTitle()];
	[distanceHeader wgFixLineBreakingTo:NSLineBreakByTruncatingTail];
	[[[listTableView tableColumnWithIdentifier:_kLocationColumnDistance] dataCell]
	    setPlaceholderString:_kLocationDistancePlaceholder];
	WGColorSelectionCell *colorCell = [[WGColorSelectionCell new] autorelease];
	[[listTableView tableColumnWithIdentifier:_kFlagsColumnName] setDataCell:colorCell];
	[[listTableView tableColumnWithIdentifier:_kIconColumnName] setDataCell:[[[WGCGImageCell alloc] init] autorelease]];
	[listTableView registerForDraggedTypes:[NSArray arrayWithObjects:_kEntriesListPasteboardType,
	                                                                 _WGCurrentSetEntryIndexesPasteboardType, nil]];
	[listTableView setTarget:self];
	[listTableView setDoubleAction:@selector(doubleAction:)];
	[listTableView setWGPlaceholderString:[placeholderStringProvider textStorage]];
	[[balloonTableView tableColumnWithIdentifier:_kFlagsColumnName] setDataCell:colorCell];
	[statusBar setImage:[NSImage imageNamed:@"tools_background"] forTexture:1];
	[popupMenuButton sendActionOn:NSEventMaskLeftMouseDown];

	[[self tableView] createCornerMenuForButton:columnSelectButton];
	NSString *distanceTitle = [[[[self tableView] tableColumnWithIdentifier:_kLocationColumnDistance] headerCell] stringValue];
	[[[[self tableView] cornerViewPopupMenu] itemWithTitle:distanceTitle] setTitle:NSLocalizedString(@"Interval Word", @"")];

	[self routeChangedNotification:nil];
	LNotificationCenter::center()->addObserver(self, @selector(measurementUnitsChangedNotification:),
	                                           Preferences::kUnitsOfMeasureWereChanged, (LObject *)0);
	id routeManager = [[AtlasController atlasController] routeManagerPaletteController];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(_validateRouteManagerButton)
	                                             name:_WGPaletteDidCloseNotification
	                                           object:routeManager];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(_validateRouteManagerButton)
	                                             name:_WGPaletteDidBecomeKey
	                                           object:routeManager];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	if (_currentSetController) {
		_currentSetController->release();
		_currentSetController = 0;
	}
	[_retainedView release];
	[_draggedEntriesIndexes release];
	[_currentRoutePath release];
	[super dealloc];
}

- (id)nibName
{
	return @"CurrentSetView";
}

// Swaps the list and the balloon tables in the pane.
- (void)setMode:(NSInteger)aMode
{
	if (_mode == aMode)
		return;
	NSView *oldView = [[[self tableView] enclosingScrollView] retain];
	[oldView removeFromSuperview];
	_mode = (int)aMode;
	if (!_retainedView)
		_retainedView = [[[self tableView] enclosingScrollView] retain];
	[pane addSubview:_retainedView];
	[_retainedView setFrame:[oldView frame]];
	[_retainedView release];
	_retainedView = oldView;
	[[self tableView] reloadData];
	[delegate currentEntryDidChangeIn:self];
}

- (NSInteger)mode
{
	return _mode;
}

- (id)tableView
{
	[self loadNibIfNeeded];
	return [self mode] ? balloonTableView : listTableView;
}

- (NSInteger)_insertionRowIndex
{
	NSInteger row = [[self tableView] selectedRow];
	if (row < 0)
		return _currentSetController->routeObject()->itemsCount();
	return row + 1;
}

- (void)_selectRowsWithRange:(NSRange)aRange
{
	_disableAutoscrollOnListSelectionChange = YES;
	NSIndexSet *indexes = [NSIndexSet indexSetWithIndexesInRange:aRange];
	[[self tableView] selectRowIndexes:indexes byExtendingSelection:NO];
	[[self tableView] scrollRowToVisible:[indexes lastIndex]];
	[[self container] mountController:self];
	_disableAutoscrollOnListSelectionChange = NO;
}

- (RouteSetItem *)_itemAtRow:(NSInteger)aRow
{
	return _currentSetController->routeObject()->itemAtIndex((int)aRow);
}

- (GenericEntry *)selectedItem
{
	NSInteger row = [[self tableView] selectedRow];
	if (row < 0 || row == NSNotFound)
		return 0;
	RouteSetItem *item = [self _itemAtRow:row];
	return item ? item->genericEntryInstance() : 0;
}

- (void)addEntryToRoute:(RouteSetItem *)aRoute
{
	if (!aRoute)
		return;
	NSInteger index = [self _insertionRowIndex];
	_currentSetController->insertItemIntoRoute(aRoute, (int)index, 0, 0);
	[self _selectRowsWithRange:NSMakeRange(index, 1)];
}

- (void)_updateTotalDistance
{
	LString *distance = Utils::distanceDescription(_currentSetController->routeObject()->totalDistance());
	[totalDistance setStringValue:[NSString stringReferenceFromLString:distance]];
}

- (void)_validateRouteManagerButton
{
	BOOL visible = [[[AtlasController atlasController] routeManagerPaletteController] isPaletteVisible];
	[routeManagerButton setState:visible ? NSControlStateValueOn : NSControlStateValueOff];
}

- (void)routeItemCountChangedNotification:(id)aNotification
{
	[self routeChangedNotification:aNotification];
	[[NSNotificationCenter defaultCenter] postNotificationName:_WGCurrentSetRouteWasChangedNotification object:self];
	[[self tableView] selectRow:[[[self tableView] selectedRowIndexes] firstIndex] byExtendingSelection:NO];
	[delegate currentEntryDidChangeIn:self];
}

// (Tiger also needed the buttons' superview redrawn.)
- (void)routeChangedNotification:(id)aNotification
{
	[[self tableView] reloadData];
	[self _updateTotalDistance];
	RouteObject *route = _currentSetController->routeObject();
	[flyMeThereButton setEnabled:route->locationsCount() > 1];
	[saveButton setEnabled:route->locationsCount() > 0];
	[clearRouteButton setEnabled:route->itemsCount() > 0];
}

- (void)measurementUnitsChangedNotification:(id)aNotification
{
	[[self tableView] reloadData];
	[self _updateTotalDistance];
	NSCell *header = [[listTableView tableColumnWithIdentifier:_kLocationColumnDistance] headerCell];
	[header setTitle:_intervalTitle()];
	[header wgFixLineBreakingTo:NSLineBreakByTruncatingTail];
}

- (void)tableViewSelectionDidChange:(id)aChange
{
	[delegate currentEntryDidChangeIn:self];
	if (!_disableAutoscrollOnListSelectionChange)
		[[[AtlasController atlasController] mainWindowController] gotoLocation:[self currentEntry] forced:NO];
}

- (RouteObject *)currentRoute
{
	return _currentSetController->routeObject();
}

- (void)replaceCurrentSet:(RouteObject *)aSet
{
	_currentSetController->replaceCurrentSet(aSet);
	[[self container] mountController:self];
	[[NSNotificationCenter defaultCenter] postNotificationName:_WGCurrentSetRouteWasAddedNotification object:self];
}

- (void)addToCurrentSet:(RouteObject *)aSet withIndex:(NSInteger)aIndex
{
	_currentSetController->addToCurrentSet(aSet, (int)aIndex);
	[[self container] mountController:self];
	[[NSNotificationCenter defaultCenter] postNotificationName:_WGCurrentSetRouteWasAddedNotification object:self];
}

- (void)addToCurrentSet:(RouteObject *)aSet
{
	NSInteger index = [[self tableView] selectedRow];
	if (index == NSNotFound)
		index = _currentSetController->routeObject()->itemsCount();
	[self addToCurrentSet:aSet withIndex:index];
}

- (id)distanceStringForDistance:(CGFloat)aDistance
{
	return [NSString stringReferenceFromLString:Utils::distanceShortDescription((float)aDistance)];
}

- (id)distanceForSubset:(NSInteger)aSubset
{
	return [self distanceStringForDistance:_currentSetController->routeObject()->distanceForSubset((int)aSubset)];
}

- (void)openColorPanelWithColor:(id)aColor
{
	[NSApp orderFrontColorPanel:self];
	[[NSColorPanel sharedColorPanel] setContinuous:NO];
	[[NSColorPanel sharedColorPanel] setColor:aColor];
}

#pragma mark Table view

- (NSInteger)numberOfRowsInTableView:(id)aView
{
	return _currentSetController->routeObject()->itemsCount();
}

- (BOOL)wgValidateDeleteItemsInTableView:(id)aView
{
	return [self numberOfRowsInTableView:aView] > 0;
}

- (BOOL)tableView:(id)aView shouldEditTableColumn:(id)aColumn row:(NSInteger)aRow
{
	id identifier = [aColumn identifier];
	if (![identifier isEqual:_kFlagsColumnName] && ![identifier isEqual:_kLocationColumnName])
		return NO;
	RouteSetItem *item = [self _itemAtRow:aRow];
	return item && item->genericEntryInstance();
}

- (BOOL)wgTableView:(id)aView shouldDisplayBreakAtColumn:(id)aColumn
{
	return !(listTableView == aView && [[aColumn identifier] isEqual:_kLocationColumnDistance]);
}

- (BOOL)wgTableView:(id)aView shouldDisplayBreakAtRow:(NSInteger)aRow
{
	RouteSetItem *item = [self _itemAtRow:aRow];
	return !(item && item->genericEntryInstance());
}

- (BOOL)isWGTableViewPrinted:(id)aPrinted
{
	return [self tableView] != aPrinted;
}

- (id)wgColumnIDForEditingInTableView:(id)aView
{
	return _kLocationColumnName;
}

- (id)wgTableView:(id)aView distanceForSubset:(NSInteger)aSubset
{
	return [self distanceForSubset:aSubset];
}

- (BOOL)tableView:(id)aView canHideTableColumn:(id)aColumn
{
	NSString *identifier = [aColumn identifier];
	return ![identifier isEqualToString:_kIconColumnName] && ![identifier isEqualToString:_kLocationColumnName];
}

- (id)orderedTableColumnIDsForTableView:(id)aView
{
	return [NSArray arrayWithObjects:_kIconColumnName, _kLocationColumnName, _kLocationColumnDistance, _kFlagsColumnName, nil];
}

- (NSUInteger)tableView:(id)aView validateDrop:(id)aDrop proposedRow:(NSInteger)aRow proposedDropOperation:(NSInteger)aOperation
{
	[aView setDropRow:aRow dropOperation:NSTableViewDropAbove];
	if (([aDrop draggingSourceOperationMask] & NSDragOperationMove) && [aDrop draggingSource] == aView)
		return NSDragOperationMove;
	return [aDrop draggingSourceOperationMask] & NSDragOperationCopy;
}

- (void)wgChangeItemColorInTableView:(id)aView
{
	[self applyColorToSelection];
}

// The column menu over the header; the context menu over the rows, selecting the clicked one.
- (id)tableView:(id)aView menuForEvent:(id)aEvent
{
	NSPoint point = [[aView superview] convertPoint:[aEvent locationInWindow] fromView:nil];
	NSView *header = [aView headerView];
	if ([header hitTest:[aView convertPoint:point toView:header]])
		return [[self tableView] cornerViewPopupMenu];
	if ([aView hitTest:point] != aView)
		return nil;
	NSInteger row = [aView rowAtPoint:point];
	if (row != -1 && ![[aView selectedRowIndexes] containsIndex:row]) {
		if ([[aView window] firstResponder] != aView)
			[[aView window] makeFirstResponder:aView];
		[aView selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
	}
	return contextMenu;
}

- (void)tableView:(id)aView didClickTableColumn:(id)aColumn
{
	[delegate currentEntryDidChangeIn:self];
}

// The dragged rows are kept; the data is provided lazily.
- (BOOL)tableView:(id)aView writeRows:(id)aRows toPasteboard:(id)aPasteboard
{
	if ([aRows count] <= 0)
		return NO;
	[aPasteboard declareTypes:[NSArray arrayWithObjects:_WGCurrentSetEntryIndexesPasteboardType, _kEntriesListPasteboardType,
	                                                    NSPasteboardTypeString, nil]
	                    owner:self];
	[aPasteboard setData:[NSData data] forType:_WGCurrentSetEntryIndexesPasteboardType];
	[_draggedEntriesIndexes release];
	_draggedEntriesIndexes = [NSMutableIndexSet new];
	for (id row in aRows)
		[_draggedEntriesIndexes addIndex:[row intValue]];
	return YES;
}

- (void)doubleAction:(id)aAction
{
	NSTableView *tableView = [self tableView];
	NSTableColumn *column = [[tableView tableColumns] objectAtIndex:[tableView clickedColumn]];
	if ([tableView clickedRow] == -1)
		return;
	if ([[column identifier] isEqual:_kFlagsColumnName]) {
		id value = [self tableView:tableView objectValueForTableColumn:column row:[tableView clickedRow]];
		id color = [value objectForKey:_kWGColorSelectionCellColorKey];
		if ([aAction respondsToSelector:@selector(color)])
			color = [aAction color];
		[self openColorPanelWithColor:color];
		return;
	}
	[[self tableView] sendAction:@selector(showDetails:) to:nil];
}

- (void)onGearButton:(id)aButton
{
	[NSMenu wgPopUpContextMenu:popupMenu withEvent:[[aButton window] currentEvent] forView:aButton];
}

- (void)flyStoped
{
	[flyMeThereButton setState:NSControlStateValueOff];
}

- (GenericEntry *)currentEntry
{
	return [self selectedItem];
}

- (void)wgPerformUndoInTableView:(id)aView
{
	CurrentSetController::undoManager()->undo();
}

- (void)wgPerformRedoInTableView:(id)aView
{
	CurrentSetController::undoManager()->redo();
}

// The distance from the previous location of the same subset, "---" at a subset's start.
- (id)_distanceForEntry:(GenericEntry *)aEntry row:(NSInteger)aRow
{
	PointEntry *point = lobj_cast<PointEntry>(aEntry);
	if (!point)
		return _kLocationDistancePlaceholder;
	PointEntry *previous = 0;
	while (!previous) {
		if (--aRow < 0)
			return _kLocationDistancePlaceholder;
		RouteSetItem *item = [self _itemAtRow:aRow];
		if (item->routeBreakInstance())
			return _kLocationDistancePlaceholder;
		previous = lobj_cast<PointEntry>(item);
	}
	return [self distanceStringForDistance:previous->distanceToEntry(point)];
}

- (id)tableView:(id)aView objectValueForTableColumn:(id)aColumn row:(NSInteger)aRow
{
	RouteSetItem *item = [self _itemAtRow:aRow];
	if (!item)
		return nil;
	GenericEntry *entry = item->genericEntryInstance();
	id identifier = [aColumn identifier];
	if (!entry) {
		// A break: the subset's distance under the distance column.
		if (item->routeBreakInstance() && ![self wgTableView:aView shouldDisplayBreakAtColumn:aColumn]) {
			NSDictionary *attributes = [[subdistanceTemplate attributedStringValue] attributesAtIndex:0 effectiveRange:NULL];
			return [[[NSAttributedString alloc] initWithString:[self distanceForSubset:aRow] attributes:attributes]
			    autorelease];
		}
		return nil;
	}
	RouteObject *route = _currentSetController->routeObject();
	if ([identifier isEqual:_kLocationColumnName]) {
		if (aView != [self tableView]) {
			NSInteger lineBreaking = [aView respondsToSelector:@selector(lineBraking)] ? [(WGPrintRouteSetTableView *)aView lineBraking] : 0;
			return [self entryExtendedInfo:entry withLineBreaking:lineBreaking ownEntryName:NO];
		}
		NSString *name = [NSString stringReferenceFromLString:route->nameForItem(entry)];
		NSAttributedString *string = name ? [[[NSAttributedString alloc] initWithString:name] autorelease] : nil;
		return [string wgSetLineBreak:NSLineBreakByTruncatingTail];
	}
	if ([identifier isEqual:_kLocationColumnDistance])
		return [self _distanceForEntry:entry row:aRow];
	if ([identifier isEqual:_kFlagsColumnName])
		return [NSDictionary dictionaryWithObjectsAndKeys:[NSColor colorWithWGColor:route->colorForItem(entry)],
		                                                  _kWGColorSelectionCellColorKey,
		                                                  [NSImage imageNamed:_kCurrentSetFlagsMaskImageName],
		                                                  _kWGColorSelectionCellImageKey, nil];
	if ([identifier isEqual:_kIconColumnName]) {
		LImage *icon = entry->previewIcon(0);
		return icon ? (id)icon->CGImage() : nil;
	}
	if ([identifier isEqual:_kDescriptionColumnName]) {
		NSString *name = [NSString stringReferenceFromLString:route->nameForItem(entry)];
		NSAttributedString *string = name ? [[[NSAttributedString alloc] initWithString:name] autorelease] : nil;
		return [string wgSetLineBreak:NSLineBreakByTruncatingTail];
	}
	return nil;
}

- (void)tableView:(id)aView setObjectValue:(id)aValue forTableColumn:(id)aColumn row:(NSInteger)aRow
{
	RouteSetItem *item = [self _itemAtRow:aRow];
	if (!item)
		return;
	GenericEntry *entry = item->genericEntryInstance();
	if (entry && [[aColumn identifier] isEqual:_kLocationColumnName])
		_currentSetController->setNameForEntry(LString::stringWithCFString((CFStringRef)aValue), entry);
}

- (void)importRouteObject:(id)aObject
{
	RouteObject *route = lobj_cast<RouteObject>([aObject getLObject]);
	if (route)
		[self replaceCurrentSet:route];
}

// "Name (kind, region)" over the coordinates, in gray.
- (id)entryExtendedInfo:(GenericEntry *)aInfo withLineBreaking:(NSInteger)aBreaking ownEntryName:(BOOL)aName
{
	if (!aInfo)
		return nil;
	NSString *coordinates = @"";
	PointEntry *point = lobj_cast<PointEntry>(aInfo);
	if (point)
		coordinates = [NSString stringWithFormat:@"\n%@", [NSString wgShortGeoCoordinatesDescription:point->geoPoint(0)]];
	LString *nameString = aName ? aInfo->ownName(0) : _currentSetController->routeObject()->nameForItem(aInfo);
	NSString *name = [NSString stringReferenceFromLString:nameString];
	NSString *kind = [NSString stringReferenceFromLString:aInfo->kind(0)];
	NSString *region = [NSString stringReferenceFromLString:aInfo->fullRegionName(0)];
	if (aName)
		name = [NSString stringWithFormat:region ? @"%@ (%@, %@)" : @"%@ (%@)", name, kind, region];

	NSDictionary *nameAttributes = [NSDictionary dictionaryWithObject:[NSFont systemFontOfSize:13.0] forKey:NSFontAttributeName];
	NSMutableAttributedString *info = [[[NSMutableAttributedString alloc] initWithString:name attributes:nameAttributes] autorelease];
	NSParagraphStyle *style = [info attribute:NSParagraphStyleAttributeName atIndex:0 effectiveRange:NULL];
	if (!style)
		style = [NSParagraphStyle defaultParagraphStyle];
	NSMutableParagraphStyle *mutableStyle = [style mutableCopy];
	[mutableStyle setLineBreakMode:(NSLineBreakMode)aBreaking];
	[info addAttribute:NSParagraphStyleAttributeName value:mutableStyle range:NSMakeRange(0, [info length])];
	[mutableStyle release];
	NSDictionary *coordinatesAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
	                                                        [NSFont systemFontOfSize:11.0], NSFontAttributeName,
	                                                        [NSColor colorWithCalibratedWhite:0.549 alpha:1.0],
	                                                        NSForegroundColorAttributeName, nil];
	[info appendAttributedString:[[[NSAttributedString alloc] initWithString:coordinates attributes:coordinatesAttributes]
	                                 autorelease]];
	return [[[NSAttributedString alloc] initWithAttributedString:info] autorelease];
}

- (void)addEntriesToRoute:(TLArray<GenericEntry*> *)aRoute
{
	if (!aRoute || !aRoute->count())
		return;
	NSInteger index = [self _insertionRowIndex];
	int count = _currentSetController->insertItemsIntoRoute((TLArray<RouteSetItem *> *)aRoute, (int)index);
	if (count)
		[self _selectRowsWithRange:NSMakeRange(index, count)];
}

- (void)addEntriesFromDescrArray:(TLArray<LF::LMap*> *)aArray startingFromRow:(NSInteger)aRow
{
	if (!aArray || !aArray->count())
		return;
	if (aRow < 0)
		aRow = [self _insertionRowIndex];
	int count = _currentSetController->insertEntriesFromDescrArray(aArray, (int)aRow);
	if (count)
		[self _selectRowsWithRange:NSMakeRange(aRow, count)];
}

#pragma mark Route commands

- (BOOL)validateRouteCommand:(id)aCommand
{
	RouteObject *route = _currentSetController->routeObject();
	switch ([aCommand tag]) {
	case kRouteCommandAddToRouteManager:
	case kRouteCommandExport:
	case kRouteCommandEmail:
	case kRouteCommandAddBreak:
	case kRouteCommandFlyMeThere:
		return route && route->locationsCount() > 0;
	case kRouteCommandToggleRouteLines:
		[aCommand setTitle:NSLocalizedString(Preferences::showRouteLines() ? @"Hide Route Lines" : @"Show Route Lines", @"")];
		return route && route->locationsCount() >= 2;
	case kRouteCommandChangeColor: {
		TLArray<GenericEntry *> *items = [self selectedItems];
		return items && items->count() > 0;
	}
	case kRouteCommandAddToCitiesPalette:
		return [[[AtlasController atlasController] citiesPaletteController] canAddEntries:[self selectedItems]];
	case kRouteCommandClear:
		return route && route->itemsCount() > 0;
	case kRouteCommandDelete:
		return [[self tableView] selectedRow] != -1;
	case kRouteCommandAddToDatabase:
		return [self _canAddSelectedItemsToDatabase];
	default:
		return YES;
	}
}

// Temporary entries and locations can be saved.
- (BOOL)_canAddSelectedItemsToDatabase
{
	TLArray<GenericEntry *> *items = [self selectedItems];
	int count = items ? items->count() : 0;
	for (int i = 0; i < count; i++) {
		GenericEntry *entry = items->objectAtIndex(i);
		if (entry->isEntryTemporary() || lobj_cast<PointEntry>(entry))
			return YES;
	}
	return NO;
}

- (void)_addSelectedItemsToDatabase
{
	TLArray<GenericEntry *> *items = [self selectedItems];
	int count = items ? items->count() : 0;
	for (int i = 0; i < count; i++) {
		GenericEntry *entry = items->objectAtIndex(i);
		if (entry->isEntryTemporary())
			entry->commit();
		else if (PointEntry *point = lobj_cast<PointEntry>(entry)) {
			UserCustomEntry *custom = UserCustomEntry::_alloc()->initWithPointEntry(point);
			if (custom) {
				custom->commit();
				custom->release();
			}
		}
	}
}

- (void)doRouteCommand:(NSInteger)aCommand
{
	RouteObject *route = _currentSetController->routeObject();
	switch (aCommand) {
	case kRouteCommandAddToRouteManager:
		[[[AtlasController atlasController] routeManagerPaletteController]
		    addRouteEntry:UserRouteObjectEntry::entryWithRouteObject(LAutorelease(route->copy()))];
		break;
	case kRouteCommandImport:
		[[WGRouteImportExport sharedRouteImportExport]
		    importRouteWithTarget:self
		           didEndSelector:@selector(importRouteObject:)
		               hostWindow:[[[AtlasController atlasController] mainWindowController] mainWindow]];
		break;
	case kRouteCommandExport:
		[[WGRouteImportExport sharedRouteImportExport]
		    exportRoute:route
		       withName:nil
		     hostWindow:[[[AtlasController atlasController] mainWindowController] mainWindow]];
		break;
	case kRouteCommandEmail: {
		LMap *representation = route->dictionaryRepresentation();
		representation->setObjectForKey(LString::stringWithFormat("%d", _routeDocRevision + 1), &_kRouteRevisionKey);
		NSMutableString *fileName = [[[[NSString stringReferenceFromLString:route->preferredName()]
		    stringByAppendingPathExtension:_kRouteSetFileExt] mutableCopy] autorelease];
		[fileName replaceOccurrencesOfString:@"/" withString:@"-" options:0 range:NSMakeRange(0, [fileName length])];
		NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent:fileName];
		if (representation->writeToFile(LString::stringWithCFString((CFStringRef)path)))
			wgCreateMailWithAttachment(path, NSLocalizedString(@"Route Email attachment text", @""));
		break;
	}
	case kRouteCommandToggleRouteLines:
		Preferences::setShowRouteLines(!Preferences::showRouteLines());
		break;
	case kRouteCommandAddBreak:
		[self addEntryToRoute:RouteSetBreak::routeSetBreak()];
		break;
	case kRouteCommandChangeColor: {
		NSWindow *window = [[[AtlasController atlasController] mainWindowController] mainWindow];
		NSTableView *tableView = [self tableView];
		if ([window firstResponder] != tableView)
			[window makeFirstResponder:tableView];
		id value = [self tableView:tableView
		    objectValueForTableColumn:[tableView tableColumnWithIdentifier:_kFlagsColumnName]
		                          row:[tableView selectedRow]];
		[self openColorPanelWithColor:[value objectForKey:_kWGColorSelectionCellColorKey]];
		break;
	}
	case kRouteCommandAddToCitiesPalette:
		[[[AtlasController atlasController] citiesPaletteController] addEntries:[self selectedItems]];
		break;
	case kRouteCommandClear:
		[self replaceCurrentSet:0];
		break;
	case kRouteCommandDelete:
		[self wgDeleteItemsInTableView:[self tableView]];
		break;
	case kRouteCommandAddToDatabase:
		[self _addSelectedItemsToDatabase];
		break;
	}
}

- (TLArray<GenericEntry*> *)selectedItems
{
	NSIndexSet *indexes = [[self tableView] selectedRowIndexes];
	TLArray<GenericEntry *> *items = (TLArray<GenericEntry *> *)LArray::array();
	for (NSUInteger index = [indexes firstIndex]; index != NSNotFound; index = [indexes indexGreaterThanIndex:index]) {
		RouteSetItem *item = [self _itemAtRow:index];
		if (item && item->genericEntryInstance())
			items->addObject(item->genericEntryInstance());
	}
	return items;
}

- (void)wgDeleteItemsInTableView:(id)aView
{
	NSIndexSet *indexes = [aView selectedRowIndexes];
	if (![indexes count])
		return;
	TLArray<LValue *> *values = (TLArray<LValue *> *)LArray::array();
	for (NSUInteger index = [indexes firstIndex]; index != NSNotFound; index = [indexes indexGreaterThanIndex:index])
		values->addObject(LValue::valueWithInt((int)index));
	_currentSetController->deleteItemsWithOrderedIndicies(values);
}

// Text: one line per item ("-" for breaks); entries: their dictionaries.
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType
{
	RouteObject *route = _currentSetController->routeObject();
	if ([aType isEqual:NSPasteboardTypeString]) {
		NSMutableString *text = nil;
		for (NSUInteger index = [_draggedEntriesIndexes firstIndex]; index != NSNotFound;
		     index = [_draggedEntriesIndexes indexGreaterThanIndex:index]) {
			RouteSetItem *item = route->itemAtIndex((int)index);
			GenericEntry *entry = item ? item->genericEntryInstance() : 0;
			NSString *line = @"";
			if (entry)
				line = [NSString stringReferenceFromLString:entry->shortCaption(0)];
			else if (item && item->routeBreakInstance())
				line = @"-";
			if (!text)
				text = [NSMutableString stringWithString:line];
			else {
				[text appendString:@"\n"];
				[text appendString:line];
			}
		}
		[aPasteboard setString:text forType:aType];
	} else if ([aType isEqual:_kEntriesListPasteboardType]) {
		LArray *descriptions = LArray::array();
		for (NSUInteger index = [_draggedEntriesIndexes firstIndex]; index != NSNotFound;
		     index = [_draggedEntriesIndexes indexGreaterThanIndex:index]) {
			RouteSetItem *item = route->itemAtIndex((int)index);
			GenericEntry *entry = item ? item->genericEntryInstance() : 0;
			if (entry)
				descriptions->addObject(entry->entryDictionary());
		}
		[aPasteboard wgSetPropertyList:descriptions forType:aType];
	}
}

- (void)applyColorToSelection
{
	NSIndexSet *indexes = [[self tableView] selectedRowIndexes];
	if (![NSColorPanel sharedColorPanelExists] || ![indexes count])
		return;
	TLArray<GenericEntry *> *entries = (TLArray<GenericEntry *> *)LArray::array();
	NSColor *color = [[NSColorPanel sharedColorPanel] color];
	for (NSUInteger index = [indexes firstIndex]; index != NSNotFound; index = [indexes indexGreaterThanIndex:index]) {
		RouteSetItem *item = [self _itemAtRow:index];
		if (item && item->genericEntryInstance())
			entries->addObject(item->genericEntryInstance());
	}
	_currentSetController->setColorForEntries([color wgColorComponents], entries);
}

// Rows dragged inside the table move (or copy); entries from the lists are inserted.
- (BOOL)tableView:(id)aView acceptDrop:(id)aDrop row:(NSInteger)aRow dropOperation:(NSInteger)aOperation
{
	NSPasteboard *pasteboard = [aDrop draggingPasteboard];
	if (![pasteboard dataForType:_WGCurrentSetEntryIndexesPasteboardType]) {
		LArray *descriptions = lobj_cast<LArray>([pasteboard wgPropertyListForType:_kEntriesListPasteboardType]);
		[self addEntriesFromDescrArray:(TLArray<LMap *> *)descriptions startingFromRow:aRow];
		return YES;
	}
	NSIndexSet *dragged = _draggedEntriesIndexes;
	if (![dragged count])
		return YES;
	NSInteger row = aRow + (aOperation == NSTableViewDropOn);
	if (!([aDrop draggingSourceOperationMask] & NSDragOperationMove)) {
		TLArray<RouteSetItem *> *items = (TLArray<RouteSetItem *> *)LArray::array();
		RouteObject *route = _currentSetController->routeObject();
		for (NSUInteger index = [dragged firstIndex]; index != NSNotFound; index = [dragged indexGreaterThanIndex:index])
			items->addObject(route->itemAtIndex((int)index));
		_currentSetController->insertItemsIntoRoute(items, (int)row);
	} else {
		TLArray<LValue *> *values = (TLArray<LValue *> *)LArray::array();
		for (NSUInteger index = [dragged firstIndex]; index != NSNotFound; index = [dragged indexGreaterThanIndex:index])
			values->addObject(LValue::valueWithInt((int)index));
		int first = _currentSetController->moveItemsWithOrderedIndexesToIndex(values, (int)row);
		if ([dragged containsIndex:[aView selectedRow]])
			[aView selectRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(first, [dragged count])]
			    byExtendingSelection:NO];
	}
	[self _updateTotalDistance];
	return YES;
}

#pragma mark Undo

static NSString *_undoTitle(NSString *verb, LString *action)
{
	return [NSString stringWithFormat:@"%@ %@", NSLocalizedString(verb, @""),
	                                  action ? [NSString stringReferenceFromLString:action] : @""];
}

- (BOOL)wgValidateRedo:(id)aRedo inTableView:(id)aView
{
	LUndoManager *undoManager = CurrentSetController::undoManager();
	[aRedo setTitle:_undoTitle(@"Redo", undoManager->redoActionName())];
	return undoManager->canRedo();
}

- (BOOL)wgValidateUndo:(id)aUndo inTableView:(id)aView
{
	LUndoManager *undoManager = CurrentSetController::undoManager();
	[aUndo setTitle:_undoTitle(@"Undo", undoManager->undoActionName())];
	return undoManager->canUndo();
}

@end
