// LIndexSet
//
// LF::LIndexSet.

#include "LIndexSet.h"

namespace LF {

LF_IMPLEMENT_CLASS(LIndexSet, 0)

int LIndexSet::_compareCallback(const void *a, const void *b, const void *)
{
	unsigned int x = *(const unsigned int *)a, y = *(const unsigned int *)b;
	if (x == y)
		return 0;
	return x < y ? -1 : 1;
}

void LIndexSet::_createArray()
{
	if (_indexes)
		return;
	_indexes = new TArray<unsigned int>;
	_indexes->setComparator(_compareCallback);
	_indexes->setKeepsSorted(true);
}

void LIndexSet::dealloc()
{
	if (_indexes) {
		delete _indexes;
		_indexes = 0;
	}
	LObject::dealloc();
}

LIndexSet *LIndexSet::initWithIndex(unsigned int index)
{
	if (!LObject::init())
		return 0;
	addIndex(index);
	return this;
}

LIndexSet *LIndexSet::initWithSet(LIndexSet *set)
{
	if (!LObject::init())
		return 0;
	addIndexes(set);
	return this;
}

void LIndexSet::reserveCapacity(unsigned int capacity)
{
	if (_indexes == 0)
		_createArray();
	_indexes->preserveCapacity(capacity);
}

void LIndexSet::addIndex(unsigned int index)
{
	if (_indexes == 0)
		_createArray();
	_indexes->addObject(index);
}

void LIndexSet::addIndexes(const LIndexSet *set)
{
	int n = set->count();
	if (n && _indexes == 0)
		_createArray();
	for (int i = 0; i < n; i++)
		_indexes->addObject((*set->_indexes)[i]);
}

void LIndexSet::removeIndex(unsigned int index)
{
	if (_indexes)
		_indexes->removeObject(index);
}

void LIndexSet::removeIndexes(const LIndexSet *set)
{
	if (set == 0 || set->_indexes == 0 || _indexes == 0)
		return;
	int n = set->_indexes->count();
	for (int i = 0; i < n; i++)
		_indexes->removeObject((*set->_indexes)[i]);
}

void LIndexSet::removeAllIndexes()
{
	if (_indexes) {
		delete _indexes;
		_indexes = 0;
	}
}

LRange LIndexSet::_ownRangeForRange(LRange range)
{
	if (_indexes == 0)
		return LMakeRange((unsigned int)-1, 0);
	unsigned int from = range.location, to = range.location + range.length;
	int first = _indexes->insertIndexForObject(&from);
	int end = _indexes->insertIndexForObject(&to);
	return LMakeRange(first, end - first);
}

void LIndexSet::removeIndexesInRange(LRange range)
{
	if (_indexes == 0)
		return;
	LRange own = _ownRangeForRange(range);
	if (own.location != (unsigned int)-1)
		_indexes->removeObjectsInRange(own);
}

bool LIndexSet::containsIndex(unsigned int index) const
{
	if (_indexes == 0 || _indexes->count() == 0)
		return false;
	return _indexes->indexOfObject(index) != -1;
}

unsigned int LIndexSet::firstIndex()
{
	if (_indexes == 0 || _indexes->count() == 0)
		return (unsigned int)-1;
	return (*_indexes)[0];
}

unsigned int LIndexSet::lastIndex()
{
	if (_indexes == 0 || _indexes->count() == 0)
		return (unsigned int)-1;
	return _indexes->lastObject();
}

// The original read past the end when every index is below `index`.
unsigned int LIndexSet::indexLessThanIndex(unsigned int index)
{
	if (_indexes == 0 || _indexes->count() == 0)
		return (unsigned int)-1;
	int i = _indexes->insertIndexForObject(&index);
	if (i < (int)_indexes->count()) {
		unsigned int found = (*_indexes)[i];
		if (found != index)
			return found;
	}
	if (i != 0)
		return (*_indexes)[i - 1];
	return (unsigned int)-1;
}

unsigned int LIndexSet::indexGreaterThanIndex(unsigned int index)
{
	if (_indexes == 0 || _indexes->count() == 0)
		return (unsigned int)-1;
	unsigned int i = _indexes->insertIndexForObject(&index);
	unsigned int n = _indexes->count();
	if (i < n) {
		i += (*_indexes)[i] == index;
		if (i < n)
			return (*_indexes)[i];
	}
	return (unsigned int)-1;
}

} // namespace LF
