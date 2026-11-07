#import "WGCustomPaletteController.h"

@implementation _WGBorderlessPanel

// Close works without a close box.
- (BOOL)validateMenuItem:(id)aItem
{
	if ([aItem action] == @selector(performClose:))
		return YES;
	return [super validateMenuItem:aItem];
}

- (void)performClose:(id)aClose
{
	id delegate = [self delegate];
	if ([delegate respondsToSelector:@selector(windowShouldClose:)] && ![delegate windowShouldClose:aClose]) {
		NSBeep();
		return;
	}
	[self close];
}

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

@end
