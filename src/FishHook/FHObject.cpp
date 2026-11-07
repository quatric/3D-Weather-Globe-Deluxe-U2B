// FHObject, CContextObject and the FishHook module functions.

#include "FHContext.h"
#include "LCoder.h"

LF_IMPLEMENT_CLASS(FHObject, 0)
LF_IMPLEMENT_CLASS(CContextObject, LF_CLASSOF(FHObject))

LConstantString kFHObjectClassKey("Class");
LConstantString ContextWillBeDetachedNotification("ContextWillBeDetachedNotification");
LConstantString ContextWasAttachedNotification("ContextWasAttachedNotification");

static CFBundleRef _sFishHookBundle = 0;

class _FishHookBundleCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (_sFishHookBundle)
			CFRelease(_sFishHookBundle);
	}
};
static _FishHookBundleCleanup _sFishHookBundleCleanup;

CFBundleRef getFishHookModule()
{
	if (_sFishHookBundle == 0)
		setFishHookModule(CFBundleGetBundleWithIdentifier(CFSTR("com.mackiev.FishHook")));
	return _sFishHookBundle;
}

void setFishHookModule(CFBundleRef bundle)
{
	if (_sFishHookBundle)
		CFRelease(_sFishHookBundle);
	_sFishHookBundle = bundle;
	if (bundle)
		CFRetain(bundle);
}

void finalizeFishHook()
{
	LAutoreleasePool::addNewPoolToTop();
	LClass *soundManager = LClass::classByName("SoundManager");
	if (soundManager)
		soundManager->killSingleton();
	LAutoreleasePool::removeTopPool();
}

// ---------------------------------------------------------------------------

FHObject *FHObject::objectWithMap(const LMap *map, void *context)
{
	LString *className = lobj_cast<LString>(map->objectForKey(&kFHObjectClassKey));
	if (className == 0)
		LF_RAISE(NilPtrException());
	LClass *cls = LClass::classByName(className);
	if (cls == 0)
		LF_RAISE(NilPtrException());
	LObject *obj = cls->alloc();
	if (obj && !obj->getClass()->isSubclassOf(LF_CLASSOF(FHObject))) {
		obj->release();
		obj = 0;
	}
	if (obj == 0)
		LF_RAISE(NilPtrException());

	FHObject *object = ((FHObject *)obj)->initWithMapAndContext(map, context);
	if (object) {
		LMapIterator it(map);
		while (it.next()) {
			if (!it.key()->isEqualTo(&kFHObjectClassKey))
				object->processInitializationPair(it.object(), it.key(), map);
		}
		LAutorelease(object);
	}
	return object;
}

// ---------------------------------------------------------------------------

CContextObject::CContextObject(TCtxObjPriority priority)
    : _context(0), _managesSelf(false), _restored(false), _priority(priority), _holder(0),
      _typeName("Generic Context Object")
{
}

CContextObject *CContextObject::initWithHolder(CHolder *holder)
{
	_holder = holder;
	if (holder)
		holder->addContextObject(this);
	return this;
}

CContextObject *CContextObject::initWithMapAndContext(const LMap *, void *context)
{
	return initWithHolder(*(CHolder **)context);
}

void CContextObject::_processInitializationContext(void *context)
{
	_holder = *(CHolder **)context;
}

void CContextObject::processInitializationPair(const LObject *, const LComparable *, const LMap *)
{
	LF_RAISE(FailedWithCodeException(-50));
}

void CContextObject::dealloc()
{
	if (_holder)
		_holder->removeContextObject(this);
	if (_context)
		releaseContextDependedPart();
	LObject::dealloc();
}

void CContextObject::instantiateInContext(CContext *context)
{
	if (_context == 0) {
		_context = context;
		if (context)
			instantiateSelf();
		_restored = true;
	}
}

void CContextObject::releaseContextDependedPart()
{
	if (_context)
		releaseSelf();
	_context = 0;
}

void CContextObject::purgeResources()
{
	if (_context)
		purgeSelf();
}

void CContextObject::restoreResources()
{
	if (_context)
		restoreSelf();
	_restored = true;
}

void CContextObject::purgeSelf()
{
	if (_managesSelf)
		releaseSelf();
}

void CContextObject::restoreSelf()
{
	if (_managesSelf)
		instantiateSelf();
}

CContextObject *CContextObject::contextObjectWithMap(LMap *map, CHolder *holder)
{
	FHObject *object = FHObject::objectWithMap(map, &holder);
	CContextObject *result = lobj_cast<CContextObject>(object);
	if (result == 0)
		LF_RAISE(NilPtrException());
	return result;
}

CContextObject *CContextObject::objectWithDescriptionFile(const LString *path, CHolder *holder)
{
	LData *data = LData::dataWithContentsOfFile(path);
	if (data == 0)
		LF_RAISE(NilPtrException());
	LMap *map = lobj_cast<LMap>(LCoder::unarchiveRootObject(data));
	if (map == 0)
		LF_RAISE(NilPtrException());
	return contextObjectWithMap(map, holder);
}

int CContextObjectComparator::compare(const void *a, const void *b, const void *)
{
	const CContextObject *x = *(const CContextObject *const *)a;
	const CContextObject *y = *(const CContextObject *const *)b;
	int d = x->_priority - y->_priority;
	if (d == 0)
		d = x < y ? -1 : (x > y ? 1 : 0);
	return d;
}
