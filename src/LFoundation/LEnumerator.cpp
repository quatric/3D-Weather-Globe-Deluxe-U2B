// LEnumerator
//
// LF::LEnumerator: the base of container enumerators; it holds a reference
// to the container it walks.

#include "LObject.h"

namespace LF {

LF_IMPLEMENT_CLASS(LEnumerator, 0)

LEnumerator *LEnumerator::initWithContainer(LObject *container)
{
	if (init() == 0)
		return 0;
	_container = LRetain(container);
	return this;
}

void LEnumerator::dealloc()
{
	if (_container) {
		_container->release();
		_container = 0;
	}
	LObject::dealloc();
}

} // namespace LF
