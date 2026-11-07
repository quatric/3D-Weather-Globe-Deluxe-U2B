#import "SMKStringFormatter.h"

// Limits a string's length and characters. Limited to the decimal digits it
// edits and returns numbers, with an empty string meaning 0.
@implementation SMKStringFormatter

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		if (_lengthLimit != 0) {
			[aCoder encodeInt:(int)_lengthLimit forKey:@"smk_lengthLimit"];
		}
		if (_allowedCharacters != nil) {
			[aCoder encodeObject:_allowedCharacters forKey:@"smk_allowedCharacters"];
		}
	}
	else {
		int version = 0;
		[aCoder encodeValueOfObjCType:"i" at:&version];
		[aCoder encodeValueOfObjCType:"I" at:&_lengthLimit];
		[aCoder encodeObject:_allowedCharacters];
	}
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	id characters = nil;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		if ([aCoder containsValueForKey:@"smk_lengthLimit"]) {
			_lengthLimit = (unsigned int)[aCoder decodeIntForKey:@"smk_lengthLimit"];
		}
		if ([aCoder containsValueForKey:@"smk_allowedCharacters"]) {
			characters = [aCoder decodeObjectForKey:@"smk_allowedCharacters"];
		}
	}
	else {
		int version, limit;
		[aCoder decodeValueOfObjCType:"i" at:&version];
		[aCoder decodeValueOfObjCType:"i" at:&limit];
		_lengthLimit = (unsigned int)limit;
		characters = [aCoder decodeObject];
	}
	[self setAllowedCharacters:characters];
	return self;
}

static BOOL _isNumeric(NSCharacterSet *characters)
{
	return characters != nil && (characters == [NSCharacterSet decimalDigitCharacterSet]
		|| [characters isEqual:[NSCharacterSet decimalDigitCharacterSet]]);
}

- (BOOL)isPartialStringValid:(id *)aValid proposedSelectedRange:(NSRange *)aRange originalString:(id)aString originalSelectedRange:(NSRange)aRange2 errorDescription:(id *)aDescription
{
	if ([*aValid length] > [self _lengthLimit]) {
		*aValid = [NSString stringWithString:aString];
		if (aRange != NULL) {
			*aRange = aRange2;
		}
		if (aDescription != NULL) {
			*aDescription = [NSString stringWithFormat:@"Maximum allowed %u characters", (unsigned int)[self _lengthLimit]];
		}
		return NO;
	}
	if (_allowedCharacters == nil) {
		return YES;
	}
	if ([*aValid length] == 0) {
		if (!_isNumeric(_allowedCharacters)) {
			return YES;
		}
		*aValid = @"0";
		*aRange = NSMakeRange(0, 1);
		return NO;
	}
	NSScanner *scanner = [NSScanner scannerWithString:*aValid];
	[scanner scanCharactersFromSet:_allowedCharacters intoString:NULL];
	if ([scanner isAtEnd]) {
		return YES;
	}
	*aValid = [NSString stringWithString:aString];
	*aRange = aRange2;
	if (aDescription != NULL) {
		*aDescription = @"String can contain only characters from a certain character set";
	}
	return NO;
}

- (BOOL)getObjectValue:(id *)aValue forString:(id)aString errorDescription:(id *)aDescription
{
	id unused = nil;
	if (aValue == NULL) {
		aValue = &unused;
	}
	BOOL numeric = _isNumeric(_allowedCharacters);
	NSString *string = aString;
	if ([aString length] > [self _lengthLimit]) {
		if (aDescription != NULL) {
			*aDescription = [NSString stringWithFormat:@"Maximum allowed %u characters", (unsigned int)[self _lengthLimit]];
		}
		return NO;
	}
	if (_allowedCharacters != nil) {
		if ([aString length] == 0) {
			if (!numeric) {
				*aValue = aString;
				return YES;
			}
			string = @"0";
		}
		else {
			NSScanner *scanner = [NSScanner scannerWithString:aString];
			[scanner scanCharactersFromSet:_allowedCharacters intoString:NULL];
			if (scanner != nil && ![scanner isAtEnd]) {
				if (aDescription != NULL) {
					*aDescription = @"String can contain only characters from a certain character set";
				}
				return NO;
			}
		}
	}
	if (!numeric) {
		*aValue = aString;
		return YES;
	}
	*aValue = [NSNumber numberWithInt:[string intValue]];
	return YES;
}

- (id)stringForObjectValue:(id)aValue
{
	if ([aValue isKindOfClass:[NSNumber class]]) {
		aValue = [aValue stringValue];
	}
	else if (![aValue isKindOfClass:[NSString class]]) {
		return nil;
	}
	if (aValue != nil && [aValue length] <= [self _lengthLimit]) {
		return [NSString stringWithString:aValue];
	}
	return nil;
}

- (void)setAllowedCharacters:(id)aCharacters
{
	if (_allowedCharacters != aCharacters) {
		[_allowedCharacters release];
		_allowedCharacters = [aCharacters retain];
	}
}

- (id)allowedCharacters
{
	return _allowedCharacters;
}

// 0 means no limit
- (NSUInteger)_lengthLimit
{
	return _lengthLimit != 0 ? _lengthLimit : 0xffffffff;
}

- (void)setLengthLimit:(NSUInteger)aLimit
{
	_lengthLimit = (unsigned int)aLimit;
}

- (NSUInteger)lengthLimit
{
	return _lengthLimit;
}

- (void)dealloc
{
	[_allowedCharacters release];
	[super dealloc];
}

@end
