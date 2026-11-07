#import "SMKBackgroundPopUpButton.h"

@implementation SMKBackgroundPopUpButton

+ (Class)cellClass
{
	return [SMKBackgroundPopUpButtonCell class];
}

- (BOOL)isFlipped
{
	return [super isFlipped];
}

- (NSInteger)state
{
	return [[self cell] state];
}

- (void)setState:(NSInteger)aState
{
	[[self cell] setState:aState];
}

@end
