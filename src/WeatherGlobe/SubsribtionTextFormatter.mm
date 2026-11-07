#import "SubscriptionController.h"

@implementation SubsribtionTextFormatter

+ (id)formatterWithMaxLength:(NSUInteger)aLength
{
	return [[[self alloc] initWithMaxLength:aLength] autorelease];
}

- (id)init
{
	self = [super init];
	if (self)
		_maxLength = 1;
	return self;
}

- (id)initWithMaxLength:(NSUInteger)aLength
{
	[self init];
	_maxLength = (unsigned int)aLength;
	return self;
}

- (id)stringForObjectValue:(id)aValue
{
	return [aValue isKindOfClass:[NSString class]] ? aValue : nil;
}

- (BOOL)getObjectValue:(id *)aValue forString:(id)aString errorDescription:(id *)aDescription
{
	*aValue = aString;
	return YES;
}

// Too long: one extra character is just refused; a longer paste is cut to
// the maximum, with a beep.
- (BOOL)isPartialStringValid:(id)aValid newEditingString:(id *)aString errorDescription:(id *)aDescription
{
	BOOL valid = [super isPartialStringValid:aValid newEditingString:aString errorDescription:aDescription];
	_lastLength = (unsigned int)[aValid length];
	if (valid && _lastLength > _maxLength) {
		*aDescription = @"string is too long";
		valid = NO;
		if (_lastLength - _maxLength < 2)
			*aString = nil;
		else {
			*aString = [aValid substringWithRange:NSMakeRange(0, _maxLength)];
			NSBeep();
		}
	}
	return valid;
}

@end
