// LClass
//
// LF::LClass: the class registry (sorted by name), each class's selector
// tables, outlets, key-value coding selector lookup, metadata and singleton.
//
// A class's selectors live in two maps: one keyed by selector string
// contents, and a cache keyed by the selector pointer a caller used (so
// repeated calls with the same string skip the string compare).
// Selectors found in a superclass are copied into the subclass's maps.

#include "LCleanupTask.h"
#include "LObject.h"
#include "LString.h"

namespace LF {

// ---------------------------------------------------------------------------
// Selector maps

namespace {

struct _methodsHashEqualFuncConst {
	static unsigned int hash(const char *k) { return (unsigned int)((intptr_t)k >> 1); }
	static bool equal(const char *stored, const char *k) { return stored == k; }
};

typedef TMap<const char *, LMethodPtr, _methodsHashEqualFuncConst> _clImpConst;
typedef TMap<const char *, LMethodPtr, LCStringKeyCallbacks<64>> _clImpRegular;
typedef TMap<const char *, LObject *, LCStringKeyCallbacks<64>, LObjectValueCallbacks> _clImpSignatures;

// Outlets are looked up with strcmp on the first 100 characters' hash.
struct _OutletMapCallBacks {
	static unsigned int hash(const char *k) { return _LCStringHash(k, 100); }
	static bool equal(const char *stored, const char *k) { return strcmp(stored, k) == 0; }
};

} // namespace

class LClass::LSelectorsMap {
public:
	LSelectorsMap() : _const(new _clImpConst), _regular(new _clImpRegular), _signatures(new _clImpSignatures) {}
	~LSelectorsMap()
	{
		delete _const;
		delete _regular;
		delete _signatures;
	}

	void addFuncPtr(LMethodPtr method, LObject *signature, const char *sel)
	{
		_regular->setObjectForKey(method, sel);
		_const->setObjectForKey(method, sel);
		if (signature)
			_signatures->setObjectForKey(signature, sel);
	}
	LMethodPtr lookupFuncPtr(const char *sel) const
	{
		LMethodPtr method = _const->objectForKey(sel);
		if (method == 0) {
			method = _regular->objectForKey(sel);
			if (method != 0)
				_const->setObjectForKey(method, sel);
		}
		return method;
	}
	LObject *lookupSignature(const char *sel) const { return _signatures->objectForKey(sel); }
	void removeFuncPtrForKey(const char *sel)
	{
		_regular->removeObjectForKey(sel);
		_const->removeObjectForKey(sel);
	}

	_clImpConst *_const;
	_clImpRegular *_regular;
	_clImpSignatures *_signatures;
};

namespace {

typedef TMap<const char *, LClass::LOutletInstaller *, _OutletMapCallBacks> _OutletMapTable;
typedef TMap<LString *, LClass::_SKVCSelectorEntry, LComparableKeyCallbacks, LObjectKeyCallbacks>
    _KVCSelectorsMapTable;
typedef TMap<const char *, intptr_t, LCStringKeyCallbacks<64>, LDummyValueCallbacks> _MetadataMap;

int _classCompare(const void *a, const void *b, const void *)
{
	return strcmp((*(LClass *const *)a)->name(), (*(LClass *const *)b)->name());
}

// Called as compareToKey(name, element); returns strcmp(element, name) as
// the original did, which the binary search expects.
int _classCompareToKey(const void *key, const void *elem, const void *)
{
	return strcmp((*(LClass *const *)elem)->name(), (const char *)key);
}

// The class list and its lock, created by the first class to register
// (during static initialisation).
CSharableResource *sClLocker;
TArray<LClass *> *sClassesArray;

} // namespace

static _OutletMapTable *_outletMap(const LClass *cls)
{
	return (_OutletMapTable *)cls->_outlets;
}

void LClass::addClass(LClass *cls)
{
	if (sClLocker == 0)
		sClLocker = new CSharableResource;
	StSharableLocker lock(sClLocker);
	if (sClassesArray == 0) {
		sClassesArray = new TArray<LClass *>;
		sClassesArray->setComparator(_classCompare, _classCompareToKey);
		sClassesArray->setKeepsSorted(true);
	}
	sClassesArray->addObject(cls);
}

LClass::LClass(const char *name, LClass *superclass, unsigned int instanceSize)
{
	_selectors = new LSelectorsMap;
	size_t n = strlen(name);
	if (n > 255)
		n = 255;
	memcpy(_name, name, n);
	_name[n] = '\0';
	_superclass = superclass;
	_singleton = 0;
	_instanceSize = instanceSize;
	_singletonLock = new CSharableResource;
	_outlets = new _OutletMapTable;
	_setKVCCache = 0;
	_getKVCCache = 0;
	_metadata = 0;
	_protocols = new ProtocolIDsList(true);
	addClass(this);
}

LClass::~LClass()
{
	if (_singleton) {
		_singleton->release();
		_singleton = 0;
	}
	delete _outletMap(this);
	_outlets = 0;
	delete _protocols;
	_protocols = 0;
	delete (_KVCSelectorsMapTable *)_setKVCCache;
	_setKVCCache = 0;
	delete (_KVCSelectorsMapTable *)_getKVCCache;
	_getKVCCache = 0;
	delete _metadata;
	_metadata = 0;
	StSharableLocker lock(sClLocker);
	LClass *self = this;
	sClassesArray->removeObject(self);
	delete _singletonLock;
	_singletonLock = 0;
	delete _selectors;
	_selectors = 0;
}

unsigned int LClass::classesCount()
{
	return sClassesArray ? sClassesArray->count() : 0;
}

LClass *LClass::classAtIndex(long index)
{
	return sClassesArray ? (*sClassesArray)[index] : 0;
}

LClass *LClass::classByName(const char *name)
{
	if (sClLocker == 0)
		sClLocker = new CSharableResource;
	StSharableLocker lock(sClLocker);
	if (name != 0 && sClassesArray != 0) {
		int i = sClassesArray->indexOfKey(name, false);
		if (i >= 0 && i < (int)sClassesArray->count())
			return (*sClassesArray)[i];
	}
	return 0;
}

LClass *LClass::classByName(LString *name)
{
	return name ? classByName(name->cString()) : 0;
}

void LClass::_initializeClass()
{
}

bool LClass::isAbstract()
{
	return false;
}

void LClass::_addProtocolWithID(LProtocolID *protocol)
{
	if (protocol)
		_protocols->addID(protocol);
}

bool LClass::isSubclassOf(LClass *other)
{
	if (other == 0)
		return false;
	for (LClass *c = this; c; c = c->_superclass)
		if (c == other)
			return true;
	return false;
}

bool LClass::implementsProtocol(LProtocolID *protocol)
{
	for (LClass *c = this; c; c = c->_superclass)
		if (c->_protocols->containsID(protocol))
			return true;
	return false;
}

// ---------------------------------------------------------------------------
// Outlets

LClass::LOutletInstaller::LOutletInstaller(LClass *cls, const LTypeID &type, const char *name, intptr_t offset)
    : _class(cls), _type(new LTypeID(type)), _name(name), _offset(offset)
{
	_outletMap(cls)->setObjectForKey(this, name);
}

LClass::LOutletInstaller::~LOutletInstaller()
{
	_outletMap(_class)->removeObjectForKey(_name);
	delete _type;
	_type = 0;
}

bool LClass::LOutletInstaller::getValueOfOwner(LObject *owner, const LTypeID &type, void *out)
{
	if (owner == 0)
		return false;
	return _type->convertValue((char *)owner + _offset, type, out);
}

// Object outlets hold a reference: the new value is retained and the old one
// released.
bool LClass::LOutletInstaller::setValueOfOwner(LObject *value, LObject *owner)
{
	if (owner == 0)
		return false;
	void *slot = (char *)owner + _offset;
	if (value == 0) {
		memset(slot, 0, _type->getArgSize());
		return true;
	}
	LTypeID valueType(value->getClass());
	LObject *old = _type->isLObject() ? *(LObject **)slot : 0;
	bool ok = valueType.convertValue(&value, *_type, slot);
	if (_type->isLObject() && *(LObject **)slot != 0)
		(*(LObject **)slot)->retain();
	if (old)
		old->release();
	return ok;
}

// Misses are remembered as null entries. The original keyed them by the
// string's temporary C string; here the key is copied (and never freed).
LClass::LOutletInstaller *LClass::_outletForKey(LString *key)
{
	const char *name = key->cString();
	_OutletMapTable *map = _outletMap(this);
	if (map->entryPtrAddrForKey(name) != 0)
		return map->objectForKey(name);
	LOutletInstaller *outlet = 0;
	for (LClass *c = _superclass; c; c = c->_superclass) {
		outlet = _outletMap(c)->objectForKey(name);
		if (outlet)
			break;
	}
	map->setObjectForKey(outlet, outlet ? outlet->_name : strdup(name));
	return outlet;
}

// ---------------------------------------------------------------------------
// Metadata

void LClass::setMetadataForKey(intptr_t value, const char *key)
{
	if (_metadata == 0)
		_metadata = new _MetadataMap;
	_metadata->setObjectForKey(value, key);
}

intptr_t LClass::metadataForKey(const char *key, bool inherited)
{
	LClass *c = this;
	_MetadataMap *map = _metadata;
	if (inherited) {
		while (map == 0 || map->entryPtrAddrForKey(key) == 0) {
			c = c->_superclass;
			if (c == 0)
				return 0;
			map = c->_metadata;
		}
	}
	return map ? map->objectForKey(key) : 0;
}

// ---------------------------------------------------------------------------
// Selectors

void LClass::_removeSelector(const char *sel)
{
	if (_selectors)
		_selectors->removeFuncPtrForKey(sel);
}

LMethodPtr LClass::_funcPtrForSelector(const char *sel)
{
	LMethodPtr method = _selectors->lookupFuncPtr(sel);
	if (method != 0)
		return method;
	for (LClass *c = _superclass; c; c = c->_superclass) {
		method = c->_selectors->lookupFuncPtr(sel);
		if (method != 0) {
			_selectors->addFuncPtr(method, c->_selectors->lookupSignature(sel), sel);
			return method;
		}
	}
	return 0;
}

LObject *LClass::_signatureForSelector(const char *sel)
{
	LObject *signature = _selectors->lookupSignature(sel);
	if (signature)
		return signature;
	if (_funcPtrForSelector(sel) != 0)
		return _selectors->lookupSignature(sel);
	return 0;
}

void LClass::_addSelector(const char *sel, LMethodPtr method, LObject *signature)
{
	if (_selectors == 0)
		_selectors = new LSelectorsMap;
	_selectors->addFuncPtr(method, signature, sel);
}

// "key" -> "Key"
static LString *_capitalizedKey(LString *key)
{
	LString *rest = key->substringFromIndex(1);
	return LString::stringWithFormat("%@%@", key->substringWithRange(0, 1)->uppercaseString(), rest);
}

static const char *const kNoSelector = (const char *)-1;

static void _cacheMiss(void **cache, LString *key)
{
	if (*cache == 0)
		*cache = new _KVCSelectorsMapTable;
	LClass::_SKVCSelectorEntry entry = { kNoSelector, 0 };
	((_KVCSelectorsMapTable *)*cache)->setObjectForKey(entry, LString::stringWithString(key));
}

static bool _cachedEntry(void *cache, LString *key, LClass::_SKVCSelectorEntry *entry)
{
	entry->sel = 0;
	entry->signature = 0;
	if (cache)
		*entry = ((_KVCSelectorsMapTable *)cache)->objectForKey(key);
	return entry->sel != kNoSelector;
}

// Tries setKey, then sel_SetKey.
bool LClass::_setValueParamsForKey(LString *key, const char **sel, LObject **signature)
{
	StSharableLocker lock(&_lock);
	_SKVCSelectorEntry entry;
	if (!_cachedEntry(_setKVCCache, key, &entry))
		return false;
	if (entry.sel == 0) {
		LString *cap = _capitalizedKey(key);
		const char *s = LSelectorFromString(LString::stringWithFormat("set%@", cap));
		if (_funcPtrForSelector(s) == 0) {
			s = LSelectorFromString(LString::stringWithFormat("sel_Set%@", cap));
			if (_funcPtrForSelector(s) == 0) {
				_cacheMiss(&_setKVCCache, key);
				return false;
			}
		}
		entry.sel = s;
		entry.signature = _signatureForSelector(s);
	}
	*sel = entry.sel;
	if (signature)
		*signature = entry.signature;
	return true;
}

// Tries key, getKey, sel_Key, sel_GetKey, _key, then _getKey.
bool LClass::_getValueParamsForKey(LString *key, const char **sel, LObject **signature)
{
	StSharableLocker lock(&_lock);
	_SKVCSelectorEntry entry;
	if (!_cachedEntry(_getKVCCache, key, &entry))
		return false;
	if (entry.sel == 0) {
		const char *s = LSelectorFromString(key);
		if (_funcPtrForSelector(s) == 0) {
			LString *cap = _capitalizedKey(key);
			const char *formats[] = { "get%@", "sel_%@", "sel_Get%@", 0, "_get%@" };
			bool found = false;
			for (int i = 0; i < 5 && !found; i++) {
				LString *name = formats[i] ? LString::stringWithFormat(formats[i], cap)
				                           : LString::stringWithFormat("_%@", key);
				s = LSelectorFromString(name);
				found = _funcPtrForSelector(s) != 0;
			}
			if (!found) {
				_cacheMiss(&_getKVCCache, key);
				return false;
			}
		}
		entry.sel = s;
		entry.signature = _signatureForSelector(s);
	}
	*sel = entry.sel;
	if (signature)
		*signature = entry.signature;
	return true;
}

// ---------------------------------------------------------------------------
// Singletons

LObject *LClass::singleton(void *param)
{
	if (_singleton == 0) {
		StSharableLocker lock(_singletonLock);
		if (_singleton == 0) {
			if (isAbstract())
				return 0;
			LAutoreleasePool::addNewPoolToTop();
			_singleton = alloc();
			_singleton->initAsSingleton(param);
			LAutoreleasePool::removeTopPool();
		}
	}
	return _singleton;
}

void LClass::killSingleton()
{
	StSharableLocker lock(_singletonLock);
	if (_singleton) {
		_singleton->release();
		_singleton = 0;
	}
}

// ---------------------------------------------------------------------------
// Selector strings

// Selectors made from strings are interned for the life of the program.
const char *LSelectorFromString(LString *str)
{
	if (str == 0 || str->length() == 0)
		return 0;
	static CSharableResource sCachedSelectorsSR;
	static TStrMap<const char *, 1024> sCachedSelectors;
	const char *s = str->cString();
	StSharableLocker lock(&sCachedSelectorsSR);
	const char *sel = sCachedSelectors.objectForKey(s);
	if (sel == 0) {
		size_t n = strlen(s) + 1;
		char *copy = new char[n];
		memcpy(copy, s, n);
		sCachedSelectors.setObjectForKey(copy, copy);
		sel = copy;
	}
	return sel;
}

LString *LStringFromClass(LClass *cls)
{
	return cls ? LString::stringWithCString(cls->name(), 0, kLMacRomanEncoding) : 0;
}

LString *LStringFromSelector(const char *sel)
{
	return LString::stringWithCString(sel, 0, kLMacRomanEncoding);
}

// Drops every class's key-value coding caches.
class _LClassCleaner : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		int n = sClassesArray ? sClassesArray->count() : 0;
		for (int i = 0; i < n; i++) {
			LClass *cls = (*sClassesArray)[i];
			StSharableLocker lock(&cls->_lock, true);
			delete (_KVCSelectorsMapTable *)cls->_setKVCCache;
			cls->_setKVCCache = 0;
			delete (_KVCSelectorsMapTable *)cls->_getKVCCache;
			cls->_getKVCCache = 0;
		}
	}
};
static _LClassCleaner sLClassCleaner;

} // namespace LF
