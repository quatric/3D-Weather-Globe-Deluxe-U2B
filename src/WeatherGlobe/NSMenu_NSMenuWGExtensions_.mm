#import "NSMenu+NSMenuWGExtensions.h"

@implementation NSMenu (NSMenuWGExtensions)

// Pops the menu up at the view's center for anything but a mouse down,
// using a key event there (the original one for a key down, else a space).
+ (void)wgPopUpContextMenu:(id)aMenu withEvent:(id)aEvent forView:(id)aView
{
	NSPoint center = NSMakePoint(NSMinX([aView bounds]) + NSWidth([aView bounds]) * 0.5,
	                             NSMinY([aView bounds]) + NSHeight([aView bounds]) * 0.5);
	NSPoint location = [aView convertPoint:center toView:nil];
	NSEvent *original = aEvent;
	NSEvent *event = original;
	NSEventType type = [original type];
	if (type != NSEventTypeLeftMouseDown) {
		if (type == NSEventTypeKeyDown) {
			event = [NSEvent keyEventWithType:NSEventTypeKeyDown location:location modifierFlags:[original modifierFlags]
			                        timestamp:[original timestamp] windowNumber:[original windowNumber]
			                          context:nil characters:[original characters]
			      charactersIgnoringModifiers:[original charactersIgnoringModifiers] isARepeat:[original isARepeat]
			                          keyCode:[original keyCode]];
		} else {
			event = [NSEvent keyEventWithType:NSEventTypeKeyDown location:location modifierFlags:[original modifierFlags]
			                        timestamp:[original timestamp] windowNumber:[[aView window] windowNumber]
			                          context:nil characters:@" " charactersIgnoringModifiers:@" " isARepeat:NO
			                          keyCode:0x31];
		}
	}
	[NSMenu popUpContextMenu:aMenu withEvent:event forView:aView];
}

@end
