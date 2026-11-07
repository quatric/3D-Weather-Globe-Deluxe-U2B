// LAbstractClass
//
// LF::LAbstractClass: the class object of a class with no instances of its
// own; allocating one raises FailedWithCodeException(-50).

#include "LObject.h"

namespace LF {

LObject *LAbstractClass::alloc()
{
	LF_RAISE(FailedWithCodeException(-50));
	return 0;
}

bool LAbstractClass::isAbstract()
{
	return true;
}

bool LSelectorsAreEqual(const char *a, const char *b)
{
	return a == b || strcmp(a, b) == 0;
}

} // namespace LF
