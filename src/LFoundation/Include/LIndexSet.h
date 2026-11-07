// LIndexSet: a sorted list of unsigned indexes. (The range methods other than
// removal were left empty in the original.)
#pragma once

#include "LString.h"

namespace LF {

class LIndexSet : public LObject {
	LF_DECLARE_CLASS(LIndexSet)
public:
	LIndexSet() : _indexes(0) {}
	virtual ~LIndexSet() {}
	virtual LIndexSet *init() { return LObject::init() ? this : 0; }
	virtual void dealloc();
	virtual LIndexSet *getOwnerLObject() { return this; }

	LIndexSet *initWithIndex(unsigned int index);
	LIndexSet *initWithSet(LIndexSet *set);
	// Does nothing.
	LIndexSet *initWithIndexesInRange(LRange range) { return this; }

	void reserveCapacity(unsigned int capacity);
	void addIndex(unsigned int index);
	void addIndexes(const LIndexSet *set);
	// Does nothing.
	void addIndexesInRange(LRange range) {}
	void removeIndex(unsigned int index);
	void removeIndexes(const LIndexSet *set);
	void removeIndexesInRange(LRange range);
	void removeAllIndexes();

	bool isEmpty() const { return _indexes == 0 || _indexes->count() == 0; }
	unsigned int count() const { return _indexes ? _indexes->count() : 0; }
	bool containsIndex(unsigned int index) const;
	// These return (unsigned)-1 when there is none.
	unsigned int firstIndex();
	unsigned int lastIndex();
	unsigned int indexLessThanIndex(unsigned int index);
	unsigned int indexGreaterThanIndex(unsigned int index);

	static int _compareCallback(const void *a, const void *b, const void *context);
	void _createArray();
	// The positions in the array of the indexes in `range` (location -1 if
	// the set is empty).
	LRange _ownRangeForRange(LRange range);

	TArray<unsigned int> *_indexes;
};

} // namespace LF
