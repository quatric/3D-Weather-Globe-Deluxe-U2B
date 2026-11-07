// CHashSet
//
// LF::CHashSet: an integer set; everything but getAllObjects is inline in
// LContainers.h.

#include "LContainers.h"

namespace LF {

void CHashSet::getAllObjects(TArray<intptr_t> *out) const
{
	out->preserveCapacity(_imp.count());
	Imp::hashTableIterator it(&_imp);
	while (it.next())
		out->addObject((intptr_t)it.entry()->key);
}

} // namespace LF
