#import "CIPaletteWindow.h"
#import "CityInfoView.h"

// The city info palette: a dark, translucent floating panel dragged by its
// background (kept below the top of the screen).
@implementation CIPaletteWindow

- (id)initWithContentRect:(NSRect)aRect styleMask:(NSUInteger)aMask backing:(NSInteger)aBacking defer:(BOOL)aDefer
{
	self = [super initWithContentRect:aRect
		styleMask:NSWindowStyleMaskUtilityWindow | NSWindowStyleMaskNonactivatingPanel
		backing:NSBackingStoreBuffered defer:NO];
	[self setBackgroundColor:[NSColor colorWithCalibratedWhite:0.2039f alpha:Preferences::paletteTransparencyLevel()]];
	[self setHidesOnDeactivate:Preferences::hidesWFPaletteOnDeactivate()];
	[self setAlphaValue:1.0];
	[self setOpaque:NO];
	[self setHasShadow:YES];
	[self setLevel:NSFloatingWindowLevel];
	[self setMovableByWindowBackground:YES];
	LNotificationCenter::center()->addObserver(self, @selector(_transparencyLevelWasChanged),
		Preferences::kCITransparencyLevelWasChanged, 0);
	LNotificationCenter::center()->addObserver(self, @selector(_hidesOnDeactivateStateWasChanged),
		Preferences::kHidesWFPaletteOnDeactivateStateWasChanged, 0);
	return self;
}

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	[super dealloc];
}

- (void)_transparencyLevelWasChanged
{
	[self setBackgroundColor:[[self backgroundColor] colorWithAlphaComponent:Preferences::paletteTransparencyLevel()]];
	[self setViewsNeedDisplay:YES];
}

- (void)_hidesOnDeactivateStateWasChanged
{
	[self setHidesOnDeactivate:Preferences::hidesWFPaletteOnDeactivate()];
}

// A translucent window's shadow follows what was drawn.
- (void)displayIfNeeded
{
	BOOL invalidate = [self hasShadow] && Preferences::paletteTransparencyLevel() < 1.0f && [self viewsNeedDisplay];
	[super displayIfNeeded];
	if (invalidate)
		[self invalidateShadow];
}

- (BOOL)validateMenuItem:(id)aItem
{
	if ([aItem action] == @selector(performClose:))
		return YES;
	return [super validateMenuItem:aItem];
}

- (void)performClose:(id)aClose
{
	id delegate = [self delegate];
	if (delegate && ![delegate windowShouldClose:aClose]) {
		NSBeep();
		return;
	}
	[self close];
}

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (NSPoint)_screenLocationOfEvent:(NSEvent *)aEvent
{
	NSPoint point = [aEvent locationInWindow];
	return [self convertRectToScreen:NSMakeRect(point.x, point.y, 0.0, 0.0)].origin;
}

- (void)mouseDown:(id)aDown
{
	NSRect frame = [self frame];
	NSPoint point = [self _screenLocationOfEvent:aDown];
	_initialLocation = NSMakePoint(point.x - frame.origin.x, point.y - frame.origin.y);
	_wasDragged = NO;
}

- (void)mouseDragged:(id)aDragged
{
	NSRect visible = [[self screen] visibleFrame];
	NSRect frame = [self frame];
	NSPoint point = [self _screenLocationOfEvent:aDragged];
	CGFloat y = point.y - _initialLocation.y;
	if (visible.origin.y + visible.size.height < y + frame.size.height)
		y = visible.size.height - frame.size.height + visible.origin.y;
	[self setFrameOrigin:NSMakePoint(point.x - _initialLocation.x, y)];
	_wasDragged = YES;
}

// A click (not a drag) deselects the city views.
- (void)mouseUp:(id)aUp
{
	if (!_wasDragged)
		[CityInfoView setSelectedView:nil];
}

- (void)setTitleOffset:(CGFloat)aOffset
{
	_titleOffset = aOffset;
}

@end
