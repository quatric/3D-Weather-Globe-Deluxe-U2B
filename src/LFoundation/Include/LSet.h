// LSet: an unordered set of retained LObjects. A plain LSet compares by
// identity; LComparableSet (the same class with _comparable set) uses the
// objects' hashValue and isEqualTo.
#pragma once

#include "LString.h"

namespace LF {

class LSet : public LObject {
	LF_DECLARE_CLASS(LSet)
public:
	struct _LAddRemoveEntryFunc {
		template <typename E> static void add(E *e) { const_cast<LObject *>((const LObject *)e->key)->retain(); }
		template <typename E> static void remove(E *e) { const_cast<LObject *>((const LObject *)e->key)->release(); }
		static const bool replace = false;
	};
	typedef TSet<LObject *, LPointerKeyCallbacks<LObject *>, _LAddRemoveEntryFunc> TLSetImp;
	typedef TSet<const LComparable *, LComparableKeyCallbacks, _LAddRemoveEntryFunc> TLComparableSetImp;

	LSet() : _set(0), _comparable(false) {}
	virtual ~LSet() {}
	virtual LSet *init();
	virtual LSet *copy() const;
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual LString *description() const;
	virtual LString *internalDescription() const;
	virtual void dealloc();
	virtual LSet *getOwnerLObject() { return this; }

	static LSet *set()
	{
		LSet *s = _alloc();
		if (s && s->init())
			return LAutorelease(s);
		return 0;
	}
	LSet *initWithArray(LArray *array);
	LSet *initWithObjectsList(LObject *first, va_list rest);
	LSet *initWithSet(LSet *set);

	unsigned int count() const { return _comparable ? _cset()->count() : _oset()->count(); }
	bool isEmpty() const { return count() == 0; }
	LObject *anyObject() const;
	// The member equal to `obj`.
	LObject *ownObjectForObject(const LObject *obj) const;
	bool containsObject(const LObject *obj) const;
	void addObject(LObject *obj);
	void removeObject(LObject *obj);
	void removeAllObjects();
	void preserveCapacity(unsigned int capacity);
	void addObjectsFromArray(const LArray *array);
	void unionSet(const LSet *set);
	void minusSet(const LSet *set);
	LArray *allObjects() const;
	LEnumerator *objectEnumerator() const;

	TLSetImp *_oset() const { return (TLSetImp *)_set; }
	TLComparableSetImp *_cset() const { return (TLComparableSetImp *)_set; }

	void *_set;	// a TLSetImp or, if _comparable, a TLComparableSetImp
	bool _comparable;
};

class LComparableSet : public LSet {
	LF_DECLARE_CLASS(LComparableSet)
public:
	LComparableSet() { _comparable = true; }
	virtual ~LComparableSet() {}
	virtual LComparableSet *init()
	{
		LSet::init();
		return this;
	}
	virtual LSet *copy() const { return LSet::copy(); }
	virtual LComparableSet *getOwnerLObject() { return this; }
	static LComparableSet *set()
	{
		LComparableSet *s = _alloc();
		if (s && s->init())
			return LAutorelease(s);
		return 0;
	}
};

class LSetIterator {
public:
	LSetIterator(const LSet *set, bool retain = true);
	~LSetIterator();
	// 0 at the end.
	LObject *nextObject();
	void toBegin();
	unsigned int count() const;

	void *_it;	// the hashTableIterator of the set's kind
	const LSet *_set;
	LSet *_retained;
	bool _comparable;
};

class _LSetEnumerator : public LEnumerator {
	LF_DECLARE_CLASS(_LSetEnumerator)
public:
	_LSetEnumerator() : _iterator(0) {}
	virtual void dealloc();
	virtual LObject *next() { return _iterator->nextObject(); }
	virtual void reset() { _iterator->toBegin(); }
	virtual _LSetEnumerator *getOwnerLObject() { return this; }
	virtual _LSetEnumerator *initWithContainer(LObject *set);

	LSetIterator *_iterator;
};

} // namespace LF
