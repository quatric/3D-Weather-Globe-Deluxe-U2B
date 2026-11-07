#import "WGBalloonPaletteController.h"
#import "WGBalloonTexturedVew.h"

@implementation WGBalloonPaletteController (Private)

// The balloon's nine textures (the top ones without a close box when it
// hides itself) and the balloon tail at the bottom left.
- (void)_setupTexturedView:(id)aView
{
	if (!_isAutoHidden) {
		[aView setImage:[NSImage imageNamed:@"bl-top-left"] forTexture:0];
		[aView setImage:[NSImage imageNamed:@"bl-top-center"] forTexture:1];
		[aView setImage:[NSImage imageNamed:@"bl-top-right"] forTexture:2];
	} else {
		[aView setImage:[NSImage imageNamed:@"bl-top-left-02"] forTexture:0];
		[aView setImage:[NSImage imageNamed:@"bl-top-center-02"] forTexture:1];
		[aView setImage:[NSImage imageNamed:@"bl-top-right-02"] forTexture:2];
	}
	[aView setImage:[NSImage imageNamed:@"bl-midl-left"] forTexture:3];
	[aView setImage:[NSImage imageNamed:@"bl-center"] forTexture:4];
	[aView setImage:[NSImage imageNamed:@"bl-midl-right"] forTexture:5];
	[aView setImage:[NSImage imageNamed:@"bl-down-left"] forTexture:6];
	[aView setImage:[NSImage imageNamed:@"bl-down-center"] forTexture:7];
	[aView setImage:[NSImage imageNamed:@"bl-down-right"] forTexture:8];
	[aView setBalloonImage:[NSImage imageNamed:@"balloon_start"]];
	[aView setBalloonPosition:5];
}

- (void)_setupPalette
{
	[_balloonPalette setBackgroundColor:[NSColor clearColor]];
	[_balloonPalette setMovableByWindowBackground:YES];
	[_balloonPalette setLevel:CGWindowLevelForKey(kCGFloatingWindowLevelKey)];
	[_balloonPalette setReleasedWhenClosed:NO];
	[_balloonPalette setHasShadow:YES];
	[_balloonPalette setOneShot:YES];
	[_balloonPalette setOpaque:NO];
	[_balloonPalette setDelegate:(id)self];
}

@end
