#import "SMKSegmentedCell.h"

// One segment of an SMKSegmentedControl: a background button cell whose
// caps depend on its place, with a width that is either set or the content's
// minimum, and an optional menu.
@implementation SMKSegmentedCell

+ (BOOL)prefersTrackingUntilMouseUp
{
	return YES;
}

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	unsigned int flags = *(unsigned int *)&_smkSegFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		[aCoder encodeInt32:(int32_t)flags forKey:@"_smkSegFlags"];
		[aCoder encodeFloat:_segmentWidth forKey:@"segmentWidth"];
		if (_toolTip != nil) {
			[aCoder encodeObject:_toolTip forKey:@"smkSegToolTip"];
		}
		return;
	}
	unsigned int version = 2;
	[aCoder encodeValueOfObjCType:"I" at:&version];
	[aCoder encodeValueOfObjCType:"I" at:&flags];
	[aCoder encodeValueOfObjCType:"f" at:&_segmentWidth];
	[aCoder encodeObject:_toolTip];
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	id toolTip = nil;
	unsigned int flags = *(unsigned int *)&_smkSegFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		if ([aCoder containsValueForKey:@"_smkSegFlags"]) {
			flags = (unsigned int)[aCoder decodeInt32ForKey:@"_smkSegFlags"];
		}
		if ([aCoder containsValueForKey:@"segmentWidth"]) {
			_segmentWidth = [aCoder decodeFloatForKey:@"segmentWidth"];
		}
		if ([aCoder containsValueForKey:@"smkSegToolTip"]) {
			toolTip = [aCoder decodeObjectForKey:@"smkSegToolTip"];
		}
	}
	else {
		unsigned int version;
		[aCoder decodeValueOfObjCType:"I" at:&version];
		[aCoder decodeValueOfObjCType:"I" at:&flags];
		[aCoder decodeValueOfObjCType:"f" at:&_segmentWidth];
		if (version > 1) {
			toolTip = [aCoder decodeObject];
		}
	}
	*(unsigned int *)&_smkSegFlags = flags;
	[self setToolTip:toolTip];
	_smkSegFlags.frameValid = 0;
	_smkSegFlags.minWidthValid = 0;
	[self _setSegmentedControl:_segmentedControl];
	return self;
}

// the segments' order in a common segmented control
- (NSInteger)compare:(id)aCompare
{
	int result = -1;
	SMKSegmentedControl *control = [self segmentedControlView];
	if (control != nil && [aCompare isKindOfClass:[SMKSegmentedCell class]] && [aCompare segmentedControlView] == control) {
		[control _compareCell:self toCell:aCompare result:&result];
	}
	return result;
}

- (void)setTracksPopUpMenuOnMouseDown:(BOOL)aDown
{
	_smkSegFlags.tracksPopUpMenuOnMouseDown = aDown != NO;
}

- (BOOL)tracksPopUpMenuOnMouseDown
{
	return _smkSegFlags.tracksPopUpMenuOnMouseDown;
}

- (void)setToolTip:(id)aTip
{
	if (_toolTip != aTip) {
		[_toolTip release];
		_toolTip = [aTip copy];
		[[self _smkSegmentedControl] _toolTipChangedForCell:self];
	}
}

- (id)toolTip
{
	return _toolTip;
}

// As compiled, a zero width is the one that is kept as custom and any other
// width goes back to the automatic one.
- (void)setSegmentWidth:(CGFloat)aWidth
{
	unsigned int wasCustom = _smkSegFlags.customWidth;
	BOOL zero = (float)aWidth == 0.0f;
	_smkSegFlags.customWidth = zero;
	if (zero) {
		_segmentWidth = (float)aWidth;
		if (wasCustom == (unsigned int)zero && !wasCustom) {
			return;
		}
	}
	else {
		if (!wasCustom) {
			return;
		}
		_smkSegFlags.widthValid = 0;
	}
	[self _segmentWidthAndFrameChanged];
}

- (CGFloat)segmentWidth
{
	return _smkSegFlags.customWidth ? _segmentWidth : 0.0;
}

- (BOOL)isRightCap
{
	return _smkSegFlags.rightCap;
}

- (BOOL)isLeftCap
{
	return _smkSegFlags.leftCap;
}

// a click elsewhere in the control highlights the clicked segment
- (void)highlight:(BOOL)aHighlight withFrame:(NSRect)aFrame inView:(id)aView
{
	SMKSegmentedControl *control = [self segmentedControlView:aView];
	id clicked = [control clickedCell];
	if (control == nil || clicked == self) {
		[super highlight:aHighlight withFrame:aFrame inView:aView];
		return;
	}
	[clicked highlight:aHighlight withFrame:[control cellFrameForCell:clicked] inView:aView];
}

- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp
{
	SMKSegmentedControl *control = [self segmentedControlView:aView];
	id clicked = [control clickedCell];
	if (control != nil && clicked != self) {
		return [clicked trackMouse:aMouse inRect:[control cellFrameForCell:clicked] ofView:aView untilMouseUp:aUp];
	}
	if ([self menu] != nil) {
		if ([aMouse modifierFlags] & NSEventModifierFlagControl) {
			[self trackMenuWithEvent:aMouse inRect:aRect ofView:aView];
			return YES;
		}
		if (_smkSegFlags.tracksPopUpMenuOnMouseDown) {
			// holding the mouse still in the segment shows the menu
			float delay, interval;
			[self getPeriodicDelay:&delay interval:&interval];
			[NSEvent startPeriodicEventsAfterDelay:delay withPeriod:interval];
			NSDate *limit = [NSDate dateWithTimeIntervalSinceReferenceDate:[NSDate timeIntervalSinceReferenceDate] + delay];
			BOOL inside = [aView mouse:[aView convertPoint:[aMouse locationInWindow] fromView:nil] inRect:aRect];
			BOOL show = NO;
			for (;;) {
				NSEvent *event = [[aView window] nextEventMatchingMask:NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged | NSEventMaskPeriodic
					untilDate:limit inMode:NSEventTrackingRunLoopMode dequeue:NO];
				if (event == nil) {
					break;
				}
				if ([event type] == NSEventTypePeriodic) {
					show = inside;
					break;
				}
				inside = [aView mouse:[aView convertPoint:[event locationInWindow] fromView:nil] inRect:aRect];
				if ([event type] != NSEventTypeLeftMouseDragged) {
					break;
				}
				[[aView window] nextEventMatchingMask:NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged | NSEventMaskPeriodic
					untilDate:limit inMode:NSEventTrackingRunLoopMode dequeue:YES];
			}
			[NSEvent stopPeriodicEvents];
			if (show) {
				[self trackMenuWithEvent:aMouse inRect:aRect ofView:aView];
				return YES;
			}
		}
	}
	return [super trackMouse:aMouse inRect:aRect ofView:aView untilMouseUp:aUp];
}

// a set width plus the caps, and the renderer's height
- (NSSize)cellSizeForBounds:(NSRect)aBounds
{
	if (!_smkSegFlags.widthValid && !_smkSegFlags.customWidth) {
		return [super cellSizeForBounds:aBounds];
	}
	id renderer = [self backgroundRenderer];
	struct _SMKInset inset = [self contentInset];
	float width = MAX(0.0f, (float)[self _segmentWidth] + inset.left + inset.right);
	return NSMakeSize(width, [renderer segmentHeightForObject:self]);
}

- (void)setImagePosition:(NSCellImagePosition)aPosition
{
	if ((NSUInteger)aPosition > NSImageAbove) {
		aPosition = NSImageRight;
	}
	if (aPosition != [self imagePosition]) {
		[self _minSegmentWidthCouldChange];
		[super setImagePosition:aPosition];
	}
}

- (void)setImage:(id)aImage
{
	[self _minSegmentWidthCouldChange];
	[super setImage:aImage];
}

- (void)setStringValue:(id)aValue
{
	[self _minSegmentWidthCouldChange];
	[super setStringValue:aValue];
}

- (void)setFont:(id)aFont
{
	[self _minSegmentWidthCouldChange];
	[super setFont:aFont];
}

- (void)setAlignment:(NSTextAlignment)aAlignment
{
	[self _minSegmentWidthCouldChange];
	[super setAlignment:aAlignment];
}

- (void)setTitle:(id)aTitle
{
	[self _minSegmentWidthCouldChange];
	[super setTitle:aTitle];
}

// without an action of its own, the control's
- (SEL)action
{
	SEL action = [super action];
	if (action != NULL) {
		return action;
	}
	NSControl *control = [self controlViewControl];
	if (control != nil && [control cell] != self) {
		return [control action];
	}
	return NULL;
}

// without an action of its own, the control's target
- (id)target
{
	if ([super action] == NULL) {
		NSControl *control = [self controlViewControl];
		if (control != nil && [control cell] != self) {
			return [control target];
		}
	}
	return [super target];
}

- (void)setState:(NSInteger)aState
{
	if (aState == [self state]) {
		return;
	}
	SMKSegmentedControl *control = [self _smkSegmentedControl];
	if (control != nil && ![control _shouldSetNewState:aState forCell:self]) {
		return;
	}
	[self _setState:aState];
}

- (void)dealloc
{
	_segmentedControl = nil;
	[super dealloc];
}

- (id)initImageCell:(id)aCell
{
	self = [super initImageCell:aCell];
	[self setButtonType:NSButtonTypePushOnPushOff];
	[self _setLeftCap:NO];
	[self _setRightCap:NO];
	return self;
}

- (id)initTextCell:(id)aCell
{
	self = [super initTextCell:aCell];
	[self setButtonType:NSButtonTypePushOnPushOff];
	[self _setLeftCap:NO];
	[self _setRightCap:NO];
	return self;
}

@end

@implementation SMKSegmentedCell (SMKSegmentedControlPrivateGeometry)

- (void)_segmentFrameChanged
{
	[[self _smkSegmentedControl] _cellContentWillChange:self];
}

- (void)_segmentWidthAndFrameChanged
{
	_smkSegFlags.minWidthValid = 0;
	[[self _smkSegmentedControl] _cellContentWillChange:self];
}

- (void)_minSegmentWidthCouldChange
{
	_smkSegFlags.minWidthValid = 0;
	[[self _smkSegmentedControl] _cellContentWillChange:self];
}

@end

@implementation SMKSegmentedCell (SMKSegmentedCell_Private)

+ (id)trackingPopUpButtonCell
{
	return [[NSPopUpButtonCell alloc] initTextCell:@" " pullsDown:YES];
}

// The menu pops up from a pull-down cell over the segment; its first item
// is the pull-down's title.
- (BOOL)showMenuWithEvent:(id)aEvent inRect:(NSRect)aRect ofView:(id)aView
{
	NSMenu *menu = [self menu];
	if (menu == nil) {
		return NO;
	}
	SMKSegmentedControl *control = [self segmentedControlView:aView];
	if ([control trackingMode] != NSSegmentSwitchTrackingMomentary) {
		[control setSelected:YES forSegment:[control segmentForCell:self]];
	}
	NSPopUpButtonCell *cell = [[self class] trackingPopUpButtonCell];
	[cell setMenu:menu];
	[cell insertItemWithTitle:@" " atIndex:0];
	[cell setFont:[self font]];
	[cell performClickWithFrame:NSInsetRect(aRect, -3.0, 0.0) inView:aView];
	[cell removeItemAtIndex:0];
	[cell setMenu:nil];
	[cell release];
	return YES;
}

- (BOOL)trackMenuWithEvent:(id)aEvent inRect:(NSRect)aRect ofView:(id)aView
{
	if ([self menu] == nil) {
		return NO;
	}
	[self setHighlighted:YES];
	BOOL result = [self showMenuWithEvent:aEvent inRect:aRect ofView:aView];
	[self setHighlighted:NO];
	return result;
}

- (void)_setAutosizeRatio:(CGFloat)aRatio
{
	_autosizeRatio = (float)aRatio;
}

// only segments without a custom width autosize
- (BOOL)_getAutosizeRatio:(float *)aRatio
{
	BOOL autosizes = !_smkSegFlags.customWidth;
	if (autosizes) {
		*aRatio = _autosizeRatio;
	}
	return autosizes;
}

- (void)_invalidateFrameInSegmentedControl
{
	_smkSegFlags.frameValid = 0;
}

- (void)_setFrameInSegmentedControl:(NSRect)aControl
{
	_frameInSegmentedControl = aControl;
	_smkSegFlags.frameValid = 1;
}

- (BOOL)_frameIsValid
{
	return _smkSegFlags.frameValid;
}

- (BOOL)_getFrameInSegmentedControl:(NSRect *)aControl
{
	BOOL valid = _smkSegFlags.frameValid != 0;
	if (valid) {
		*aControl = _frameInSegmentedControl;
	}
	return valid;
}

- (void)_setSegmentedControl:(id)aControl
{
	_smkSegFlags.isPartOfSmkSegmentedControl = 0;
	_segmentedControl = aControl;
	if (aControl != nil && [aControl isKindOfClass:[SMKSegmentedControl class]]) {
		_smkSegFlags.isPartOfSmkSegmentedControl = 1;
	}
}

- (id)_segmentedControl
{
	return _segmentedControl;
}

- (BOOL)_hasCustomSegmentWidth
{
	return _smkSegFlags.customWidth;
}

- (CGFloat)_segmentWidth
{
	if (!_smkSegFlags.widthValid && !_smkSegFlags.customWidth) {
		[self _setSegmentWidth:[self _minSegmentWidth]];
	}
	return _segmentWidth;
}

- (CGFloat)_minSegmentWidth
{
	return [self contentSizeForBounds:NSMakeRect(0.0, 0.0, 10000.0, 10000.0)].width;
}

- (NSSize)_cellSizeAtMinimumWidth
{
	if (_smkSegFlags.customWidth) {
		return [self cellSize];
	}
	float width = _segmentWidth;
	_segmentWidth = (float)[self _minSegmentWidth];
	_smkSegFlags.customWidth = 1;
	NSSize size = [self cellSize];
	_segmentWidth = width;
	_smkSegFlags.customWidth = 0;
	return size;
}

- (void)_resetSegmentWidth
{
	if (!_smkSegFlags.customWidth) {
		_smkSegFlags.widthValid = 0;
		[self _segmentFrameChanged];
	}
}

- (void)_setSegmentWidth:(CGFloat)aWidth
{
	_segmentWidth = (float)aWidth;
	_smkSegFlags.widthValid = 1;
}

- (BOOL)_allowsChangeState
{
	SMKSegmentedControl *control = [self segmentedControlView];
	return control == nil || [control _allowsChangeStateForCell:self];
}

// sets the state without asking the control
- (void)_setState:(NSInteger)aState
{
	[super setState:aState];
}

- (void)_setRightCap:(BOOL)aCap
{
	BOOL old = _smkSegFlags.rightCap;
	_smkSegFlags.rightCap = aCap;
	if (old != aCap) {
		[self _invalidateImageCache];
		[self _segmentWidthAndFrameChanged];
	}
}

// the original compared after assigning, so a left cap change never
// redrew or resized the segment
- (void)_setLeftCap:(BOOL)aCap
{
	BOOL old = _smkSegFlags.leftCap;
	_smkSegFlags.leftCap = aCap;
	if (old != aCap) {
		[self _invalidateImageCache];
		[self _segmentWidthAndFrameChanged];
	}
}

@end

@implementation SMKSegmentedCell (SMKControlViewAccessors)

- (id)_smkSegmentedControl
{
	return _smkSegFlags.isPartOfSmkSegmentedControl ? _segmentedControl : nil;
}

- (id)controlViewControl
{
	return [[self controlView] isKindOfClass:[NSControl class]] ? [self controlView] : nil;
}

- (id)segmentedControlView:(id)aView
{
	return [aView isKindOfClass:[SMKSegmentedControl class]] ? aView : nil;
}

- (id)segmentedControlView
{
	return [self segmentedControlView:[self controlView]];
}

@end
