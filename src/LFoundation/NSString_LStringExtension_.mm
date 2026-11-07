// NSString(LStringExtension)
//
// NSString(LStringExtension). Built without ARC.

#include "NSString_LStringExtension.h"

@implementation NSString (LStringExtension)

+ (id)stringWithLString:(LF::LString *)str
{
	if (str == 0)
		return nil;
	return [self stringWithString:(NSString *)str->_cf];
}

+ (id)stringReferenceFromLString:(LF::LString *)str
{
	return str ? (NSString *)str->_cf : nil;
}

- (LF::LString *)LString
{
	return LF::LString::stringWithCFString((CFStringRef)self);
}

@end
