#import "WBLicenseController.h"

@implementation LicenseWindow

// Borderless, transparent and movable by its background.
- (id)initWithContentRect:(NSRect)aRect styleMask:(NSUInteger)aMask backing:(NSInteger)aBacking defer:(BOOL)aDefer
{
	self = [super initWithContentRect:aRect styleMask:NSWindowStyleMaskBorderless backing:(NSBackingStoreType)aBacking defer:aDefer];
	[self setBackgroundColor:[NSColor clearColor]];
	[self setAlphaValue:1.0];
	[self setOpaque:NO];
	[self setHasShadow:YES];
	[self setMovableByWindowBackground:YES];
	return self;
}

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

@end
