#import "SMKBackgroundButtonCell.h"

// A borderless button cell drawn over a background from a renderer (left
// cap, fill, right cap images per state), with a plain button cell drawing
// its title and image inside the caps. The background is cached per width
// and control tint.
@implementation SMKBackgroundButtonCell

- (BOOL)accessibilityIsIgnored
{
	return NO;
}

- (id)accessibilityHitTest:(NSPoint)aTest
{
	return self;
}

// the cell's own frame, in screen coordinates
- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityPositionAttribute]) {
		NSView *view = [self controlView];
		NSPoint p = [view convertPoint:_frameRect.origin toView:nil];
		p = [[view window] convertPointToScreen:p];
		return [NSValue valueWithPoint:NSMakePoint(p.x, p.y - _frameRect.size.height)];
	}
	if ([aValue isEqualToString:NSAccessibilitySizeAttribute]) {
		return [NSValue valueWithSize:_frameRect.size];
	}
	return [super accessibilityAttributeValue:aValue];
}

- (void)setControlSize:(NSControlSize)aSize
{
	[self _invalidateImageCache];
	if (aSize != [self controlSize]) {
		[self setBackgroundRenderer:nil];
	}
	[super setControlSize:aSize];
}

- (NSInteger)displayedImagePosition
{
	return [self imagePosition];
}

- (void)setEnabled:(BOOL)aEnabled
{
	[self _invalidateImageCache];
	[super setEnabled:aEnabled];
}

- (BOOL)isEnabled
{
	return [super isEnabled];
}

- (void)setHighlighted:(BOOL)aHighlighted
{
	if ([self highlightsBy] & NSChangeBackgroundCellMask) {
		[self _invalidateImageCache];
	}
	[super setHighlighted:aHighlighted];
}

- (void)setState:(NSInteger)aState
{
	if ([self showsStateBy] & NSChangeBackgroundCellMask) {
		[self _invalidateImageCache];
	}
	[super setState:aState];
}

// on, or pressed, where that changes the background
- (BOOL)isSelected
{
	if ([self state] == NSControlStateValueOn && ([self showsStateBy] & NSChangeBackgroundCellMask)) {
		return YES;
	}
	return [self isHighlighted] && ([self highlightsBy] & NSChangeBackgroundCellMask);
}

- (BOOL)isRightCap
{
	return YES;
}

- (BOOL)isLeftCap
{
	return YES;
}

// the pressed background changes too often to cache
- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp
{
	if ([self highlightsBy] & NSChangeBackgroundCellMask) {
		_smkBCFlags.dontCacheImages = 1;
	}
	BOOL result = [super trackMouse:aMouse inRect:aRect ofView:aView untilMouseUp:aUp];
	_smkBCFlags.dontCacheImages = 0;
	return result;
}

- (void)performClick:(id)aClick
{
	if ([self highlightsBy] & NSChangeBackgroundCellMask) {
		_smkBCFlags.dontCacheImages = 1;
	}
	[super performClick:aClick];
	[[self controlView] setKeyboardFocusRingNeedsDisplayInRect:_frameRect];
	_smkBCFlags.dontCacheImages = 0;
}

- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	_frameRect = aFrame;
	NSImage *background = [self _backgroundImageForWidth:aFrame.size.width flipped:[aView isFlipped]];
	NSRect from = NSZeroRect;
	from.size = [background size];
	NSRect to = aFrame;
	to.size = from.size;
	BOOL ring = [self showsFirstResponder];
	if (ring) {
		[NSGraphicsContext saveGraphicsState];
		NSSetFocusRingStyle(NSFocusRingAbove);
	}
	[background drawInRect:to fromRect:from operation:NSCompositingOperationSourceOver fraction:1.0];
	if (ring) {
		[NSGraphicsContext restoreGraphicsState];
	}
	struct _SMKInset inset = [self contentInset];
	if ([aView isFlipped]) {
		float top = inset.top;
		inset.top = inset.bottom;
		inset.bottom = top;
	}
	NSRect content = to;
	content.origin.x += inset.left;
	content.size.width = MAX(0.0, content.size.width - (inset.left + inset.right));
	content.origin.y += inset.bottom;
	content.size.height = MAX(0.0, content.size.height - (inset.bottom + inset.top));
	if (!NSIsEmptyRect(content)) {
		[[self contentRenderer] drawInteriorWithFrame:content inView:aView];
	}
}

// the content's width plus the caps, and the renderer's height
- (NSSize)cellSizeForBounds:(NSRect)aBounds
{
	struct _SMKInset inset = [self contentInset];
	NSRect r = aBounds;
	r.origin.x += inset.left;
	r.size.width = MAX(0.0, r.size.width - (inset.left + inset.right));
	r.origin.y += inset.bottom;
	r.size.height = MAX(0.0, r.size.height - (inset.bottom + inset.top));
	NSSize size = [self contentSizeForBounds:r];
	float width = MAX(0.0f, (float)size.width + inset.left + inset.right);
	return NSMakeSize(width, [[self backgroundRenderer] segmentHeightForObject:self]);
}

// image only buttons have no insets
- (struct _SMKInset)contentInset
{
	if ([self displayedImagePosition] == NSImageOnly) {
		return SMKZeroInset;
	}
	return [[self backgroundRenderer] contentInsetForObject:self];
}

- (NSSize)contentSizeForBounds:(NSRect)aBounds
{
	return [[self contentRenderer] cellSizeForBounds:aBounds];
}

- (id)_backgroundImageForWidth:(CGFloat)aWidth flipped:(BOOL)aFlipped
{
	NSImage *uncached = nil;
	if (_backgroundImage == nil || (float)aWidth != (float)_cachedBackgroundImageWidth || _smkBCFlags.dontCacheImages
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
		if (_smkBCFlags.dontCacheImages) {
			uncached = [_backgroundImage autorelease];
			_backgroundImage = nil;
		}
	}
	if ([self isHighlighted] && ([self highlightsBy] & NSPushInCellMask)) {
		if (_smkBCFlags.dontCacheImages) {
			return [uncached smkHighlightedImage];
		}
		if (_highlightedBackground == nil && _backgroundImage != nil) {
			_highlightedBackground = [[_backgroundImage smkHighlightedImage] retain];
		}
		return _highlightedBackground;
	}
	return uncached != nil ? uncached : _backgroundImage;
}

- (void)setBackgroundRenderer:(id)aRenderer
{
	if (_backgroundRenderer != aRenderer) {
		[_backgroundRenderer release];
		_backgroundRenderer = [aRenderer retain];
		[self _invalidateImageCache];
	}
}

- (id)backgroundRenderer
{
	if (_backgroundRenderer == nil) {
		_backgroundRenderer = [_SMKBackgroundRenderer newBackgroundRendererForControlSize:[self controlSize]];
	}
	return _backgroundRenderer;
}

- (id)initWithButtonCell:(id)aCell
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
	[self _acceptPropertiesFromButtonCell:aCell];
	return self;
}

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	unsigned int flags = *(unsigned int *)&_smkBCFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		[aCoder encodeInt32:(int32_t)flags forKey:@"_smkBCFlags"];
	}
	else {
		[aCoder encodeValueOfObjCType:"I" at:&flags];
	}
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	[self _smkCellInit];
	unsigned int flags = *(unsigned int *)&_smkBCFlags;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		if ([aCoder containsValueForKey:@"_smkBCFlags"]) {
			flags = (unsigned int)[aCoder decodeInt32ForKey:@"_smkBCFlags"];
		}
	}
	else {
		[aCoder decodeValueOfObjCType:"I" at:&flags];
	}
	*(unsigned int *)&_smkBCFlags = flags;
	return self;
}

- (void)controlTintDidChange:(id)aChange
{
	id view = [self controlView];
	if ([view isKindOfClass:[NSControl class]]) {
		[view updateCell:self];
	}
}

// NSCell copies bitwise; take references to the copied objects
- (void)_finishCopying
{
	[_backgroundRenderer retain];
	[_contentRenderer retain];
	[_backgroundImage retain];
	[_highlightedBackground retain];
}

- (void)_setContentRenderer:(id)aRenderer
{
	if (_contentRenderer != aRenderer) {
		[_contentRenderer release];
		_contentRenderer = [aRenderer retain];
	}
}

- (id)copyWithZone:(NSZone *)aZone
{
	SMKBackgroundButtonCell *copy = [super copyWithZone:aZone];
	[copy _finishCopying];
	return copy;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSControlTintDidChangeNotification object:nil];
	[_backgroundRenderer release];
	_backgroundRenderer = nil;
	[_contentRenderer release];
	_contentRenderer = nil;
	[self _invalidateImageCache];
	[super dealloc];
}

- (id)initImageCell:(id)aCell
{
	self = [super initImageCell:aCell];
	[self _smkCellInit];
	return self;
}

- (id)initTextCell:(id)aCell
{
	self = [super initTextCell:aCell];
	[self _smkCellInit];
	return self;
}

- (void)_smkCellInit
{
	[super setBezelStyle:NSBezelStyleShadowlessSquare];
	[super setBordered:NO];
	[super setBezeled:NO];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(controlTintDidChange:)
		name:NSControlTintDidChangeNotification object:nil];
}

// copies everything a button cell has
- (void)_acceptPropertiesFromButtonCell:(id)aCell
{
	NSButtonCell *cell = aCell;
	[self setState:[cell state]];
	[self setTarget:[cell target]];
	[self setAction:[cell action]];
	[self setTag:[cell tag]];
	NSString *title = [cell title];
	[self setTitle:title];
	[self setEnabled:[cell isEnabled]];
	[self setContinuous:[cell isContinuous]];
	[self setEditable:[cell isEditable]];
	[self setSelectable:[cell isSelectable]];
	[self setBordered:NO];
	[self setBezeled:NO];
	[self setScrollable:[cell isScrollable]];
	[self setHighlighted:[cell isHighlighted]];
	[self setAlignment:[cell alignment]];
	[self setWraps:[cell wraps]];
	[self setFont:[cell font]];
	// entryType and mnemonicLocation are gone from AppKit
	[self setFormatter:[cell formatter]];
	id value = [cell objectValue];
	if (value != title) {
		[self setObjectValue:value];
	}
	value = [cell stringValue];
	if (value != title) {
		[self setStringValue:value];
	}
	[self setImage:[cell image]];
	[self setControlTint:[cell controlTint]];
	[self setControlSize:[cell controlSize]];
	[self setRepresentedObject:[cell representedObject]];
	[self setMenu:[cell menu]];
	[self setSendsActionOnEndEditing:[cell sendsActionOnEndEditing]];
	[self setRefusesFirstResponder:[cell refusesFirstResponder]];
	[self setShowsFirstResponder:[cell showsFirstResponder]];
	if ([cell attributedStringValue] != nil) {
		[self setAttributedStringValue:[cell attributedStringValue]];
	}
	[self setAllowsEditingTextAttributes:[cell allowsEditingTextAttributes]];
	[self setImportsGraphics:[cell importsGraphics]];
	[self setAllowsMixedState:[cell allowsMixedState]];
	if ([self respondsToSelector:@selector(setFocusRingType:)] && [cell respondsToSelector:@selector(focusRingType)]) {
		[self setFocusRingType:[cell focusRingType]];
	}
	[self setAlternateTitle:[cell alternateTitle]];
	[self setAlternateImage:[cell alternateImage]];
	[self setImagePosition:[cell imagePosition]];
	[self setHighlightsBy:[cell highlightsBy]];
	[self setShowsStateBy:[cell showsStateBy]];
	[self setTransparent:[cell isTransparent]];
	float delay, interval;
	[cell getPeriodicDelay:&delay interval:&interval];
	[self setPeriodicDelay:delay interval:interval];
	[self setKeyEquivalent:[cell keyEquivalent]];
	[self setKeyEquivalentModifierMask:[cell keyEquivalentModifierMask]];
	[self setKeyEquivalentFont:[cell keyEquivalentFont]];
	[self setGradientType:[cell gradientType]];
	[self setImageDimsWhenDisabled:[cell imageDimsWhenDisabled]];
	[self setShowsBorderOnlyWhileMouseInside:[cell showsBorderOnlyWhileMouseInside]];
	if ([cell attributedTitle] != nil) {
		[self setAttributedTitle:[cell attributedTitle]];
	}
	if ([cell attributedAlternateTitle] != nil) {
		[self setAttributedAlternateTitle:[cell attributedAlternateTitle]];
	}
	[self setBezelStyle:[cell bezelStyle]];
	[self setSound:[cell sound]];
}

- (void)setBordered:(BOOL)aBordered
{
	[super setBordered:NO];
}

- (void)setBezeled:(BOOL)aBezeled
{
	[super setBezeled:NO];
}

// the plain cell that draws the title and image
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

// the content cell gets everything but the background changes
- (void)_setupContentRenderer
{
	NSButtonCell *r = _contentRenderer;
	[r setAllowsMixedState:[self allowsMixedState]];
	[r setState:[self state]];
	[r setTitle:[self title]];
	[r setEnabled:[self isEnabled]];
	[r setHighlighted:[self isHighlighted]];
	[r setAlignment:[self alignment]];
	[r setWraps:[self wraps]];
	[r setImage:[self image]];
	[r setControlSize:[self controlSize]];
	[r setFont:[self font]];
	[r setAlternateTitle:[self alternateTitle]];
	[r setAlternateImage:[self alternateImage]];
	[r setTransparent:[self isTransparent]];
	[r setImageDimsWhenDisabled:[self imageDimsWhenDisabled]];
	if ([self attributedTitle] != nil) {
		[r setAttributedTitle:[self attributedTitle]];
	}
	if ([self attributedAlternateTitle] != nil) {
		[r setAttributedAlternateTitle:[self attributedAlternateTitle]];
	}
	[r setImagePosition:[self imagePosition]];
	[r setHighlightsBy:[self highlightsBy] & ~(NSChangeGrayCellMask | NSChangeBackgroundCellMask)];
	[r setShowsStateBy:[self showsStateBy] & ~(NSChangeGrayCellMask | NSChangeBackgroundCellMask)];
}

- (void)_invalidateImageCache
{
	[_backgroundImage release];
	_backgroundImage = nil;
	[_highlightedBackground release];
	_highlightedBackground = nil;
}

@end
