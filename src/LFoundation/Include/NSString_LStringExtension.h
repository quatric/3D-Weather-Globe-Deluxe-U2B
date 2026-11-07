// NSString(LStringExtension): conversions between NSString and LString.
#pragma once

#import <Foundation/Foundation.h>
#include "LString.h"

@interface NSString (LStringExtension)
+ (id)stringWithLString:(LF::LString *)str;
// The LString's own CFString, not a copy.
+ (id)stringReferenceFromLString:(LF::LString *)str;
// Autoreleased.
- (LF::LString *)LString;
@end
