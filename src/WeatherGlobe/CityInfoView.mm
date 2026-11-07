#import "CityInfoView.h"
#import "CityInfoController.h"
#import "AtlasController.h"
#import "MainWindowController.h"

// One drag at a time: the dragged view's frame in its superview (it is the
// content view of a borderless window while dragged), the superview it came
// from and the drag image's last screen location.
static BOOL _draggingInProgress = NO;
static BOOL _draggingAborted = NO;
static BOOL _needQuickDraw = NO;
static id _selectedView = nil;
static NSRect _draggedFrame;
static NSView *_originalSuperView = nil;
static NSPoint _draggedImageLocation;
static CityInfoSelectionView *_sCityInfoSelectionView = nil;

// A city's box in the city info palette. A click selects the city; a
// double click opens its details; holding the mouse tears the box off to
// reorder the cities, or drops it outside the palette to remove the city.
@implementation CityInfoView

+ (BOOL)isDraggingInProgress
{
	return _draggingInProgress;
}

+ (id)_selectionView
{
	if (!_sCityInfoSelectionView) {
		_sCityInfoSelectionView = [CityInfoSelectionView new];
		[_sCityInfoSelectionView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	}
	return _sCityInfoSelectionView;
}

+ (id)selectedView
{
	return _selectedView;
}

+ (void)setSelectedView:(id)aView
{
	NSView *selection = [self _selectionView];
	[selection removeFromSuperview];
	_selectedView = aView;
	[selection setFrame:[[aView contentView] bounds]];
	[selection setMenu:[_selectedView menu]];
	[_selectedView addSubview:selection];
}

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self) {
		[self setBorderType:NSNoBorder];
		[self setContentViewMargins:NSMakeSize(0.0, -1.0)];
		NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
		[center addObserver:self selector:@selector(_acceptPrepareForDragging:)
			name:@"CityInfoStartDraggingNotification" object:nil];
		[center addObserver:self selector:@selector(_acceptFinishDragging:)
			name:@"CityInfoStopDraggingNotification" object:nil];
	}
	return self;
}

- (void)dealloc
{
	if (_selectedView == self)
		[[self class] setSelectedView:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_image release];
	[_content release];
	[_draggedWindow release];
	[super dealloc];
}

- (BOOL)mouseDownCanMoveWindow
{
	return NO;
}

// While a drag runs, the boxes draw their cached images (their contents
// are taken out).
- (void)drawRect:(NSRect)aRect
{
	if (_isDragged)
		return;
	if (_needQuickDraw)
		[_image drawInRect:aRect fromRect:aRect operation:NSCompositingOperationCopy fraction:1.0];
	else
		_needCreateImage = YES;
}

- (NSRect)frame
{
	if (_isDragged)
		return _draggedFrame;
	return [super frame];
}

- (void)setFrame:(NSRect)aFrame
{
	if (_isDragged) {
		_draggedFrame = aFrame;
		return;
	}
	[self setFrameOrigin:aFrame.origin];
	[self setFrameSize:aFrame.size];
}

- (void)setFrameSize:(NSSize)aSize
{
	if (NSEqualSizes([self frame].size, aSize))
		return;
	[super setFrameSize:aSize];
	[[self class] setSelectedView:[[self class] selectedView]];
	if ([self postsFrameChangedNotifications])
		[[NSNotificationCenter defaultCenter] postNotificationName:_kCIViewSizeDidChangeNotification object:self
			userInfo:nil];
}

- (id)owner
{
	return _owner;
}

- (void)setOwner:(id)aOwner
{
	_owner = aOwner;
}

- (void)_poofAnimationFinished
{
	_draggingInProgress = NO;
}

// Moves the view into a borderless window of its own (placed where the
// drag image goes) and leaves its old place to be redrawn empty.
- (void)_createDraggedImage
{
	NSColor *background = [[self window] backgroundColor];
	_originalSuperView = [self superview];
	_draggedFrame = [self frame];
	[self retain];
	[self removeFromSuperview];
	_needTearOffImage = YES;
	_draggedWindow = [[NSWindow alloc] initWithContentRect:_draggedFrame styleMask:NSWindowStyleMaskBorderless
		backing:NSBackingStoreBuffered defer:NO];
	[_draggedWindow setLevel:CGWindowLevelForKey(kCGStatusWindowLevelKey)];
	[_draggedWindow setOpaque:NO];
	[_draggedWindow setBackgroundColor:background];
	[_draggedWindow setContentView:self];
	[[[self class] _selectionView] removeFromSuperview];
	[self displayIfNeeded];
	[_selectedView addSubview:[[self class] _selectionView]];
	_isDragged = YES;
	[self _createImage];
}

// At 2000 points a second.
- (void)_animateDraggedWindowTo:(NSPoint)aTo
{
	NSDate *start = [NSDate date];
	NSRect frame = [_draggedWindow frame];
	float dx = fabsf((float)(aTo.x - frame.origin.x));
	float dy = fabsf((float)(aTo.y - frame.origin.y));
	double duration = (dx > dy ? dx : dy) / 2000.0f;
	for (double elapsed = 0.0; elapsed < duration; elapsed = -[start timeIntervalSinceNow]) {
		float t = elapsed / duration;
		[_draggedWindow setFrameOrigin:NSMakePoint(t * (aTo.x - frame.origin.x) + frame.origin.x,
			(aTo.y - frame.origin.y) * t + frame.origin.y)];
	}
}

// While boxes are reordered by a drag, they slide to their places in 0.05 s.
- (void)setFrameAnimated:(NSRect)aAnimated
{
	if (!_draggingInProgress) {
		[self setFrame:aAnimated];
		return;
	}
	NSDate *start = [NSDate date];
	[_originalSuperView addSubview:self];
	NSRect from = [self frame];
	double elapsed = 0.0;
	do {
		float t = elapsed / 0.05;
		NSRect to = NSMakeRect((aAnimated.origin.x - from.origin.x) * t + from.origin.x,
			t * (aAnimated.origin.y - from.origin.y) + from.origin.y, from.size.width, from.size.height);
		NSRect old = [self frame];
		[self setFrameOrigin:to.origin];
		[_originalSuperView displayRect:NSIntegralRect(NSUnionRect(old, to))];
		elapsed = -[start timeIntervalSinceNow];
	} while (elapsed < 0.05);
	NSRect old = [self frame];
	[self setFrameOrigin:aAnimated.origin];
	[_originalSuperView displayRect:NSIntegralRect(NSUnionRect(old, aAnimated))];
	[[_originalSuperView window] invalidateShadow];
}

- (void)createImageIfNeeded
{
	if (_needCreateImage)
		[self _createImage];
}

- (void)tearOffImageIfNeeded
{
	if (_needTearOffImage) {
		[_originalSuperView displayRect:_draggedFrame];
		_needTearOffImage = NO;
	}
}

- (void)showDraggedImageIfCan
{
	if (_needQuickDraw) {
		[_draggedWindow setFrameOrigin:_draggedImageLocation];
		[_draggedWindow orderFront:self];
	}
}

- (NSRect)opaqueBounds
{
	return [self bounds];
}

- (void)abortDragging
{
	_draggingAborted = YES;
}

- (BOOL)isDraggingAborted
{
	return _draggingInProgress && _draggingAborted;
}

- (NSUInteger)draggingSourceOperationMaskForLocal:(BOOL)aLocal
{
	return aLocal ? NSDragOperationEvery : NSDragOperationNone;
}

- (void)draggedImage:(id)aImage movedTo:(NSPoint)aTo
{
	_draggedImageLocation = aTo;
}

// The other boxes take their contents out while a drag runs and draw
// their cached images instead.
- (void)_acceptPrepareForDragging:(id)aDragging
{
	if (_isDragged)
		return;
	[self createImageIfNeeded];
	_content = [[self contentView] retain];
	[self setContentView:nil];
}

- (void)_acceptFinishDragging:(id)aDragging
{
	if (_isDragged)
		return;
	[self setContentView:_content];
	[_content release];
	_content = nil;
}

// With more than one city the box is torn off and dragged.
- (void)_startSelfDragging:(id)aDragging
{
	if (_startSelfDraggingTimer != aDragging)
		[_startSelfDraggingTimer invalidate];
	_startSelfDraggingTimer = nil;
	if (Preferences::entriesList()->count() <= 1)
		return;
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center postNotificationName:@"NeedStopAnimationsNotification" object:self userInfo:nil];
	NSPoint at = [self convertPoint:NSZeroPoint toView:nil];
	NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
	[pasteboard declareTypes:[NSArray arrayWithObject:@"CityInfoPboardType"] owner:nil];
	[self _createDraggedImage];
	[center postNotificationName:@"CityInfoStartDraggingNotification" object:self userInfo:nil];
	_draggingInProgress = YES;
	_needQuickDraw = YES;
	_draggingAborted = NO;
	[[_originalSuperView window] setViewsNeedDisplay:NO];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	[self dragImage:_image at:at offset:NSZeroSize event:_mouseDownEvent pasteboard:pasteboard source:self
		slideBack:NO];
#pragma clang diagnostic pop
}

// A double click (within 0.3 s) opens the city's details; a click held for
// 0.2 s starts the drag.
- (void)mouseDown:(id)aDown
{
	static NSDate *doubleClickStartTime = nil;
	if (_startSelfDraggingTimer)
		return;
	[super mouseDown:aDown];
	_continueDraggedEvent = YES;
	NSPoint point = [self convertPoint:[aDown locationInWindow] fromView:nil];
	if (![self mouse:point inRect:[self opaqueBounds]])
		return;
	if (doubleClickStartTime) {
		[doubleClickStartTime autorelease];
		if ([doubleClickStartTime timeIntervalSinceNow] >= -0.3) {
			doubleClickStartTime = nil;
			_continueDraggedEvent = NO;
			[[[AtlasController atlasController] mainWindowController] showDetailsForEntry:[_owner cityEntry]
				openPane:YES openInfoForBillboard:1];
			return;
		}
	}
	doubleClickStartTime = [[NSDate date] retain];
	[_mouseDownEvent release];
	_mouseDownEvent = [aDown retain];
	_startSelfDraggingTimer = [NSTimer scheduledTimerWithTimeInterval:0.2 target:self
		selector:@selector(_startSelfDragging:) userInfo:nil repeats:NO];
}

- (void)mouseDragged:(id)aDragged
{
	[_startSelfDraggingTimer invalidate];
	_startSelfDraggingTimer = nil;
	if (_continueDraggedEvent)
		[super mouseDragged:aDragged];
}

- (void)mouseUp:(id)aUp
{
	if (_startSelfDraggingTimer) {
		[_startSelfDraggingTimer invalidate];
		_startSelfDraggingTimer = nil;
		[[self class] setSelectedView:self];
		[[[AtlasController atlasController] mainWindowController] showDetailsForEntry:[_owner cityEntry]
			openPane:NO openInfoForBillboard:0];
	}
	NSPoint point = [self convertPoint:[aUp locationInWindow] fromView:nil];
	if (![self mouse:point inRect:[self opaqueBounds]])
		[super mouseUp:aUp];
}

// A drop the palette took (or a key pressed during the drag aborts it)
// slides the box back; a drop outside removes the city with a poof.
- (void)draggedImage:(id)aImage endedAt:(NSPoint)aAt operation:(NSUInteger)aOperation
{
	[self showDraggedImageIfCan];
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	if (aOperation != NSDragOperationEvery && !_draggingAborted) {
		NSEvent *event = [NSApp currentEvent];
		if ([event type] == NSEventTypeKeyDown && [[event charactersIgnoringModifiers] characterAtIndex:0])
			_draggingAborted = YES;
		[center postNotificationName:@"DraggingExitedNotification" object:_owner userInfo:nil];
		[[_originalSuperView window] displayIfNeeded];
	}
	[center postNotificationName:@"CityInfoStopDraggingNotification" object:self userInfo:nil];
	_needQuickDraw = NO;
	_isDragged = NO;
	if (aOperation != NSDragOperationEvery && !_draggingAborted) {
		[center postNotificationName:@"DraggingRemoveCityNotification" object:_owner userInfo:nil];
		NSSize size = [aImage size];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		NSShowAnimationEffect(NSAnimationEffectPoof, NSMakePoint(ceilf(size.width * 0.5f) + aAt.x,
			aAt.y + ceilf(size.height * 0.5f)), NSMakeSize(64.0, 64.0), self, @selector(_poofAnimationFinished), 0);
#pragma clang diagnostic pop
	} else {
		NSWindow *window = [_originalSuperView window];
		NSPoint origin = [_originalSuperView convertPoint:_draggedFrame.origin toView:nil];
		origin = [window convertRectToScreen:NSMakeRect(origin.x, origin.y, 0.0, 0.0)].origin;
		if ([_originalSuperView isFlipped])
			origin.y -= _draggedFrame.size.height;
		[self _animateDraggedWindowTo:origin];
		[self setFrame:_draggedFrame];
		[_originalSuperView addSubview:self];
		[self displayIfNeeded];
		_draggingInProgress = NO;
	}
	[_draggedWindow close];
	_draggedWindow = nil;
	[[_originalSuperView window] setViewsNeedDisplay:NO];
	_originalSuperView = nil;
	[self release];
}

// The box as drawn, for the quick redraws during a drag. The original read
// the focused view (initWithFocusedViewRect:), which layer-backed views no
// longer support; the image keeps the view's size in points.
- (void)_createImage
{
	if (![self window] || [self isHiddenOrHasHiddenAncestor])
		return;
	NSRect bounds = [self bounds];
	NSBitmapImageRep *rep = [self bitmapImageRepForCachingDisplayInRect:bounds];
	if (!rep)
		return;
	[self cacheDisplayInRect:bounds toBitmapImageRep:rep];
	[_image release];
	_image = [[NSImage alloc] initWithSize:bounds.size];
	[_image addRepresentation:rep];
	_needCreateImage = NO;
}

@end
