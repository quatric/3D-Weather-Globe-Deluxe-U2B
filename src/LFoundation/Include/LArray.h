// LArray: an ordered list of retained LObjects, kept in a CArray of
// pointers. _LArrayEnumerator walks one forwards or backwards.
#pragma once

#include "LString.h"

namespace LF {

class LArray : public LObject, public CArray {
	LF_DECLARE_CLASS(LArray)
public:
	LArray() : CArray(sizeof(LObject *)) {}
	virtual ~LArray() {}
	virtual LArray *init();
	virtual LArray *copy() const;
	virtual bool isEqualTo(const LObject *other) const;
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual LString *description() const;
	virtual LString *internalDescription() const;
	virtual LString *fillTextRepresentation(unsigned int indent) const;
	virtual void dealloc();
	virtual LString *_plistRepresentation(unsigned int indent) const;
	virtual LArray *getOwnerLObject() { return this; }

	// Factories (autoreleased)
	static LArray *array();
	static LArray *arrayWithCapacity(unsigned int capacity)
	{
		LArray *a = array();
		if (a)
			a->preserveCapacity(capacity);
		return a;
	}
	// The list ends with a 0.
	static LArray *arrayWithObjects(LObject *first, ...);
	static LArray *arrayWithContentsOfFile(const LString *path)
	{
		LArray *a = _alloc();
		if (a && (a = a->initWithContentsOfFile(path)) != 0)
			return LAutorelease(a);
		return 0;
	}
	static LArray *alloc_with_new() { return new LArray; }

	LArray *initWithCapacity(unsigned int capacity);
	LArray *initWithArray(const LArray *other);
	LArray *initWithObjectsList(LObject *first, va_list rest);
	LArray *initWithObjects(LObject *first, ...);
	LArray *initWithTextRepresentation(const LData *data);
	LArray *initWithContentsOfFile(const LString *path);
	bool writeToFile(const LString *path);

	unsigned int count() const { return _count; }
	LObject *objectAtIndex(int index) const { return ((LObject **)_data)[index]; }
	LObject *lastObject() const { return _count ? objectAtIndex(_count - 1) : 0; }
	LObject *const *objects() const { return (LObject *const *)_data; }
	int indexOfObject(const LObject *obj) const { return CArray::indexOfObject(&obj, false); }
	bool containsObject(const LObject *obj) const { return indexOfObject(obj) >= 0; }
	// Uses isEqualTo, despite the name.
	int indexOfObjectIdenticalTo(const LObject *obj) const;
	LString *componentsJoinedByString(LString *separator) const;
	LEnumerator *objectEnumerator() const;
	LEnumerator *reverseObjectEnumerator() const;

	void addObject(const LObject *obj);
	void insertObjectAtIndex(const LObject *obj, int index);
	void replaceObjectAtIndex(const LObject *obj, int index);
	void removeObjectAtIndex(int index);
	void removeObject(const LObject *obj);
	void removeObjectsInRange(LRange range);
	void removeAllObjectsAfter(int index);
	void removeAllObjects(unsigned int capacity = 0);

	void _copyDataFromArray(const LArray *other);
	void _addDataFromArray(const LArray *other);
	void _callRemoveForAllObjects();
	void _callRemoveForObjectsInRange(LRange range);
};

class _LArrayEnumerator : public LEnumerator {
	LF_DECLARE_CLASS(_LArrayEnumerator)
public:
	_LArrayEnumerator() : _index(0), _reverse(false) {}
	virtual LObject *next();
	virtual void reset();
	virtual _LArrayEnumerator *getOwnerLObject() { return this; }
	using LEnumerator::initWithContainer;
	_LArrayEnumerator *initWithContainer(const LArray *array, bool reverse);

	int _index;
	bool _reverse;
};

// A typed view of an LArray of T.
template <typename T>
class TLArray : public LArray {
public:
	static TLArray *array() { return (TLArray *)LArray::array(); }
	static TLArray *arrayWithCapacity(unsigned int capacity) { return (TLArray *)LArray::arrayWithCapacity(capacity); }
	TLArray *initWithObjects(T first, ...)
	{
		va_list ap;
		va_start(ap, first);
		initWithObjectsList(first, ap);
		va_end(ap);
		return this;
	}
	T objectAtIndex(int index) const { return (T)LArray::objectAtIndex(index); }
	T lastObject() const { return (T)LArray::lastObject(); }
	void addObject(T obj) { LArray::addObject(obj); }
	void insertObjectAtIndex(T obj, int index) { LArray::insertObjectAtIndex(obj, index); }
	void replaceObjectAtIndex(T obj, int index) { LArray::replaceObjectAtIndex(obj, index); }
	void removeObject(T obj) { LArray::removeObject(obj); }
	int indexOfObject(T obj) const { return LArray::indexOfObject(obj); }
};

// Licence-key helpers that live in LArray.cpp in the original.
int SHIFER_streamConvert(char c, unsigned int index, LString *publicKey, LString *ownKey);
bool fillMacAddrBuffer(unsigned char *buffer);
LString *SHIFER_ownKey();
LString *SHIFER_genPublicKey(LString *a, LString *b);

} // namespace LF
