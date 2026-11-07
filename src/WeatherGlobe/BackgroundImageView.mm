#import "BackgroundImageView.h"

@implementation BackgroundImageView

- (BOOL)mouseDownCanMoveWindow
{
	return YES;
}

// The mouse goes to the superview.
- (void)mouseDown:(id)aDown
{
	[[self superview] mouseDown:aDown];
}

- (void)mouseDragged:(id)aDragged
{
	[[self superview] mouseDragged:aDragged];
}

- (void)mouseUp:(id)aUp
{
	[[self superview] mouseUp:aUp];
}

@end
