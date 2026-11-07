#import "SMKBackgroundPopUpButtonCell.h"

// A pop-up button cell drawn like SMKBackgroundButtonCell, with a pull-down
// arrow. It can also track like a button: a click toggles its state and
// sends the action, and holding the mouse (or a control-click) shows the
// menu. With custom content it shows its own title and image instead of
// the selected item's.
@implementation SMKBackgroundPopUpButtonCell

- (void)accessibilityPerformAction:(id)aAction
{
	if ([aAction isEqualToString:NSAccessibilityPressAction]) {
		[self performClick:nil];
		return;
	}
	[super accessibilityPerformAction:aAction];
}

- (BOOL)isEnabled
{
	return [super isEnabled];
}

- (BOOL)isSelected
{
	if ([self state] == NSControlStateValueOn && ([self showsStateBy] & NSChangeBackgroundCellMask)) {
		return YES;
	}
	return _smkBppFlags.isHighlighted && ([self highlightsBy] & NSChangeBackgroundCellMask);
}

- (BOOL)isRightCap
{
	return YES;
}

- (BOOL)isLeftCap
{
	return YES;
}

- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp
{
	NSPoint p = [aView convertPoint:[aMouse locationInWindow] fromView:nil];
	NSRect frame = aRect;
	frame.size.height = MIN(frame.size.height, [self cellSizeForBounds:aRect].height);
	BOOL result = YES;
	NSControl *control = [aView isKindOfClass:[NSControl class]] ? aView : nil;
	if ([(NSEvent *)aMouse type] == NSEventTypeLeftMouseDown) {
		if (![aView mouse:p inRect:frame]) {
			result = NO;
		}
		else if (_smkBppFlags.tracksLikeButton && ([aMouse modifierFlags] & NSEventModifierFlagControl)) {
			[self _showMenuWithEvent:aMouse inView:aView cellFrame:frame];
		}
		else {
			[self highlight:YES withFrame:frame inView:aView];
			BOOL showMenu = YES;
			if (_smkBppFlags.tracksLikeButton) {
				// a click toggles; holding still until the periodic event shows the menu
				NSWindow *window = [aView window];
				float delay, interval;
				[self getPeriodicDelay:&delay interval:&interval];
				[NSEvent startPeriodicEventsAfterDelay:delay withPeriod:interval];
				BOOL inside = YES;
				BOOL still = YES;
				showMenu = NO;
				for (;;) {
					NSEvent *event = [window nextEventMatchingMask:NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged | NSEventMaskPeriodic];
					NSEventType type = [event type];
					if (type != NSEventTypePeriodic) {
						inside = [aView mouse:[aView convertPoint:[event locationInWindow] fromView:nil] inRect:frame];
					}
					if (type == NSEventTypeLeftMouseDragged) {
						if (inside != _smkBppFlags.isHighlighted) {
							[self highlight:inside withFrame:frame inView:aView];
							still = NO;
						}
					}
					else if (type == NSEventTypePeriodic) {
						[NSEvent stopPeriodicEvents];
						if (still) {
							_smkBppFlags.state = 1;
							showMenu = YES;
							break;
						}
					}
					else if (type == NSEventTypeLeftMouseUp) {
						[NSEvent stopPeriodicEvents];
						if (inside && control != nil) {
							_smkBppFlags.state ^= 1;
							[control sendAction:[control action] to:[control target]];
						}
						break;
					}
				}
			}
			if (showMenu) {
				[self _showMenuWithEvent:aMouse inView:aView cellFrame:frame];
			}
		}
	}
	else if ([(NSEvent *)aMouse type] == NSEventTypeKeyDown) {
		// space toggles a button-like cell
		if (!_smkBppFlags.tracksLikeButton || ![[aMouse characters] isEqual:@" "]
			|| ([aMouse modifierFlags] & (NSEventModifierFlagShift | NSEventModifierFlagControl | NSEventModifierFlagOption | NSEventModifierFlagCommand))) {
			return [super trackMouse:aMouse inRect:aRect ofView:aView untilMouseUp:aUp];
		}
		if (control != nil) {
			_smkBppFlags.state ^= 1;
			[control sendAction:[control action] to:[control target]];
			[control setKeyboardFocusRingNeedsDisplayInRect:[control bounds]];
		}
	}
	else {
		result = NO;
	}
	if (_smkBppFlags.isHighlighted) {
		[self highlight:NO withFrame:frame inView:aView];
	}
	return result;
}

- (void)highlight:(BOOL)aHighlight withFrame:(NSRect)aFrame inView:(id)aView
{
	NSControl *control = [aView isKindOfClass:[NSControl class]] ? aView : nil;
	_smkBppFlags.isHighlighted = aHighlight != NO;
	[self _invalidateImageCache];
	[control updateCell:self];
}

// The menu pops up below the cell's left edge, as a context menu from a
// copy of the event moved there.
- (void)_showMenuWithEvent:(id)aShowMenuEvent inView:(id)aView cellFrame:(NSRect)aFrame
{
	NSEvent *aEvent = aShowMenuEvent;
	NSPoint p = [aView convertPoint:NSMakePoint(aFrame.origin.x + 1.0, (float)(NSMaxY(aFrame) + 5.0f) - 1.0f) toView:nil];
	NSEvent *event;
	NSEventType type = [aEvent type];
	if (type == NSEventTypeLeftMouseDown) {
		event = [NSEvent mouseEventWithType:type location:p modifierFlags:[aEvent modifierFlags] timestamp:[aEvent timestamp]
			windowNumber:[aEvent windowNumber] context:nil eventNumber:[aEvent eventNumber] clickCount:[aEvent clickCount]
			pressure:[aEvent pressure]];
	}
	else if (type == NSEventTypeKeyDown) {
		event = [NSEvent keyEventWithType:type location:p modifierFlags:[aEvent modifierFlags] timestamp:[aEvent timestamp]
			windowNumber:[aEvent windowNumber] context:nil characters:[aEvent characters]
			charactersIgnoringModifiers:[aEvent charactersIgnoringModifiers] isARepeat:[aEvent isARepeat] keyCode:[aEvent keyCode]];
	}
	else {
		return;
	}
	if (event != nil) {
		[NSMenu popUpContextMenu:[self menu] withEvent:event forView:aView withFont:[self font]];
	}
}

- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	NSSize arrowSize = NSZeroSize;
	float arrowX = 0.0f;
	NSRect content = aFrame;
	NSImage *arrow = [self arrowImage];
	_frameRect = aFrame;
	if (arrow != nil) {
		[arrow setFlipped:[aView isFlipped]];
		arrowSize = [_arrowImage size];
	}
	BOOL ring = [self showsFirstResponder];
	if (![self drawsBackground]) {
		if (arrowSize.width > 0.0) {
			float width = (float)aFrame.size.width - ((float)arrowSize.width + 2.0f);
			arrowX = (float)aFrame.origin.x + width;
			content.size.width = width - 2.0f;
		}
		if (ring) {
			[NSGraphicsContext saveGraphicsState];
			NSSetFocusRingStyle(NSFocusRingAbove);
			if ([self displayedImagePosition] == NSImageOnly) {
				[[self contentRenderer] drawInteriorWithFrame:content inView:aView];
			}
			else {
				[[NSBezierPath bezierPathWithRect:aFrame] fill];
			}
			[NSGraphicsContext restoreGraphicsState];
		}
	}
	else {
		NSImage *background = [self _backgroundImageForWidth:aFrame.size.width flipped:[aView isFlipped]];
		NSRect from = NSZeroRect;
		from.size = [background size];
		NSRect to = NSMakeRect(aFrame.origin.x, aFrame.origin.y, from.size.width, from.size.height);
		if (ring) {
			[NSGraphicsContext saveGraphicsState];
			NSSetFocusRingStyle(NSFocusRingAbove);
		}
		[background drawInRect:to fromRect:from operation:NSCompositingOperationSourceOver fraction:1.0];
		if (ring) {
			[NSGraphicsContext restoreGraphicsState];
		}
		struct _SMKInset inset = [[self backgroundRenderer] contentInsetForObject:self];
		if ([aView isFlipped]) {
			float top = inset.top;
			inset.top = inset.bottom;
			inset.bottom = top;
		}
		content.origin.x = aFrame.origin.x + inset.left;
		content.size.width = MAX(0.0f, (float)from.size.width - (inset.left + inset.right));
		content.origin.y = aFrame.origin.y + inset.bottom;
		content.size.height = MAX(0.0f, (float)from.size.height - (inset.bottom + inset.top));
		if (arrowSize.width > 0.0) {
			float width = (float)content.size.width - (float)arrowSize.width;
			arrowX = (float)content.origin.x + width;
			content.size.width = width - 2.0f;
		}
	}
	if (arrow != nil) {
		NSRect r = NSMakeRect(arrowX, content.origin.y + smk_roundf(((float)content.size.height - (float)arrowSize.height) * 0.5f),
			arrowSize.width, arrowSize.height);
		if (!NSIsEmptyRect(r)) {
			NSRect from = { NSZeroPoint, arrowSize };
			[arrow drawInRect:r fromRect:from operation:NSCompositingOperationSourceOver fraction:1.0];
		}
	}
	if (!NSIsEmptyRect(content)) {
		[[self contentRenderer] drawInteriorWithFrame:content inView:aView];
	}
}

// the content's size plus the arrow and, with a background, the caps
- (NSSize)cellSizeForBounds:(NSRect)aBounds
{
	NSSize arrowSize = NSZeroSize;
	NSImage *arrow = [self arrowImage];
	if (arrow != nil) {
		arrowSize = [arrow size];
	}
	if (_smkBppFlags.dontDrawBackground) {
		NSRect r = aBounds;
		if (arrowSize.width > 0.0) {
			r.size.width -= arrowSize.width + 4.0;
		}
		NSSize size = [[self contentRenderer] cellSizeForBounds:r];
		if (arrowSize.width > 0.0) {
			size.width += arrowSize.width + 4.0;
		}
		return size;
	}
	struct _SMKInset inset = [[self backgroundRenderer] contentInsetForObject:self];
	NSRect r = aBounds;
	r.origin.x += inset.left;
	r.size.width = MAX(0.0, r.size.width - (inset.left + inset.right));
	r.origin.y += inset.bottom;
	r.size.height = MAX(0.0, r.size.height - (inset.bottom + inset.top));
	if (arrowSize.width > 0.0) {
		r.size.width -= arrowSize.width + 2.0;
	}
	NSSize size = [[self contentRenderer] cellSizeForBounds:r];
	if (arrowSize.width > 0.0) {
		size.width += arrowSize.width + 2.0;
	}
	float width = MAX(0.0f, (float)size.width + inset.left + inset.right);
	return NSMakeSize(width, [[self backgroundRenderer] segmentHeightForObject:self]);
}

- (id)_backgroundImageForWidth:(CGFloat)aWidth flipped:(BOOL)aFlipped
{
	if (_backgroundImage == nil || (float)aWidth != (float)_cachedBackgroundImageWidth
		|| _cachedControlTint != [_SMKBackgroundRenderer currentControlTint]) {
		[self _invalidateImageCache];
		NSRect bounds = NSZeroRect;
		bounds.size.width = aWidth;
		bounds.size.height = [[self backgroundRenderer] segmentHeightForObject:self];
		_backgroundImage = [[NSImage alloc] initWithSize:bounds.size];
		if (aFlipped) {
			[_backgroundImage setFlipped:YES];
		}
		[_backgroundImage lockFocus];
		NSRectFillUsingOperation(bounds, NSCompositingOperationClear);
		[[self backgroundRenderer] drawBackgroundImageForObject:self bounds:bounds style:0];
		[_backgroundImage unlockFocus];
		if (aFlipped) {
			[_backgroundImage setFlipped:NO];
		}
		_cachedBackgroundImageWidth = (int)aWidth;
		_cachedControlTint = (int)[_SMKBackgroundRenderer currentControlTint];
	}
	if (_smkBppFlags.isHighlighted && ([self highlightsBy] & NSPushInCellMask)) {
		if (_highlightedBackground == nil && _backgroundImage != nil) {
			_highlightedBackground = [[_backgroundImage smkHighlightedImage] retain];
		}
		return _highlightedBackground;
	}
	return _backgroundImage;
}

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	id title = nil;
	id image = nil;
	if (_smkBppFlags.hasCustomContent) {
		title = [self displayedTitle];
		image = [self displayedImage];
	}
	unsigned int flags = *(unsigned int *)&_smkBppFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		[aCoder encodeInt32:(int32_t)flags forKey:@"_smkBppFlags"];
		if (title != nil) {
			[aCoder encodeObject:title forKey:@"displayedTitle"];
		}
		if (image != nil) {
			[aCoder encodeObject:image forKey:@"displayedImage"];
		}
		return;
	}
	int version = 2;
	[aCoder encodeValueOfObjCType:"i" at:&version];
	[aCoder encodeValueOfObjCType:"I" at:&flags];
	[aCoder encodeObject:title];
	[aCoder encodeObject:image];
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	[self _smkCellInit];
	id title = nil;
	id image = nil;
	unsigned int flags = *(unsigned int *)&_smkBppFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		if ([aCoder containsValueForKey:@"_smkBppFlags"]) {
			flags = (unsigned int)[aCoder decodeInt32ForKey:@"_smkBppFlags"];
		}
		if ([aCoder containsValueForKey:@"displayedTitle"]) {
			title = [aCoder decodeObjectForKey:@"displayedTitle"];
		}
		if ([aCoder containsValueForKey:@"displayedImage"]) {
			image = [aCoder decodeObjectForKey:@"displayedImage"];
		}
	}
	else {
		int version;
		[aCoder decodeValueOfObjCType:"i" at:&version];
		[aCoder decodeValueOfObjCType:"I" at:&flags];
		if (version > 1) {
			title = [aCoder decodeObject];
			image = [aCoder decodeObject];
		}
	}
	*(unsigned int *)&_smkBppFlags = flags;
	if (_smkBppFlags.hasCustomContent) {
		[self setDisplayedTitle:title != nil ? title : @""];
		[self setDisplayedImage:image];
	}
	return self;
}

- (void)controlTintDidChange:(id)aChange
{
	id view = [self controlView];
	if ([view isKindOfClass:[NSControl class]]) {
		[view updateCell:self];
	}
}

- (id)initWithPopUpButtonCell:(id)aCell
{
	if ([(NSCell *)aCell type] == NSImageCellType) {
		self = [super initImageCell:nil];
	}
	else {
		self = [super initTextCell:@""];
	}
	if (self == nil) {
		return nil;
	}
	[self _acceptPropertiesFromPopUpButtonCell:aCell];
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSControlTintDidChangeNotification object:nil];
	[_backgroundRenderer release];
	_backgroundRenderer = nil;
	[_contentRenderer release];
	_contentRenderer = nil;
	[_arrowImage release];
	_arrowImage = nil;
	[self _invalidateImageCache];
	[super dealloc];
}

// always a pop-up, never a pull-down
- (id)initTextCell:(id)aCell pullsDown:(BOOL)aDown
{
	self = [super initTextCell:aCell pullsDown:NO];
	[self _smkCellInit];
	return self;
}

- (void)_smkCellInit
{
	[super setBezelStyle:NSBezelStyleShadowlessSquare];
	[super setBordered:NO];
	[super setBezeled:NO];
	[self setArrowPosition:NSPopUpNoArrow];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(controlTintDidChange:)
		name:NSControlTintDidChangeNotification object:nil];
}

- (void)_acceptPropertiesFromPopUpButtonCell:(id)aCell
{
	[self setControlSize:[aCell controlSize]];
	[self setMenu:[aCell menu]];
}

- (void)_updateControlView
{
	id view = [self controlView];
	if ([view isKindOfClass:[NSControl class]]) {
		[view updateCell:self];
	}
}

- (void)setShowsStateBy:(NSInteger)aBy
{
	NSInteger old = (char)[self showsStateBy];
	if (old == aBy) {
		return;
	}
	_smkBppFlags.showsStateBy = (unsigned int)aBy;
	if (_smkBppFlags.state == 1 && _smkBppFlags.tracksLikeButton == 1
		&& (([self showsStateBy] & NSChangeBackgroundCellMask) || (old & NSChangeBackgroundCellMask))) {
		[self _invalidateImageCache];
		[self _updateControlView];
	}
}

- (NSInteger)showsStateBy
{
	return _smkBppFlags.showsStateBy;
}

- (void)setHighlightsBy:(NSInteger)aBy
{
	_smkBppFlags.highlightsBy = (unsigned int)aBy;
}

- (NSInteger)highlightsBy
{
	return _smkBppFlags.highlightsBy;
}

- (void)setState:(NSInteger)aState
{
	_smkBppFlags.state = aState == NSControlStateValueOn;
	if (_smkBppFlags.tracksLikeButton && ([self showsStateBy] & NSChangeBackgroundCellMask)) {
		[self _invalidateImageCache];
		[self _updateControlView];
	}
}

- (NSInteger)state
{
	return _smkBppFlags.state;
}

- (void)setTracksLikeButton:(BOOL)aButton
{
	_smkBppFlags.tracksLikeButton = aButton & 1;
	if (!(aButton & 1)) {
		[self setState:NSControlStateValueOff];
	}
	[self _updateControlView];
}

- (BOOL)tracksLikeButton
{
	return _smkBppFlags.tracksLikeButton;
}

- (void)setDisplayedImage:(id)aImage
{
	if (_smkBppFlags.hasCustomContent) {
		[[self contentRenderer] setImage:aImage];
	}
}

- (id)displayedImage
{
	return _smkBppFlags.hasCustomContent ? [_contentRenderer image] : [self image];
}

- (void)setDisplayedTitle:(id)aTitle
{
	if (_smkBppFlags.hasCustomContent) {
		[[self contentRenderer] setTitle:aTitle];
	}
}

- (id)displayedTitle
{
	if (!_smkBppFlags.hasCustomContent) {
		return [self title];
	}
	return _contentRenderer != nil ? [_contentRenderer title] : @"";
}

- (void)setDisplayedImagePosition:(NSInteger)aPosition
{
	NSInteger old = [self displayedImagePosition];
	_smkBppFlags.displayedImagePosition = (unsigned int)aPosition;
	if (old != [self displayedImagePosition]) {
		[self _updateControlView];
	}
}

- (NSInteger)displayedImagePosition
{
	return _smkBppFlags.displayedImagePosition;
}

- (void)setDisplayedAlignment:(NSInteger)aAlignment
{
	NSInteger old = [self displayedAlignment];
	_smkBppFlags.displayedAlignment = (unsigned int)aAlignment;
	if (old != [self displayedAlignment]) {
		[self _updateControlView];
	}
}

- (NSInteger)displayedAlignment
{
	return _smkBppFlags.displayedAlignment;
}

// custom content starts as the current title, without an image
- (void)setHasCustomContent:(BOOL)aContent
{
	BOOL custom = aContent != NO;
	if (_smkBppFlags.hasCustomContent == custom) {
		return;
	}
	NSString *title = [[self displayedTitle] copy];
	_smkBppFlags.hasCustomContent = custom;
	if (custom) {
		[self setDisplayedTitle:title != nil ? title : @""];
		[self setDisplayedImage:nil];
	}
	[title release];
	[self _updateControlView];
}

- (BOOL)hasCustomContent
{
	return _smkBppFlags.hasCustomContent;
}

- (void)setDrawsArrow:(BOOL)aArrow
{
	_smkBppFlags.dontDrawArrow = !aArrow;
	[self _updateControlView];
}

- (BOOL)drawsArrow
{
	return !_smkBppFlags.dontDrawArrow;
}

- (void)setDrawsBackground:(BOOL)aBackground
{
	_smkBppFlags.dontDrawBackground = !aBackground;
	[self _updateControlView];
}

- (BOOL)drawsBackground
{
	return !_smkBppFlags.dontDrawBackground;
}

- (void)setControlSize:(NSControlSize)aSize
{
	[self _invalidateImageCache];
	if (aSize != [self controlSize]) {
		[self setBackgroundRenderer:nil];
	}
	[super setControlSize:aSize];
}

- (void)setEnabled:(BOOL)aEnabled
{
	[self _invalidateImageCache];
	[super setEnabled:aEnabled];
}

- (id)arrowImage
{
	return _smkBppFlags.dontDrawArrow ? nil : [self _arrowImage];
}

- (id)_arrowImage
{
	if (_arrowImage == nil) {
		NSString *path = [[NSBundle bundleForClass:[SMKBackgroundPopUpButtonCell class]] pathForResource:@"pulldown" ofType:@"tiff"];
		_arrowImage = [[NSImage alloc] initWithContentsOfFile:path];
	}
	return _arrowImage;
}

- (void)setBackgroundRenderer:(id)aRenderer
{
	if (_backgroundRenderer != aRenderer) {
		[_backgroundRenderer release];
		_backgroundRenderer = [aRenderer retain];
	}
}

- (id)backgroundRenderer
{
	if (_backgroundRenderer == nil) {
		_backgroundRenderer = [_SMKBackgroundRenderer newBackgroundRendererForControlSize:[self controlSize]];
	}
	return _backgroundRenderer;
}

- (id)contentRenderer
{
	if (_contentRenderer == nil) {
		_contentRenderer = [[NSButtonCell alloc] init];
		[_contentRenderer setBezelStyle:NSBezelStyleShadowlessSquare];
		[_contentRenderer setBordered:NO];
		[_contentRenderer setBezeled:NO];
		[_contentRenderer setButtonType:NSButtonTypeMomentaryChange];
	}
	[self _setupContentRenderer];
	return _contentRenderer;
}

- (void)_setupContentRenderer
{
	NSButtonCell *r = _contentRenderer;
	[r setEnabled:[self isEnabled]];
	[r setControlSize:[self controlSize]];
	[r setFont:[self font]];
	[r setTransparent:[self isTransparent]];
	[r setImageDimsWhenDisabled:[self imageDimsWhenDisabled]];
	NSInteger position = [self displayedImagePosition];
	NSInteger alignment = [self displayedAlignment];
	NSImage *image = [self displayedImage];
	NSString *title = [self displayedTitle];
	if (image != [r image]) {
		[r setImage:image];
	}
	[r setTitle:title != nil ? title : @""];
	[r setImagePosition:position];
	[r setAlignment:alignment];
	[r setHighlighted:_smkBppFlags.isHighlighted];
	[r setHighlightsBy:[self highlightsBy] & ~(NSChangeGrayCellMask | NSChangeBackgroundCellMask)];
	if (_smkBppFlags.tracksLikeButton) {
		[r setShowsStateBy:[self showsStateBy] & ~(NSChangeGrayCellMask | NSChangeBackgroundCellMask)];
		[r setState:[self state]];
	}
}

- (void)_invalidateImageCache
{
	[_backgroundImage release];
	_backgroundImage = nil;
	[_highlightedBackground release];
	_highlightedBackground = nil;
}

@end
