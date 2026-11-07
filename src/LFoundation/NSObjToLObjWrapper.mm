// NSObjToLObjWrapper
//
// NSObjToLObjWrapper, LF::LObject's Objective-C wrapper and
// LF::ObserverWrapper::performSelector. Built without ARC.

#include "NSObjToLObjWrapper.h"
#include "LNotification.h"

@implementation NSObjToLObjWrapper

+ (id)wrapperWithLObject:(LF::LObject *)object
{
	return [[[NSObjToLObjWrapper alloc] initWithLObject:object] autorelease];
}

- (id)initWithLObject:(LF::LObject *)object
{
	self = [super init];
	if (self)
		_object = LF::LRetain(object);
	return self;
}

- (void)dealloc
{
	if (_object) {
		_object->release();
		_object = 0;
	}
	[super dealloc];
}

- (LF::LObject *)getLObject
{
	return _object;
}

- (void)_setLObject:(LF::LObject *)object
{
	_object = object;
}

@end

namespace LF {

// The wrapper made by detachNSWrapper doesn't retain the object; the object
// lets go of it when it is deallocated.
void LObject::_invalidateNSWrapper()
{
	if (_nsWrapper) {
		NSObjToLObjWrapper *w = (NSObjToLObjWrapper *)_nsWrapper;
		[w _setLObject:0];
		[w release];
		_nsWrapper = 0;
	}
}

void *LObject::detachNSWrapper()
{
	if (_nsWrapper == 0) {
		static CSharableResource mutex;
		StSharableLocker locker(&mutex);
		if (_nsWrapper == 0) {
			NSObjToLObjWrapper *w = [NSObjToLObjWrapper new];
			_nsWrapper = w;
			[w _setLObject:this];
		}
	}
	return [[(NSObjToLObjWrapper *)_nsWrapper retain] autorelease];
}

void ObserverWrapper::performSelector(const char *sel, LNotification *ntf)
{
	NSObjToLObjWrapper *w = [[NSObjToLObjWrapper alloc] initWithLObject:ntf];
	[(NSObject *)_observer performSelector:(SEL)sel withObject:w];
	[w release];
}

} // namespace LF
