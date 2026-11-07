#import "WGCustomPaletteController.h"
#import "WGTexturedView.h"

@implementation WGCustomPaletteController

// The balloon's nine-patch.
- (void)_setupContentView
{
	static NSString *const names[9] = {@"bl-top-left",  @"bl-top-center", @"bl-top-right",
	                                   @"bl-midl-left", @"bl-center",     @"bl-midl-right",
	                                   @"bl-down-left", @"bl-down-center", @"bl-down-right"};
	for (int i = 0; i < 9; i++)
		[_contentView setImage:[NSImage imageNamed:names[i]] forTexture:i];
}

- (void)_setupPalette
{
	[_palette setBackgroundColor:[NSColor clearColor]];
	[_palette setMovableByWindowBackground:YES];
	[_palette setLevel:CGWindowLevelForKey(kCGFloatingWindowLevelKey)];
	[_palette setReleasedWhenClosed:NO];
	[_palette setDelegate:(id)self];
	[_palette setHasShadow:YES];
	[_palette setOneShot:YES];
	[_palette setOpaque:NO];
	[_palette center];
}

// The nib's palette is replaced by a borderless one holding the content view.
- (void)awakeFromNib
{
	[_palette release];
	_palette = [[_WGBorderlessPanel alloc] initWithContentRect:[_contentView frame]
	                                                 styleMask:NSWindowStyleMaskUtilityWindow
	                                                   backing:NSBackingStoreBuffered
	                                                     defer:YES];
	[[_palette contentView] addSubview:_contentView];
	[self _setupContentView];
	[self _setupPalette];
}

- (void)dealloc
{
	[_contentView release];
	[super dealloc];
}

- (void)closeButtonAction:(id)aAction
{
	[self hidePalette];
}

@end
