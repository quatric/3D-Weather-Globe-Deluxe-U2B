// ScreenSaverGLView: the saver's 3D view. It ignores the mouse (the saver
// ends on it) and drains a pool around each draw.

#import <Cocoa/Cocoa.h>
#include "ScreenSaverLib.h"

@implementation ScreenSaverGLView

- (void)drawRect:(NSRect)rect
{
	LAutoreleasePool::addNewPoolToTop();
	[super drawRect:rect];
	LAutoreleasePool::removeTopPool();
}

- (BOOL)acceptsFirstResponder
{
	return NO;
}

- (void)mouseDown:(NSEvent *)event {}
- (void)mouseDragged:(NSEvent *)event {}
- (void)mouseUp:(NSEvent *)event {}
- (void)rightMouseDown:(NSEvent *)event {}
- (void)rightMouseDragged:(NSEvent *)event {}
- (void)rightMouseUp:(NSEvent *)event {}
- (void)otherMouseDown:(NSEvent *)event {}
- (void)otherMouseDragged:(NSEvent *)event {}
- (void)otherMouseUp:(NSEvent *)event {}
- (void)mouseMoved:(NSEvent *)event {}
- (void)scrollWheel:(NSEvent *)event {}

@end
