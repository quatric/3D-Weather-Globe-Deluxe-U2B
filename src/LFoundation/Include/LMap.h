// LMap: a dictionary from LComparable keys to LObjects, both retained.
// LMapIterator walks one (optionally keeping it retained); _LMapEnumerator
// is the LEnumerator over its keys or values.
#pragma once

#include "LString.h"

namespace LF {

class LMap : public LObject {
	LF_DECLARE_CLASS(LMap)
public:
	struct _LAddRemoveMapEntryFunc {
		template <typename E> static void add(E *e)
		{
			const_cast<LComparable *>(e->key)->retain();
			e->value->retain();
		}
		template <typename E> static void remove(E *e)
		{
			const_cast<LComparable *>(e->key)->release();
			e->value->release();
		}
		static const bool replace = true;
	};
	typedef TMap<const LComparable *, LObject *, LComparableKeyCallbacks, _LAddRemoveMapEntryFunc> _TLMapImp;

	LMap() {}
	virtual ~LMap() {}
	virtual LMap *init();
	virtual LMap *copy() const;
	virtual bool isEqualTo(const LObject *other) const;
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual LString *description() const;
	virtual LString *internalDescription() const;
	virtual LString *fillTextRepresentation(unsigned int indent) const;
	virtual void dealloc();
	virtual LString *_plistRepresentation(unsigned int indent) const;
	virtual LMap *getOwnerLObject() { return this; }

	// Factories (autoreleased)
	static LMap *map();
	// Values and keys alternate; the list ends with a 0.
	static LMap *mapWithObjectsAndKeys(LObject *object, const LComparable *key, ...);
	static LMap *mapWithMap(const LMap *other)
	{
		LMap *m = _alloc()->initWithMap(other);
		return m ? LAutorelease(m) : 0;
	}
	static LMap *mapWithTextRepresentation(const LData *data)
	{
		LMap *m = _alloc()->initWithTextRepresentation(data);
		return m ? LAutorelease(m) : 0;
	}
	static LMap *mapWithContentsOfFile(const LString *path)
	{
		LMap *m = _alloc();
		if (m && (m = m->initWithContentsOfFile(path)) != 0)
			return LAutorelease(m);
		return 0;
	}

	LMap *initWithMap(const LMap *other);
	LMap *initWithObjectsAndKeysList(LObject *object, const LComparable *key, va_list rest);
	LMap *initWithObjectsAndKeys(LObject **objects, const LComparable **keys, unsigned int count);
	LMap *initWithTextRepresentation(const LData *data);
	LMap *initWithContentsOfFile(const LString *path);
	bool writeToFile(const LString *path);

	bool isEmpty() const { return _imp.count() == 0; }
	unsigned int pairsCount() const { return _imp.count(); }
	LObject *objectForKey(const LComparable *key) const;
	void setObjectForKey(LObject *object, const LComparable *key) { _imp.setObjectForKey(object, key); }
	void removeObjectForKey(const LComparable *key);
	void removeAllObjects() { _imp.removeAllObjects(); }
	void preserveCapacity(unsigned int capacity) { _imp.reserve(capacity); }
	void addEntriesFromMap(const LMap *other);
	LEnumerator *objectEnumerator() const;
	LEnumerator *keyEnumerator() const;

	_TLMapImp _imp;
};

class LMapIterator {
public:
	// `retain` keeps the map alive while iterating.
	LMapIterator(const LMap *map, bool retain = true)
	    : _it(map ? &map->_imp : 0), _map(retain ? LRetain(const_cast<LMap *>(map)) : 0), _table(map ? &map->_imp : 0)
	{
	}
	~LMapIterator()
	{
		if (_map) {
			_map->release();
			_map = 0;
		}
	}
	unsigned int count() const { return _table ? _table->count() : 0; }
	void reset() { _it = LMap::_TLMapImp::hashTableIterator(_table); }
	bool next() { return _it.next(); }
	LObject *object() const { return _it.entry() ? _it.entry()->value : 0; }
	const LComparable *key() const { return _it.entry() ? _it.entry()->key : 0; }

	LMap::_TLMapImp::hashTableIterator _it;
	LMap *_map;
	const LMap::_TLMapImp *_table;
};

class _LMapEnumerator : public LEnumerator {
	LF_DECLARE_CLASS(_LMapEnumerator)
public:
	_LMapEnumerator() : _iterator(0), _keys(true) {}
	virtual void dealloc();
	virtual LObject *next();
	virtual void reset();
	virtual _LMapEnumerator *getOwnerLObject() { return this; }
	using LEnumerator::initWithContainer;
	_LMapEnumerator *initWithContainer(LObject *map, bool keys);

	LMapIterator *_iterator;
	bool _keys;
};

// A typed view of an LMap with V values and K keys.
template <typename V, typename K>
class TLMap : public LMap {
public:
	static TLMap *map() { return (TLMap *)LMap::map(); }
	static TLMap *mapWithObjectsAndKeys(V object, K key, ...)
	{
		va_list ap;
		va_start(ap, key);
		LMap *m = LMap::_alloc()->initWithObjectsAndKeysList(object, key, ap);
		va_end(ap);
		return (TLMap *)LAutorelease(m);
	}
	V objectForKey(K key) const { return (V)LMap::objectForKey(key); }
	void setObjectForKey(V object, K key) { LMap::setObjectForKey(object, key); }
	void removeObjectForKey(K key) { LMap::removeObjectForKey(key); }
};

void InsertIntValueForLSTRKey(LMap *map, int value, LString *key);
void InsertIntValueForKey(LMap *map, int value, const char *key);

} // namespace LF
