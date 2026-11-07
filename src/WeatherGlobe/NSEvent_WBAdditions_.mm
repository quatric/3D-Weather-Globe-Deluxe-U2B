#import "NSEvent+WBAdditions.h"

static BOOL _isEnterKey(NSEvent *event)
{
	unichar c = [[event charactersIgnoringModifiers] characterAtIndex:0];
	return c == '\r' || c == '\n' || c == 3;	// return, line feed, enter
}

@implementation NSEvent (WBAdditions)

- (BOOL)isEnterDown
{
	return [self type] == NSEventTypeKeyDown && _isEnterKey(self);
}

- (BOOL)isEnterUp
{
	return [self type] == NSEventTypeKeyUp && _isEnterKey(self);
}

@end
