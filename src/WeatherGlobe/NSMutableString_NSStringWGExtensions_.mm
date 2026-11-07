#import "NSMutableString+NSStringWGExtensions.h"

@implementation NSMutableString (NSStringWGExtensions)

- (void)wgRemoveCharactersInSet:(id)aSet
{
	NSRange range;
	while ((range = [self rangeOfCharacterFromSet:aSet]).location != NSNotFound)
		[self deleteCharactersInRange:range];
}

@end
