// ProtocolIDsList
//
// LF::ProtocolIDsList: the protocols a class conforms to, or a protocol's
// parents or children. Membership is answered from a set of the whole
// hierarchy (following parents or children), built on first use and not
// updated by later additions.

#include "LObject.h"

namespace LF {

static CSharableResource sLocker;

unsigned int ProtocolIDsList::LProtocolIDKeyCallbacks::hash(LProtocolID *p)
{
	return p->hashValue();
}

bool ProtocolIDsList::LProtocolIDKeyCallbacks::equal(LProtocolID *stored, LProtocolID *p)
{
	return *stored == *p;
}

ProtocolIDsList::ProtocolIDsList(bool parents) : _ids(0), _set(0), _parents(parents)
{
}

ProtocolIDsList::~ProtocolIDsList()
{
	delete _ids;
	_ids = 0;
	delete _set;
	_set = 0;
}

void ProtocolIDsList::addID(LProtocolID *pid)
{
	if (_ids == 0)
		_ids = new TArray<LProtocolID *>;
	_ids->addObject(pid);
}

void ProtocolIDsList::_addHierarchyToSet(_ProtocolIDsSet *set) const
{
	int n = _ids ? (int)_ids->count() : 0;
	for (int i = 0; i < n; i++) {
		LProtocolID *pid = (*_ids)[i];
		set->addObject(pid);
		(_parents ? pid->_parentsList : pid->_childrenList)._addHierarchyToSet(set);
	}
}

void ProtocolIDsList::_createSetIfNeeded() const
{
	if (_set == 0) {
		StSharableLocker lock(&sLocker);
		if (_set == 0) {
			_ProtocolIDsSet *set = new _ProtocolIDsSet;
			_set = set;
			_addHierarchyToSet(set);
		}
	}
}

void ProtocolIDsList::getAllIDs(CHashSet *out) const
{
	if (out == 0)
		return;
	_createSetIfNeeded();
	_ProtocolIDsSet::hashTableIterator it(_set);
	while (it.next())
		out->addObject((intptr_t)it.entry()->key);
}

bool ProtocolIDsList::containsID(LProtocolID *pid) const
{
	_createSetIfNeeded();
	return _set->entryPtrAddrForKey(pid) != 0;
}

} // namespace LF
