#import "SMKAngleSlider.h"

// A round slider choosing an angle; the arrow keys turn it.
@implementation SMKAngleSlider

+ (Class)cellClass
{
	return [SMKAngleSliderCell class];
}

- (BOOL)isOpaque
{
	return NO;
}

- (BOOL)acceptsFirstMouse:(id)aMouse
{
	return YES;
}

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self != nil)
		[self setContinuous:YES];
	return self;
}

// Only changed values are sent.
- (BOOL)sendAction:(SEL)aAction to:(id)aTo
{
	float value = [[self cell] floatValue];
	if (value == _cachedValue)
		return NO;
	_cachedValue = value;
	[self setFloatValue:value];
	return [super sendAction:aAction to:aTo];
}

// Up and right turn one way, down and left the other.
- (void)keyDown:(id)aDown
{
	NSEventModifierFlags flags = [aDown modifierFlags];
	NSString *characters = [[aDown charactersIgnoringModifiers] retain];
	NSUInteger length = [characters length], i = length;
	for (NSUInteger n = 0; n != length; n++) {
		i--;
		unichar c = [characters characterAtIndex:i];
		if (c == NSDownArrowFunctionKey || c == NSLeftArrowFunctionKey)
			[[self cell] moveInDirection:1 withFlags:flags];
		else if (c == NSUpArrowFunctionKey || c == NSRightArrowFunctionKey)
			[[self cell] moveInDirection:0 withFlags:flags];
		else
			[super keyDown:aDown];
	}
	[characters release];
}

@end
