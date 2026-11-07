// LComparable
//
// LF::LComparable: objects with an ordering and a cached hash. Two
// comparables of the same class with different hashes are unequal without
// calling _isEqualTo().

#include "LObject.h"

namespace LF {

LF_IMPLEMENT_CLASS(LComparable, 0)

// Orders by compareWith(); a null on either side compares the pointers.
int _LComparableObjectCompareCallback(const void *a, const void *b, const void *)
{
	const LComparable *x = *(const LComparable *const *)a;
	const LComparable *y = *(const LComparable *const *)b;
	if (x != 0 && y != 0)
		return x->compareWith(y);
	return y > x ? 1 : (y < x ? -1 : 0);
}

bool LComparable::isEqualTo(const LObject *other) const
{
	if (other == 0)
		return false;
	if (!other->getClass()->isSubclassOf(LF_CLASSOF(LComparable)))
		return LObject::isEqualTo(other);
	const LComparable *o = (const LComparable *)other;
	if (getClass() == other->getClass() && hashValue() != o->hashValue())
		return false;
	return _isEqualTo(o);
}

} // namespace LF
