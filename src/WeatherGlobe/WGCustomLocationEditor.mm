#import "WGNewLocationController.h"

@implementation WGCustomLocationEditor

// The paste delegate may take the pasted string itself.
- (void)paste:(id)aPaste
{
	if ([_pasteDelegate respondsToSelector:@selector(wgCustomLocationEditor:pasteString:)]) {
		NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
		NSString *string = nil;
		if ([pasteboard availableTypeFromArray:[self readablePasteboardTypes]]) {
			NSString *type = [self preferredPasteboardTypeFromArray:[pasteboard types]
			                    restrictedToTypesFromArray:[self readablePasteboardTypes]];
			string = [pasteboard stringForType:type];
		}
		if ([_pasteDelegate wgCustomLocationEditor:self pasteString:string])
			return;
	}
	[super paste:aPaste];
}

- (id)pasteDelegate
{
	return _pasteDelegate;
}

- (void)setPasteDelegate:(id)aDelegate
{
	_pasteDelegate = aDelegate;
}

@end
