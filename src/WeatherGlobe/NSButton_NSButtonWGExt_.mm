#import "NSButton+NSButtonWGExt.h"

@implementation NSButton (NSButtonWGExt)

- (void)setTitleColor:(id)aColor
{
	NSMutableAttributedString *title = [[[self attributedTitle] mutableCopy] autorelease];
	[title addAttribute:NSForegroundColorAttributeName value:aColor range:NSMakeRange(0, [title length])];
	[self setAttributedTitle:title];
}

@end
