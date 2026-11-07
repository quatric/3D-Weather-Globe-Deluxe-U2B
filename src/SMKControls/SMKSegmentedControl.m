#import "SMKSegmentedControl.h"

// raises for a segment index outside the control
#define SMK_CHECK_SEGMENT(aSegment) \
	if ((aSegment) < 0 || (NSUInteger)(aSegment) >= [_cells count]) { \
		[NSException raise:NSRangeException format:@"-[SMKSegmentedControl %@] called for illegal segment index (%d)", \
			NSStringFromSelector(_cmd), (int)(aSegment)]; \
	}

#define SMK_INCONSISTENCY() \
	[NSException raise:NSInternalInconsistencyException format:@"-[%@ %@]: %@", NSStringFromClass([self class]), \
		NSStringFromSelector(_cmd), @"internal inconsistency"]

// An NSSegmentedControl work-alike of SMKSegmentedCells laid out side by
// side from (5, 5), optionally autosized to fill the control. Tracking
// modes: 0 select one, 1 select any, 2 momentary.
@implementation SMKSegmentedControl

+ (Class)cellClass
{
	return [SMKSegmentedCell class];
}

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	unsigned int flags = *(unsigned int *)&_smkFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		[aCoder encodeObject:_cells forKey:@"cells"];
		[aCoder encodeInt:_selectedSegment forKey:@"selectedSegment"];
		[aCoder encodeInt32:(int32_t)flags forKey:@"flags"];
		return;
	}
	[aCoder encodeObject:_cells];
	int version = 1;
	[aCoder encodeValueOfObjCType:"i" at:&version];
	int selected = _selectedSegment;
	[aCoder encodeValueOfObjCType:"i" at:&selected];
	[aCoder encodeValueOfObjCType:"I" at:&flags];
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	[_cells release];
	_cells = nil;
	[self _setToolTipSupport:NO];
	unsigned int flags = *(unsigned int *)&_smkFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		if ([aCoder containsValueForKey:@"cells"]) {
			_cells = [[aCoder decodeObjectForKey:@"cells"] retain];
		}
		if ([aCoder containsValueForKey:@"selectedSegment"]) {
			_selectedSegment = [aCoder decodeIntForKey:@"selectedSegment"];
		}
		if ([aCoder containsValueForKey:@"flags"]) {
			flags = (unsigned int)[aCoder decodeInt32ForKey:@"flags"];
		}
	}
	else {
		_cells = [[aCoder decodeObject] retain];
		int version, selected;
		[aCoder decodeValueOfObjCType:"i" at:&version];
		[aCoder decodeValueOfObjCType:"i" at:&selected];
		_selectedSegment = selected;
		[aCoder decodeValueOfObjCType:"I" at:&flags];
	}
	*(unsigned int *)&_smkFlags = flags;
	// nothing cached survives archiving
	_smkFlags.listensToFrameDidChange = 0;
	_smkFlags.toolTipsSetUp = 0;
	_smkFlags.pendingUpdates = 0;
	_smkFlags.updatesDelayed = 0;
	_smkFlags.hasAutosizedSegments = 0;
	_smkFlags.autosizeInfoValid = 0;
	_smkFlags.cellFramesValid = 0;
	if (_cells == nil) {
		_cells = [NSMutableArray new];
	}
	else {
		[_cells makeObjectsPerformSelector:@selector(_setSegmentedControl:) withObject:self];
	}
	[self _setToolTipSupport:[self window] != nil];
	return self;
}

- (id)toolTipForCell:(id)aCell
{
	return [self toolTipForSegment:[self segmentForCell:aCell]];
}

- (void)setToolTip:(id)aTip forCell:(id)aCell
{
	[self setToolTip:aTip forSegment:[self segmentForCell:aCell]];
}

- (BOOL)selectCellWithTag:(NSInteger)aTag
{
	NSInteger segment = [self segmentWithTag:aTag];
	if (segment == -1) {
		return NO;
	}
	[self setSelected:YES forSegment:segment];
	return [[_cells objectAtIndex:segment] state] == NSControlStateValueOn;
}

- (void)deselectAllCells
{
	if (_smkFlags.allowsEmptySelection) {
		[self setSelectedSegment:-1];
	}
}

- (void)deselectSelectedCell
{
	if (_selectedSegment != -1) {
		[self setSelected:NO forSegment:_selectedSegment];
	}
}

- (id)selectedCells
{
	if (_selectedSegment == -1) {
		return [NSArray array];
	}
	if ([self trackingMode] != NSSegmentSwitchTrackingSelectAny) {
		return [NSArray arrayWithObject:[_cells objectAtIndex:_selectedSegment]];
	}
	NSInteger count = (NSInteger)[_cells count];
	if (count < 1) {
		return [NSArray array];
	}
	NSMutableArray *cells = [NSMutableArray new];
	for (NSInteger i = 0; i < count; i++) {
		id cell = [_cells objectAtIndex:i];
		if ([cell state] == NSControlStateValueOn) {
			[cells addObject:cell];
		}
	}
	NSArray *result = [[cells copy] autorelease];
	[cells release];
	return result;
}

- (id)selectedCell
{
	return _selectedSegment == -1 ? nil : [_cells objectAtIndex:_selectedSegment];
}

- (id)cells
{
	return _cells != nil ? [[_cells copy] autorelease] : [NSArray array];
}

- (BOOL)allowsEmptySelection
{
	return _smkFlags.allowsEmptySelection;
}

- (void)setAllowsEmptySelection:(BOOL)aSelection
{
	if (_smkFlags.allowsEmptySelection == (unsigned int)(aSelection & 1)) {
		return;
	}
	if ([self trackingMode] == NSSegmentSwitchTrackingMomentary) {
		[NSException raise:NSInternalInconsistencyException format:@"SMKSegmentedControl cannot have selection in SMKSegmentTrackingMomentary mode"];
	}
	_smkFlags.allowsEmptySelection = aSelection & 1;
	if (!(aSelection & 1) && _selectedSegment == -1 && [_cells count] != 0) {
		_selectedSegment = 0;
		[[_cells objectAtIndex:0] _setState:NSControlStateValueOn];
	}
}

- (id)toolTipForSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	return [[_cells objectAtIndex:aSegment] toolTip];
}

- (void)setToolTip:(id)aTip forSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	[[_cells objectAtIndex:aSegment] setToolTip:aTip];
}

- (NSInteger)tagForSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	return [[_cells objectAtIndex:aSegment] tag];
}

- (void)setTag:(NSInteger)aTag forSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	[[_cells objectAtIndex:aSegment] setTag:aTag];
}

- (BOOL)isEnabledForSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	return [[_cells objectAtIndex:aSegment] isEnabled];
}

- (void)setEnabled:(BOOL)aEnabled forSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	[[_cells objectAtIndex:aSegment] setEnabled:aEnabled];
}

- (BOOL)isSelectedForSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	return [[_cells objectAtIndex:aSegment] state] == NSControlStateValueOn;
}

// keeps the selection consistent with the tracking mode
- (void)setSelected:(BOOL)aSelected forSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	SMKSegmentedCell *cell = [_cells objectAtIndex:aSegment];
	NSInteger state = aSelected ? NSControlStateValueOn : NSControlStateValueOff;
	NSInteger old = [cell state];
	NSInteger mode = [self trackingMode];
	if (mode == NSSegmentSwitchTrackingSelectAny) {
		if (state == NSControlStateValueOn) {
			_selectedSegment = (int)aSegment;
		}
		else if (old == NSControlStateValueOn) {
			int selected = _selectedSegment;
			if (selected != -1 && selected == aSegment) {
				NSInteger other = [self _firstSelectedSegmentExceptSegment:selected];
				if (other == -1 && !_smkFlags.allowsEmptySelection) {
					_selectedSegment = selected;
					return;
				}
				_selectedSegment = (int)other;
			}
			[cell _setState:state];
			return;
		}
	}
	else if (mode == NSSegmentSwitchTrackingSelectOne) {
		int selected = _selectedSegment;
		if (selected == -1) {
			if (state == NSControlStateValueOn) {
				_selectedSegment = (int)aSegment;
			}
		}
		else if (selected == aSegment) {
			if (!_smkFlags.allowsEmptySelection) {
				state = NSControlStateValueOn;
			}
			else if (state == NSControlStateValueOff) {
				_selectedSegment = -1;
			}
		}
		else if (state == NSControlStateValueOn) {
			[[_cells objectAtIndex:selected] _setState:NSControlStateValueOff];
			_selectedSegment = (int)aSegment;
		}
		else if (old == NSControlStateValueOn) {
			[NSException raise:NSInternalInconsistencyException format:@"-[SMKSegmentedControl %@] selection wrong:%i, mode %i, %@ (%i)",
				NSStringFromSelector(_cmd), selected, (int)_smkFlags.trackingMode, @"another segment is selected", (int)aSegment];
			[cell _setState:state];
			return;
		}
	}
	else if (mode == NSSegmentSwitchTrackingMomentary) {
		if (state != NSControlStateValueOn && old == state) {
			return;
		}
		[NSException raise:NSInternalInconsistencyException format:@"SMKSegmentedControl cannot have selection in SMKSegmentTrackingMomentary mode"];
	}
	if (state != old) {
		[cell _setState:state];
	}
}

- (id)menuForSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	return [[_cells objectAtIndex:aSegment] menu];
}

- (void)setMenu:(id)aMenu forSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	[[_cells objectAtIndex:aSegment] setMenu:aMenu];
}

- (id)labelForSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	return [[_cells objectAtIndex:aSegment] title];
}

- (void)setLabel:(id)aLabel forSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	[[_cells objectAtIndex:aSegment] setTitle:aLabel];
}

- (id)imageForSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	return [[_cells objectAtIndex:aSegment] image];
}

- (void)setImage:(id)aImage forSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	[[_cells objectAtIndex:aSegment] setImage:aImage];
}

- (CGFloat)widthForSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	return [[_cells objectAtIndex:aSegment] segmentWidth];
}

- (void)setWidth:(CGFloat)aWidth forSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	[[_cells objectAtIndex:aSegment] setSegmentWidth:aWidth];
}

- (NSInteger)selectedSegment
{
	return _selectedSegment;
}

- (void)setSelectedSegment:(NSInteger)aSegment
{
	if (aSegment == -1) {
		if (!_smkFlags.allowsEmptySelection) {
			[NSException raise:NSInternalInconsistencyException format:@"-[SMKSegmentedControl %@] empty selection is not allowed",
				NSStringFromSelector(_cmd)];
			return;
		}
		_selectedSegment = -1;
		for (SMKSegmentedCell *cell in _cells) {
			[cell _setState:NSControlStateValueOff];
		}
		return;
	}
	SMK_CHECK_SEGMENT(aSegment);
	if ([self trackingMode] == NSSegmentSwitchTrackingMomentary) {
		[NSException raise:NSInternalInconsistencyException format:@"SMKSegmentedControl cannot have selection in SMKSegmentTrackingMomentary mode"];
	}
	if (_selectedSegment == aSegment) {
		return;
	}
	SMKSegmentedCell *cell = [_cells objectAtIndex:aSegment];
	NSInteger state = [cell state];
	if ([self trackingMode] == NSSegmentSwitchTrackingSelectOne) {
		if (state == NSControlStateValueOff) {
			if (_selectedSegment != -1) {
				[[_cells objectAtIndex:_selectedSegment] _setState:NSControlStateValueOff];
			}
		}
		else if (state == NSControlStateValueOn) {
			[NSException raise:NSInternalInconsistencyException format:@"-[SMKSegmentedControl %@] _selectedSegment=%i while segment %i is selected",
				NSStringFromSelector(_cmd), _selectedSegment, (int)aSegment];
		}
	}
	_selectedSegment = (int)aSegment;
	[cell _setState:NSControlStateValueOn];
}

- (NSInteger)segmentCount
{
	return (NSInteger)[_cells count];
}

// the first and last segments get the caps
- (void)setSegmentCount:(NSInteger)aCount
{
	NSInteger count = (NSInteger)[_cells count];
	if (count == aCount) {
		return;
	}
	[self _setUpdatesDelayed:YES];
	[self _numberOfCellFramesChanged];
	if (aCount < 0) {
		[NSException raise:NSRangeException format:@"-[SMKSegmentedControl %@] called for negative count (%d)",
			NSStringFromSelector(_cmd), (int)aCount];
	}
	if (count < aCount) {
		[[_cells lastObject] _setRightCap:NO];
		if (count == 0) {
			[[self _addCell] _setLeftCap:YES];
			count = 1;
		}
		for (; count < aCount; count++) {
			[self _addCell];
		}
	}
	else {
		NSRange removed = NSMakeRange(aCount, count - aCount);
		[[_cells subarrayWithRange:removed] makeObjectsPerformSelector:@selector(_setSegmentedControl:) withObject:nil];
		[_cells removeObjectsInRange:removed];
	}
	[[_cells lastObject] _setRightCap:YES];
	if ((NSUInteger)_selectedSegment >= [_cells count]) {
		_selectedSegment = -1;
		if (!_smkFlags.allowsEmptySelection && [_cells count] != 0) {
			[self setSelectedSegment:0];
		}
	}
	[self _setUpdatesDelayed:NO];
	[self setNeedsDisplay:YES];
}

// an NSSegmentedControl with the same segments
- (id)nsSegmentedControl
{
	NSInteger count = (NSInteger)[_cells count];
	NSSegmentedControl *control = [[NSClassFromString(@"NSSegmentedControl") alloc] initWithFrame:[self frame]];
	if (count > 0) {
		[control setSegmentCount:count];
		[(NSSegmentedCell *)[control cell] setTrackingMode:[self trackingMode]];
		for (NSInteger i = 0; i < count; i++) {
			SMKSegmentedCell *cell = [_cells objectAtIndex:i];
			[control setWidth:[cell segmentWidth] forSegment:i];
			[control setImage:[cell image] forSegment:i];
			[control setLabel:[cell title] forSegment:i];
			[control setMenu:[self menuForSegment:i] forSegment:i];
			[control setSelected:[self isSelectedForSegment:i] forSegment:i];
			[control setEnabled:[cell isEnabled] forSegment:i];
			[(NSSegmentedCell *)[control cell] setToolTip:[self toolTipForSegment:i] forSegment:i];
			[(NSSegmentedCell *)[control cell] setTag:[cell tag] forSegment:i];
		}
	}
	return [control autorelease];
}

- (BOOL)autosizeIgnoresProportions
{
	return _smkFlags.ignoreProportions;
}

- (void)setAutosizeIgnoresProportions:(BOOL)aProportions
{
	if (_smkFlags.ignoreProportions != (unsigned int)(aProportions != NO)) {
		_smkFlags.ignoreProportions = aProportions != NO;
		if (_smkFlags.autosizesCells) {
			[self _autosizeCellsWithSize:[self bounds].size];
		}
	}
}

- (BOOL)autosizesCells
{
	return _smkFlags.autosizesCells;
}

- (void)setAutosizesCells:(BOOL)aCells
{
	if (_smkFlags.autosizesCells == (unsigned int)(aCells != NO)) {
		return;
	}
	_smkFlags.autosizesCells = aCells != NO;
	if (aCells) {
		[self _autosizeCellsWithSize:[self bounds].size];
	}
	else {
		[self _sizeCellsToMinimum];
	}
}

- (NSControlSize)controlSize
{
	return [[self cell] controlSize];
}

- (void)setControlSize:(NSControlSize)aSize
{
	[self _setUpdatesDelayed:YES];
	[[self cell] setControlSize:aSize];
	for (SMKSegmentedCell *cell in _cells) {
		[cell setControlSize:aSize];
	}
	[self _setUpdatesDelayed:NO];
}

- (NSInteger)trackingMode
{
	return _smkFlags.trackingMode;
}

- (void)setTrackingMode:(NSInteger)aMode
{
	unsigned int old = _smkFlags.trackingMode;
	if (old == (unsigned int)aMode) {
		return;
	}
	_smkFlags.trackingMode = (unsigned int)aMode;
	if ([_cells count] == 0) {
		return;
	}
	if (old == NSSegmentSwitchTrackingMomentary) {
		for (SMKSegmentedCell *cell in _cells) {
			[cell setButtonType:NSButtonTypePushOnPushOff];
		}
	}
	if (aMode == NSSegmentSwitchTrackingMomentary) {
		_selectedSegment = -1;
		_smkFlags.allowsEmptySelection = 1;
		for (SMKSegmentedCell *cell in _cells) {
			[cell _setState:NSControlStateValueOff];
			[cell setButtonType:NSButtonTypeMomentaryLight];
		}
		return;
	}
	if (aMode == NSSegmentSwitchTrackingSelectOne && _selectedSegment != -1) {
		// only the selected segment stays on
		NSInteger count = (NSInteger)[_cells count];
		for (NSInteger i = 0; i < count; i++) {
			if (i != _selectedSegment && [[_cells objectAtIndex:i] state] == NSControlStateValueOn) {
				[[_cells objectAtIndex:i] _setState:NSControlStateValueOff];
			}
		}
	}
	else if (aMode != NSSegmentSwitchTrackingSelectOne && aMode != NSSegmentSwitchTrackingSelectAny) {
		return;
	}
	if (_selectedSegment == -1 && !_smkFlags.allowsEmptySelection) {
		_selectedSegment = 0;
		[[_cells objectAtIndex:0] _setState:NSControlStateValueOn];
	}
}

- (id)clickedCell
{
	return _clickedCell;
}

- (id)cellWithTag:(NSInteger)aTag
{
	id cell = nil;
	return [self _getSegment:NULL cell:&cell withTag:aTag] ? cell : nil;
}

- (NSInteger)segmentWithTag:(NSInteger)aTag
{
	int segment = -1;
	return [self _getSegment:&segment cell:NULL withTag:aTag] ? segment : -1;
}

- (id)cellAtPoint:(NSPoint)aPoint
{
	id cell = nil;
	return [self _getSegment:NULL cell:&cell atPoint:aPoint cellFrame:NULL] ? cell : nil;
}

- (NSInteger)segmentAtPoint:(NSPoint)aPoint
{
	int segment = -1;
	return [self _getSegment:&segment cell:NULL atPoint:aPoint cellFrame:NULL] ? segment : -1;
}

- (NSRect)cellFrameForCell:(id)aCell
{
	if (_cells != nil) {
		NSUInteger index = [_cells indexOfObject:aCell];
		if (index != NSNotFound) {
			return [self cellFrameForSegment:index];
		}
	}
	return NSZeroRect;
}

- (NSRect)cellFrameForSegment:(NSInteger)aSegment
{
	SMK_CHECK_SEGMENT(aSegment);
	NSRect frame = NSZeroRect;
	[self _ensureFramesAreValid];
	if (![[_cells objectAtIndex:aSegment] _getFrameInSegmentedControl:&frame]) {
		SMK_INCONSISTENCY();
	}
	return frame;
}

- (id)cellForSegment:(NSInteger)aSegment
{
	if (aSegment >= 0 && (NSUInteger)aSegment < [_cells count]) {
		return [_cells objectAtIndex:aSegment];
	}
	return nil;
}

- (NSInteger)segmentForCell:(id)aCell
{
	if (aCell != nil && _cells != nil) {
		NSUInteger index = [_cells indexOfObject:aCell];
		if (index != NSNotFound) {
			return (NSInteger)index;
		}
	}
	return -1;
}

- (void)viewDidMoveToWindow
{
	if ([self window] != nil) {
		[self _setToolTipSupport:YES];
	}
}

- (void)viewWillMoveToWindow:(id)aWindow
{
	if ([self window] != nil) {
		[self _setToolTipSupport:NO];
	}
}

- (void)setFont:(id)aFont
{
	[self _setUpdatesDelayed:YES];
	[_cells makeObjectsPerformSelector:@selector(setFont:) withObject:aFont];
	[super setFont:aFont];
	[self _setUpdatesDelayed:NO];
}

// segments without an action send the control's
- (BOOL)sendAction:(SEL)aAction to:(id)aTo
{
	if (aAction == NULL) {
		aAction = [self action];
		aTo = [self target];
	}
	return [super sendAction:aAction to:aTo];
}

// redraws only the changed segment where its frame stayed the same
- (void)updateCell:(id)aCell
{
	if (_smkFlags.updatesDelayed) {
		_smkFlags.pendingUpdates = 1;
		return;
	}
	NSUInteger index = _cells != nil ? [_cells indexOfObject:aCell] : NSNotFound;
	NSRect frame = NSZeroRect;
	NSRect oldFrame = NSZeroRect;
	SMKSegmentedCell *cell = [aCell isKindOfClass:[SMKSegmentedCell class]] ? aCell : nil;
	BOOL hadFrame = [cell _getFrameInSegmentedControl:&oldFrame];
	if (index != NSNotFound) {
		if (cell == nil) {
			SMK_INCONSISTENCY();
		}
		[self _ensureFramesAreValid];
		if (![cell _getFrameInSegmentedControl:&frame]) {
			SMK_INCONSISTENCY();
		}
	}
	if (hadFrame && NSEqualRects(oldFrame, frame)) {
		if (NSIsEmptyRect(frame)) {
			[super updateCell:aCell];
		}
		else {
			[self setNeedsDisplayInRect:frame];
		}
		return;
	}
	[self setNeedsDisplay:YES];
}

- (void)setFrame:(NSRect)aFrame
{
	NSRect old = [self frame];
	[super setFrame:aFrame];
	if (old.size.width != aFrame.size.width) {
		[self _autosizeCellsWithSize:aFrame.size];
	}
}

- (void)setFrameSize:(NSSize)aSize
{
	NSRect old = [self frame];
	[super setFrameSize:aSize];
	if (old.size.width != aSize.width) {
		[self _autosizeCellsWithSize:aSize];
	}
}

// the segments' minimum sizes; ignoring proportions, every autosized
// segment is as wide as the widest of them
- (void)sizeToFit
{
	[self _setUpdatesDelayed:YES];
	[self _calculateAutosizeInfo];
	if (_smkFlags.autosizesCells) {
		[self _sizeCellsToMinimum];
	}
	float width = (float)_autoMinSize.width;
	float height = (float)_autoMinSize.height;
	if (_smkFlags.ignoreProportions && _smkFlags.hasAutosizedSegments && _smkFlags.autosizesCells) {
		NSInteger count = (NSInteger)[_cells count];
		float widest = 0.0f;
		for (NSInteger i = 0; i < count; i++) {
			SMKSegmentedCell *cell = [_cells objectAtIndex:i];
			if (![cell _hasCustomSegmentWidth]) {
				widest = MAX(widest, (float)[cell _minSegmentWidth]);
			}
		}
		width = ((float)_autoMinSize.width - _autosizedSegmentsMinSum) + (float)_autosizedSegmentsCount * widest;
	}
	[super setFrameSize:NSMakeSize(width, height)];
	if (_smkFlags.autosizesCells) {
		[self setNeedsDisplay:YES];
	}
	[self _setUpdatesDelayed:NO];
}

// a right click shows a segment's menu
- (void)rightMouseDown:(id)aDown
{
	id cell = nil;
	NSPoint p = [self convertPoint:[aDown locationInWindow] fromView:nil];
	NSRect frame = NSZeroRect;
	if ([self _getSegment:NULL cell:&cell atPoint:p cellFrame:&frame] && [cell menu] != nil) {
		[cell trackMenuWithEvent:aDown inRect:[self cellFrameForCell:cell] ofView:self];
	}
	[super rightMouseDown:aDown];
}

// the clicked segment tracks; its new state goes through the selection
- (void)mouseDown:(id)aDown
{
	id cell = nil;
	NSPoint p = [self convertPoint:[aDown locationInWindow] fromView:nil];
	NSRect frame = NSZeroRect;
	if ([self _getSegment:NULL cell:&cell atPoint:p cellFrame:&frame]) {
		_clickedCell = cell;
	}
	[super mouseDown:aDown];
	cell = _clickedCell;
	_clickedCell = nil;
	if (_cells != nil) {
		NSUInteger index = [_cells indexOfObject:cell];
		if (index != NSNotFound) {
			[self setSelected:[cell state] == NSControlStateValueOn forSegment:index];
		}
	}
}

- (void)drawRect:(NSRect)aRect
{
	NSInteger count = (NSInteger)[_cells count];
	NSRect frame = NSZeroRect;
	[self _ensureFramesAreValid];
	for (NSInteger i = 0; i < count; i++) {
		SMKSegmentedCell *cell = [_cells objectAtIndex:i];
		if (![cell _getFrameInSegmentedControl:&frame]) {
			SMK_INCONSISTENCY();
		}
		else {
			[cell drawWithFrame:frame inView:self];
		}
	}
}

- (BOOL)isFlipped
{
	return YES;
}

- (BOOL)isOpaque
{
	return NO;
}

- (BOOL)resignFirstResponder
{
	BOOL result = [super resignFirstResponder];
	if (result) {
		[self setKeySegment:-1];
		[super setKeyboardFocusRingNeedsDisplayInRect:[self bounds]];
	}
	return result;
}

- (BOOL)becomeFirstResponder
{
	BOOL result = [super becomeFirstResponder];
	if (result) {
		// the selected segment takes the focus, or the first
		[self setKeySegment:_selectedSegment != -1 ? _selectedSegment : 0];
		[super setKeyboardFocusRingNeedsDisplayInRect:[self bounds]];
	}
	return result;
}

// toggles the key segment
- (void)performClick:(id)aClick
{
	[self setSelected:![self isSelectedForSegment:_keySegment] forSegment:_keySegment];
	[[self target] performSelector:[self action] withObject:self];
	[[self window] makeFirstResponder:self];
	[self setKeySegment:_keySegment];
	[super setKeyboardFocusRingNeedsDisplayInRect:[self bounds]];
}

- (void)moveLeft:(id)aLeft
{
	if (_keySegment > 0) {
		[self setKeySegment:_keySegment - 1];
	}
	[super setKeyboardFocusRingNeedsDisplayInRect:[self bounds]];
	NSAccessibilityPostNotification([self keyCell], NSAccessibilityFocusedUIElementChangedNotification);
}

- (void)moveRight:(id)aRight
{
	if (_keySegment != -1 && (NSUInteger)(_keySegment + 1) < [_cells count]) {
		[self setKeySegment:_keySegment + 1];
	}
	[super setKeyboardFocusRingNeedsDisplayInRect:[self bounds]];
	NSAccessibilityPostNotification([self keyCell], NSAccessibilityFocusedUIElementChangedNotification);
}

// updates while delayed are collected into one redraw
- (void)_setUpdatesDelayed:(BOOL)aDelayed
{
	_smkFlags.updatesDelayed = aDelayed;
	if (_smkFlags.pendingUpdates && !_smkFlags.updatesDelayed) {
		_smkFlags.pendingUpdates = 0;
		[self setNeedsDisplay:YES];
	}
}

- (id)_addCell
{
	SMKSegmentedCell *cell = [[SMKSegmentedCell alloc] initTextCell:@""];
	[cell setFont:[self font]];
	if ([self trackingMode] == NSSegmentSwitchTrackingMomentary) {
		[cell setButtonType:NSButtonTypeMomentaryLight];
	}
	[_cells addObject:cell];
	[cell _setSegmentedControl:self];
	[cell release];
	return cell;
}

- (NSInteger)_firstSelectedSegmentExceptSegment:(NSInteger)aSegment
{
	NSInteger count = (NSInteger)[_cells count];
	for (NSInteger i = 0; i < count; i++) {
		if (i != aSegment && [[_cells objectAtIndex:i] state] == NSControlStateValueOn) {
			return i;
		}
	}
	return -1;
}

- (BOOL)_getSegment:(int *)aSegment cell:(id *)aCell withTag:(NSInteger)aTag
{
	NSInteger count = (NSInteger)[_cells count];
	for (NSInteger i = 0; i < count; i++) {
		id cell = [_cells objectAtIndex:i];
		if ([cell tag] == aTag) {
			if (aSegment != NULL) {
				*aSegment = (int)i;
			}
			if (aCell != NULL) {
				*aCell = cell;
			}
			return YES;
		}
	}
	return NO;
}

- (void)_numberOfCellFramesChanged
{
	_smkFlags.autosizeInfoValid = 0;
	_smkFlags.cellFramesValid = 0;
}

- (BOOL)_getSegment:(int *)aSegment cell:(id *)aCell atPoint:(NSPoint)aPoint cellFrame:(NSRect *)aFrame
{
	if (!NSMouseInRect(aPoint, [self bounds], [self isFlipped])) {
		return NO;
	}
	NSInteger count = (NSInteger)[_cells count];
	[self _ensureFramesAreValid];
	for (NSInteger i = 0; i < count; i++) {
		NSRect frame;
		if (![[_cells objectAtIndex:i] _getFrameInSegmentedControl:&frame]) {
			SMK_INCONSISTENCY();
			continue;
		}
		if (NSMouseInRect(aPoint, frame, [self isFlipped])) {
			if (aSegment != NULL) {
				*aSegment = (int)i;
			}
			if (aCell != NULL) {
				*aCell = [_cells objectAtIndex:i];
			}
			if (aFrame != NULL) {
				*aFrame = frame;
			}
			return YES;
		}
	}
	return NO;
}

- (void)_ensureFramesAreValid
{
	if (_smkFlags.autosizesCells && !_smkFlags.autosizeInfoValid) {
		[self _autosizeCellsWithSize:[self bounds].size];
	}
	NSInteger count = (NSInteger)[_cells count];
	for (NSInteger i = 0; i < count && _smkFlags.cellFramesValid; i++) {
		if (![[_cells objectAtIndex:i] _frameIsValid]) {
			_smkFlags.cellFramesValid = 0;
		}
	}
	[self _calculateCellFrames];
}

// Shares the width beyond the minimum among the autosized segments, in
// proportion to their minimum widths or evenly.
- (void)_autosizeCellsWithSize:(NSSize)aSize
{
	if (!_smkFlags.autosizesCells) {
		return;
	}
	[self _setUpdatesDelayed:YES];
	[self _calculateAutosizeInfo];
	if (_smkFlags.hasAutosizedSegments) {
		float minWidth = (float)_autoMinSize.width;
		if (!_smkFlags.ignoreProportions) {
			float extra = smk_roundf((float)aSize.width - minWidth);
			for (int i = 0; i <= _lastAutosizedIdx; i++) {
				SMKSegmentedCell *cell = [_cells objectAtIndex:i];
				float ratio = 0.0f;
				if ([cell _getAutosizeRatio:&ratio]) {
					float add = smk_roundf(extra * ratio);
					[cell _setSegmentWidth:MAX(0.0f, (float)[cell _minSegmentWidth] + add)];
					_smkFlags.cellFramesValid = 0;
				}
			}
		}
		else {
			int rest = (int)((float)aSize.width - (minWidth - _autosizedSegmentsMinSum));
			int each = rest / _autosizedSegmentsCount;
			int remainder = rest % _autosizedSegmentsCount;
			int n = 0;
			for (int i = 0; i <= _lastAutosizedIdx; i++) {
				SMKSegmentedCell *cell = [_cells objectAtIndex:i];
				if ([cell _hasCustomSegmentWidth]) {
					continue;
				}
				float width = (float)each;
				if (n < remainder) {
					width += 1.0f;
				}
				rest = (int)((float)rest - width);
				if (i == _lastAutosizedIdx) {
					width += (float)rest;
					rest = 0;
				}
				[cell _setSegmentWidth:width];
				_smkFlags.cellFramesValid = 0;
				n++;
			}
		}
	}
	[self _setUpdatesDelayed:NO];
}

- (void)_sizeCellsToMinimum
{
	_smkFlags.cellFramesValid = 0;
	[_cells makeObjectsPerformSelector:@selector(_resetSegmentWidth)];
}

// The segments' total minimum size with a margin of 5, and each autosized
// segment's share of the autosized minimum widths.
- (void)_calculateAutosizeInfo
{
	if (_smkFlags.autosizeInfoValid) {
		return;
	}
	NSInteger count = (NSInteger)[_cells count];
	NSRect r = NSZeroRect;
	_smkFlags.hasAutosizedSegments = 0;
	_autosizedSegmentsMinSum = 0.0f;
	_autosizedSegmentsCount = 0;
	float width = 0.0f;
	float height = 0.0f;
	for (NSInteger i = 0; i < count; i++) {
		SMKSegmentedCell *cell = [_cells objectAtIndex:i];
		NSSize size;
		if (![cell _hasCustomSegmentWidth]) {
			_smkFlags.hasAutosizedSegments = 1;
			_autosizedSegmentsCount++;
			_lastAutosizedIdx = (int)i;
			_autosizedSegmentsMinSum += (float)[cell _minSegmentWidth];
			size = [cell _cellSizeAtMinimumWidth];
		}
		else {
			size = [cell cellSize];
		}
		width += (float)size.width;
		height = MAX(height, (float)size.height);
	}
	r.size = NSMakeSize(width, height);
	r = NSIsEmptyRect(r) ? NSZeroRect : NSInsetRect(r, -5.0, -5.0);
	if (_smkFlags.hasAutosizedSegments) {
		for (NSInteger i = 0; i < count; i++) {
			SMKSegmentedCell *cell = [_cells objectAtIndex:i];
			if (![cell _hasCustomSegmentWidth]) {
				[cell _setAutosizeRatio:(float)[cell _minSegmentWidth] / _autosizedSegmentsMinSum];
			}
		}
	}
	_autoMinSize = r.size;
	_smkFlags.autosizeInfoValid = 1;
	_smkFlags.cellFramesValid = 0;
}

// segments side by side from (5, 5), all as tall as the tallest
- (void)_calculateCellFrames
{
	if (_smkFlags.cellFramesValid) {
		return;
	}
	NSInteger count = (NSInteger)[_cells count];
	NSRect frame = NSMakeRect(5.0, 5.0, 0.0, 0.0);
	BOOL sameHeights = YES;
	int height = 0;
	for (NSInteger i = 0; i < count; i++) {
		frame.size = [[_cells objectAtIndex:i] cellSize];
		int h = (int)smk_ceilf((float)frame.size.height);
		if (i == 0) {
			height = h;
		}
		else if (h != height) {
			height = MAX(h, height);
			sameHeights = NO;
		}
		[[_cells objectAtIndex:i] _setFrameInSegmentedControl:frame];
		frame.origin.x = NSMaxX(frame);
	}
	if (!sameHeights) {
		for (NSInteger i = 0; i < count; i++) {
			SMKSegmentedCell *cell = [_cells objectAtIndex:i];
			if (![cell _getFrameInSegmentedControl:&frame]) {
				SMK_INCONSISTENCY();
				continue;
			}
			frame.size.height = (float)height;
			[cell _setFrameInSegmentedControl:frame];
		}
	}
	_smkFlags.cellFramesValid = 1;
	[self _resetToolTips];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	_clickedCell = nil;
	[_cells makeObjectsPerformSelector:@selector(_setSegmentedControl:) withObject:nil];
	[_cells release];
	[super dealloc];
}

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self != nil) {
		_cells = [NSMutableArray new];
		_selectedSegment = -1;
	}
	return self;
}

- (id)init
{
	return [self initWithFrame:NSZeroRect];
}

@end

@implementation SMKSegmentedControl (KeyboardUI)

- (id)keyCell
{
	return _keySegment == -1 ? nil : [_cells objectAtIndex:_keySegment];
}

- (void)setKeyCell:(id)aCell
{
	[self setKeySegment:[self segmentForCell:aCell]];
}

- (BOOL)tabKeyTraversesCells
{
	return NO;
}

- (void)setTabKeyTraversesCells:(BOOL)aCells
{
}

// only the key segment shows the focus ring
- (void)setKeySegment:(NSInteger)aSegment
{
	if (_keySegment != aSegment && _cells != nil) {
		for (SMKSegmentedCell *cell in _cells) {
			[cell setShowsFirstResponder:NO];
		}
	}
	_keySegment = (int)aSegment;
	// the original allowed an index one past the end here
	if (aSegment != -1 && (NSUInteger)aSegment < [_cells count]) {
		[[_cells objectAtIndex:_keySegment] setShowsFirstResponder:YES];
	}
}

@end

@implementation SMKSegmentedControl (SMKSegmentedControl_ToolTipsSupport)

- (id)accessibilityFocusedUIElement
{
	id cell = [self keyCell];
	if (cell != nil && [[self window] firstResponder] == self) {
		return cell;
	}
	return [super accessibilityFocusedUIElement];
}

- (id)accessibilityHitTest:(NSPoint)aTest
{
	NSPoint p = [[self window] convertPointFromScreen:aTest];
	p = [self convertPoint:p fromView:nil];
	id element = [[self cellAtPoint:p] accessibilityHitTest:aTest];
	return element != nil ? element : NSAccessibilityUnignoredAncestor(self);
}

- (BOOL)accessibilityIsAttributeSettable:(id)aSettable
{
	if ([aSettable isEqualToString:NSAccessibilityChildrenAttribute] || [aSettable isEqualToString:NSAccessibilityRoleAttribute]) {
		return NO;
	}
	return [super accessibilityIsAttributeSettable:aSettable];
}

- (BOOL)accessibilityIsIgnored
{
	return NO;
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityChildrenAttribute]) {
		return [self cells];
	}
	if ([aValue isEqualToString:NSAccessibilityRoleAttribute]) {
		return NSAccessibilityGroupRole;
	}
	return [super accessibilityAttributeValue:aValue];
}

// tool tip rects follow the frame while in a window
- (void)_setToolTipSupport:(BOOL)aSupport
{
	if (!aSupport) {
		if (_smkFlags.listensToFrameDidChange) {
			[[NSNotificationCenter defaultCenter] removeObserver:self name:NSViewFrameDidChangeNotification object:self];
			_smkFlags.listensToFrameDidChange = 0;
		}
		[self _cleanupToolTips];
		return;
	}
	if (!_smkFlags.listensToFrameDidChange) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_resetToolTips)
			name:NSViewFrameDidChangeNotification object:self];
		_smkFlags.listensToFrameDidChange = 1;
	}
	[self _resetToolTips];
}

- (id)view:(id)aView stringForToolTip:(NSInteger)aTip point:(NSPoint)aPoint userData:(void *)aData
{
	if (!_smkFlags.toolTipsSetUp) {
		return nil;
	}
	NSParameterAssert(aView == self);
	return [[self cellAtPoint:aPoint] toolTip];
}

- (void)_resetToolTips
{
	if (_smkFlags.toolTipsSetUp) {
		[self _cleanupToolTips];
	}
	[self _setupToolTips];
}

- (void)_setupToolTips
{
	NSAssert(!_smkFlags.toolTipsSetUp, @"");
	if ([self window] == nil) {
		return;
	}
	NSInteger count = (NSInteger)[_cells count];
	[self _ensureFramesAreValid];
	for (NSInteger i = 0; i < count; i++) {
		NSRect frame;
		if (![[_cells objectAtIndex:i] _getFrameInSegmentedControl:&frame]) {
			SMK_INCONSISTENCY();
			continue;
		}
		[self addToolTipRect:frame owner:self userData:NULL];
	}
	_smkFlags.toolTipsSetUp = 1;
}

- (void)_cleanupToolTips
{
	if (_smkFlags.toolTipsSetUp) {
		[self removeAllToolTips];
		_smkFlags.toolTipsSetUp = 0;
	}
}

@end

@implementation SMKSegmentedControl (SMKSegmentedControl_Private)

- (void)_toolTipChangedForCell:(id)aCell
{
	[self _resetToolTips];
}

- (BOOL)_allowsChangeStateForCell:(id)aCell
{
	return _smkFlags.trackingMode < NSSegmentSwitchTrackingMomentary;
}

- (BOOL)_compareCell:(id)aCell toCell:(id)aCell2 result:(int *)aResult
{
	NSParameterAssert(aResult != NULL);
	if (_cells == nil) {
		return NO;
	}
	NSUInteger a = [_cells indexOfObject:aCell];
	NSUInteger b = [_cells indexOfObject:aCell2];
	if (a == NSNotFound || b == NSNotFound) {
		return NO;
	}
	*aResult = a < b ? -1 : (a != b);
	return YES;
}

- (void)_toggleAutosizeForCell:(id)aCell
{
	if (_smkFlags.autosizesCells) {
		_smkFlags.autosizeInfoValid = 0;
		_smkFlags.cellFramesValid = 0;
	}
}

// the segment and those after it move
- (void)_cellContentWillChange:(id)aChange
{
	if (_cells == nil) {
		return;
	}
	NSUInteger index = [_cells indexOfObject:aChange];
	if (index == NSNotFound) {
		return;
	}
	NSUInteger count = [_cells count];
	_smkFlags.cellFramesValid = 0;
	[self _cleanupToolTips];
	_smkFlags.autosizeInfoValid = 0;
	for (; index < count; index++) {
		[[_cells objectAtIndex:index] _invalidateFrameInSegmentedControl];
	}
}

// A segment's own state changes go through the selection; the cell never
// sets them itself. Clicking the selected segment of a select-one control
// keeps it on.
- (BOOL)_shouldSetNewState:(NSInteger)aState forCell:(id)aCell
{
	if (_cells == nil) {
		return NO;
	}
	NSUInteger index = [_cells indexOfObject:aCell];
	if (index == NSNotFound) {
		return NO;
	}
	NSInteger mode = [self trackingMode];
	if (mode == NSSegmentSwitchTrackingSelectOne) {
		if (aCell == _clickedCell && aState == NSControlStateValueOff && (int)index == _selectedSegment) {
			aState = NSControlStateValueOn;
		}
	}
	else if (mode != NSSegmentSwitchTrackingSelectAny) {
		return NO;
	}
	[self setSelected:aState == NSControlStateValueOn forSegment:index];
	return NO;
}

@end
