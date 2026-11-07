#import "WGBalloonPaletteController.h"

@implementation _WGBalloonPanel

- (void)setIsClosable:(BOOL)aClosable
{
	_isClosable = aClosable;
}

- (BOOL)validateMenuItem:(id)aItem
{
	if (_isClosable && [aItem action] == @selector(performClose:))
		return YES;
	return [super validateMenuItem:aItem];
}

// Closable balloons have no close button, so close them here (asking the
// delegate first).
- (void)performClose:(id)aClose
{
	if (!_isClosable) {
		[super performClose:aClose];
		return;
	}
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

// Escape closes the other balloons.
- (void)keyDown:(id)aDown
{
	if (!_isClosable && [aDown keyCode] == 0x35) {
		[self close];
		return;
	}
	[super keyDown:aDown];
}

@end
