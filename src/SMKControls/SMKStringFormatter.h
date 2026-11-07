#import "SMKControlsPrefix.h"


@interface SMKStringFormatter : NSFormatter
{
    unsigned int _lengthLimit;
    NSCharacterSet *_allowedCharacters;
}
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (BOOL)isPartialStringValid:(id *)aValid proposedSelectedRange:(NSRange *)aRange originalString:(id)aString originalSelectedRange:(NSRange)aRange2 errorDescription:(id *)aDescription;
- (BOOL)getObjectValue:(id *)aValue forString:(id)aString errorDescription:(id *)aDescription;
- (id)stringForObjectValue:(id)aValue;
- (void)setAllowedCharacters:(id)aCharacters;
- (id)allowedCharacters;
- (NSUInteger)_lengthLimit;
- (void)setLengthLimit:(NSUInteger)aLimit;
- (NSUInteger)lengthLimit;
- (void)dealloc;
@end

