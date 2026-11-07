// CHolder: the context objects, textures, render profiles and named objects
// of one 3D view, and the context they live in.

#include "FHContext.h"
#include "FHRender.h"

LF_IMPLEMENT_CLASS(CHolder, 0)

static LConstantString kAccumulationListsKey("kAccumulationListsKey");

CHolder::CHolder()
    : _view(0), _context(0), _accumulationDepth(0), _objects(0), _textures(0), _profiles(0), _namedObjects(0),
      _bbEmulator(0)
{
}

CHolder *CHolder::initWithView(C3DView *view)
{
	_view = view;
	_objects = new CHashSet;
	_textures = new _holderTexturesMap;
	_profiles = LMap::_alloc()->init();
	return this;
}

void CHolder::dealloc()
{
	setBackBufferEmulator(0);
	{
		StSharableLocker lock(&_namedObjectsLock);
		LMapIterator it(_namedObjects);
		while (it.next()) {
			CHolderObjectDelegate *delegate = dynamic_cast<CHolderObjectDelegate *>(it.object());
			if (delegate)
				delegate->holderWillDie(this);
		}
		LMap *objects = _namedObjects;
		_namedObjects = 0;
		LRelease(objects);
	}
	if (_objects) {
		StSharableLocker lock(&_objectsLock);
		CHashSetIterator it(_objects);
		while (it.next())
			((CContextObject *)it.object())->_holder = 0;
		delete _objects;
		_objects = 0;
	}
	{
		StSharableLocker lock(&_profilesLock);
		if (_profiles) {
			// The profiles outlive the holder (the original kept them alive too).
			LMapIterator it(_profiles);
			while (it.next())
				it.object()->retain();
			LRelease(_profiles);
			_profiles = 0;
		}
	}
	delete _textures;
	_textures = 0;
	LObject::dealloc();
}

void *CHolder::systemViewPtr()
{
	return _view ? _view->systemView() : 0;
}

// ---------------------------------------------------------------------------
// Accumulation blocks.

LArray *CHolder::currentAccumulationArray()
{
	if (_accumulationDepth > 0) {
		LArray *lists = (LArray *)_threadData.perThreadMap()->objectForKey(&kAccumulationListsKey);
		if (lists && lists->count() > 0)
			return (LArray *)lists->lastObject();
	}
	return 0;
}

void CHolder::beginAccumulationBlock()
{
	LMap *map = _threadData.perThreadMap();
	LArray *lists = (LArray *)map->objectForKey(&kAccumulationListsKey);
	if (lists == 0) {
		lists = LArray::array();
		map->setObjectForKey(lists, &kAccumulationListsKey);
	}
	lists->addObject(LArray::array());
	_accumulationDepth++;
}

void CHolder::endAccumulationBlock()
{
	if (_context)
		_context->device()->ValidateContext();
	LArray *lists = lobj_cast<LArray>(_threadData.perThreadMap()->objectForKey(&kAccumulationListsKey));
	if (lists && lists->count() > 0) {
		LArray *list = (LArray *)lists->lastObject();
		if (list) {
			list->setCompareFunc(CContextObjectComparator::compare);
			list->sort();
			int count = list->count();
			for (int i = 0; i < count; i++) {
				CContextObject *object = (CContextObject *)list->objectAtIndex(i);
				object->instantiateInContext(_context);
				if (!_objects->containsObject((intptr_t)object))
					_objects->addObject((intptr_t)object);
			}
			lists->removeObjectAtIndex(lists->count() - 1);
		}
	}
	_accumulationDepth--;
}

void CHolder::addContextObject(CContextObject *object)
{
	if (_context == 0) {
		StSharableLocker lock(&_objectsLock);
		_objects->addObject((intptr_t)object);
	} else {
		LArray *list = currentAccumulationArray();
		if (list == 0)
			LF_RAISE(NilPtrException());
		list->addObject(object);
	}
}

void CHolder::removeContextObject(CContextObject *object)
{
	LArray *list = currentAccumulationArray();
	if (list)
		list->removeObject(object);
	StSharableLocker lock(&_objectsLock);
	if (_objects->containsObject((intptr_t)object))
		_objects->removeObject((intptr_t)object);
}

static void _sortedContextObjects(CHashSet *objects, TArray<intptr_t> *sorted)
{
	objects->getAllObjects(sorted);
	sorted->setCompareFunc(CContextObjectComparator::compare);
	sorted->sort();
}

void CHolder::adoptToContext(CContext *context)
{
	if (_accumulationDepth > 0)
		LF_RAISE(WrongConditionException());
	if (_context) {
		_context->device()->ValidateContext();
		LNotificationCenter::center()->postNotification(&ContextWillBeDetachedNotification, this, 0);
		StSharableLocker lock(&_objectsLock);
		TArray<intptr_t> sorted;
		_sortedContextObjects(_objects, &sorted);
		for (int i = (int)sorted.count() - 1; i >= 0; i--) {
			CContextObject *object = (CContextObject *)sorted[i];
			// Releasing one object can release others.
			if (_objects->containsObject((intptr_t)object))
				object->releaseContextDependedPart();
		}
	}
	_context = context;
	if (context) {
		context->device()->ValidateContext();
		{
			StSharableLocker lock(&_objectsLock);
			TArray<intptr_t> sorted;
			_sortedContextObjects(_objects, &sorted);
			for (unsigned int i = 0; i < sorted.count(); i++)
				((CContextObject *)sorted[i])->instantiateInContext(_context);
		}
		LNotificationCenter::center()->postNotification(&ContextWasAttachedNotification, this, 0);
	}
}

void CHolder::resetCurrentContext(CDeviceFormat *format)
{
	if (_context == 0)
		return;
	_context->device()->ValidateContext();
	StSharableLocker lock(&_objectsLock);
	{
		CHashSetIterator it(_objects);
		while (it.next())
			((CContextObject *)it.object())->purgeResources();
	}
	if (!_context->resetContext(format))
		LF_RAISE(FailedWithCodeException(-50));
	CHashSetIterator it(_objects);
	while (it.next())
		((CContextObject *)it.object())->restoreResources();
}

// ---------------------------------------------------------------------------
// Profiles, textures and named objects.

CRenderProfile *CHolder::renderProfileByFileDesc(LString *path)
{
	StSharableLocker lock(&_profilesLock);
	if (_profiles) {
		CRenderProfile *profile = (CRenderProfile *)_profiles->objectForKey(path);
		if (profile)
			return profile;
	}
	CRenderProfile *profile = LAutorelease(CRenderProfile::_alloc());
	if (profile == 0)
		return 0;
	LAutoreleasePool::addNewPoolToTop();
	CRenderProfile *result = 0;
	LData *data = LData::dataWithContentsOfFile(path);
	if (data) {
		beginAccumulationBlock();
		if (profile->initWithHolderAndData(this, data, kInitDataText, path)) {
			_profiles->setObjectForKey(profile, path);
			profile->release();
			result = profile;
		}
		endAccumulationBlock();
	}
	LAutoreleasePool::removeTopPool();
	return result;
}

CRenderProfile *CHolder::renderProfileByKey(LString *key)
{
	return _profiles ? (CRenderProfile *)_profiles->objectForKey(key) : 0;
}

void CHolder::removeRenderProfile(CRenderProfile *profile)
{
	StSharableLocker lock(&_profilesLock);
	_profiles->removeObjectForKey(profile->fileDescription());
}

CBaseTexture *CHolder::textureByFileDesc(LString *path)
{
	StSharableLocker lock(&_texturesLock);
	return _textures->objectForKey(path);
}

void CHolder::setTextureForFileDescr(CBaseTexture *texture, LString *path)
{
	StSharableLocker lock(&_texturesLock);
	_textures->setObjectForKey(texture, path);
}

void CHolder::removeTextureWithFileDescr(LString *path)
{
	StSharableLocker lock(&_texturesLock);
	_textures->removeObjectForKey(path);
}

LObject *CHolder::objectForKey(const LString *key)
{
	StSharableLocker lock(&_namedObjectsLock);
	return _namedObjects ? _namedObjects->objectForKey(key) : 0;
}

void CHolder::setObjectForKey(LObject *object, const LString *key)
{
	StSharableLocker lock(&_namedObjectsLock);
	if (_namedObjects == 0)
		_namedObjects = LMap::_alloc()->init();
	_namedObjects->setObjectForKey(object, key);
}

void CHolder::removeObjectForKey(const LString *key)
{
	StSharableLocker lock(&_namedObjectsLock);
	if (_namedObjects)
		_namedObjects->removeObjectForKey(key);
}

// ---------------------------------------------------------------------------
// The view.

void CHolder::doAcceleratedRendering(bool postNotification, bool endFrame, bool present)
{
	_view->drawAccelerated(postNotification, endFrame, present);
}

LSize CHolder::backBufferSize()
{
	if (_bbEmulator)
		return _bbEmulator->backBufferSize();
	return _view->frame().size;
}

LSize CHolder::systemFrameSize()
{
	return _view->frame().size;
}

LSize CHolder::virtualFrameSize()
{
	if (_bbEmulator)
		return _bbEmulator->virtualFrameSize();
	return backBufferSize();
}

LSize CHolder::fullFrameSize()
{
	if (_bbEmulator)
		return _bbEmulator->size();
	return backBufferSize();
}

// The virtual frame scaled to the part of the full frame one back buffer shows.
LSize CHolder::relativeFrameSize()
{
	CHolder *holder = _context->holder();
	LSize bb = holder->backBufferSize();
	LSize virt = holder->virtualFrameSize();
	LSize full = holder->fullFrameSize();
	return LMakeSize(virt.width * bb.width / full.width, virt.height * bb.height / full.height);
}

void CHolder::refreshView()
{
	if (_view)
		_view->refresh();
}

void CHolder::refreshViewFromScene(CScene *scene)
{
	if (_view && _view->scene() == scene && scene)
		_view->refresh();
}

void CHolder::onBackbufferReady()
{
	if (_bbEmulator)
		_bbEmulator->onBackbufferReady();
}

bool CHolder::setCurrentGLContext()
{
	return _view ? _view->setCurrentGLContext() : false;
}

void CHolder::flushBuffer()
{
	if (_view)
		_view->flushBuffer();
}

void CHolder::setBackBufferEmulator(LargeBBEmulator *emulator)
{
	if (_bbEmulator == emulator)
		return;
	LSize oldSize = backBufferSize();
	LRetain(emulator);
	LRelease(_bbEmulator);
	_bbEmulator = emulator;
	if (emulator == 0)
		_view->exitCaptureMode();
	LSize newSize = backBufferSize();
	bool changed = oldSize.width != newSize.width || oldSize.height != newSize.height;
	if (emulator == 0) {
		if (!changed)
			return;
	} else {
		// The view's own size while it moves into the capture window.
		_bbEmulator = 0;
		_view->enterCaptureModeWithSize(newSize);
		_bbEmulator = emulator;
	}
	_view->recreateContext(0);
}
