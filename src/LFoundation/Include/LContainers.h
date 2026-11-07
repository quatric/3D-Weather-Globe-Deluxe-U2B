// LFoundation's low-level containers: the byte block with inline storage
// (StPointerBlock), the untyped array (CArray) and its typed wrapper (TArray),
// and the chained hash table (TCoreHashTable) under TMap and TSet.
//
// The hash table keeps the original's algorithm (hash functions, bucket
// counts, where entries are linked, when it grows and shrinks), so iteration
// order is the original's for string and integer keys. Pointer keys hash by
// address, so their order was never stable.
#pragma once

#include "LFBase.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace LF {

// The original pooled small blocks in size classes; the system allocator does
// that already, so these only keep the API.
struct FixedAllocator {
	static void *alloc(size_t size) { return malloc(size ? size : 1); }
	static void *realloc(void *p, size_t size) { return ::realloc(p, size ? size : 1); }
	static void free(void *p) { ::free(p); }
};

struct LRange {
	unsigned int location;
	unsigned int length;
};
static inline LRange LMakeRange(unsigned int loc, unsigned int len) { LRange r = { loc, len }; return r; }
extern const LRange LZeroRange;
LRange LIntersectionRange(LRange a, LRange b);

// A growable buffer of T with room for Inline elements inside the object. It
// grows in steps that double with the size (_TAllocator<T, Inline, 4>).
template <typename T, unsigned Inline>
class _TAllocator {
public:
	_TAllocator() : _data(_inline), _count(0), _capacity(Inline), _step(Inline), _reserved(0) {}
	~_TAllocator() { if (_data != _inline) FixedAllocator::free(_data); }

	T *data() const { return _data; }
	unsigned int count() const { return _count; }

	void resize(unsigned int n)
	{
		if (n == 0) {
			_count = 0;
			if (_data != _inline) {
				FixedAllocator::free(_data);
				_data = _inline;
				_capacity = Inline;
			}
		} else if (n < _count) {
			if (_capacity - n >= _step * 2 && (_reserved == 0 || _reserved < n))
				_collapseToLength(n);
			else
				_count = n;
		} else if (n > _capacity) {
			_extendToLength(n);
		} else {
			_count = n;
		}
	}
	void preserveCapacity(unsigned int n)
	{
		if (_count < n) {
			resize(n);
			_step = n;
		}
		_reserved = n;
	}

private:
	_TAllocator(const _TAllocator &);
	_TAllocator &operator=(const _TAllocator &);

	static unsigned int _stepFor(unsigned int n)
	{
		unsigned int bits = 1;
		for (unsigned int v = (n >> 4) >> 1; v != 0; v >>= 1)
			bits++;
		unsigned int step = 1u << bits;
		return step < 16 ? 16 : step;
	}
	void _growTo(unsigned int n)
	{
		_step = _stepFor(n);
		_capacity = _step * (n / _step + 1);
		if (_data == _inline) {
			T *p = (T *)FixedAllocator::alloc(_capacity * sizeof(T));
			memcpy(p, _data, _count * sizeof(T));
			_data = p;
		} else {
			_data = (T *)FixedAllocator::realloc(_data, _capacity * sizeof(T));
		}
		_count = n;
	}
	void _collapseToLength(unsigned int n)
	{
		if (n < Inline) {
			if (_data != _inline) {
				memcpy(_inline, _data, n * sizeof(T));
				FixedAllocator::free(_data);
				_data = _inline;
				_step = Inline;
				_capacity = Inline;
			}
			_count = n;
		} else {
			_growTo(n);
		}
	}
	void _extendToLength(unsigned int n) { _growTo(n); }

	T *_data;
	unsigned int _count;
	T _inline[Inline];
	unsigned int _capacity;
	unsigned int _step;
	unsigned int _reserved;
};

// A block of bytes, kept inline while it is under 64 bytes.
class StPointerBlock {
public:
	StPointerBlock(unsigned int size = 0, bool zero = false)
	{
		setSize(size);
		if (zero)
			memset(bytes(), 0, size);
	}
	StPointerBlock(const void *src, unsigned int size)
	{
		setSize(size);
		if (src)
			memmove(bytes(), src, size);
	}
	~StPointerBlock() {}

	void *bytes() const { return _alloc.data(); }
	unsigned int size() const { return _alloc.count(); }
	void setSize(unsigned int size) { _alloc.resize(size); }
	void preserveCapacity(unsigned int size) { _alloc.preserveCapacity(size); }
	void append(const void *src, unsigned int len)
	{
		unsigned int old = size();
		_alloc.resize(old + len);
		memmove((char *)bytes() + old, src, len);
	}

private:
	StPointerBlock(const StPointerBlock &);
	StPointerBlock &operator=(const StPointerBlock &);
	_TAllocator<char, 64> _alloc;
};

typedef int (*CArrayComparator)(const void *a, const void *b, const void *context);

// An array of fixed-size elements. It can keep itself sorted: then inserts go
// to their sorted position. `compare` orders two elements; `compareToKey`
// takes (key, element) and compares the element against the key.
class CArray {
public:
	CArray(unsigned int elementSize = 4);
	CArray(const CArray &other);
	~CArray() {}
	CArray &operator=(const CArray &other);

	unsigned int count() const { return _count; }
	unsigned int elementSize() const { return _elementSize; }
	void *bytes() const { return _data; }
	void *objectAtIndex(int index) const { return (char *)_data + index * _elementSize; }

	void addObject(const void *obj);
	void addObjects(const void *objs, unsigned int count);
	void insertObjectAtIndex(const void *obj, int index);
	void replaceObjectAtIndex(const void *obj, int index);
	void removeObjectAtIndex(int index);
	void removeObject(const void *obj);
	void removeObjectsInRange(LRange range);
	void removeAllObjectsAfter(int index);
	void removeAllObjects(unsigned int capacity = 0);
	void preserveCapacity(unsigned int count);

	int indexOfObject(const void *obj, bool sorted = false) const;
	int indexOfKey(const void *key, bool sorted = false) const;
	int insertIndexForObject(const void *obj, bool sorted = false) const;
	int insertIndexForKey(const void *key, bool sorted = false) const;

	void sort();
	void setComparator(CArrayComparator compare, CArrayComparator compareToKey = 0, const void *context = 0)
	{
		_compare = compare;
		_compareToKey = compareToKey;
		_context = context;
	}
	void setKeepsSorted(bool keep)
	{
		_keepSorted = keep;
		if (keep && !_sorted)
			sort();
	}
	bool isSorted() const { return _sorted; }
	bool keepsSorted() const { return _keepSorted; }
	// Replaces only the element comparator, re-sorting if kept sorted.
	void setCompareFunc(CArrayComparator compare)
	{
		_compare = compare;
		if (_keepSorted)
			sort();
	}
	void setCompareContext(const void *context) { _context = context; }

	void _init();
	void _copyDataFromArray(const CArray *other);
	void _addDataFromArray(const CArray *other);

protected:
	int _b_search(const void *obj) const;
	int _b_key_search(const void *key) const;
	void _sync()
	{
		_data = _block.bytes();
		_bytes = _block.size();
		_count = _bytes / _elementSize;
	}

	StPointerBlock _block;
	unsigned int _elementSize;
	void *_data;
	unsigned int _bytes;
	unsigned int _count;
	bool _sorted;
	bool _keepSorted;
	CArrayComparator _compare;
	CArrayComparator _compareToKey;
	const void *_context;
};

int _DWORDComparatorFunc(const void *a, const void *b, const void *context);
int _SInt64ComparatorFunc(const void *a, const void *b, const void *context);

struct QSORT {
	static void sortInContext(void *base, unsigned int count, unsigned int size, const void *context,
	                          CArrayComparator compare);
	static void sort(void *base, unsigned int count, unsigned int size, CArrayComparator compare)
	{
		sortInContext(base, count, size, 0, compare);
	}
};

// TArray removal policies: called when an element leaves the array.
template <typename T>
struct _TArrayEmptyAddRemoveFunc {
	static void remove(T &) {}
	static const bool active = false;
};

// An array that owns its pointers: removing one deletes it.
template <typename T>
struct _TPtrArrayAddRemoveFunc {
	static void remove(T *&p) { delete p; }
	static const bool active = true;
};

template <typename T, typename F = _TArrayEmptyAddRemoveFunc<T> >
class TArray : public CArray {
public:
	TArray() : CArray(sizeof(T)) {}
	TArray(const TArray &other) : CArray(sizeof(T)) { _copyDataFromArray(&other); }
	~TArray() { _callRemoveForAllObjects(); }
	TArray &operator=(const TArray &other)
	{
		if (this != &other) {
			_callRemoveForAllObjects();
			_copyDataFromArray(&other);
		}
		return *this;
	}

	T *items() const { return (T *)_data; }
	T &operator[](int i) const { return ((T *)_data)[i]; }
	T &objectAtIndex(int i) const { return ((T *)_data)[i]; }
	T &lastObject() const { return ((T *)_data)[_count - 1]; }

	void addObject(const T &obj) { CArray::addObject(&obj); }
	void addObjects(const T *objs, unsigned int n) { CArray::addObjects(objs, n); }
	void insertObjectAtIndex(const T &obj, int index) { CArray::insertObjectAtIndex(&obj, index); }
	void replaceObjectAtIndex(const T &obj, int index)
	{
		F::remove(((T *)_data)[index]);
		CArray::replaceObjectAtIndex(&obj, index);
	}
	void removeObjectAtIndex(int index)
	{
		if (index >= 0 && index < (int)_count) {
			F::remove(((T *)_data)[index]);
			CArray::removeObjectAtIndex(index);
		}
	}
	void removeObject(const T &obj)
	{
		int i = indexOfObject(obj);
		if (i >= 0)
			removeObjectAtIndex(i);
	}
	void removeObjectsInRange(LRange r)
	{
		if (F::active)
			for (unsigned int i = r.location; i < r.location + r.length && i < _count; i++)
				F::remove(((T *)_data)[i]);
		CArray::removeObjectsInRange(r);
	}
	void removeAllObjectsAfter(int index)
	{
		if (F::active)
			for (int i = index + 1; i < (int)_count; i++)
				F::remove(((T *)_data)[i]);
		CArray::removeAllObjectsAfter(index);
	}
	void removeAllObjects(unsigned int capacity = 0)
	{
		_callRemoveForAllObjects();
		CArray::removeAllObjects(capacity);
	}
	int indexOfObject(const T &obj, bool sorted = false) const { return CArray::indexOfObject(&obj, sorted); }
	bool containsObject(const T &obj) const { return indexOfObject(obj) >= 0; }
	void _copyDataFromArray(const TArray *other) { CArray::_copyDataFromArray(other); }

private:
	void _callRemoveForAllObjects()
	{
		if (F::active)
			for (unsigned int i = 0; i < _count; i++)
				F::remove(((T *)_data)[i]);
	}
};

// ---------------------------------------------------------------------------
// Hash tables

static inline unsigned int _LCStringHash(const char *s, unsigned int maxLen)
{
	unsigned int h = 0;
	for (unsigned int i = 0; i < maxLen && s[i] != '\0'; i++) {
		h = (h << 4) + (int)(signed char)s[i];
		unsigned int g = h & 0xf0000000;
		h ^= g | (g >> 24);
	}
	return h;
}

template <unsigned N>
struct LCStringKeyCallbacks {
	static unsigned int hash(const char *k) { return _LCStringHash(k, N); }
	static bool equal(const char *stored, const char *k) { return strncmp(stored, k, N) == 0; }
};

struct LIntPtrKeyCallBacks {
	static unsigned int hash(intptr_t k)
	{
		uintptr_t v = (uintptr_t)k;
		unsigned int x = (unsigned int)v ^ (unsigned int)(v >> 32);
		return (unsigned int)((int)x >> 8 ^ (int)x ^ (int)x >> 16 ^ (int)x >> 24);
	}
	static bool equal(intptr_t stored, intptr_t k) { return stored == k; }
};

struct CHashTableHashEqualFuncs {
	static unsigned int hash(uintptr_t k) { return (unsigned int)(k >> 3); }
	static bool equal(uintptr_t stored, uintptr_t k) { return stored == k; }
};

template <typename T>
struct LPointerKeyCallbacks {
	static unsigned int hash(T k) { return (unsigned int)((uintptr_t)k >> 2); }
	static bool equal(T stored, T k) { return stored == k; }
};

// Value policies: add() when an entry goes in, remove() when it comes out.
// Replace says whether setting an existing key replaces its entry (only the
// dummy callbacks keep the old one).
struct LDummyValueCallbacks {
	template <typename E> static void add(E *) {}
	template <typename E> static void remove(E *) {}
	static const bool replace = false;
};

struct LNoValueCallbacks {
	template <typename E> static void add(E *) {}
	template <typename E> static void remove(E *) {}
	static const bool replace = true;
};

template <typename K, typename V>
struct TMapEntry {
	TMapEntry *next;
	TMapEntry *prev;
	K key;
	TMapEntry *_rnext;	// used while rehashing
	TMapEntry *_rprev;
	V value;
};

template <typename K>
struct TSetEntry {
	TSetEntry *next;
	TSetEntry *prev;
	K key;
	TSetEntry *_rnext;
	TSetEntry *_rprev;
};

template <typename K, typename E, typename KeyCB, typename ValueCB>
class TCoreHashTable {
public:
	typedef E Entry;

	TCoreHashTable() : _buckets(0), _count(0), _size(11), _shrinkBelow(0) {}
	~TCoreHashTable() { removeAllEntries(); }

	unsigned int count() const { return _count; }
	// Sizes the table for n entries and stops it shrinking.
	void reserve(unsigned int n)
	{
		_resizeHashTableTo(n < 8 ? 8 : n);
		_shrinkBelow = 0;
	}

	// The slot (bucket head or the previous entry's next) holding the entry.
	E **entryPtrAddrForKey(K key) const
	{
		if (_buckets == 0)
			return 0;
		E **slot = &_buckets->data()[KeyCB::hash(key) % _buckets->count()];
		for (; *slot != 0; slot = &(*slot)->next)
			if (KeyCB::equal((*slot)->key, key))
				return slot;
		return 0;
	}
	E *entryForKey(K key) const
	{
		E **slot = entryPtrAddrForKey(key);
		return slot ? *slot : 0;
	}

	void setEntry(E *entry)
	{
		if (_buckets == 0)
			_buckets = _newBuckets(_size);
		unsigned int h = KeyCB::hash(entry->key);
		unsigned int idx = h % _buckets->count();
		E *e = _buckets->data()[idx];
		if (e != 0) {
			E *last;
			do {
				last = e;
				if (KeyCB::equal(e->key, entry->key)) {
					if (ValueCB::replace) {
						ValueCB::add(entry);
						ValueCB::remove(e);
						e->key = entry->key;
						_copyValue(e, entry);
					}
					delete entry;
					return;
				}
				e = e->next;
			} while (e != 0);
			if (++_count > _size) {
				_resizeHashTableTo(_size + (_size >> 1));
				idx = h % _buckets->count();
				e = _buckets->data()[idx];
				if (e == 0) {
					entry->next = 0;
					entry->prev = 0;
					_buckets->data()[idx] = entry;
					ValueCB::add(entry);
					return;
				}
				do {
					last = e;
					e = e->next;
				} while (e != 0);
			}
			last->next = entry;
			entry->prev = last;
			entry->next = 0;
		} else {
			if (++_count > _size) {
				_resizeHashTableTo(_size + (_size >> 1));
				idx = h % _buckets->count();
				E *head = _buckets->data()[idx];
				if (head != 0) {
					entry->prev = 0;
					entry->next = head;
					head->prev = entry;
					_buckets->data()[idx] = entry;
					ValueCB::add(entry);
					return;
				}
			}
			entry->next = 0;
			entry->prev = 0;
			_buckets->data()[idx] = entry;
		}
		ValueCB::add(entry);
	}

	void removeEntryWithKey(K key)
	{
		if (_buckets == 0)
			return;
		E **slot = entryPtrAddrForKey(key);
		if (slot == 0)
			return;
		E *e = *slot;
		if (e->prev == 0)
			*slot = e->next;
		else
			e->prev->next = e->next;
		if (e->next != 0)
			e->next->prev = e->prev;
		_deleteEntry(e);
	}

	void removeAllEntries()
	{
		if (_buckets == 0)
			return;
		unsigned int n = _buckets->count();
		E **b = _buckets->data();
		for (unsigned int i = 0; i < n; i++) {
			E *e = b[i];
			if (e != 0) {
				// from the tail back, like the original
				while (e->next != 0)
					e = e->next;
				while (e != 0) {
					E *prev = e->prev;
					ValueCB::remove(e);
					delete e;
					e = prev;
				}
			}
			b[i] = 0;
		}
		_count = 0;
		delete _buckets;
		_buckets = 0;
	}

	E *firstAvailableEntry() const
	{
		if (_buckets != 0)
			for (unsigned int i = 0; i < _buckets->count(); i++)
				if (_buckets->data()[i] != 0)
					return _buckets->data()[i];
		return 0;
	}

	void _resizeHashTableTo(unsigned int n)
	{
		unsigned int oldN = _buckets ? _buckets->count() : 0;
		unsigned int newN = n < 8 ? 8 : n + 5;
		if (oldN == newN)
			return;
		_size = newN;
		_shrinkBelow = newN >> 2;
		if ((_shrinkBelow & 1) == 0)
			_shrinkBelow--;
		if (newN < 9)
			_shrinkBelow = 0;
		Buckets *nb = _newBuckets(newN);
		if (_buckets == 0) {
			_buckets = nb;
			return;
		}
		E **ob = _buckets->data();
		E **b = nb->data();
		for (unsigned int i = 0; i < oldN; i++) {
			for (E *e = ob[i]; e != 0; e = e->next) {
				E **slot = &b[KeyCB::hash(e->key) % newN];
				e->_rprev = 0;
				e->_rnext = *slot;
				if (*slot != 0)
					(*slot)->_rprev = e;
				*slot = e;
			}
		}
		for (unsigned int i = 0; i < newN; i++)
			for (E *e = b[i]; e != 0; e = e->_rnext) {
				e->next = e->_rnext;
				e->prev = e->_rprev;
			}
		delete _buckets;
		_buckets = nb;
	}

	// Walks the entries bucket by bucket.
	class hashTableIterator {
	public:
		hashTableIterator(const TCoreHashTable *table) : _table(table), _entry(0), _bucket(-1) {}
		bool next()
		{
			if ((_entry == 0 && _bucket != -1) || _table == 0 || _table->_buckets == 0)
				return false;
			unsigned int n = _table->_buckets->count();
			E **b = _table->_buckets->data();
			if (_entry == 0) {
				for (_bucket = 0; (unsigned int)_bucket < n; _bucket++)
					if (b[_bucket] != 0) {
						_entry = b[_bucket];
						break;
					}
			} else {
				_entry = _entry->next;
				if (_entry != 0)
					return true;
				for (_bucket++; (unsigned int)_bucket < n; _bucket++)
					if (b[_bucket] != 0) {
						_entry = b[_bucket];
						break;
					}
			}
			return _entry != 0;
		}
		E *entry() const { return _entry; }

	private:
		const TCoreHashTable *_table;
		E *_entry;
		int _bucket;
	};

protected:
	typedef _TAllocator<E *, 8> Buckets;

	static Buckets *_newBuckets(unsigned int n)
	{
		Buckets *b = new Buckets;
		if (n != 0)
			b->preserveCapacity(n);
		b->resize(n);
		memset(b->data(), 0, n * sizeof(E *));
		return b;
	}
	void _deleteEntry(E *e)
	{
		ValueCB::remove(e);
		delete e;
		if (--_count < _shrinkBelow)
			_resizeHashTableTo((_shrinkBelow + _size) >> 1);
	}
	template <typename X> static void _copyValue(TMapEntry<K, X> *dst, TMapEntry<K, X> *src) { dst->value = src->value; }
	static void _copyValue(TSetEntry<K> *, TSetEntry<K> *) {}

	Buckets *_buckets;
	unsigned int _count;
	unsigned int _size;
	unsigned int _shrinkBelow;

private:
	TCoreHashTable(const TCoreHashTable &);
	TCoreHashTable &operator=(const TCoreHashTable &);
};

template <typename K, typename V, typename KeyCB, typename ValueCB = LNoValueCallbacks>
class TMap : public TCoreHashTable<K, TMapEntry<K, V>, KeyCB, ValueCB> {
	typedef TCoreHashTable<K, TMapEntry<K, V>, KeyCB, ValueCB> Base;

public:
	typedef TMapEntry<K, V> Entry;

	void setObjectForKey(V value, K key)
	{
		Entry *e = new Entry;
		e->value = value;
		e->key = key;
		this->setEntry(e);
	}
	V objectForKey(K key) const
	{
		Entry *e = this->entryForKey(key);
		return e ? e->value : V();
	}
	bool getObjectForKey(K key, V *value) const
	{
		Entry *e = this->entryForKey(key);
		if (e && value)
			*value = e->value;
		return e != 0;
	}
	void removeObjectForKey(K key) { this->removeEntryWithKey(key); }
	void removeAllObjects() { this->removeAllEntries(); }
};

template <typename K, typename KeyCB, typename ValueCB = LNoValueCallbacks>
class TSet : public TCoreHashTable<K, TSetEntry<K>, KeyCB, ValueCB> {
public:
	typedef TSetEntry<K> Entry;

	void addObject(K key)
	{
		if (this->entryPtrAddrForKey(key) == 0) {
			Entry *e = new Entry;
			e->key = key;
			this->setEntry(e);
		}
	}
	bool containsObject(K key) const { return this->entryPtrAddrForKey(key) != 0; }
	void removeObject(K key) { this->removeEntryWithKey(key); }
	void removeAllObjects() { this->removeAllEntries(); }
};

// A string-keyed map that doesn't own its keys or values.
template <typename V, unsigned N = 1024>
class TStrMap : public TMap<const char *, V, LCStringKeyCallbacks<N>, LNoValueCallbacks> {
};

// An untyped integer map and set, used where the original stored pointers as ints.
class CHashTable {
public:
	CHashTable() {}
	CHashTable(int capacity) { _imp.reserve(capacity); }
	virtual ~CHashTable() {}
	void setValueForKey(intptr_t value, intptr_t key) { _imp.setObjectForKey(value, key); }
	bool getValueForKey(intptr_t *value, intptr_t key) const
	{
		intptr_t v;
		bool found = _imp.getObjectForKey(key, &v);
		if (found && value)
			*value = v;
		return found;
	}
	void removeValueForKey(intptr_t key) { _imp.removeObjectForKey(key); }
	void removeAllValues() { _imp.removeAllObjects(); }
	unsigned int pairsCount() const { return _imp.count(); }
	bool isEmpty() const { return _imp.count() == 0; }

	typedef TMap<uintptr_t, intptr_t, CHashTableHashEqualFuncs> Imp;
	Imp _imp;
};

class CHashTableIterator {
public:
	CHashTableIterator(const CHashTable *table) : _it(&table->_imp) {}
	bool next() { return _it.next(); }
	intptr_t key() const { return _it.entry() ? (intptr_t)_it.entry()->key : 0; }
	intptr_t value() const { return _it.entry() ? _it.entry()->value : 0; }

private:
	CHashTable::Imp::hashTableIterator _it;
};

class CHashSet {
public:
	CHashSet() {}
	virtual ~CHashSet() {}
	void addObject(intptr_t obj) { _imp.addObject(obj); }
	void removeObject(intptr_t obj) { _imp.removeObject(obj); }
	bool containsObject(intptr_t obj) const { return _imp.containsObject(obj); }
	void removeAllObjects() { _imp.removeAllObjects(); }
	unsigned int itemsCount() const { return _imp.count(); }
	bool isEmpty() const { return _imp.count() == 0; }
	void reserveCapacity(unsigned int n) { _imp.reserve(n); }
	void getAllObjects(TArray<intptr_t> *out) const;

	typedef TSet<uintptr_t, CHashTableHashEqualFuncs> Imp;
	Imp _imp;
};

class CHashSetIterator {
public:
	CHashSetIterator(const CHashSet *set) : _it(&set->_imp) {}
	bool next() { return _it.next(); }
	intptr_t object() const { return _it.entry() ? (intptr_t)_it.entry()->key : 0; }

private:
	CHashSet::Imp::hashTableIterator _it;
};

} // namespace LF
