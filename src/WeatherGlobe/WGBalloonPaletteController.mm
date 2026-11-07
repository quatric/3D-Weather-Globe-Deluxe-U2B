#import "WGBalloonPaletteController.h"
#import "WGBalloonTexturedVew.h"

// A balloon panel pointing at a position; auto-hidden balloons close when
// they lose key. It retains itself while shown.
@implementation WGBalloonPaletteController

- (void)dealloc
{
	[_balloonBackground release];
	[_balloonPalette setDelegate:nil];
	[_balloonPalette release];
	[super dealloc];
}

// The content view goes 7 points above the balloon's tail.
- (void)setPalettePosition:(NSPoint)aPosition contentView:(id)aView
{
	if (!_balloonBackground) {
		_balloonBackground = [[WGBalloonTexturedVew alloc] initWithFrame:NSZeroRect];
		[self _setupTexturedView:_balloonBackground];
	}
	if (!_balloonPalette) {
		_balloonPalette = [[_WGBalloonPanel alloc] initWithContentRect:[_balloonBackground frame]
		                                                     styleMask:NSWindowStyleMaskUtilityWindow
		                                                       backing:NSBackingStoreBuffered
		                                                         defer:YES];
		[_balloonBackground setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		[(_WGBalloonPanel *)_balloonPalette setIsClosable:!_isAutoHidden];
		[[_balloonPalette contentView] addSubview:_balloonBackground];
		[self _setupPalette];
	}
	while ([[_balloonBackground subviews] count])
		[[[_balloonBackground subviews] lastObject] removeFromSuperview];
	NSSize size = [aView frame].size;
	[aView setFrame:NSMakeRect(NSZeroRect.origin.x, 7.0, size.width, size.height)];
	[_balloonBackground addSubview:aView];
	[_balloonPalette setContentSize:NSMakeSize(size.width, size.height + 7.0)];
	[_balloonPalette setFrameOrigin:[_balloonBackground balloonPositionForProposedPosition:aPosition]];
}

- (void)setIsAutoHidden:(BOOL)aHidden
{
	_isAutoHidden = aHidden;
	[(_WGBalloonPanel *)_balloonPalette setIsClosable:!aHidden];
	if (_balloonBackground)
		[self _setupTexturedView:_balloonBackground];
}

- (void)showBalloonPalette
{
	[_balloonBackground setIsBalloonVisible:_isAutoHidden];
	[_balloonPalette makeKeyAndOrderFront:nil];
	[self retain];
}

- (void)hideBalloonPalette
{
	[_balloonPalette close];
}

- (void)windowDidResignKey:(id)aKey
{
	if ([aKey object] == _balloonPalette && _isAutoHidden)
		[self hideBalloonPalette];
}

- (void)windowDidMove:(id)aMove
{
	if ([aMove object] == _balloonPalette) {
		[_balloonBackground setIsBalloonVisible:NO];
		[_balloonPalette invalidateShadow];
	}
}

@end
