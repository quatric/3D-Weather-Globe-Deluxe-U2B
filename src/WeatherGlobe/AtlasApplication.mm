#import "AtlasApplication.h"
#import "WGHelpBook.h"

static const NSUInteger _kModifiersMask = NSEventModifierFlagShift | NSEventModifierFlagControl |
                                          NSEventModifierFlagOption | NSEventModifierFlagCommand;

@implementation AtlasApplication

// Command-/ opens the help, Command-§ toggles the remote control focus and
// Command-= zooms in; releasing the arrow and zoom keys with Command goes
// to the key window.
- (void)sendEvent:(id)aEvent
{
	NSEvent *event = aEvent;
	SEL action = NULL;
	id sender = nil;
	if ([event type] == NSEventTypeKeyDown && [[event characters] isEqualToString:@"/"] &&
	    ([event modifierFlags] & _kModifiersMask) == NSEventModifierFlagCommand) {
		action = @selector(showHelp:);
	} else if ([event type] == NSEventTypeKeyDown && [event keyCode] == 10 &&
	           ([event modifierFlags] & _kModifiersMask) == NSEventModifierFlagCommand) {
		action = @selector(toggleRemoteControleFocus:);
	} else if ([event type] == NSEventTypeKeyDown && [event keyCode] == 0x18 &&
	           ([event modifierFlags] & _kModifiersMask) == NSEventModifierFlagCommand) {
		action = @selector(viewControlMenu:);
		sender = _zoomInMenuItem;
	} else {
		if ([event type] == NSEventTypeKeyUp) {
			unsigned short key = [event keyCode];
			if ((key == 0x7b || key == 0x7c || key == 0x7d || key == 0x7e || key == 0x18 || key == 0x45 ||
			     key == 0x1b || key == 0x4e) &&
			    ([event modifierFlags] & NSEventModifierFlagCommand)) {
				[[NSApp keyWindow] sendEvent:event];
				return;
			}
		}
		[super sendEvent:event];
		return;
	}
	[self sendAction:action to:nil from:sender];
}

- (void)showHelp:(id)aHelp
{
	[WGHelpBook gotoPage:nil anchor:nil];
}

// No page setup.
- (BOOL)validateMenuItem:(id)aItem
{
	if ([aItem action] == @selector(runPageLayout:))
		return NO;
	return [super validateMenuItem:aItem];
}

@end
