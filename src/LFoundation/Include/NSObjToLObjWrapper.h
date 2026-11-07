// NSObjToLObjWrapper: an NSObject that holds a retained LObject, used to hand
// LObjects to Objective-C code (LObject::detachNSWrapper, ObserverWrapper).
#pragma once

#import <Foundation/Foundation.h>
#include "LObject.h"

@interface NSObjToLObjWrapper : NSObject {
	LF::LObject *_object;
}
+ (id)wrapperWithLObject:(LF::LObject *)object;
- (id)initWithLObject:(LF::LObject *)object;
- (LF::LObject *)getLObject;
// Doesn't retain.
- (void)_setLObject:(LF::LObject *)object;
@end
