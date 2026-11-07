// WGRouteManagerController
#import "WGRouteManagerController.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "WGCurrentSetController.h"
#import "WGRouteEntryDetailsController.h"
#import "WGRouteImportExport.h"
#import "WGImageAndTextCell.h"
#import "WGSplitView.h"
#import "WGSplitter.h"
#import "WGEarthView.h"
#import "WGCustomFieldEditor.h"
#import "WGCustomTitleView.h"
#import "WGFilledView.h"
#import "WGTexturedButton.h"
#import "WGHelpBook.h"
#import "DragSourceTableView.h"
#import "NSMenu+NSMenuWGExtensions.h"
#import "NSOutlineView+NSOutlineViewWGExtensions.h"
#import "WGEarthViewPlaceholder.h"
#import "NSImage+NSImageWGExtensions.h"
#import "NSString_LStringExtension.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

static NSString *const _kOLVColumnIdentifier = @"OLVColumn";
static NSString *const _kRouteNodesPasteboardDataType = @"RouteNodesPasteboardDataType";

// The outline views stacked in the left scroll view, one per root node.
static const CGFloat kOutlineViewHeight = 20.0;
static const CGFloat kOutlineViewsTop = 6.0;
static const CGFloat kOutlineViewsGap = 8.0;
static const CGFloat kOutlineViewsSeparatorHeight = 2.0;

@implementation WGRouteManagerController

- (id)draggedNodes
{
	return _draggedNodes;
}

- (void)_setDraggedNodes:(id)aNodes
{
	if (_draggedNodes != aNodes) {
		[_draggedNodes release];
		_draggedNodes = [aNodes retain];
	}
}

- (void)_editItem:(id)aItem
{
	NSInteger row = [_editableOutline rowForItem:aItem];
	[(WGRouteManagerOutlineView *)_editableOutline setEditingFlag:NO];
	[_editableOutline selectRow:row byExtendingSelection:NO];
	[_editableOutline editColumn:[_editableOutline columnWithIdentifier:_kOLVColumnIdentifier]
	                         row:row
	                   withEvent:nil
	                      select:YES];
}

- (void)_cancelEditing
{
	_isEditingCanceled = YES;
	[_palette endEditingFor:nil];
	_isEditingCanceled = NO;
}

// The node selected in the first outline view with a selection, or the
// folder holding it.
- (GenericRouteNode *)_geteSelectedRouteNodeFolder:(BOOL)aFolder
{
	NSEnumerator *outlineViews = [_outlineViewsArray objectEnumerator];
	NSArray *selected;
	do {
		id outlineView = [outlineViews nextObject];
		if (!outlineView)
			return 0;
		selected = [outlineView allSelectedItems];
	} while ([selected count] == 0);
	GenericRouteNode *node = (GenericRouteNode *)[[selected objectAtIndex:0] getLObject];
	if (aFolder && node->entry())
		return (GenericRouteNode *)node->superObject();
	return node;
}

// Moves the views below aView by aDelta, growing the document view to fit.
- (void)_shiftSubviewsUnderOutlineView:(id)aView byDelta:(CGFloat)aDelta
{
	if (aDelta == 0.0)
		return;
	NSView *documentView = [_mainScrollView documentView];
	NSArray *subviews = [documentView subviews];
	NSUInteger count = [subviews count];
	CGFloat top = [aView frame].origin.y;
	for (NSUInteger i = 0; i < count; i++) {
		NSView *subview = [subviews objectAtIndex:i];
		if (top < [subview frame].origin.y)
			[subview setAutoresizingMask:NSViewMinYMargin];
	}
	NSRect frame = [documentView frame];
	[documentView setFrameSize:NSMakeSize(frame.size.width, frame.size.height + aDelta)];
	[_mainScrollView tile];
	[_mainScrollView setNeedsDisplay:YES];
	for (NSUInteger i = 0; i < count; i++)
		[(NSView *)[subviews objectAtIndex:i] setAutoresizingMask:NSViewWidthSizable];
}

- (void)_outlineViewDidChange:(id)aChange
{
	CGFloat rowHeight = [aChange rowHeight] + [aChange intercellSpacing].height;
	[self _shiftSubviewsUnderOutlineView:aChange
	                             byDelta:((CGFloat)[aChange numberOfRows] - (CGFloat)_tmpOutlineViewRowsCount) * rowHeight];
}

// Reloads aView, or all the outline views when it is nil.
- (void)_reloadDataInOutlineView:(id)aView
{
	if (aView) {
		_tmpOutlineViewRowsCount = (unsigned int)[aView numberOfRows];
		[aView reloadData];
		[self _outlineViewDidChange:aView];
		return;
	}
	NSEnumerator *outlineViews = [_outlineViewsArray objectEnumerator];
	id outlineView;
	while ((outlineView = [outlineViews nextObject])) {
		_tmpOutlineViewRowsCount = (unsigned int)[outlineView numberOfRows];
		[outlineView reloadData];
		[self _outlineViewDidChange:outlineView];
	}
}

- (id)nibName
{
	return @"RouteManager";
}

- (id)autosaveName
{
	return [self className];
}

- (id)_outlineVewForItem:(GenericRouteNode *)aItem withFrame:(NSRect)aFrame andID:(NSInteger)aID
{
	WGRouteManagerOutlineView *outlineView = [[WGRouteManagerOutlineView alloc] initWithFrame:aFrame];
	[outlineView setRowHeight:aFrame.size.height];
	[outlineView setDataSource:(id)self];
	[outlineView setDelegate:(id)self];
	[outlineView setBackgroundColor:[_mainScrollView backgroundColor]];
	[outlineView setAllowsEmptySelection:YES];
	[outlineView setAutoresizesOutlineColumn:NO];
	[outlineView setAutoresizingMask:NSViewWidthSizable];
	[outlineView setTag:aID];
	NSTableColumn *column = [[[NSTableColumn alloc] initWithIdentifier:_kOLVColumnIdentifier] autorelease];
	WGImageAndTextCell *cell = [[[WGImageAndTextCell alloc] init] autorelease];
	[column setWidth:aFrame.size.width];
	bool editable = _routeManagerController->isItemEditable(aItem);
	[cell setEditable:editable];
	if ([cell respondsToSelector:@selector(setLineBreakMode:)])
		[cell setLineBreakMode:NSLineBreakByTruncatingTail];
	if (editable) {
		_editableOutline = [outlineView retain];
		[outlineView setAllowsMultipleSelection:YES];
	}
	[outlineView registerForDraggedTypes:[NSArray arrayWithObject:_kRouteNodesPasteboardDataType]];
	[cell setFormatter:[RouteNodeTextFormatter routeNodeTextFormatter]];
	[column setDataCell:cell];
	[outlineView addTableColumn:column];
	[outlineView setOutlineTableColumn:column];
	return [outlineView autorelease];
}

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	if (_routeManagerController) {
		_routeManagerController->release();
		_routeManagerController = 0;
	}
	if (_draggedEntries) {
		_draggedEntries->release();
		_draggedEntries = 0;
	}
	[_fieldEditor release];
	[_leftView release];
	[_earthViewPlaceholder release];
	[_outlineViewsArray release];
	[_editableOutline release];
	[_draggedNodes release];
	[_popupMenu release];
	[_outlineVewsRootFont release];
	[_entryDetailsController release];
	[super dealloc];
}

// Each outline view shows one root node: its tag is the root's index.
- (id)outlineView:(id)aView child:(NSInteger)aChild ofItem:(id)aItem
{
	GenericRouteNode *node = 0;
	if (aItem)
		node = (GenericRouteNode *)[aItem getLObject];
	else
		aChild = [aView tag];
	return (id)_routeManagerController->childAtIndexForItem((int)aChild, node)->detachNSWrapper();
}

- (BOOL)outlineView:(id)aView isItemExpandable:(id)aExpandable
{
	return _routeManagerController->isItemExpandable((GenericRouteNode *)[aExpandable getLObject]);
}

- (NSInteger)outlineView:(id)aView numberOfChildrenOfItem:(id)aItem
{
	if (aItem)
		return _routeManagerController->childrenCountForItem((GenericRouteNode *)[aItem getLObject]);
	return 1;
}

- (void)outlineView:(id)aView willDisplayCell:(id)aCell forTableColumn:(id)aColumn item:(id)aItem
{
	GenericRouteNode *node = (GenericRouteNode *)[aItem getLObject];
	NSImage *image = nil;
	if ([[aColumn identifier] isEqualToString:_kOLVColumnIdentifier])
		image = [NSImage wgImageWithLImage:_routeManagerController->iconForItem(node, [aView isItemExpanded:aItem])];
	[aCell setImage:image];
}

- (BOOL)outlineView:(id)aView shouldSelectItem:(id)aItem
{
	return _routeManagerController->isItemSelectable((GenericRouteNode *)[aItem getLObject]);
}

- (void)outlineViewSelectionDidChange:(id)aChange
{
	[self _selectionDidChangeInOutlineView:[aChange object]];
}

- (void)outlineViewItemWillExpand:(id)aExpand
{
	_tmpOutlineViewRowsCount = (unsigned int)[[aExpand object] numberOfRows];
}

- (void)outlineViewItemDidExpand:(id)aExpand
{
	[self _outlineViewDidChange:[aExpand object]];
}

- (void)outlineViewItemWillCollapse:(id)aCollapse
{
	_tmpOutlineViewRowsCount = (unsigned int)[[aCollapse object] numberOfRows];
}

- (void)outlineViewItemDidCollapse:(id)aCollapse
{
	id outlineView = [aCollapse object];
	[self _outlineViewDidChange:outlineView];
	_tmpOutlineViewRowsCount = (unsigned int)[outlineView numberOfRows];
}

- (void)splitView:(id)aView resizeSubviewsWithOldSize:(NSSize)aSize
{
	[aView wgCustomAdjustSubviews:YES];
}

- (CGFloat)splitView:(id)aView constrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	return [aView wgConstrainMinCoordinate:aCoordinate ofSubviewAt:aAt];
}

- (CGFloat)splitView:(id)aView constrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	return [aView wgConstrainMaxCoordinate:aCoordinate ofSubviewAt:aAt];
}

- (void)currentRouteSetChanged
{
	[self _validateButtons];
}

- (void)windowDidResize:(id)aResize
{
	[_palette endEditingFor:nil];
}

- (void)splitViewWillResizeSubviews:(id)aSubviews
{
	[_palette endEditingFor:nil];
}

- (void)windowWillClose:(id)aClose
{
	[_palette endEditingFor:nil];
	[super windowWillClose:aClose];
}

// The details pane's editable fields share a custom field editor.
- (id)windowWillReturnFieldEditor:(id)aEditor toObject:(id)aObject
{
	if ([aObject isKindOfClass:[NSView class]]
	    && [aObject isDescendantOf:[[self entryDetailsController] pane]]
	    && [aObject respondsToSelector:@selector(isEditable)]
	    && [aObject isEditable]) {
		if (!_fieldEditor) {
			_fieldEditor = [WGCustomFieldEditor new];
			[_fieldEditor setFieldEditor:YES];
		}
		return _fieldEditor;
	}
	return nil;
}

- (NSUInteger)outlineView:(id)aView validateDrop:(id)aDrop proposedItem:(id)aItem proposedChildIndex:(NSInteger)aIndex
{
	NSArray *types = [NSArray arrayWithObject:_kRouteNodesPasteboardDataType];
	if (![[aDrop draggingPasteboard] availableTypeFromArray:types])
		return NSDragOperationNone;
	TLArray<GenericRouteNode*> *nodes =
	    [self _routeNodesFromRouteNodesWrappers:[[[aDrop draggingSource] dataSource] draggedNodes]];
	bool valid = _routeManagerController->validateDrop(nodes, (GenericRouteNode *)[aItem getLObject], (int)aIndex);
	[aView setDropItem:aItem dropChildIndex:NSOutlineViewDropOnItemIndex];
	return valid ? NSDragOperationGeneric : NSDragOperationNone;
}

- (BOOL)outlineView:(id)aView acceptDrop:(id)aDrop item:(id)aItem childIndex:(NSInteger)aIndex
{
	NSArray *types = [NSArray arrayWithObject:_kRouteNodesPasteboardDataType];
	if (![[aDrop draggingPasteboard] availableTypeFromArray:types])
		return NO;
	WGRouteManagerController *source = [[aDrop draggingSource] dataSource];
	if (source != self)
		return NO;
	TLArray<GenericRouteNode*> *nodes = [source _routeNodesFromRouteNodesWrappers:[source draggedNodes]];
	BOOL accepted = source->_routeManagerController->acceptDrop(nodes, (GenericRouteNode *)[aItem getLObject], (int)aIndex);
	[source _setDraggedNodes:nil];
	return accepted;
}

- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType
{
	[self provideEntriesDataForType:aType toPasteboard:aPasteboard];
}

- (TLArray<GenericEntry*> *)draggedEntries
{
	return _draggedEntries;
}

// A new folder in the selected folder, edited right away.
- (void)addRoute:(id)aRoute
{
	[_palette endEditingFor:nil];
	GenericRouteNode *folder = [self _getSelectedRouteNodeInOLV:_editableOutline isFolder:YES];
	if (!folder)
		return;
	DBUserRouteNode *node = _routeManagerController->createNewChildForItem(folder, 0, 0);
	if (!node)
		return;
	[_editableOutline expandItem:(id)folder->detachNSWrapper()];
	[self performSelector:@selector(_editItem:) withObject:(id)node->detachNSWrapper() afterDelay:0.001];
}

- (void)deleteRoute:(id)aRoute
{
	[self performSelector:@selector(_deleteRoute:) withObject:aRoute afterDelay:0.0];
}

- (void)renameRoute:(id)aRoute
{
	GenericRouteNode *node = [self _getSelectedRouteNodeInOLV:_editableOutline isFolder:NO];
	if (node)
		[self _editItem:(id)node->detachNSWrapper()];
}

- (void)loadRoute:(id)aRoute
{
	[[WGRouteImportExport sharedRouteImportExport] importRouteWithTarget:self
	                                                      didEndSelector:@selector(importRouteObject:)
	                                                          hostWindow:nil];
}

- (void)addSelectionToRouteObject:(MutableRouteObject *)aObject
{
	NSEnumerator *outlineViews = [_outlineViewsArray objectEnumerator];
	id outlineView;
	while ((outlineView = [outlineViews nextObject])) {
		TLArray<GenericRouteNode*> *nodes = [self _routeNodesFromRouteNodesWrappers:[outlineView allSelectedItems]];
		_routeManagerController->addRouteObjectNodesContentToRouteObject(nodes, aObject);
	}
}

- (void)_validateEditModeButton
{
	[_editEntryButton setState:[[self entryDetailsController] isEditing] ? NSControlStateValueOn : NSControlStateValueOff];
	[_editEntryButton setEnabled:[self _canEdit]];
}

- (BOOL)_canEdit
{
	return [[self entryDetailsController] isMounted] && [[self entryDetailsController] canEdit];
}

// Saves aEntry in the selected folder and starts editing its name.
- (void)addRouteEntry:(UserRouteObjectEntry *)aEntry
{
	[self loadNibIfNeeded];
	[_palette endEditingFor:nil];
	GenericRouteNode *folder = [self _getSelectedRouteNodeInOLV:_editableOutline isFolder:YES];
	if (!folder)
		return;
	DBUserRouteNode *node = _routeManagerController->createNewChildForItem(folder, aEntry, 0);
	if (!node)
		return;
	[_palette makeKeyAndOrderFront:nil];
	[_editableOutline expandItem:(id)folder->detachNSWrapper()];
	[self performSelector:@selector(_editItem:) withObject:(id)node->detachNSWrapper() afterDelay:0.001];
}

- (void)popupMenuButtonAction:(id)aAction
{
	[NSMenu wgPopUpContextMenu:_popupMenu withEvent:[[aAction window] currentEvent] forView:aAction];
}

- (void)sharedSetButtonCellAction:(id)aAction
{
}

- (void)editEntryAction:(id)aAction
{
	[[self entryDetailsController] setIsEditing:![[self entryDetailsController] isEditing]];
	[self _validateEditModeButton];
}

- (void)help:(id)aHelp
{
	[WGHelpBook gotoPage:@"topics/routes_RM.html" anchor:nil];
}

- (id)entryDetailsController
{
	if (!_entryDetailsController) {
		_entryDetailsController = [WGRouteEntryDetailsController new];
		[_entryDetailsController setDelegate:self];
	}
	return _entryDetailsController;
}

- (id)earthPreview
{
	[self loadNibIfNeeded];
	return _earthView;
}

// Reloads the outline view of the changed root, or all of them.
- (void)_routesWereChanged:(id)aChanged
{
	LNotification *notification = lobj_cast<LNotification>([aChanged getLObject]);
	LMap *userInfo = notification ? notification->userInfo() : 0;
	NSEnumerator *outlineViews = [_outlineViewsArray objectEnumerator];
	id changedOutlineView = nil;
	LObject *root = userInfo ? userInfo->objectForKey(RouteManagerController::kRootNodeKey) : 0;
	id rootWrapper = root ? (id)root->detachNSWrapper() : nil;
	if (rootWrapper) {
		while ((changedOutlineView = [outlineViews nextObject]))
			if ([changedOutlineView itemAtRow:0] == rootWrapper)
				break;
	}
	[self _reloadDataInOutlineView:changedOutlineView];
}

- (void)importRouteObject:(id)aObject
{
	RouteObject *route = lobj_cast<RouteObject>([aObject getLObject]);
	if (route)
		[self addRouteEntry:UserRouteObjectEntry::entryWithRouteObject(route)];
}

- (void)_validateButtons
{
	GenericRouteNode *node = [self _geteSelectedRouteNodeFolder:NO];
	BOOL routeSelected = node && node->entry();
	BOOL currentSetHasItems = NO;
	WGCurrentSetController *currentSet = [[[AtlasController atlasController] mainWindowController] currentSetController];
	if ([currentSet currentRoute])
		currentSetHasItems = [currentSet currentRoute]->itemsCount() > 0;
	[_addToCurrSetBtn setEnabled:routeSelected];
	[_replaceCurrSetBtn setEnabled:routeSelected && currentSetHasItems];
	[self _validateEditModeButton];
}

// A selection in one outline view clears the others.
- (void)_selectionDidChangeInOutlineView:(id)aView
{
	if (!_shouldReactOnSelectionDidChange)
		return;
	GenericRouteNode *node = (GenericRouteNode *)[[aView selectedItem] getLObject];
	RouteObjectEntry *entry = node ? node->entry() : 0;
	[_earthView setSelectedRoute:node];
	[_entryDetailsController setCurrentEntry:entry];
	_shouldReactOnSelectionDidChange = NO;
	for (NSUInteger i = 0; i < [_outlineViewsArray count]; i++) {
		id outlineView = [_outlineViewsArray objectAtIndex:i];
		if (outlineView != aView)
			[outlineView deselectAll:self];
	}
	_shouldReactOnSelectionDidChange = YES;
	[self _validateButtons];
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

- (TLArray<GenericRouteNode*> *)_routeNodesFromRouteNodesWrappers:(id)aWrappers
{
	TLArray<GenericRouteNode*> *nodes = (TLArray<GenericRouteNode*> *)LArray::_alloc()->init();
	nodes->l_autorelease();
	NSEnumerator *wrappers = [aWrappers objectEnumerator];
	id wrapper;
	while ((wrapper = [wrappers nextObject])) {
		LObject *node = [wrapper getLObject];
		if (node)
			((LArray *)nodes)->addObject(node);
	}
	return nodes;
}

// Asks before deleting nodes holding routes.
- (void)_deleteRoute:(id)aRoute
{
	TLArray<GenericRouteNode*> *nodes = [self _routeNodesFromRouteNodesWrappers:[_editableOutline allSelectedItems]];
	int count = nodes ? (int)nodes->count() : 0;
	for (int i = 0; i < count; i++) {
		if (!nodes->objectAtIndex(i)->isEmpty()) {
			if (NSRunAlertPanel(NSLocalizedString(@"DeleteRoutesTitle", @""), @"%@",
			                    NSLocalizedString(@"Delete", @""), NSLocalizedString(@"Cancel", @""), nil,
			                    NSLocalizedString(@"DeleteRoutesMessage", @"")) != NSAlertDefaultReturn)
				return;
			break;
		}
	}
	[_palette endEditingFor:nil];
	NSInteger selectedRow = [_editableOutline selectedRow];
	NSInteger rowsCount = [_editableOutline numberOfRows];
	_routeManagerController->removeItems(nodes);
	if ([_editableOutline selectedItem])
		[self _selectionDidChangeInOutlineView:_editableOutline];
	// Deleting the last row selects the new last one.
	if (selectedRow == rowsCount - 1 && [_editableOutline numberOfRows] != 1)
		[_editableOutline selectRow:[_editableOutline numberOfRows] - 1 byExtendingSelection:NO];
}

- (void)outlineView:(id)aView setObjectValue:(id)aValue forTableColumn:(id)aColumn byItem:(id)aItem
{
	if (_isEditingCanceled)
		return;
	GenericRouteNode *node = (GenericRouteNode *)[aItem getLObject];
	if (!node || !node->superObject())
		return;
	NSString *name = nil;
	if ([aValue isKindOfClass:[_FolderName class]])
		name = [aValue name];
	else if ([aValue isKindOfClass:[NSString class]])
		name = aValue;
	_routeManagerController->setObjectValueForItem([name LString], node);
}

// Roots show in bold, folders with their children count.
- (id)outlineView:(id)aView objectValueForTableColumn:(id)aColumn byItem:(id)aItem
{
	if (![[aColumn identifier] isEqualToString:_kOLVColumnIdentifier])
		return nil;
	GenericRouteNode *node = (GenericRouteNode *)[aItem getLObject];
	NSString *name = [NSString stringReferenceFromLString:_routeManagerController->objectValueForItem(node)];
	if (!node)
		return nil;
	if (!node->superObject()) {
		NSDictionary *attributes = [NSDictionary dictionaryWithObject:_outlineVewsRootFont forKey:NSFontAttributeName];
		return [[[NSAttributedString alloc] initWithString:name attributes:attributes] autorelease];
	}
	if (node->entry())
		return name;
	return [[[_FolderName alloc] initWithName:name childrenCount:node->childrenCount()] autorelease];
}

// The selected node (or its folder), else the outline view's root.
- (GenericRouteNode *)_getSelectedRouteNodeInOLV:(id)aOLV isFolder:(BOOL)aFolder
{
	GenericRouteNode *node = (GenericRouteNode *)[[aOLV selectedItem] getLObject];
	if (!node)
		return (GenericRouteNode *)[[aOLV itemAtRow:0] getLObject];
	if (aFolder && node->entry())
		return (GenericRouteNode *)node->superObject();
	return node;
}

// Route nodes drag as nodes within the route manager, and their entries
// as an entries list.
- (BOOL)outlineView:(id)aView writeItems:(id)aItems toPasteboard:(id)aPasteboard
{
	TLArray<GenericRouteNode*> *nodes = [self _routeNodesFromRouteNodesWrappers:aItems];
	bool canDragNodes = _routeManagerController->validateDrag(nodes);
	TLArray<GenericEntry*> *entries = 0;
	unsigned int count = nodes ? nodes->count() : 0;
	for (unsigned int i = 0; i < count; i++) {
		RouteObjectEntry *entry = nodes->objectAtIndex(i)->entry();
		if (entry) {
			if (!entries)
				entries = (TLArray<GenericEntry*> *)LArray::arrayWithCapacity(count);
			((LArray *)entries)->addObject(entry);
		}
	}
	NSMutableArray *types = [NSMutableArray array];
	if (entries)
		[types addObject:_kEntriesListPasteboardType];
	if (canDragNodes)
		[types addObject:_kRouteNodesPasteboardDataType];
	[self declareEntries:entries inPasteboard:aPasteboard withAdditionTypes:types];
	if (canDragNodes) {
		[self _setDraggedNodes:aItems];
		[aPasteboard setData:[NSData data] forType:_kRouteNodesPasteboardDataType];
	}
	return canDragNodes || entries;
}

- (void)redo:(id)aRedo
{
	[self _cancelEditing];
	RouteManagerController::undoManager()->redo();
}

- (void)undo:(id)aUndo
{
	[self _cancelEditing];
	RouteManagerController::undoManager()->undo();
}

- (BOOL)validateMenuItem:(id)aItem
{
	SEL action = [aItem action];
	if (action == @selector(undo:) || action == @selector(redo:)) {
		LUndoManager *undoManager = RouteManagerController::undoManager();
		BOOL isUndo = action == @selector(undo:);
		BOOL enabled = isUndo ? undoManager->canUndo() : undoManager->canRedo();
		LString *actionName = isUndo ? undoManager->undoActionName() : undoManager->redoActionName();
		NSString *name = actionName ? [NSString stringReferenceFromLString:actionName] : @"";
		[aItem setTitle:[NSString stringWithFormat:@"%@ %@",
		                          NSLocalizedString(isUndo ? @"Undo" : @"Redo", @""), name]];
		return enabled;
	}
	if (action == @selector(editEntryAction:)) {
		[aItem setState:[[self entryDetailsController] isEditing] ? NSControlStateValueOn : NSControlStateValueOff];
		return [self _canEdit];
	}
	BOOL nothingSelected = YES;
	for (NSUInteger i = 0; i < [_outlineViewsArray count]; i++) {
		if (nothingSelected && [[_outlineViewsArray objectAtIndex:i] selectedRow] == -1)
			nothingSelected = YES;
		else
			nothingSelected = NO;
	}
	if (action == @selector(addRoute:))
		return nothingSelected || [_editableOutline selectedRow] != -1;
	if (action == @selector(deleteRoute:) || action == @selector(renameRoute:))
		return [_editableOutline selectedRow] != -1;
	if (action != @selector(exportRoute:) && action != @selector(sendRouteByEmail:))
		return YES;
	GenericRouteNode *node = [self _geteSelectedRouteNodeFolder:NO];
	return node && node->entry();
}

// Mails the selected route as a route set file.
- (void)sendRouteByEmail:(id)aEmail
{
	GenericRouteNode *node = [self _geteSelectedRouteNodeFolder:NO];
	if (!node || !node->entry())
		return;
	[_palette endEditingFor:nil];
	RouteObject *route = node->entry()->routeObject();
	if (!route)
		return;
	LMap *representation = route->dictionaryRepresentation();
	if (!representation)
		return;
	NSString *name = [NSString stringReferenceFromLString:_routeManagerController->objectValueForItem(node)];
	NSString *directory = NSTemporaryDirectory();
	if (!name)
		name = [NSString stringReferenceFromLString:route->preferredName()];
	NSString *path = [directory stringByAppendingPathComponent:[name stringByAppendingPathExtension:_kRouteSetFileExt]];
	if (representation->writeToFile(LString::stringWithCFString((CFStringRef)path)))
		wgCreateMailWithAttachment(path, NSLocalizedString(@"Route Email attachment text", @""));
}

- (void)exportRoute:(id)aRoute
{
	MutableRouteObject *route = MutableRouteObject::_alloc()->init();
	if (!route)
		return;
	GenericRouteNode *node = [self _geteSelectedRouteNodeFolder:NO];
	NSString *name = nil;
	if (node)
		name = [NSString stringReferenceFromLString:_routeManagerController->objectValueForItem(node)];
	[self addSelectionToRouteObject:route];
	route->l_autorelease();
	[[WGRouteImportExport sharedRouteImportExport] exportRoute:route withName:name hostWindow:nil];
}

- (void)addRouteToCurrentSet:(id)aSet
{
	WGCurrentSetController *currentSet = [[[AtlasController atlasController] mainWindowController] currentSetController];
	MutableRouteObject *route = MutableRouteObject::_alloc()->init();
	[self addSelectionToRouteObject:route];
	[currentSet addToCurrentSet:route];
	if (route)
		route->release();
	[_addToCurrSetBtn setEnabled:NO];
}

- (void)replaceCurrentSet:(id)aSet
{
	WGCurrentSetController *currentSet = [[[AtlasController atlasController] mainWindowController] currentSetController];
	MutableRouteObject *route = MutableRouteObject::_alloc()->init();
	[self addSelectionToRouteObject:route];
	[currentSet replaceCurrentSet:route];
	if (route)
		route->release();
}

static void _setupCurrentSetButton(WGTexturedButton *aButton)
{
	[aButton setImagePosition:NSImageOverlaps];
	[aButton setImage:[NSImage imageNamed:@"Normal_Left"] forPosition:0];
	[aButton setImage:[NSImage imageNamed:@"Normal_Center"] forPosition:1];
	[aButton setImage:[NSImage imageNamed:@"Normal_Right"] forPosition:2];
	[aButton setAlternateImage:[NSImage imageNamed:@"Aqua_Left"] forPosition:0];
	[aButton setAlternateImage:[NSImage imageNamed:@"Aqua_Center"] forPosition:1];
	[aButton setAlternateImage:[NSImage imageNamed:@"Aqua_Right"] forPosition:2];
}

- (void)awakeFromNib
{
	_routeManagerController = RouteManagerController::_alloc()->init();
	_shouldReactOnSelectionDidChange = YES;

	WGCustomTitleView *titleView = [_palette customTitleView];
	[titleView setImage:[NSImage imageNamed:@"RouteManager_title"] forTexture:1];
	[titleView setTitleShadowColor:[[NSColor blackColor] colorWithAlphaComponent:0.6]];
	[titleView setTitleShadowOffset:NSMakeSize(0.0, 1.0)];
	[titleView setDrawsTitle:YES];
	[_texturedBottomView setImage:[NSImage imageNamed:@"RouteManager_bottom"] forTexture:7];
	[_texturedBarView setImage:[NSImage imageNamed:@"tools_background"] forTexture:1];
	[_bottomFilledView setFillColor:[NSColor colorWithCalibratedWhite:0.204 alpha:1.0]];
	_setupCurrentSetButton(_replaceCurrSetBtn);
	_setupCurrentSetButton(_addToCurrSetBtn);
	[_gearButton sendActionOn:NSEventMaskLeftMouseDown];
	[_palette setMovableByWindowBackground:YES];
	[_splitView setDividerThickness:1.0];
	[_splitView setDividerFillColor:[NSColor blackColor]];
	[_rightPlaceholder addSubview:_earthViewPlaceholder];
	[_leftPlaceholder addSubview:_leftView];
	[_splitter setSplitterImage:[NSImage imageNamed:@"Splitter"]];
	[_splitter sizeToFit];
	_outlineViewsArray = [[NSMutableArray alloc] init];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(_validateEditModeButton)
	                                             name:_WGEntryDetailsControllerAttributesDidChangeNotification
	                                           object:[self entryDetailsController]];

	// One outline view per root node, separated by lines, in a flipped
	// document view.
	NSRect documentFrame = [[_mainScrollView documentView] frame];
	CGFloat width = [_mainScrollView documentVisibleRect].size.width;
	NSView *documentView = [[WGRouteManagerFlippedDocumentView alloc]
	    initWithFrame:NSMakeRect(documentFrame.origin.x, documentFrame.origin.y, width, NSZeroRect.size.height)];
	[documentView setAutoresizingMask:NSViewWidthSizable];
	CGFloat x = NSZeroRect.origin.x;
	int rootsCount = _routeManagerController->childrenCountForItem(0);
	CGFloat y = kOutlineViewsTop;
	for (int i = 0; i < rootsCount; i++) {
		CGFloat outlineY = y;
		if (i != 0) {
			outlineY = y + kOutlineViewsGap;
			WGFilledView *separator = [[[WGFilledView alloc]
			    initWithFrame:NSMakeRect(x + 5.0, y, width - 10.0, kOutlineViewsSeparatorHeight)] autorelease];
			[separator setFillColor:[NSColor colorWithCalibratedWhite:0.604 alpha:1.0]];
			[separator setAutoresizingMask:NSViewWidthSizable];
			[documentView addSubview:separator];
		}
		GenericRouteNode *root = _routeManagerController->childAtIndexForItem(i, 0);
		id outlineView = [self _outlineVewForItem:root
		                                withFrame:NSMakeRect(x, outlineY, width, kOutlineViewHeight)
		                                    andID:i];
		y = outlineY + kOutlineViewHeight + 6.0;
		[documentView addSubview:outlineView];
		[_outlineViewsArray addObject:outlineView];
	}
	[documentView setFrameSize:NSMakeSize([documentView frame].size.width, y)];
	[_mainScrollView setDocumentView:[documentView autorelease]];
	for (NSUInteger i = 0; i < [_outlineViewsArray count]; i++) {
		id outlineView = [_outlineViewsArray objectAtIndex:i];
		[outlineView reloadData];
		id root = [outlineView itemAtRow:0];
		if (!_routeManagerController->isItemCollapsable((GenericRouteNode *)[root getLObject]))
			[outlineView expandItem:root];
	}
	_outlineVewsRootFont = [[NSFont boldSystemFontOfSize:11.0] retain];
	[[_outlineViewsArray objectAtIndex:0] setAccessibilityRoleDesc:NSLocalizedString(@"SampleRoutes OLV", @"")];
	[[_outlineViewsArray objectAtIndex:1] setAccessibilityRoleDesc:NSLocalizedString(@"MyRoutes OLV", @"")];

	LNotificationCenter::center()->addObserver(self, @selector(_routesWereChanged:),
	                                           RouteManagerController::kRoutesWereChangedNotification,
	                                           _routeManagerController);
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(currentRouteSetChanged)
	                                             name:_WGCurrentSetRouteWasChangedNotification
	                                           object:nil];
	[self currentRouteSetChanged];

	WGRouteEntryDetailsController *details = [self entryDetailsController];
	NSView *pane = [details pane];
	[pane setFrame:[_detailsViewPlaceholder bounds]];
	[_detailsViewPlaceholder addSubview:pane];
	[details didMounted:YES];
}

@end

#pragma clang diagnostic pop
