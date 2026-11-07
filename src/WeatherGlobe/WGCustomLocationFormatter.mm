#import "WGNewLocationController.h"

// Coordinates as typed: digits plus the degree/minute/second marks (in DMS
// mode) or "-", ",", "." (in decimal mode). Edits are always rewritten, so
// isPartialStringValid: returns NO with the cleaned string.
@implementation WGCustomLocationFormatter

- (id)removeAllNonDecimalsFromString:(id)aString excludeChars:(id)aChars
{
	NSMutableString *string = [NSMutableString stringWithString:aString];
	NSMutableCharacterSet *kept = [[NSCharacterSet decimalDigitCharacterSet] mutableCopy];
	for (NSUInteger i = 0; i < [aChars count]; i++)
		[kept addCharactersInString:[aChars objectAtIndex:i]];
	[kept invert];
	NSRange range;
	while ((range = [string rangeOfCharacterFromSet:kept]).length)
		[string deleteCharactersInRange:range];
	[kept release];
	return string;
}

- (void)setDelegate:(id)aDelegate
{
	_delegate = aDelegate;
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

- (id)attributedStringForObjectValue:(id)aValue withDefaultAttributes:(id)aAttributes
{
	return [aValue isKindOfClass:[NSAttributedString class]] ? aValue : nil;
}

- (id)editingStringForObjectValue:(id)aValue
{
	return [aValue isKindOfClass:[NSString class]] ? aValue : nil;
}

// DMS: a deleted mark is put back, and everything before the seconds mark
// keeps only digits and marks. Decimal: only digits and "-,.".
- (BOOL)isPartialStringValid:(id *)aValid proposedSelectedRange:(NSRange *)aRange originalString:(id)aString originalSelectedRange:(NSRange)aRange2 errorDescription:(id *)aDescription
{
	if (![*aValid length]) {
		if (_delegate && [_delegate respondsToSelector:@selector(nullWasEntered)])
			[_delegate nullWasEntered];
		return YES;
	}
	if (_delegate && [_delegate respondsToSelector:@selector(isCoordsModeDMS)] && ![_delegate isCoordsModeDMS]) {
		NSArray *excluded = [NSArray arrayWithObjects:@"-", @",", @".", nil];
		NSString *digits = [self removeAllNonDecimalsFromString:*aValid excludeChars:excluded];
		NSString *previous = [*aValid substringToIndex:[*aValid length] - 1];
		if ([digits length] && ![previous isEqualToString:digits])
			*aValid = [NSString stringWithString:digits];
		return NO;
	}

	NSBundle *bundle = [NSBundle mainBundle];
	NSString *degrees = [bundle localizedStringForKey:@"DegreesCharacter" value:@"" table:nil];
	NSString *minutes = [bundle localizedStringForKey:@"MinutesCharacter" value:@"" table:nil];
	NSString *seconds = [bundle localizedStringForKey:@"SecondsCharacter" value:@"" table:nil];
	unichar degreesChar = [degrees characterAtIndex:0];
	unichar minutesChar = [minutes characterAtIndex:0];
	unichar secondsChar = [seconds characterAtIndex:0];
	NSArray *marks = [NSArray arrayWithObjects:degrees, minutes, seconds, nil];
	if (![*aValid rangeOfString:degrees].length || ![*aValid rangeOfString:minutes].length
	    || ![*aValid rangeOfString:seconds].length) {
		if (![[self removeAllNonDecimalsFromString:*aValid excludeChars:marks] length])
			return NO;
		for (NSUInteger i = 0; i < [aString length]; i++) {
			unichar c = [aString characterAtIndex:i];
			unichar here = i < [*aValid length] ? [*aValid characterAtIndex:i] : 0;
			unichar next = i + 1 < [*aValid length] ? [*aValid characterAtIndex:i + 1] : 0;
			if (next != c && here != c && (c == degreesChar || c == minutesChar || c == secondsChar)) {
				NSUInteger at = MIN(i, [*aValid length]);
				NSMutableString *string = [NSMutableString string];
				[string appendString:[*aValid substringToIndex:at]];
				[string appendFormat:@"%C", c];
				[string appendString:[*aValid substringFromIndex:at]];
				*aValid = string;
			}
		}
	}
	NSRange secondsRange = [*aValid rangeOfString:seconds];
	NSString *head = [NSString stringWithString:*aValid];
	NSString *tail = [NSString string];
	if (secondsRange.length) {
		head = [*aValid substringToIndex:secondsRange.location];
		tail = [*aValid substringFromIndex:secondsRange.location];
	}
	NSMutableString *result = [NSMutableString stringWithString:[self removeAllNonDecimalsFromString:head excludeChars:marks]];
	[result appendString:tail];
	*aValid = result;
	return NO;
}

@end
