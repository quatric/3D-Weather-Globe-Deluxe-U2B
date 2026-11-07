// LObjToNSObjWrapper
//
// LF::LObjToNSObjWrapper.

#import <Foundation/Foundation.h>
#include "LObjToNSObjWrapper.h"

namespace LF {

LF_IMPLEMENT_CLASS(LObjToNSObjWrapper, 0)

LObjToNSObjWrapper *LObjToNSObjWrapper::wrapperWithNSObject(id object)
{
	LObjToNSObjWrapper *wrapper = _alloc()->initWithNSObject(object);
	return wrapper ? LAutorelease(wrapper) : 0;
}

LObjToNSObjWrapper *LObjToNSObjWrapper::initWithNSObject(id object)
{
	if (LObject::init() == 0)
		return 0;
	_object = [object retain];
	return this;
}

void LObjToNSObjWrapper::dealloc()
{
	[_object release];
	LObject::dealloc();
}

} // namespace LF
