#import "AboutController.h"
#import "NSCursor+NSCursorWGExtensions.h"

@implementation AboutController

// Links in the credits get the big pointing hand.
- (void)awakeFromNib
{
	id textView = [_creditsView documentView];
	if ([textView respondsToSelector:@selector(setLinkTextAttributes:)] &&
	    [textView respondsToSelector:@selector(linkTextAttributes)]) {
		NSMutableDictionary *attributes = [[textView performSelector:@selector(linkTextAttributes)] mutableCopy];
		[attributes setObject:[NSCursor pointingHandBigCursor] forKey:@"NSCursor"];
		[textView performSelector:@selector(setLinkTextAttributes:) withObject:attributes];
		[attributes release];
	}
}

- (void)dealloc
{
	[_panel release];
	[super dealloc];
}

- (void)showPanel
{
	if (!_panel && ![NSBundle loadNibNamed:@"About" owner:self])
		return;
	[_tabView selectFirstTabViewItem:self];
	[_panel center];
	[_panel makeKeyAndOrderFront:self];
}

@end
