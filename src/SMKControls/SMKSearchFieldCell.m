#import "SMKSearchFieldCell.h"
#import <objc/message.h>

// An NSSearchFieldCell work-alike for systems that had none: a text field
// cell with a search button (and its recent searches menu) on the left and
// a cancel button on the right, sending partial strings while typing.
@implementation SMKSearchFieldCell

- (void)resetCursorRect:(NSRect)aRect inView:(id)aView
{
	NSRect bounds = { NSZeroPoint, aRect.size };
	NSRect r = NSOffsetRect([self searchTextRectForBounds:bounds], aRect.origin.x, aRect.origin.y);
	[super resetCursorRect:r inView:aView];
}

// a return adds the string to the recent searches
- (void)_smkTextDidEndEditing:(id)aEditing
{
	int movement = [[[aEditing userInfo] objectForKey:@"NSTextMovement"] intValue];
	if (!_sfFlags.sendsWholeSearchString) {
		[self _setTimerRunning:NO];
	}
	if (movement == NSReturnTextMovement) {
		NSString *string = [[[aEditing userInfo] objectForKey:@"NSFieldEditor"] string];
		if ([string length] != 0) {
			[self _addRecent:[[string copy] autorelease]];
		}
	}
}

- (void)endEditing:(id)aEditing
{
	[self _setRegistered:NO forNotificationFromObject:[aEditing delegate]];
	[self _smkControls_setEditor:nil];
	[super endEditing:aEditing];
}

- (void)selectWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate start:(NSInteger)aStart length:(NSInteger)aLength
{
	[self _setRegistered:YES forNotificationFromObject:aDelegate];
	[self _smkControls_setEditor:aEditor];
	[super selectWithFrame:aFrame inView:aView editor:aEditor delegate:aDelegate start:aStart length:aLength];
}

- (void)editWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate event:(id)aEvent
{
	[self _setRegistered:YES forNotificationFromObject:aDelegate];
	[self _smkControls_setEditor:aEditor];
	[super editWithFrame:aFrame inView:aView editor:aEditor delegate:aDelegate event:aEvent];
}

- (void)_smkControls_setEditor:(id)aEditor
{
	if (aEditor == nil) {
		_activeEditor = nil;
	}
	else if (_activeEditor == nil) {
		_activeEditor = aEditor;
	}
	else {
		NSAssert(_activeEditor == aEditor, @"");
	}
}

- (void)_setRegistered:(BOOL)aRegistered forNotificationFromObject:(id)aObject
{
	if (!aRegistered) {
		if (_sfFlags.registeredForNotif) {
			[[NSNotificationCenter defaultCenter] removeObserver:self name:NSControlTextDidEndEditingNotification object:aObject];
			_sfFlags.registeredForNotif = 0;
		}
	}
	else if (!_sfFlags.registeredForNotif && [aObject isKindOfClass:[NSControl class]]) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_smkTextDidEndEditing:)
			name:NSControlTextDidEndEditingNotification object:aObject];
		_sfFlags.registeredForNotif = 1;
	}
}

- (void)setStringValue:(id)aValue
{
	_sfFlags.cancelVisible = [aValue length] != 0;
	[super setStringValue:aValue];
}

// Shows or hides the cancel button; an emptied field sends its action at
// once, otherwise the partial string goes after a pause.
- (void)textDidChange:(id)aChange
{
	unsigned int wasVisible = _sfFlags.cancelVisible;
	NSText *text = [aChange object];
	_sfFlags.cancelVisible = [[(NSTextView *)text textStorage] length] != 0;
	if (wasVisible != _sfFlags.cancelVisible) {
		[[self controlViewControl] updateCell:self];
	}
	if (!_sfFlags.sendsWholeSearchString) {
		[self _setTimerRunning:NO];
	}
	if (!_sfFlags.cancelVisible) {
		[[self controlViewControl] sendAction:[self action] to:[self target]];
	}
	else if (!_sfFlags.sendsWholeSearchString) {
		[self _setTimerRunning:YES];
	}
	// older cells forwarded this; current ones don't implement it
	if ([NSTextFieldCell instancesRespondToSelector:_cmd]) {
		struct objc_super sup = { self, [NSTextFieldCell class] };
		((void (*)(struct objc_super *, SEL, id))objc_msgSendSuper)(&sup, _cmd, aChange);
	}
}

- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp
{
	NSPoint p = [aView convertPoint:[aMouse locationInWindow] fromView:nil];
	NSRect r = [self searchButtonRectForBounds:aRect];
	if ([aView mouse:p inRect:r]) {
		return [self _trackButton:_searchButtonCell forEvent:aMouse inRect:r ofView:aView];
	}
	if (_sfFlags.cancelVisible) {
		r = [self cancelButtonRectForBounds:aRect];
		if ([aView mouse:p inRect:r]) {
			return [self _trackButton:_cancelButtonCell forEvent:aMouse inRect:r ofView:aView];
		}
	}
	return [super trackMouse:aMouse inRect:aRect ofView:aView untilMouseUp:aUp];
}

- (void)highlight:(BOOL)aHighlight withFrame:(NSRect)aFrame inView:(id)aView
{
	[aView setNeedsDisplayInRect:aFrame];
}

- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	[super drawInteriorWithFrame:aFrame inView:aView];
	if (_searchButtonCell != nil) {
		[_searchButtonCell drawInteriorWithFrame:[self searchButtonRectForBounds:aFrame] inView:aView];
	}
	if (_cancelButtonCell != nil && _sfFlags.cancelVisible) {
		[_cancelButtonCell drawInteriorWithFrame:[self cancelButtonRectForBounds:aFrame] inView:aView];
	}
}

- (NSRect)drawingRectForBounds:(NSRect)aBounds
{
	return [self searchTextRectForBounds:aBounds];
}

- (NSSize)cellSizeForBounds:(NSRect)aBounds
{
	return [super cellSizeForBounds:aBounds];
}

// the cancel button's size at the bounds' bottom right
- (NSRect)cancelButtonRectForBounds:(NSRect)aBounds
{
	NSRect r = NSZeroRect;
	if (_cancelButtonCell != nil) {
		r.size = [_cancelButtonCell cellSize];
		r.origin.x = NSMaxX(aBounds) - r.size.width;
		r.origin.y = aBounds.origin.y;
	}
	return r;
}

// the search button's size at the bounds' origin
- (NSRect)searchButtonRectForBounds:(NSRect)aBounds
{
	NSRect r = NSZeroRect;
	if (_searchButtonCell != nil) {
		r.size = [_searchButtonCell cellSize];
		r.origin = aBounds.origin;
	}
	return r;
}

// the text field's drawing rect between the buttons
- (NSRect)searchTextRectForBounds:(NSRect)aBounds
{
	NSRect r = [super drawingRectForBounds:aBounds];
	NSRect search = [self searchButtonRectForBounds:aBounds];
	NSRect cancel = [self cancelButtonRectForBounds:aBounds];
	float x = MAX((float)r.origin.x, (float)NSMaxX(search));
	float end = MIN((float)NSMaxX(r), (float)cancel.origin.x);
	r.origin.x = x;
	r.size.width = MAX(0.0f, end - x);
	return r;
}

// the standard button images follow the size
- (void)setControlSize:(NSControlSize)aSize
{
	NSControlSize old = [self controlSize];
	if (old != aSize) {
		if (_searchButtonCell != nil) {
			BOOL popup = [_searchButtonCell menu] != nil;
			if ([_searchButtonCell image] == SearchImageForSize(old, popup)) {
				NSImage *image = SearchImageForSize(aSize, popup);
				[_searchButtonCell setImage:image];
				[_searchButtonCell setAlternateImage:image];
			}
		}
		if (_cancelButtonCell != nil && [_cancelButtonCell image] == CancelImageForSize(old, NO)) {
			[_cancelButtonCell setImage:CancelImageForSize(aSize, NO)];
			[_cancelButtonCell setAlternateImage:CancelImageForSize(aSize, YES)];
		}
	}
	[super setControlSize:aSize];
}

- (void)setEnabled:(BOOL)aEnabled
{
	[_searchButtonCell setEnabled:aEnabled];
	[_cancelButtonCell setEnabled:aEnabled];
	[super setEnabled:aEnabled];
}

- (id)copyWithZone:(NSZone *)aZone
{
	return [super copyWithZone:aZone];
}

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	unsigned int flags = *(unsigned int *)&_sfFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		[aCoder encodeInt32:(int32_t)flags forKey:@"_sfFlags"];
		return;
	}
	[aCoder encodeValueOfObjCType:"I" at:&flags];
	[aCoder encodeValueOfObjCType:"I" at:&_reserved1];
	[aCoder encodeValueOfObjCType:"I" at:&_reserved2];
	[aCoder encodeValueOfObjCType:"I" at:&_reserved3];
	[aCoder encodeValueOfObjCType:"I" at:&_reserved4];
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	unsigned int flags = *(unsigned int *)&_sfFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		if ([aCoder containsValueForKey:@"_sfFlags"]) {
			flags = (unsigned int)[aCoder decodeInt32ForKey:@"_sfFlags"];
		}
	}
	else {
		[aCoder decodeValueOfObjCType:"I" at:&flags];
		[aCoder decodeValueOfObjCType:"I" at:&_reserved1];
		[aCoder decodeValueOfObjCType:"I" at:&_reserved2];
		[aCoder decodeValueOfObjCType:"I" at:&_reserved3];
		[aCoder decodeValueOfObjCType:"I" at:&_reserved4];
	}
	*(unsigned int *)&_sfFlags = flags;
	// the buttons are not archived
	_sfFlags.customSearch = 0;
	_sfFlags.customCancel = 0;
	_sfFlags.menuTracking = 0;
	[self resetSearchButtonCell];
	[self resetCancelButtonCell];
	return self;
}

- (void)dealloc
{
	[_partialStringTimer invalidate];
	[_partialStringTimer release];
	_partialStringTimer = nil;
	[_searchMenu release];
	_searchMenu = nil;
	[_recentSearches release];
	_recentSearches = nil;
	[_searchMenuTemplate release];
	_searchMenuTemplate = nil;
	[_searchButtonCell release];
	[_cancelButtonCell release];
	[super dealloc];
}

// an NSSearchFieldCell with the same settings, where there is one
- (id)nsSearchFieldCell
{
	Class class = NSClassFromString(@"NSSearchFieldCell");
	if (class == Nil) {
		return nil;
	}
	NSSearchFieldCell *cell = [[[class alloc] initTextCell:[self title]] autorelease];
	[cell setSearchMenuTemplate:[self searchMenuTemplate]];
	[cell setSendsWholeSearchString:[self sendsWholeSearchString]];
	[cell setMaximumRecents:[self maximumRecents]];
	[cell setRecentSearches:[self recentSearches]];
	[cell setBackgroundColor:[self backgroundColor]];
	[cell setDrawsBackground:[self drawsBackground]];
	[cell setTextColor:[self textColor]];
	[cell setBezelStyle:[self bezelStyle]];
	[cell setPlaceholderString:[self placeholderString]];
	[cell setFont:[self font]];
	[cell setAlignment:[self alignment]];
	[cell setBezeled:[self isBezeled]];
	[cell setEnabled:[self isEnabled]];
	[cell setEditable:[self isEditable]];
	[cell setSelectable:[self isSelectable]];
	[cell setScrollable:[self isScrollable]];
	[cell setWraps:[self wraps]];
	[cell setTarget:[self target]];
	[cell setAction:[self action]];
	[cell setTag:[self tag]];
	return cell;
}

- (id)initWithTextFieldCell:(id)aCell
{
	NSTextFieldCell *cell = aCell;
	self = [self initTextCell:[cell title]];
	if (self != nil) {
		[self setBezeled:[cell isBezeled]];
		[self setEditable:[cell isEditable]];
		[self setTarget:[cell target]];
		[self setAction:[cell action]];
		[self setControlSize:[cell controlSize]];
		[self setFont:[cell font]];
		[self setBackgroundColor:[cell backgroundColor]];
		[self setDrawsBackground:[cell drawsBackground]];
		[self setTextColor:[cell textColor]];
		[self setAlignment:[cell alignment]];
	}
	return self;
}

- (id)init
{
	return [self initTextCell:@""];
}

- (id)initTextCell:(id)aCell
{
	self = [super initTextCell:aCell];
	if (self != nil) {
		if ([self respondsToSelector:@selector(setBezelStyle:)]) {
			[self setBezelStyle:NSTextFieldRoundedBezel];
		}
		[self resetSearchButtonCell];
		[self resetCancelButtonCell];
		[self setSendsActionOnEndEditing:NO];
		[self setMaximumRecents:-1];
		[self setScrollable:YES];
	}
	return self;
}

- (id)recentSearches
{
	return _recentSearches != nil ? [[_recentSearches copy] autorelease] : [NSArray array];
}

- (void)setRecentSearches:(id)aSearches
{
	NSUInteger maximum = [self _maximumRecents];
	[_recentSearches removeAllObjects];
	if ([aSearches count] > maximum) {
		aSearches = [aSearches subarrayWithRange:NSMakeRange(0, maximum)];
	}
	if (_recentSearches == nil) {
		_recentSearches = [[NSMutableArray alloc] initWithCapacity:maximum];
	}
	if ([aSearches count] != 0) {
		[_recentSearches addObjectsFromArray:aSearches];
	}
	[self _prepareSearchMenu];
}

// -1 (255) means the default of ten
- (void)setMaximumRecents:(NSInteger)aRecents
{
	_sfFlags.maximumRecents = (unsigned int)aRecents;
	if ([self _maximumRecents] < [_recentSearches count]) {
		[self setRecentSearches:_recentSearches];
	}
}

- (NSInteger)maximumRecents
{
	return _sfFlags.maximumRecents;
}

- (BOOL)sendsWholeSearchString
{
	return _sfFlags.sendsWholeSearchString;
}

- (void)setSendsWholeSearchString:(BOOL)aString
{
	_sfFlags.sendsWholeSearchString = aString != NO;
	if (_sfFlags.sendsWholeSearchString) {
		[self _invalidateTimer];
	}
}

- (id)searchMenuTemplate
{
	return _searchMenuTemplate;
}

- (void)setSearchMenuTemplate:(id)aTemplate
{
	if (_searchMenuTemplate != aTemplate) {
		[_searchMenu release];
		_searchMenu = nil;
		[_searchMenuTemplate release];
		_searchMenuTemplate = [aTemplate retain];
		[self _prepareSearchMenu];
		[_searchButtonCell setMenu:_searchMenu];
		[self _syncSearchImage];
	}
}

- (void)resetCancelButtonCell
{
	if (_sfFlags.customCancel) {
		[self setCancelButtonCell:nil];
	}
	if (_cancelButtonCell == nil) {
		_cancelButtonCell = [[NSButtonCell alloc] initImageCell:nil];
	}
	NSImage *image = CancelImageForSize([self controlSize], NO);
	NSImage *alternate = CancelImageForSize([self controlSize], YES);
	[_cancelButtonCell setButtonType:NSButtonTypeMomentaryChange];
	[_cancelButtonCell setImage:image];
	[_cancelButtonCell setAlternateImage:alternate];
	[_cancelButtonCell setImagePosition:NSImageOnly];
	[_cancelButtonCell setBezelStyle:NSBezelStyleShadowlessSquare];
	[_cancelButtonCell setBordered:NO];
	[_cancelButtonCell setTarget:self];
	[_cancelButtonCell setAction:@selector(_searchFieldCancel:)];
	[_cancelButtonCell setState:NSControlStateValueOn];
}

- (void)resetSearchButtonCell
{
	if (_sfFlags.customSearch) {
		[self setSearchButtonCell:nil];
	}
	if (_searchButtonCell == nil) {
		_searchButtonCell = [[NSButtonCell alloc] initImageCell:nil];
	}
	NSImage *image = SearchImageForSize([self controlSize], _searchMenu != nil);
	[_searchButtonCell setButtonType:NSButtonTypeMomentaryChange];
	[_searchButtonCell setImage:image];
	[_searchButtonCell setAlternateImage:image];
	[_searchButtonCell setImagePosition:NSImageOnly];
	[_searchButtonCell setBordered:NO];
	[_searchButtonCell setTarget:self];
	[_searchButtonCell setAction:@selector(_searchFieldSearch:)];
	[_searchButtonCell setMenu:_searchMenu];
}

- (void)setCancelButtonCell:(id)aCell
{
	if (_cancelButtonCell != aCell) {
		[_cancelButtonCell release];
		_cancelButtonCell = [aCell retain];
		_sfFlags.customCancel = _cancelButtonCell != nil;
	}
}

- (id)cancelButtonCell
{
	return _cancelButtonCell;
}

- (void)setSearchButtonCell:(id)aCell
{
	if (_searchButtonCell != aCell) {
		[_searchButtonCell release];
		_searchButtonCell = [aCell retain];
		_sfFlags.customSearch = _searchButtonCell != nil;
	}
}

- (id)searchButtonCell
{
	return _searchButtonCell;
}

@end

@implementation SMKSearchFieldCell (SMKSearchFieldCell_Private)

// The partial string timer fires half a second after typing. Stopped, it is
// kept with a distant fire date.
- (void)_setTimerRunning:(BOOL)aRunning
{
	if (!aRunning) {
		if (NSAppKitVersionNumber <= 620.0) {
			[self _invalidateTimer];
		}
		else {
			[_partialStringTimer setFireDate:[NSDate distantFuture]];
		}
		return;
	}
	if (_partialStringTimer == nil) {
		_partialStringTimer = [[NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(_sendPartialString)
			userInfo:nil repeats:YES] retain];
		return;
	}
	[_partialStringTimer setFireDate:[NSDate dateWithTimeIntervalSinceNow:0.5]];
}

- (void)_invalidateTimer
{
	[_partialStringTimer invalidate];
	[_partialStringTimer release];
	_partialStringTimer = nil;
}

- (void)_sendPartialString
{
	[self _setTimerRunning:NO];
	[[self controlViewControl] sendAction:[self action] to:[self target]];
}

- (void)_searchFieldClearRecents:(id)aRecents
{
	[self setRecentSearches:nil];
}

- (void)_searchFieldDoRecent:(id)aRecent
{
	[self setStringValue:[aRecent title]];
	[[self controlViewControl] sendAction:[self action] to:[self target]];
}

- (void)_searchFieldCancel:(id)aCancel
{
	if (_activeEditor != nil) {
		[_activeEditor delete:aCancel];
	}
	else {
		[self setStringValue:@""];
	}
	[aCancel sendAction:[self action] to:[self target]];
}

- (void)_searchFieldSearch:(id)aSearch
{
	if ([[self stringValue] length] != 0) {
		[aSearch sendAction:[self action] to:[self target]];
	}
}

// A button with a menu pops it up below its corner; one without is
// tracked like a push button.
- (BOOL)_trackButton:(id)aButton forEvent:(id)aEvent inRect:(NSRect)aRect ofView:(id)aView
{
	NSEvent *event = aEvent;
	if ([aButton menu] == nil) {
		[self setShowsFirstResponder:NO];
		[aView setNeedsDisplay:NO];
		do {
			NSPoint p = [aView convertPoint:[event locationInWindow] fromView:nil];
			if ([aView mouse:p inRect:aRect]) {
				[self _highlight:YES buttonCell:aButton withFrame:aRect inView:aView];
				BOOL tracked = [aButton trackMouse:event inRect:aRect ofView:aView untilMouseUp:NO];
				if (!tracked) {
					[aButton setHighlighted:NO];
					[aView setNeedsDisplay:YES];
				}
				[self _highlight:NO buttonCell:aButton withFrame:aRect inView:aView];
				if (tracked) {
					break;
				}
			}
			event = [[aView window] nextEventMatchingMask:(NSEventMask)0x80000e6];
		} while ([event type] != NSEventTypeLeftMouseUp);
		[self _highlight:NO buttonCell:aButton withFrame:aRect inView:aView];
		return YES;
	}
	NSEventType type = [event type];
	NSPoint corner = NSMakePoint(aRect.origin.x + 6.0f, [aView isFlipped] ? (float)NSMaxY(aRect) + 6.0f : (float)aRect.origin.y - 6.0f);
	NSPoint p = [aView convertPoint:corner toView:nil];
	NSEvent *menuEvent;
	if (type >= NSEventTypeLeftMouseDown && type <= NSEventTypeRightMouseDragged) {
		menuEvent = [NSEvent mouseEventWithType:type location:p modifierFlags:[event modifierFlags] timestamp:[event timestamp]
			windowNumber:[event windowNumber] context:nil eventNumber:[event eventNumber] clickCount:[event clickCount]
			pressure:[event pressure]];
	}
	else if (type == NSEventTypeKeyDown || type == NSEventTypeKeyUp) {
		menuEvent = [NSEvent keyEventWithType:type location:p modifierFlags:[event modifierFlags] timestamp:[event timestamp]
			windowNumber:[event windowNumber] context:nil characters:[event characters]
			charactersIgnoringModifiers:[event charactersIgnoringModifiers] isARepeat:[event isARepeat] keyCode:[event keyCode]];
	}
	else {
		return YES;
	}
	if (menuEvent != nil) {
		[NSMenu popUpContextMenu:[aButton menu] withEvent:menuEvent forView:aView withFont:[self font]];
	}
	return YES;
}

- (void)_highlight:(BOOL)aHighlight buttonCell:(id)aCell withFrame:(NSRect)aFrame inView:(id)aView
{
	if ([aView isKindOfClass:[NSControl class]] && aView != nil) {
		[aCell setHighlighted:aHighlight];
		[aView updateCell:aCell];
	}
	else {
		[aCell highlight:aHighlight withFrame:aFrame inView:aView];
	}
}

// Puts the search first in the recents, moving it there if it is in them
// already, and updates the menu's recent items in place where it can.
- (void)_addRecent:(id)aRecent
{
	NSInteger count = (NSInteger)[_recentSearches count];
	NSUInteger maximum = [self _maximumRecents];
	NSUInteger removed = NSNotFound;
	NSUInteger index = NSNotFound;
	BOOL inserted;
	if (_recentSearches != nil) {
		index = [_recentSearches indexOfObject:aRecent];
	}
	if (index == NSNotFound) {
		if (maximum == 0) {
			[self _prepareSearchMenu];
			return;
		}
		if (_recentSearches == nil) {
			[self setRecentSearches:nil];
		}
		if ((NSUInteger)count == maximum) {
			removed = count - 1;
			[_recentSearches removeObjectAtIndex:removed];
		}
		[_recentSearches insertObject:aRecent atIndex:0];
		inserted = YES;
	}
	else {
		if (index != 0) {
			[aRecent retain];
			[_recentSearches removeObjectAtIndex:index];
			[_recentSearches insertObject:aRecent atIndex:0];
			[aRecent release];
			removed = index;
		}
		inserted = index != 0;
		if (maximum == 0) {
			[self _prepareSearchMenu];
			return;
		}
	}
	if (count <= 0 || _searchMenu == nil || index == 0) {
		[self _prepareSearchMenu];
		return;
	}
	NSInteger items = [_searchMenu numberOfItems];
	NSInteger i = 0;
	while (i < items) {
		if ([[_searchMenu itemAtIndex:i] tag] != NSSearchFieldRecentsMenuItemTag) {
			i++;
			continue;
		}
		if (removed != NSNotFound) {
			[_searchMenu removeItemAtIndex:i + removed];
			items--;
		}
		if (inserted) {
			NSMenuItem *item = [[_searchMenu itemAtIndex:i] copyWithZone:[[_searchMenu itemAtIndex:i] zone]];
			[item setTitle:aRecent];
			[_searchMenu insertItem:item atIndex:i];
			items++;
			[item release];
		}
		i += [_recentSearches count];
	}
}

- (unsigned char)_maximumRecents
{
	unsigned char maximum = _sfFlags.maximumRecents;
	return maximum == 0xff ? 10 : maximum;
}

// The menu is the template's items: the recents title, the recents and
// clear recents items only with recents, the no recents item only without,
// and the recents item once per recent search.
- (void)_prepareSearchMenu
{
	if (_searchMenuTemplate == nil) {
		return;
	}
	NSArray *recents = [self recentSearches];
	NSInteger count = (NSInteger)[recents count];
	if (_searchMenu == nil) {
		_searchMenu = [[NSMenu alloc] initWithTitle:[_searchMenuTemplate title]];
	}
	for (NSInteger i = [_searchMenu numberOfItems] - 1; i >= 0; i--) {
		[_searchMenu removeItemAtIndex:i];
	}
	NSInteger items = [_searchMenuTemplate numberOfItems];
	for (NSInteger i = 0; i < items; i++) {
		NSMenuItem *templateItem = [_searchMenuTemplate itemAtIndex:i];
		NSInteger tag = [templateItem tag];
		if (tag == NSSearchFieldNoRecentsMenuItemTag) {
			if (count > 0) {
				continue;
			}
		}
		else if (tag >= NSSearchFieldRecentsTitleMenuItemTag && tag <= NSSearchFieldClearRecentsMenuItemTag && count <= 0) {
			continue;
		}
		NSMenuItem *item = [templateItem copyWithZone:[templateItem zone]];
		if (tag == NSSearchFieldRecentsMenuItemTag) {
			[item setTarget:self];
			[item setAction:@selector(_searchFieldDoRecent:)];
			for (NSInteger k = 0; k < count - 1; k++) {
				NSMenuItem *recent = [item copy];
				[recent setTitle:[recents objectAtIndex:k]];
				[_searchMenu addItem:recent];
				[recent release];
			}
			[item setTitle:[recents lastObject]];
		}
		else if (tag == NSSearchFieldClearRecentsMenuItemTag) {
			[item setTarget:self];
			[item setAction:@selector(_searchFieldClearRecents:)];
		}
		[_searchMenu addItem:item];
		[item release];
	}
}

// the standard search image follows whether there is a menu
- (void)_syncSearchImage
{
	if (_searchButtonCell == nil) {
		return;
	}
	NSImage *image = [_searchButtonCell image];
	NSImage *popup = SearchImageForSize([self controlSize], YES);
	NSImage *plain = SearchImageForSize([self controlSize], NO);
	BOOL hasMenu = _searchMenu != nil;
	if (popup == image) {
		if (!hasMenu) {
			[_searchButtonCell setImage:plain];
		}
	}
	else if (plain == image) {
		if (hasMenu) {
			[_searchButtonCell setImage:popup];
		}
	}
	else {
		return;
	}
	if ([_searchButtonCell alternateImage] == image) {
		[_searchButtonCell setAlternateImage:[_searchButtonCell image]];
	}
}

- (id)controlViewControl
{
	NSView *view = [self controlView];
	return [view isKindOfClass:[NSControl class]] ? view : nil;
}

@end
