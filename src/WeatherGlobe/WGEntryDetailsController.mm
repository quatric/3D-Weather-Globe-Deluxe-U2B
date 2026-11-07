#import "WGEntryDetailsController.h"
#import "WGEntryDetailsHeaderViewController.h"
#import "WGEntryDetailsPaneController.h"
#import "WGPlaceholderView.h"
#import "WGTexturedView.h"
#import "WGCustomButton.h"
#import "NSView+WGExtensions.h"
#import "MainWindowController.h"

NSString *_WGEntryObjectKey = @"entryValue";
NSString *_WGEntryDetailsCurrentEntryDidChangeNotification = @"WGEntryDetailsCurrentEntryDidChangeNotification";
NSString *_WGEntryDetailsCurrentEntryWillChangeNotification = @"WGEntryDetailsCurrentEntryWillChangeNotification";
NSString *_WGEntryDetailsControllerWillCommitEntryNotification = @"WGEntryDetailsControllerWillCommitEntryNotification";
NSString *_WGEntryDetailsControllerAttributesDidChangeNotification = @"WGEntryDetailsControllerAttributesDidChangeNotification";
NSString *_WGEntryDetailsControllerEditModeDidChangeNotification = @"WGEntryDetailsControllerEditModeDidChangeNotification";

// The details of the current entry: a header, then the panes of the
// selected mode (one segment per pane controller class of the subclass),
// stacked in a scroll view; "No Selection" without an entry. Edits go
// through an EntryInfoController and are committed when editing ends or
// the entry changes.
@implementation WGEntryDetailsController

- (id)init
{
	self = [super init];
	if (self) {
		_entryInfoController = (EntryInfoController *)EntryInfoController::_alloc()->init();
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_appWillTerminate)
			name:NSApplicationWillTerminateNotification object:nil];
	}
	return self;
}

- (void)awakeFromNib
{
	NSFont *font = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeRegular]];
	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:[NSColor disabledControlTextColor],
		NSForegroundColorAttributeName, font, NSFontAttributeName, nil];
	[placeholder setWGBackgroundColor:[NSColor controlBackgroundColor]];
	[(id)pane setWGBackgroundColor:[NSColor whiteColor]];
	[placeholder setWGPlaceholderString:[[[NSAttributedString alloc]
		initWithString:[[NSBundle mainBundle] localizedStringForKey:@"No Selection" value:@"" table:nil]
		attributes:[pane wgAdjustedPlacholderStringAttributes:attributes]] autorelease]];
	scrollView = [placeholder enclosingScrollView];
	[placeholder retain];
	[self setupButtonCells];
	[self mountPaneController:[self headerViewController]];
	[self setCurrentPaneController:[[modeMatrix selectedCell] representedObject]];
	[backgroundView setImage:[NSImage imageNamed:@"tools_background"] forTexture:1];
	// Forces the state change below.
	_entryInfoController->setIsEditing(1);
	[self setIsEditing:NO];
	[self validateCurrentPane];
}

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	if (_entryInfoController) {
		_entryInfoController->release();
		_entryInfoController = 0;
	}
	[[modeMatrix cells] makeObjectsPerformSelector:@selector(setRepresentedObject:) withObject:nil];
	[placeholder release];
	[_controllers release];
	if (_dragTypeOwnerMap)
		NSFreeMapTable(_dragTypeOwnerMap);
	[super dealloc];
}

+ (id)wgIdentifier
{
	return @"EntryDetails";
}

- (id)nibName
{
	return @"DetailsView";
}

- (void)didMounted:(BOOL)aMounted
{
	[super didMounted:aMounted];
	if (aMounted) {
		[[NSNotificationCenter defaultCenter] postNotificationName:_WGEntryDetailsCurrentEntryDidChangeNotification
			object:self];
		[self validateCurrentPane];
	}
}

- (GenericEntry *)currentEntry
{
	return _entryInfoController->entry();
}

// The old entry is committed if it was being edited.
- (void)setCurrentEntry:(GenericEntry *)aEntry
{
	GenericEntry *old = [self currentEntry];
	if (old == aEntry || _isComitting)
		return;
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center postNotificationName:_WGEntryDetailsCurrentEntryWillChangeNotification object:self
		userInfo:[NSDictionary dictionaryWithObject:[NSValue valueWithPointer:aEntry] forKey:_WGEntryObjectKey]];
	_isComitting = YES;
	if ([self isEditing] && old) {
		[center postNotificationName:_WGEntryDetailsControllerWillCommitEntryNotification object:self];
		old->commit();
	}
	LNotificationCenter::center()->removeObserver(self, GenericEntry::kEntryDidChangeNotification, old);
	_entryInfoController->setEntry(aEntry);
	if (aEntry)
		LNotificationCenter::center()->addObserver(self, @selector(entryDidChange:),
			GenericEntry::kEntryDidChangeNotification, aEntry);
	_isComitting = NO;
	if ([self isMounted]) {
		[center postNotificationName:_WGEntryDetailsCurrentEntryDidChangeNotification object:self];
		[self validateCurrentPane];
	}
}

- (EntryInfoController *)entryInfoController
{
	return _entryInfoController;
}

- (BOOL)canEdit
{
	if (![_currentController conformsToProtocol:@protocol(WGEditableController)])
		return NO;
	return [(id)_currentController canEdit];
}

- (BOOL)isEditing
{
	return _entryInfoController->isEditing();
}

// Ending editing commits the entry.
- (void)setIsEditing:(BOOL)aEditing
{
	if (aEditing == _entryInfoController->isEditing())
		return;
	if (aEditing && ![self canEdit])
		return;
	_entryInfoController->setIsEditing(aEditing);
	if (!aEditing && [self currentEntry]) {
		_isComitting = YES;
		[[NSNotificationCenter defaultCenter] postNotificationName:_WGEntryDetailsControllerWillCommitEntryNotification
			object:self];
		[self currentEntry]->commit();
		_isComitting = NO;
	}
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center postNotificationName:_WGEntryDetailsControllerEditModeDidChangeNotification object:self];
	[center postNotificationName:_WGEntryDetailsControllerAttributesDidChangeNotification object:self];
	if (aEditing)
		[self tryOpenFieldEditor];
}

- (void)validateEditing
{
	if ([self isEditing] && ![self canEdit])
		[self setIsEditing:NO];
}

- (BOOL)tryOpenFieldEditor
{
	if ([[self headerViewController] tryOpenFieldEditor])
		return YES;
	if ([_currentController conformsToProtocol:@protocol(WGEditableController)])
		return [(id)_currentController tryOpenFieldEditor];
	return NO;
}

// The panes are stacked from the top of the (flipped) container, which
// gets their total height.
- (void)rebuildLayout
{
	_isLayoutingSubviews = YES;
	int y = 0;
	NSEnumerator *enumerator = [[paneContainer subviews] objectEnumerator];
	NSView *view;
	while ((view = [enumerator nextObject])) {
		NSRect frame = [view frame];
		[view setFrameOrigin:NSMakePoint(frame.origin.x, y)];
		y += (int)frame.size.height;
	}
	NSRect frame = [paneContainer frame];
	frame.size.height = y;
	[paneContainer setFrame:frame];
	[scrollView tile];
	[paneContainer setNeedsDisplay:YES];
	[scrollView setNeedsDisplay:YES];
	_isLayoutingSubviews = NO;
}

- (void)rebuildLayout:(id)aLayout
{
	[self rebuildLayout];
}

// Drops go to the first owner registered for one of the dragged types.
- (NSUInteger)view:(id)aView validateDrop:(id)aDrop
{
	if (!_dragTypeOwnerMap)
		return NSDragOperationNone;
	NSArray *types = [[aDrop draggingPasteboard] types];
	NSUInteger count = [types count];
	for (NSUInteger i = 0; i < count; i++) {
		id owner = (id)NSMapGet(_dragTypeOwnerMap, [types objectAtIndex:i]);
		if (owner && [owner respondsToSelector:@selector(view:validateDrop:)]) {
			NSUInteger operation = [owner view:aView validateDrop:aDrop];
			if (operation)
				return operation;
		}
	}
	return NSDragOperationNone;
}

- (BOOL)view:(id)aView acceptDrop:(id)aDrop
{
	if (!_dragTypeOwnerMap)
		return NO;
	NSArray *types = [[aDrop draggingPasteboard] types];
	NSUInteger count = [types count];
	for (NSUInteger i = 0; i < count; i++) {
		id owner = (id)NSMapGet(_dragTypeOwnerMap, [types objectAtIndex:i]);
		if (owner && [owner respondsToSelector:@selector(view:acceptDrop:)] && [owner view:aView acceptDrop:aDrop])
			return YES;
	}
	return NO;
}

- (BOOL)isGotoLocationEnabled
{
	return NO;
}

- (void)gotoCurrentLocation
{
	if ([self isGotoLocationEnabled] && [delegate respondsToSelector:@selector(gotoLocation:forced:)])
		[delegate gotoLocation:[self currentEntry] forced:YES];
}

// Subclasses list their pane controller classes.
- (id)panesControllersClasses
{
	return [NSArray array];
}

- (void)setCurrentPaneController:(id)aController
{
	if (_currentController == aController && aController)
		return;
	[self unmountPaneController:_currentController];
	_currentController = aController;
	[self mountPaneController:aController];
	if ([[modeMatrix selectedCell] representedObject] != _currentController)
		[modeMatrix selectCellAtRow:0 column:[[self panesControllersClasses] indexOfObject:[_currentController class]]];
	[self validateEditing];
	[[NSNotificationCenter defaultCenter] postNotificationName:_WGEntryDetailsControllerAttributesDidChangeNotification
		object:self];
}

// Enables the modes that apply to the entry and switches to the first
// enabled one when the current doesn't; without an entry (or a usable
// pane) the placeholder fills the scroll view.
- (void)validateCurrentPane
{
	id firstEnabled = nil;
	GenericEntry *entry = [self currentEntry];
	NSEnumerator *enumerator = [[modeMatrix cells] objectEnumerator];
	NSCell *cell;
	while ((cell = [enumerator nextObject])) {
		BOOL enabled = [[cell representedObject] isEnabled];
		[cell setEnabled:enabled];
		if (!firstEnabled && enabled)
			firstEnabled = [cell representedObject];
	}
	BOOL showPanes;
	if ([_currentController isEnabled])
		showPanes = entry != 0;
	else if (firstEnabled) {
		[self setCurrentPaneController:firstEnabled];
		showPanes = entry != 0;
	} else
		showPanes = NO;
	if (showPanes) {
		if ([placeholder superview]) {
			NSRect frame = [paneContainer frame];
			frame.size.width = [[scrollView contentView] documentVisibleRect].size.width;
			[paneContainer setFrame:frame];
			[placeholder removeFromSuperview];
			[scrollView setDocumentView:paneContainer];
		}
	} else {
		[placeholder setFrame:[[scrollView contentView] documentVisibleRect]];
		[paneContainer removeFromSuperview];
		[scrollView setDocumentView:placeholder];
	}
	[self validateEditing];
	[[NSNotificationCenter defaultCenter] postNotificationName:_WGEntryDetailsControllerAttributesDidChangeNotification
		object:self];
}

// The pane accepts the union of the types the owners registered; a nil
// owner unregisters the types.
- (void)registerDraggedTypes:(id)aTypes forOwner:(id)aOwner
{
	NSEnumerator *enumerator = [aTypes objectEnumerator];
	NSString *type;
	while ((type = [enumerator nextObject])) {
		if (!aOwner) {
			if (_dragTypeOwnerMap)
				NSMapRemove(_dragTypeOwnerMap, type);
		} else {
			if (!_dragTypeOwnerMap)
				_dragTypeOwnerMap = NSCreateMapTable(NSObjectMapKeyCallBacks, NSNonRetainedObjectMapValueCallBacks, 0);
			NSMapInsert(_dragTypeOwnerMap, type, aOwner);
		}
	}
	[pane unregisterDraggedTypes];
	if (_dragTypeOwnerMap)
		[pane registerForDraggedTypes:NSAllMapTableKeys(_dragTypeOwnerMap)];
}

- (void)_appWillTerminate
{
	[self setCurrentEntry:0];
}

- (void)entryDidChange:(id)aChange
{
	if ([self isMounted]) {
		[[NSNotificationCenter defaultCenter] postNotificationName:_WGEntryDetailsCurrentEntryDidChangeNotification
			object:self];
		[self validateCurrentPane];
	}
}

- (void)buttonAction:(id)aAction
{
	[self setCurrentPaneController:[[aAction selectedCell] representedObject]];
}

// One segment per pane controller; a single one hides the segments. The
// scroll view takes the height the cells add (all of it when hidden); the
// matrix keeps its nib size.
- (void)setupButtonCells
{
	NSArray *classes = [self panesControllersClasses];
	NSInteger count = [classes count];
	NSRect frame = [modeMatrix frame];
	CGFloat height = frame.size.height;
	[modeMatrix setPrototype:[modeMatrix cellAtRow:0 column:0]];
	[[modeMatrix prototype] setState:NSControlStateValueOff];
	[modeMatrix renewRows:1 columns:count];
	for (NSInteger i = 0; i < count; i++) {
		id controller = [self controllerForClass:[classes objectAtIndex:i]];
		NSCell *cell = [modeMatrix cellAtRow:0 column:i];
		[cell setTag:i];
		[cell setRepresentedObject:controller];
		[cell setTarget:self];
		[cell setAction:@selector(buttonAction:)];
		[modeMatrix putCell:cell atRow:0 column:i];
		[self setupCell:cell withController:controller];
	}
	[modeMatrix sizeToCells];
	if (count < 2) {
		[modeMatrix setHidden:YES];
		height = 0.0;
	}
	CGFloat added = [modeMatrix frame].size.height - height;
	[modeMatrix setFrameSize:NSMakeSize(frame.size.width, height)];
	NSSize size = [scrollView frame].size;
	[scrollView setFrameSize:NSMakeSize(size.width, size.height + added)];
}

// The first segment has a square left end, the others a separator on the
// left; all get a rounded right end.
- (void)setupCell:(id)aCell withController:(id)aController
{
	NSInteger tag = [aCell tag];
	NSImage *offTile = [NSImage imageNamed:@"fl_segment_off_tile"];
	NSImage *offRight = tag ? [NSImage imageNamed:@"fl_segment_off_right"] : offTile;
	NSImage *onTile = [NSImage imageNamed:@"fl_segment_on_tile"];
	NSImage *onRight = [NSImage imageNamed:@"fl_segment_on_right"];
	NSImage *onLeft = [NSImage imageNamed:@"fl_segment_on_left"];
	[aCell addImage:offTile altImage:onTile align:4 scale:1];
	[aCell addImage:offRight altImage:onRight align:8 scale:2];
	if (tag)
		[aCell addImage:offTile altImage:onLeft align:4 scale:2];
	[aCell setTitle:[aController title]];
	[aCell setImagePosition:NSImageLeft];
	[aCell setAlignment:NSTextAlignmentLeft];
	[aCell setImage:[NSImage imageNamed:[aController iconName]]];
}

- (id)headerViewController
{
	return [self controllerForClass:[WGEntryDetailsHeaderViewController class]];
}

- (id)currentController
{
	return _currentController;
}

// One controller per class, made on demand.
- (id)controllerForClass:(Class)aClass
{
	NSString *name = NSStringFromClass(aClass);
	if (!_controllers)
		_controllers = [NSMutableDictionary new];
	id controller = [_controllers objectForKey:name];
	if (!controller) {
		controller = [[[aClass alloc] initWithOwner:self] autorelease];
		[_controllers setObject:controller forKey:name];
	}
	return controller;
}

- (void)mountPaneController:(id)aController
{
	NSView *view = [aController pane];
	if ([[paneContainer subviews] containsObject:view])
		return;
	[paneContainer addSubview:view];
	NSRect frame = [view frame];
	NSRect container = [paneContainer frame];
	if (frame.size.width != container.size.width)
		[view setFrameSize:NSMakeSize(container.size.width, frame.size.height)];
	[aController didMounted:YES];
	[self rebuildLayout];
}

- (void)unmountPaneController:(id)aController
{
	[[aController pane] removeFromSuperview];
	[aController didMounted:NO];
}

@end
