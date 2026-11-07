// CScene and CScenePropertiesSet.

#include "FHSprites.h"
#include "LCoder.h"

LF_IMPLEMENT_CLASS(CScene, 0)
LF_IMPLEMENT_CLASS(CScenePropertiesSet, 0)

LConstantString SceneStatesWereCaptured("SceneStatesWereCaptured");
LConstantString SceneStatesSetKey("SceneStatesSetKey");

const char *kStateObjNameSceneCamera = "_FH_SceneCamera";
const char *kStateObjNameSceneLight0 = "_FH_SceneLight0";
const char *kStateObjNameSceneLight1 = "_FH_SceneLight1";
const char *kStateObjNameSceneLight2 = "_FH_SceneLight2";
const char *kStateObjNameSceneLight3 = "_FH_SceneLight3";

void FHSceneUnregisterStateOwner(CScene *scene, CSceneStatePropertyOwner *owner)
{
	scene->unregisterStateOwner(owner);
}

// ---------------------------------------------------------------------------
// CScene

CScene::CScene()
    : _holder(0), _hoveredControl(0), _pressedControl(0), _controlsVisible(true), _capturers(0),
      _capturedProperties(0), _grabbedFrameSource(0), _stateOwners(0), _controls(0), _controlsFont(0),
      _recordsState(false), _camera(0), _layoutWidth(0), _layoutHeight(0), _layoutValid(false)
{
	_frameDrawer = CFrameDrawer::_alloc()->init();
	_lights = LArray::_alloc()->init();
	_clipPlanes = LArray::_alloc()->init();
}

void CScene::dealloc()
{
	LRelease(_capturedProperties);
	_capturedProperties = 0;
	LRelease(_grabbedFrameSource);
	_grabbedFrameSource = 0;
	LRelease(_controls);
	_controls = 0;
	LRelease(_controlsFont);
	_controlsFont = 0;
	if (_capturers) {
		for (unsigned int i = 0; i < _capturers->count(); i++)
			(*_capturers)[i]->sceneWillDie();
		delete _capturers;
		_capturers = 0;
	}
	if (_stateOwners) {
		TStrMap<CSceneStatePropertyOwner *, 128>::hashTableIterator it(_stateOwners);
		while (it.next())
			it.entry()->value->hostSceneWillDie(this);
		delete _stateOwners;
		_stateOwners = 0;
	}
	LRelease(_camera);
	_camera = 0;
	LRelease(_frameDrawer);
	_frameDrawer = 0;
	LRelease(_lights);
	_lights = 0;
	LRelease(_clipPlanes);
	_clipPlanes = 0;
	LObject::dealloc();
}

void CScene::registerStateOwner(CSceneStatePropertyOwner *owner)
{
	if (owner == 0)
		return;
	if (_stateOwners == 0)
		_stateOwners = new TStrMap<CSceneStatePropertyOwner *, 128>;
	_stateOwners->setObjectForKey(owner, owner->_stateOwnerName);
	owner->wasRegisteredInScene(this);
}

void CScene::unregisterStateOwner(CSceneStatePropertyOwner *owner)
{
	if (owner == 0)
		return;
	if (_stateOwners)
		_stateOwners->removeObjectForKey(owner->_stateOwnerName);
	owner->hostSceneWillDie(this);
}

CSceneStatePropertyOwner *CScene::stateOwnerWithName(const char *name)
{
	return _stateOwners ? _stateOwners->objectForKey(name) : 0;
}

void CScene::unregisterSceneCapturer(CSceneCapturer *capturer)
{
	if (_capturers == 0)
		return;
	_capturers->removeObject(capturer);
	if (_capturers->count() == 0) {
		delete _capturers;
		_capturers = 0;
	}
}

void CScene::setCamera(CContextCamera *camera)
{
	LRetain(camera);
	if (_camera) {
		if (_recordsState)
			unregisterStateOwner(_camera);
		_camera->release();
	}
	_camera = camera;
	if (camera && _recordsState)
		registerStateOwner(camera);
}

void CScene::addLight(CContextLight *light)
{
	StSharableLocker lock(&_lightsLock);
	_lights->addObject(light);
	if (light && _recordsState)
		registerStateOwner(light);
}

void CScene::removeLight(CContextLight *light)
{
	StSharableLocker lock(&_lightsLock);
	if (light && _recordsState)
		unregisterStateOwner(light);
	_lights->removeObject(light);
}

void CScene::setFrameDrawer(CFrameDrawer *drawer)
{
	LAssign(_frameDrawer, drawer);
}

void CScene::setGrabbedFrameSource(CScenePropertiesSet *source)
{
	LAssign(_grabbedFrameSource, source);
}

void CScene::setControlsFont(CFontDrawable *font)
{
	LAssign(_controlsFont, font);
}

void CScene::refreshView()
{
	_holder->refreshViewFromScene(this);
}

CSceneCapturer *CScene::detachCapturer()
{
	CSceneCapturer *capturer = CSceneCapturer::_alloc()->initWithScene(this);
	if (capturer) {
		if (_capturers == 0)
			_capturers = new TArray<CSceneCapturer *>;
		_capturers->addObject(capturer);
		LAutorelease(capturer);
	}
	return capturer;
}

// Draws a frame, recording it when a capturer captures and playing a
// recorded one back instead of the drawables when one is set.
void CScene::drawScene()
{
	if (_frameDrawer == 0)
		return;
	int capturers = _capturers ? _capturers->count() : 0;
	if (_grabbedFrameSource == 0) {
		for (int i = 0; i < capturers; i++) {
			if ((*_capturers)[i]->isCapturing()) {
				LRelease(_capturedProperties);
				_capturedProperties = CScenePropertiesSet::_alloc()->init();
				break;
			}
		}
	}
	if (_capturedProperties == 0)
		adjustEnvBeforeDraw();
	_frameDrawer->beginFrame(this, _capturedProperties);
	if (_capturedProperties)
		_capturedProperties->captureStartedWithScene(this);
	if (_grabbedFrameSource == 0) {
		renderAllDrawables(_frameDrawer);
		if (_controlsVisible)
			_renderControls(_frameDrawer);
	} else {
		_grabbedFrameSource->drawStoredObjects(this);
	}
	_frameDrawer->flushRenderingQueue();
	_frameDrawer->endFrame();
	if (_capturedProperties) {
		_capturedProperties->captureFinished();
		LMap *info = LMap::mapWithObjectsAndKeys(_capturedProperties, &SceneStatesSetKey, (LObject *)0);
		LNotificationCenter::center()->postNotification(&SceneStatesWereCaptured, this, info);
		double time = LTimer::currentTimeValue();
		for (int i = 0; i < capturers; i++) {
			CSceneCapturer *capturer = (*_capturers)[i];
			if (capturer->isCapturing())
				capturer->addFrameForTime(_capturedProperties, time);
		}
		LRelease(_capturedProperties);
		_capturedProperties = 0;
	}
}

void CScene::addControl(CSceneControl *control)
{
	if (control == 0)
		return;
	if (_controls == 0)
		_controls = LArray::_alloc()->init();
	_controls->addObject(control);
	invalControlsLayout();
	control->wasAddedToScene(this);
}

void CScene::removeControl(CSceneControl *control)
{
	if (control == 0 || _controls == 0)
		return;
	control->willBeRemovedFromScene(this);
	_controls->removeObject(control);
	if (_controls->count() == 0) {
		_controls->release();
		_controls = 0;
	}
	invalControlsLayout();
}

LRect CScene::viewRectForControl(CSceneControl *control)
{
	if (control == 0)
		return LZeroRect;
	performControlsLayoutIfNeeded();
	return control->viewRect();
}

void CScene::performControlsLayoutIfNeeded()
{
	LSize size = LZeroSize;
	int width = (int)size.width, height = (int)size.height;
	if (_layoutValid) {
		size = _holder->virtualFrameSize();
		width = (int)size.width;
		height = (int)size.height;
		if (_layoutWidth == width && _layoutHeight == height)
			return;
	}
	_layoutHeight = height;
	_layoutWidth = width;
	_layoutValid = true;
	_performControlsLayout(size);
}

void CScene::_performControlsLayout(LSize size)
{
	int count = _controls ? _controls->count() : 0;
	for (int i = 0; i < count; i++)
		((CSceneControl *)_controls->objectAtIndex(i))->evalViewRectForViewFrameWithSize(size);
}

// The point is flipped to the controls' top-down coordinates.
CSceneControl *CScene::controlHitTest(LPoint point)
{
	int count = _controls ? _controls->count() : 0;
	if (count == 0)
		return 0;
	performControlsLayoutIfNeeded();
	point.y = _holder->systemFrameSize().height - point.y;
	for (int i = 0; i < count; i++) {
		CSceneControl *control = (CSceneControl *)_controls->objectAtIndex(i);
		LRect r = control->viewRect();
		if (r.origin.x <= point.x && r.origin.y <= point.y && point.x <= r.origin.x + r.size.width &&
		    point.y <= r.origin.y + r.size.height) {
			LPoint local = {point.x - r.origin.x, point.y - r.origin.y};
			if (control->hitTest(local))
				return control;
		}
	}
	return 0;
}

static LPoint _flipped(CHolder *holder, LPoint point)
{
	point.y = holder->systemFrameSize().height - point.y;
	return point;
}

void CScene::mouseDown(unsigned int, LPoint point)
{
	CSceneControl *control = controlHitTest(_flipped(_holder, point));
	if (control) {
		_pressedControl = control;
		control->beginTracking();
		_holder->refreshView();
	}
}

void CScene::mouseMove(unsigned int, LPoint point)
{
	CSceneControl *control = controlHitTest(_flipped(_holder, point));
	if (control != _hoveredControl) {
		if (_hoveredControl)
			_hoveredControl->mouseLeave();
		_hoveredControl = control;
		if (control)
			control->mouseEnter();
		_holder->refreshView();
	}
}

void CScene::mouseUp(unsigned int, LPoint)
{
	if (_pressedControl) {
		_pressedControl->endTracking();
		_pressedControl = 0;
		_holder->refreshView();
	}
}

void CScene::mouseDragged(unsigned int, LPoint point)
{
	if (_pressedControl == 0)
		return;
	CSceneControl *control = controlHitTest(_flipped(_holder, point));
	if (control == _pressedControl) {
		int state = control->controlState();
		_pressedControl->mouseEnter();
		if (state == kControlStatePressed)
			return;
	} else {
		if (_pressedControl->controlState() != kControlStatePressed)
			return;
		_pressedControl->mouseLeave();
	}
	_holder->refreshView();
}

// The controls are billboards drawn last, with their titles in the
// controls' font.
void CScene::_renderControls(CFrameDrawer *drawer)
{
	int count = _controls ? _controls->count() : 0;
	if (count == 0)
		return;
	performControlsLayoutIfNeeded();
	if (_controlsFont == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_controlsFont = CFontDrawable::_alloc()->initWithHolderAndParameters(
		    _holder, LString::stringWithCString("Arial"), 0xe, 0);
		if (holder)
			holder->endAccumulationBlock();
	}
	CFontDrawableData *font = 0;
	if (_controlsFont) {
		CDrawableModelData *data = _controlsFont->detachModelDataInstance();
		if (data && data->getClass()->isSubclassOf(LF_CLASSOF(CFontDrawableData))) {
			font = (CFontDrawableData *)data;
			font->setDrawFlags(kTextCenterV | kTextCenterH);
			font->_storesState = false;
		}
	}
	for (int i = 0; i < count; i++)
		((CSceneControl *)_controls->objectAtIndex(i))->render(drawer, font);
	if (font)
		drawer->renderDrawableData(font, 4);
}

// ---------------------------------------------------------------------------
// CScenePropertiesSet

// A record's header.
struct SStateRecord {
	int dataOffset;	// from the record's start
	unsigned int size;
	unsigned int flags;
};

CScenePropertiesSet::CScenePropertiesSet() : _data(0) {}

CScenePropertiesSet *CScenePropertiesSet::init()
{
	return LObject::init() ? this : 0;
}

void CScenePropertiesSet::dealloc()
{
	LRelease(_data);
	_data = 0;
	LObject::dealloc();
}

void CScenePropertiesSet::encodeWithCoder(LCoder *coder) const
{
	coder->encodeObject(_data);
}

LObject *CScenePropertiesSet::initWithDecoder(const LCoder *coder)
{
	LObject *obj = coder->decodeObject();
	_data = obj && obj->getClass()->isSubclassOf(LF_CLASSOF(LData)) ? (LData *)obj : 0;
	return this;
}

void CScenePropertiesSet::storePropertyOwner(CSceneStatePropertyOwner *owner, unsigned int flags)
{
	if (owner == 0 || owner->_stateOwnerName == 0 || *owner->_stateOwnerName == 0)
		return;
	const char *name = owner->_stateOwnerName;
	if (_data == 0)
		_data = LData::_alloc()->init();
	size_t length = strlen(name);
	SStateRecord header = {(int)(length + 13), 0, flags};
	unsigned int start = _data->length();
	_data->appendBytes(&header, sizeof header);
	_data->appendBytes(name, (unsigned int)length + 1);
	owner->storeSceneStateInStream(_data, flags);
	((SStateRecord *)((char *)_data->bytes() + start))->size = _data->length() - start;
}

void CScenePropertiesSet::storeDrawableData(CDrawableModelData *data, TOrder order)
{
	storePropertyOwner(data, order);
}

void CScenePropertiesSet::captureStartedWithScene(CScene *scene)
{
	storePropertyOwner(scene->_camera, 0);
	LArray *lights = scene->_lights;
	int count = lights ? lights->count() : 0;
	for (int i = 0; i < count; i++)
		storePropertyOwner((CContextLight *)lights->objectAtIndex(i), 0);
}

// Restores each stored owner and lets it draw itself.
void CScenePropertiesSet::drawStoredObjects(CScene *scene)
{
	if (_data == 0)
		return;
	const char *p = (const char *)_data->bytes();
	unsigned int left = _data->length();
	while (left != 0) {
		if (left < sizeof(SStateRecord))
			return;
		const SStateRecord *record = (const SStateRecord *)p;
		if (left < record->size)
			return;
		CSceneStatePropertyOwner *owner = scene->stateOwnerWithName(p + sizeof(SStateRecord));
		if (owner) {
			owner->restoreSceneStateFromData(p + record->dataOffset, record->flags);
			owner->renderSelfHolderInScene(scene, record->flags);
		}
		left -= record->size;
		p += record->size;
		if ((int)left <= 0 || record->size == 0)
			break;
	}
}
