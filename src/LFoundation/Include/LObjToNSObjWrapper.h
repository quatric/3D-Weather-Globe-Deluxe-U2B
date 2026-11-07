// LObjToNSObjWrapper: an LObject that keeps an Objective-C object, so one can
// be stored in LFoundation containers.
#pragma once

#include "LObject.h"
#include <objc/objc.h>

namespace LF {

class LObjToNSObjWrapper : public LObject {
	LF_DECLARE_CLASS(LObjToNSObjWrapper)
public:
	LObjToNSObjWrapper() : _object(0) {}
	virtual ~LObjToNSObjWrapper() {}
	virtual void dealloc();
	virtual LObjToNSObjWrapper *getOwnerLObject() { return this; }

	static LObjToNSObjWrapper *wrapperWithNSObject(id object);
	LObjToNSObjWrapper *initWithNSObject(id object);
	id nsObject() const { return _object; }

	id _object;
};

} // namespace LF
