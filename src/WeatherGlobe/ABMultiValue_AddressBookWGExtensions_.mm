#import "ABMultiValue+AddressBookWGExtensions.h"

@implementation ABMultiValue (AddressBookWGExtensions)

// The primary value, else the first.
- (id)primaryValue
{
	NSString *identifier = [self primaryIdentifier];
	NSUInteger index;
	if (!identifier) {
		if ([self count] == 0)
			return nil;
		index = 0;
	} else {
		index = [self indexForIdentifier:identifier];
		if (index == NSNotFound)
			return nil;
	}
	return [self valueAtIndex:index];
}

@end
