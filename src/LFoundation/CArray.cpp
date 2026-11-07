// CArray
//
// LF::CArray: StPointerBlock storage, element size, data/byte/element counts,
// sorted flags, comparators (i386 layout 0x54..0x70).

#include "LContainers.h"

namespace LF {

int _DWORDComparatorFunc(const void *a, const void *b, const void *)
{
	return *(const int *)a - *(const int *)b;
}

int _SInt64ComparatorFunc(const void *a, const void *b, const void *)
{
	int64_t d = *(const int64_t *)a - *(const int64_t *)b;
	return d < 0 ? -1 : (d > 0 ? 1 : 0);
}

void CArray::_init()
{
	_elementSize = 4;
	_data = 0;
	_bytes = 0;
	_count = 0;
	_sorted = false;
	_keepSorted = false;
	_compare = _DWORDComparatorFunc;
	_compareToKey = 0;
	_context = 0;
}

CArray::CArray(unsigned int elementSize)
{
	_init();
	_elementSize = elementSize;
	_sync();
}

CArray::CArray(const CArray &other)
{
	_init();
	_copyDataFromArray(&other);
}

CArray &CArray::operator=(const CArray &other)
{
	_copyDataFromArray(&other);
	return *this;
}

int CArray::insertIndexForObject(const void *obj, bool sorted) const
{
	if (!sorted && !_sorted)
		return _count;
	int mid = 0, low = 0, high = (int)_count - 1, cmp = 0;
	while (low <= high) {
		mid = (low + high) / 2;
		if (_compare == 0)
			return mid;
		cmp = _compare(obj, (char *)_data + mid * _elementSize, _context);
		if (cmp == 0)
			return mid;
		if (cmp < 0)
			high = mid - 1;
		else
			low = mid + 1;
	}
	if (cmp > 0)
		mid++;
	return mid;
}

int CArray::insertIndexForKey(const void *key, bool sorted) const
{
	if (!sorted && !_sorted)
		return _count;
	int mid = 0, low = 0, high = (int)_count - 1, cmp = 0;
	while (low <= high) {
		mid = (low + high) / 2;
		if (_compareToKey == 0)
			return mid;
		cmp = _compareToKey(key, (char *)_data + mid * _elementSize, _context);
		if (cmp == 0)
			return mid;
		if (cmp < 0)
			high = mid - 1;
		else
			low = mid + 1;
	}
	if (cmp > 0)
		mid++;
	return mid;
}

void CArray::removeObjectAtIndex(int index)
{
	int n = _count;
	if (n == 0 || index == -1)
		return;
	if (index < 0)
		index = 0;
	else if (index > n)
		index = n;
	if (index < n - 1)
		memmove((char *)_data + index * _elementSize, (char *)_data + (index + 1) * _elementSize,
		        (n - index - 1) * _elementSize);
	_block.setSize((_count - 1) * _elementSize);
	_sync();
}

void CArray::sort()
{
	if (_compare == 0)
		LF_RAISE(NilPtrException());
	QSORT::sortInContext(_data, _count, _elementSize, _context, _compare);
	_sorted = true;
}

void CArray::insertObjectAtIndex(const void *obj, int index)
{
	int at;
	if (index < 0)
		at = 0;
	else
		at = index <= (int)_count ? index : (int)_count;
	if (!_keepSorted) {
		_sorted = false;
	} else {
		if (!_sorted)
			sort();
		at = insertIndexForObject(obj, false);
	}
	_block.setSize((_count + 1) * _elementSize);
	_sync();
	if (at < (int)_count - 1)
		memmove((char *)_data + (at + 1) * _elementSize, (char *)_data + at * _elementSize,
		        (_count - at - 1) * _elementSize);
	memmove((char *)_data + at * _elementSize, obj, _elementSize);
}

void CArray::addObject(const void *obj)
{
	if (_keepSorted) {
		insertObjectAtIndex(obj, 0);
		return;
	}
	_block.setSize((_count + 1) * _elementSize);
	_sorted = false;
	_data = _block.bytes();
	memmove((char *)_data + _count * _elementSize, obj, _elementSize);
	_sync();
}

// Linear search calls compare(element, obj); the sorted path is a binary search.
int CArray::indexOfObject(const void *obj, bool sorted) const
{
	if (_compare == 0)
		LF_RAISE(NilPtrException());
	if ((int)_count > 0) {
		if (_sorted || sorted)
			return _b_search(obj);
		const char *p = (const char *)_data;
		for (int i = 0; i < (int)_count; i++, p += _elementSize) {
			if (_compare == 0 || _compare(p, obj, _context) == 0)
				return i;
		}
	}
	return -1;
}

int CArray::indexOfKey(const void *key, bool sorted) const
{
	if (_compareToKey == 0)
		LF_RAISE(NilPtrException());
	if ((int)_count > 0) {
		if (_sorted || sorted)
			return _b_key_search(key);
		const char *p = (const char *)_data;
		for (int i = 0; i < (int)_count; i++, p += _elementSize) {
			if (_compareToKey == 0 || _compareToKey(key, p, _context) == 0)
				return i;
		}
	}
	return -1;
}

// The binary searches call compare(key, element) and step left when it is
// positive, as the original does: its key comparators compare the element
// against the key.
int CArray::_b_search(const void *obj) const
{
	int low = 0, high = (int)_count - 1;
	while (low <= high) {
		int mid = (low + high) / 2;
		if (_compare == 0)
			return mid;
		int cmp = _compare(obj, (char *)_data + mid * _elementSize, _context);
		if (cmp == 0)
			return mid;
		if (cmp > 0)
			high = mid - 1;
		else
			low = mid + 1;
	}
	return -1;
}

int CArray::_b_key_search(const void *key) const
{
	int low = 0, high = (int)_count - 1;
	while (low <= high) {
		int mid = (low + high) / 2;
		if (_compareToKey == 0)
			return mid;
		int cmp = _compareToKey(key, (char *)_data + mid * _elementSize, _context);
		if (cmp == 0)
			return mid;
		if (cmp > 0)
			high = mid - 1;
		else
			low = mid + 1;
	}
	return -1;
}

void CArray::preserveCapacity(unsigned int count)
{
	_block.preserveCapacity(count * _elementSize);
	_data = _block.bytes();
}

void CArray::replaceObjectAtIndex(const void *obj, int index)
{
	memmove((char *)_data + index * _elementSize, obj, _elementSize);
}

void CArray::removeObjectsInRange(LRange range)
{
	if (range.location == (unsigned int)-1 || range.length == 0)
		return;
	unsigned int end = range.location + range.length;
	if (end > _count)
		LF_RAISE(WrongConditionException());
	if (end < _count)
		memmove((char *)_data + range.location * _elementSize, (char *)_data + end * _elementSize,
		        (_count - end) * _elementSize);
	_block.setSize((_count - range.length) * _elementSize);
	_sync();
}

void CArray::removeAllObjectsAfter(int index)
{
	if (_count == 0)
		return;
	if (index < 0)
		index = -1;
	else if (index >= (int)_count)
		return;
	_block.setSize((index + 1) * _elementSize);
	_sync();
}

void CArray::removeAllObjects(unsigned int capacity)
{
	_block.setSize(0);
	_sync();
	preserveCapacity(capacity);
}

void CArray::addObjects(const void *objs, unsigned int count)
{
	preserveCapacity(count + _count);
	for (unsigned int i = 0; i < count; i++)
		addObject((const char *)objs + i * _elementSize);
}

void CArray::_copyDataFromArray(const CArray *other)
{
	if (this == other)
		return;
	unsigned int n = other->_count;
	_elementSize = other->_elementSize;
	_block.setSize(_elementSize * n);
	_sync();
	memcpy(_data, other->_data, _bytes);
	_sorted = other->_sorted;
	_keepSorted = other->_keepSorted;
	_compare = other->_compare;
	_compareToKey = other->_compareToKey;
	_context = other->_context;
}

void CArray::_addDataFromArray(const CArray *other)
{
	if (this == other || other == 0)
		return;
	if (other->_elementSize != _elementSize)
		LF_RAISE(WrongConditionException());
	unsigned int oldBytes = _bytes, oldCount = _count;
	_block.setSize(_block.size() + other->_count * _elementSize);
	_sync();
	memcpy((char *)_data + oldCount * _elementSize, other->_data, _bytes - oldBytes);
}

void CArray::removeObject(const void *obj)
{
	int i = indexOfObject(obj, false);
	if (i >= 0)
		removeObjectAtIndex(i);
}

} // namespace LF
