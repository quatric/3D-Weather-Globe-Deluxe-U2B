// LFoundation's object model: reference-counted LObject, the per-class
// LClass objects (name, superclass, selectors for key-value coding and
// invocations, protocols, singletons), protocol IDs and LTypeID.
//
// Each class declares itself with LF_DECLARE_CLASS(Name) (or
// LF_DECLARE_ABSTRACT_CLASS) in its body and LF_IMPLEMENT_CLASS(Name, Super)
// in its source file; Super is LF_CLASSOF(Base), or 0 where the original
// registered no superclass (classes derived straight from LObject).
// Name::_alloc() creates an instance with a retain count of 1 that still
// needs its init method, like the original's _classImp_::_alloc().
#pragma once

#include "LContainers.h"
#include <typeinfo>

namespace LF {

class LObject;
class LClass;
class LMetaclass;
class LString;
class LArray;
class LMap;
class LCoder;
class LInvocation;
class LProtocolID;
class LProtocol;
class LTypeID;
class LData;
class LValue;
class LCharSet;
class LDate;

// A selector is a C string; LSelectorFromString() returns the shared copy.
typedef const char *SEL_L;

// Methods reachable by selector take their arguments as a packed block of
// pointer-sized slots (read with LVAList) and return an int-sized result.
typedef intptr_t (LObject::*LMethodPtr)(char *args);

class LMetaclass {
public:
	virtual LClass *getClass();
	virtual LObject *alloc();
	virtual ~LMetaclass() {}
};

class LObject {
public:
	class metaclass : public LMetaclass {
	};

	LObject() : _retainCount(1), _deallocating(false), _hasPreDealloc(false), _nsWrapper(0) {}
	virtual ~LObject() {}
	virtual void initAsSingleton(void *param);
	virtual void initAsMetaclass();
	virtual void removeFromLeakerAnalyserTable();
	virtual LObject *init();
	virtual LObject *copy() const;
	virtual LClass *getClass() const;
	virtual LMetaclass *getMetaclass() const;
	virtual const char *className() const;
	virtual void forwardInvocation(LInvocation *inv);
	virtual void doesNotRecognizeSelector(const char *sel);
	virtual LObject *methodSignatureForSelector(const char *sel);
	virtual LObject *valueForKey(LString *key);
	virtual void setValueForKey(LObject *value, LString *key);
	virtual LObject *valueForUndefinedKey(LString *key);
	virtual void setValueForUndefinedKey(LObject *value, LString *key);
	virtual bool isEqualTo(const LObject *other) const;
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual LString *description() const;
	virtual LString *internalDescription() const;
	virtual LString *fillTextRepresentation(unsigned int indent) const;
	virtual LString *plistRepresentation(unsigned int indent) const;
	virtual void pre_dealloc();
	virtual void dealloc();
	virtual LString *_plistRepresentation(unsigned int indent) const;
	virtual LProtocol *findProtocolInterface(LProtocolID *protocol);

	LObject *retain()
	{
		_retainCount++;
		return this;
	}
	void release();
	LObject *l_autorelease();
	int retainCount() const { return _retainCount; }

	bool conformsToProtocol(LProtocolID *protocol);
	bool respondsToSelector(const char *sel);
	bool convertToType(const LTypeID &type, void *out);
	bool setValueForOutlet(LObject *value, LString *key);
	bool getValueForOutlet(LString *key, const LTypeID &type, void *out);
	intptr_t invokeSelectorWithArgs(const char *sel, char *args);
	// The arguments go into pointer-sized slots, followed by a 0 slot (the
	// original's callers often ended their lists with 0 themselves).
	template <typename... A> intptr_t invokeSelector(const char *sel, A... a)
	{
		intptr_t v[] = { _lfSlot(a)..., 0 };
		return invokeSelectorWithArgs(sel, (char *)v);
	}
	// Queue the call on the main run loop; `wait` blocks until it has run.
	template <typename... A> void invokeSelectorInMainThread(const char *sel, bool wait, A... a)
	{
		intptr_t v[] = { _lfSlot(a)..., 0 };
		_invokeInMainThread(sel, wait, v, sizeof...(A));
	}
	// Same, with LObjects that stay retained while queued (unless
	// `dontRetain`). A trailing 0 is added.
	template <typename... A>
	void invokeSelectorInMainThreadWithObjects(const char *sel, bool wait, bool dontRetain, A... a)
	{
		LObject *v[] = { (LObject *)a..., 0 };
		_invokeInMainThreadWithObjects(sel, wait, dontRetain, v);
	}
	void _invokeInMainThread(const char *sel, bool wait, const intptr_t *args, unsigned int argCount);
	void _invokeInMainThreadWithObjects(const char *sel, bool wait, bool dontRetain, LObject *const *objects);
	template <typename T> static intptr_t _lfSlot(T *p) { return (intptr_t)p; }
	template <typename T> static intptr_t _lfSlot(T v) { return (intptr_t)v; }
	void _invalidateNSWrapper();
	// An autoreleased NSObject that wraps this object.
	void *detachNSWrapper();

	int _retainCount;
	bool _deallocating;
	bool _hasPreDealloc;
	void *_nsWrapper;
#ifdef LF_I386_LAYOUT
	bool _i386Pad;	// tools/lay.py: the original LObject ended with a byte at 0x10 (subclasses reuse 0x11..)
#endif
};

// Retains/releases for typed pointers.
template <typename T> static inline T *LRetain(T *o)
{
	if (o)
		o->retain();
	return o;
}
static inline void LRelease(LObject *o)
{
	if (o)
		o->release();
}
template <typename T> static inline T *LAutorelease(T *o)
{
	if (o)
		o->l_autorelease();
	return o;
}
// Replaces *slot with a retained `value`, releasing the old one.
template <typename T, typename U> static inline void LAssign(T *&slot, U *value)
{
	if (slot != value) {
		if (value)
			value->retain();
		if (slot)
			slot->release();
		slot = value;
	}
}

// ---------------------------------------------------------------------------
// Protocols: named interfaces with parents, and the IDs classes list.

class ProtocolIDsList {
public:
	ProtocolIDsList(bool parents);
	~ProtocolIDsList();
	void addID(LProtocolID *pid);
	bool containsID(LProtocolID *pid) const;
	void getAllIDs(CHashSet *out) const;

	struct _ProtocolIDsSetCallbacks {
		template <typename E> static void add(E *) {}
		template <typename E> static void remove(E *) {}
		static const bool replace = false;
	};
	struct LProtocolIDKeyCallbacks {
		static unsigned int hash(LProtocolID *p);
		static bool equal(LProtocolID *stored, LProtocolID *p);
	};
	typedef TSet<LProtocolID *, LProtocolIDKeyCallbacks, _ProtocolIDsSetCallbacks> _ProtocolIDsSet;

private:
	void _addHierarchyToSet(_ProtocolIDsSet *set) const;
	void _createSetIfNeeded() const;

	TArray<LProtocolID *> *_ids;
	mutable _ProtocolIDsSet *_set;
	bool _parents;	// follow parents (true) or children
};

// Protocol IDs are static objects that installers in other files add to
// during static initialisation, so they are constructed first:
//   LF_PROTOCOL_INIT_PRIORITY LProtocolID kMyProtocolID("MyProtocol");
#define LF_PROTOCOL_INIT_PRIORITY __attribute__((init_priority(150)))

class LProtocolID {
public:
	LProtocolID(const char *name);
	~LProtocolID() {}
	unsigned int hashValue() const { return _hash; }
	bool operator==(const LProtocolID &o) const { return strncmp(_name, o._name, 64) == 0; }
	bool operator!=(const LProtocolID &o) const { return strncmp(_name, o._name, 64) != 0; }
	int compareWith(const LProtocolID &o) const;
	void addParent(LProtocolID *parent);
	void addChild(LProtocolID *child);
	bool isSubprotocolOf(LProtocolID *other);
	const char *name() const { return _name; }

	const char *_name;
	ProtocolIDsList _parentsList;
	ProtocolIDsList _childrenList;
	unsigned int _hash;
};

// Registers a protocol's parent, or a protocol a class conforms to.
class LProtocolIDInstaller {
public:
	LProtocolIDInstaller(LProtocolID *protocol, LProtocolID *parent) { protocol->addParent(parent); }
	LProtocolIDInstaller(LClass *cls, LProtocolID *protocol);
};

// Base of the protocol interfaces an LObject subclass can implement.
class LProtocol {
public:
	virtual LObject *getOwnerLObject() = 0;
	virtual ~LProtocol() {}
	LProtocol *retain_p()
	{
		LObject *o = getOwnerLObject();
		if (o)
			o->retain();
		return this;
	}
	LProtocol *autorelease_p()
	{
		LObject *o = getOwnerLObject();
		if (o)
			o->l_autorelease();
		return this;
	}
	void release_p()
	{
		LObject *o = getOwnerLObject();
		if (o)
			o->release();
	}
};

// ---------------------------------------------------------------------------
// Type descriptions used by key-value coding and outlets.

class LTypeID {
public:
	enum TType {
		kNone = 0,	// also the terminator of a type list
		kStruct = 1,	// raw bytes of _structSize each
		kInt = 2,
		kBool = 3,
		kIntPtr = 4,
		kFloat = 5,
		kDouble = 6,
	};

	LTypeID() : _typeOrClass(0), _numElements(0), _structSize(0), _typeInfo(&typeid(void)) {}
	LTypeID(TType type, unsigned int numElements = 1)
	    : _typeOrClass((uintptr_t)type), _numElements(numElements), _structSize(0), _typeInfo(&typeid(void)) {}
	LTypeID(LClass *cls) : _typeOrClass((uintptr_t)cls), _numElements(1), _structSize(0), _typeInfo(&typeid(void)) {}
	LTypeID(const std::type_info &ti, int size) : _typeOrClass(0), _numElements(1), _structSize(size), _typeInfo(&ti) {}

	TType getType() const
	{
		if (_structSize != 0)
			return kStruct;
		return _typeOrClass < 7 ? (TType)_typeOrClass : kNone;
	}
	LClass *getClass() const { return _typeOrClass < 8 ? 0 : (LClass *)_typeOrClass; }
	unsigned int numElements() const { return _numElements; }
	bool isTerminator() const { return _structSize == 0 && _typeOrClass == 0; }
	bool isLObject() const { return getClass() != 0; }
	bool operator==(const LTypeID &o) const;
	// Bytes one argument of this type takes (pointer-sized for objects and intptr).
	unsigned int getArgSize() const;
	bool convertValue(const void *src, const LTypeID &dstType, void *dst) const;

	uintptr_t _typeOrClass;
	unsigned int _numElements;
	unsigned int _structSize;
	const std::type_info *_typeInfo;
};

// ---------------------------------------------------------------------------
// Classes

class LClass {
public:
	LClass(const char *name, LClass *superclass, unsigned int instanceSize);
	virtual LObject *alloc() = 0;
	virtual LMetaclass *getMetaclass() = 0;
	virtual bool isAbstract();
	virtual unsigned int bytesAllocated() const = 0;
	virtual unsigned int objectsAllocated() const = 0;
	virtual void _initializeClass();
	virtual ~LClass();

	const char *name() const { return _name; }
	LClass *superclass() const { return _superclass; }
	unsigned int instanceSize() const { return _instanceSize; }

	static void addClass(LClass *cls);
	static LClass *classByName(const char *name);
	static LClass *classByName(LString *name);
	static unsigned int classesCount();
	static LClass *classAtIndex(long index);

	bool isSubclassOf(LClass *other);
	bool implementsProtocol(LProtocolID *protocol);
	void _addProtocolWithID(LProtocolID *protocol);

	// The class's shared instance, created (and sent initAsSingleton) on first use.
	LObject *singleton(void *param = 0);
	void killSingleton();
	bool hasSingleton() const { return _singleton != 0; }

	void setMetadataForKey(intptr_t value, const char *key);
	intptr_t metadataForKey(const char *key, bool inherited = true);

	// Selectors
	struct SFuncPtr {
		LMethodPtr method;
		bool found() const { return method != 0; }
	};
	void _addSelector(const char *sel, LMethodPtr method, LObject *signature);
	void _removeSelector(const char *sel);
	LMethodPtr _funcPtrForSelector(const char *sel);
	LObject *_signatureForSelector(const char *sel);
	bool _setValueParamsForKey(LString *key, const char **sel, LObject **signature);
	bool _getValueParamsForKey(LString *key, const char **sel, LObject **signature);

	// Typed member variables reachable by name (never installed by the app).
	class LOutletInstaller {
	public:
		LOutletInstaller(LClass *cls, const LTypeID &type, const char *name, intptr_t offset);
		virtual ~LOutletInstaller();
		bool setValueOfOwner(LObject *value, LObject *owner);
		bool getValueOfOwner(LObject *owner, const LTypeID &type, void *out);

		LClass *_class;
		LTypeID *_type;
		const char *_name;
		intptr_t _offset;
	};
	LOutletInstaller *_outletForKey(LString *key);

	// Registers a method under a selector name for the life of the program.
	class LSelectorInstaller {
	public:
		LSelectorInstaller(LClass *cls, const char *sel, LMethodPtr method, LObject *signature = 0)
		    : _class(cls), _sel(sel)
		{
			cls->_addSelector(sel, method, signature);
		}
		~LSelectorInstaller() { _class->_removeSelector(_sel); }
		LClass *_class;
		const char *_sel;
	};

	class LSelectorsMap;
	struct _SKVCSelectorEntry {
		const char *sel;	// (const char *)-1 caches "no selector"
		LObject *signature;
	};

	char _name[256];
	LClass *_superclass;
	LSelectorsMap *_selectors;
	unsigned int _instanceSize;
	ProtocolIDsList *_protocols;
	void *_outlets;	// outlet (or null for a miss) by name
	void *_setKVCCache;	// TMap<LString*, _SKVCSelectorEntry>
	void *_getKVCCache;
	CSharableResource _lock;
	TMap<const char *, intptr_t, LCStringKeyCallbacks<64>, LDummyValueCallbacks> *_metadata;
	LObject *_singleton;
	CSharableResource *_singletonLock;
};

// The class object of a class that can't be instantiated.
class LAbstractClass : public LClass {
public:
	LAbstractClass(const char *name, LClass *superclass) : LClass(name, superclass, 0) {}
	virtual LObject *alloc();
	virtual bool isAbstract();
	virtual unsigned int bytesAllocated() const { return 0; }
	virtual unsigned int objectsAllocated() const { return 0; }
};

template <typename T>
class TMetaclassImp : public LObject::metaclass {
public:
	virtual LClass *getClass() { return T::classObject(); }
	virtual LObject *alloc() { return T::_alloc(); }
};

// Name::_classImp_: the class object of a concrete class.
template <typename T>
class TClassImp : public LClass {
public:
	TClassImp(const char *name, LClass *superclass) : LClass(name, superclass, sizeof(T)), _initialized(false) {}
	T *_alloc()
	{
		if (!_initialized) {
			StSharableLocker lock(&_initLock);
			if (!_initialized)
				_initializeClass();
			_initialized = true;
		}
		return new T;
	}
	virtual LObject *alloc() { return _alloc(); }
	virtual LMetaclass *getMetaclass() { return &_metaclass; }
	virtual unsigned int bytesAllocated() const { return 0; }
	virtual unsigned int objectsAllocated() const { return 0; }

private:
	bool _initialized;
	CSharableResource _initLock;
	TMetaclassImp<T> _metaclass;
};

template <typename T>
class TAbstractClassImp : public LAbstractClass {
public:
	TAbstractClassImp(const char *name, LClass *superclass) : LAbstractClass(name, superclass) {}
	virtual LMetaclass *getMetaclass() { return &_metaclass; }

private:
	class _meta : public LObject::metaclass {
	public:
		virtual LClass *getClass() { return T::classObject(); }
		virtual LObject *alloc() { return 0; }
	} _metaclass;
};

#define LF_CLASSOF(Name) ((LF::LClass *)Name::classObject())

#define LF_DECLARE_CLASS(Name)                                                   \
public:                                                                          \
	typedef LF::TClassImp<Name> _classImp_;                                  \
	static _classImp_ _sClassImp;                                            \
	static LF::LClass *classObject() { return (LF::LClass *)&_sClassImp; }   \
	static Name *_alloc() { return _sClassImp._alloc(); }                    \
	static Name *instance() { return (Name *)classObject()->singleton(0); }  \
	virtual LF::LClass *getClass() const { return (LF::LClass *)&_sClassImp; }

#define LF_DECLARE_ABSTRACT_CLASS(Name)                                          \
public:                                                                          \
	typedef LF::TAbstractClassImp<Name> _classImp_;                          \
	static _classImp_ _sClassImp;                                            \
	static LF::LClass *classObject() { return (LF::LClass *)&_sClassImp; }   \
	virtual LF::LClass *getClass() const { return (LF::LClass *)&_sClassImp; }

// Class objects are constructed before other static objects (such as
// LThread::sMainThread, which allocates an LRunLoop), whatever the link order.
#define LF_CLASS_INIT_PRIORITY __attribute__((init_priority(200)))
#define LF_IMPLEMENT_CLASS(Name, Super) LF_CLASS_INIT_PRIORITY Name::_classImp_ Name::_sClassImp(#Name, Super);
#define LF_IMPLEMENT_CLASS_NAMED(Name, Str, Super) LF_CLASS_INIT_PRIORITY Name::_classImp_ Name::_sClassImp(Str, Super);

// Registers `Class::method` under the selector "method".
#define LF_SELECTOR(Class, method)                                                       \
	static LF::LClass::LSelectorInstaller _##Class##_lmethod_##method(                 \
	    LF_CLASSOF(Class), #method, (LF::LMethodPtr)(&Class::method))

#define LF_CONFORMS_TO(Class, protocolID) \
	static LF::LProtocolIDInstaller _##Class##_conforms_##protocolID(LF_CLASSOF(Class), &protocolID)

const char *LSelectorFromString(LString *str);
LString *LStringFromSelector(const char *sel);
LString *LStringFromClass(LClass *cls);
bool LSelectorsAreEqual(const char *a, const char *b);

// ---------------------------------------------------------------------------
// Objects with an ordering and a hash (strings, values, dates...).

class LComparable : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(LComparable)
public:
	LComparable() : _flag10(false), _hashCached(false), _hash(0) {}
	virtual bool isEqualTo(const LObject *other) const;
	virtual int compareWith(const LComparable *other) const = 0;
	virtual unsigned int _calcHashValue() const = 0;
	virtual bool _isEqualTo(const LComparable *other) const = 0;

	unsigned int hashValue() const
	{
		if (!_hashCached) {
			_hashCached = true;
			_hash = _calcHashValue();
		}
		return _hash;
	}
	void _resetHash() { _hashCached = false; }

	bool _flag10;
	mutable bool _hashCached;
	mutable unsigned int _hash;
};

int _LComparableObjectCompareCallback(const void *a, const void *b, const void *context);

// Hash table policies for object keys and values.
struct LComparableKeyCallbacks {
	static unsigned int hash(const LComparable *k) { return k->hashValue(); }
	static bool equal(const LComparable *stored, const LComparable *k) { return stored->isEqualTo(k); }
};

struct LObjectKeyCallbacks {
	template <typename E> static void add(E *e) { e->key->retain(); }
	template <typename E> static void remove(E *e) { LRelease(e->key); }
	static const bool replace = true;
};

struct LObjectValueCallbacks {
	template <typename E> static void add(E *e) { e->value->retain(); }
	template <typename E> static void remove(E *e) { LRelease(e->value); }
	static const bool replace = true;
};

// A map from integers (or pointers) to retained objects.
template <typename V>
class TLIntPtrMap : public TMap<intptr_t, V, LIntPtrKeyCallBacks, LObjectValueCallbacks> {
};

// A set of retained objects compared by identity.
typedef TSet<LObject *, LPointerKeyCallbacks<LObject *>, LObjectKeyCallbacks> TLObjectIdentitySet;

// The array policy that retains elements while they are in the array.
template <typename T>
struct _TLObjectArrayAddRemoveFunc {
	static void remove(T &o) { LRelease(o); }
	static const bool active = true;
};

class LEnumerator : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(LEnumerator)
public:
	LEnumerator() : _container(0) {}
	virtual void dealloc();
	virtual LObject *next() = 0;
	virtual void reset() = 0;
	virtual LEnumerator *initWithContainer(LObject *container);

	LObject *_container;
};

// obj as a T if it is one (by its class), else 0.
template <typename T> static inline T *lobj_cast(LObject *obj)
{
	return obj && obj->getClass()->isSubclassOf(T::classObject()) ? (T *)obj : 0;
}
template <typename T> static inline const T *lobj_cast(const LObject *obj)
{
	return obj && obj->getClass()->isSubclassOf(T::classObject()) ? (const T *)obj : 0;
}

} // namespace LF
