#import "PreferencesController.h"

@implementation PreferencesWindow

// Escape closes the window.
- (void)keyDown:(id)aDown
{
	if ([[aDown characters] isEqualToString:@"\x1b"])
		[self performClose:[self standardWindowButton:NSWindowCloseButton]];
	else
		[super keyDown:aDown];
}

@end
