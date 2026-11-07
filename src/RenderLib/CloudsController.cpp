// The clouds frames: their textures, the animation through them, and their
// drawing over the earth.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CloudsController, 0)
LF_SELECTOR(CloudsController, _cloudsEnableStateWasChanged);

static CloudsManager *cloudsManager()
{
	return CWManager::instance()->cloudsManager();
}

CloudsController::CloudsController()
	: _holder(0), _east(0), _west(0), _eastNext(0), _westNext(0), _frameStart(0.0), _invalid(true),
	  _blendFactor(0.0f), _visibility(1.0f), _frame(0), _paused(false), _animation(kCloudsAnimationNone),
	  _shownFrame(-10)
{
}

void CloudsController::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	releaseAndClear(_east);
	releaseAndClear(_west);
	releaseAndClear(_eastNext);
	releaseAndClear(_westNext);
	LObject::dealloc();
}

CloudsController *CloudsController::initWithHolder(CHolder *holder)
{
	_holder = holder;
	findCloudsTextureFormat();
	LNotificationCenter *center = LNotificationCenter::center();
	center->addObserver(this, "_contextWasAttached", &ContextWasAttachedNotification, _holder);
	center->addObserver(this, "_cloudsTextureSizeWasChanged", Preferences::kCloudsTextureSizeWasChanged, 0);
	center->addObserver(this, "_cloudsEnableStateWasChanged", Preferences::kCloudsEnableStateWasChanged, 0);
	center->addObserver(this, "_cloudsEnableStateWasChanged", Preferences::kCloudsAnimationQualityWasChanged, 0);
	return this;
}

void CloudsController::invalidateCloudsTextures()
{
	releaseAndClear(_west);
	releaseAndClear(_east);
	releaseAndClear(_westNext);
	releaseAndClear(_eastNext);
}

void CloudsController::invalidateAnimationParams()
{
	_invalid = true;
	_frameStart = LTimer::currentTimeValue();
}

void CloudsController::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeIntInStream(_frame, stream);
	storeIntInStream(_animation, stream);
	storeFloatInStream(_blendFactor, stream);
	storeFloatInStream(_visibility, stream);
}

const void *CloudsController::restoreSceneStateFromData(const void *data, unsigned int)
{
	data = loadIntFromStream(data, &_frame);
	data = loadIntFromStream(data, (int *)&_animation);
	data = loadFloatFromStream(data, &_blendFactor);
	return loadFloatFromStream(data, &_visibility);
}

// The data of a layer, with the current and the next frame's textures of
// the hemisphere.
CCloudsMeshData *CloudsController::_detachCloudsMeshData(CCloudsMesh *mesh, unsigned char east)
{
	if (!mesh)
		return 0;
	CCloudsMeshData *data = lobj_cast<CCloudsMeshData>(mesh->detachModelDataInstance());
	if (!data)
		return 0;
	data->_storesState = false;
	data->setTextureForSource(east ? _east : _west, 0);
	data->setTextureForSource(east ? _eastNext : _westNext, 1);
	return data;
}

void CloudsController::_render(BaseScene *scene, CFrameDrawer *drawer)
{
	if (_visibility < 0.01f)
		return;
	PanelDrawIterator *iterator = scene->_controller->detachDrawIterator(0);
	float blend = _blendFactor;
	while (iterator->next()) {
		CCloudsMeshData *east = _detachCloudsMeshData(scene->_cloudsMesh, 1);
		CCloudsMeshData *west = _detachCloudsMeshData(scene->_cloudsMesh, 0);
		scene->_adjustCloudsMeshData(east, 1, iterator);
		scene->_adjustCloudsMeshData(west, 0, iterator);
		east->adjustWithBlendFactor(blend);
		east->setVisibility(_visibility);
		west->adjustWithBlendFactor(blend);
		west->setVisibility(_visibility);
		drawer->renderDrawableData(east, 3);
		drawer->renderDrawableData(west, 3);
	}
	delete iterator;
}

// DXT1 where the card has it.
void CloudsController::findCloudsTextureFormat()
{
	CContext *context = _holder->context();
	if (!context)
		return;
	bool compressed = context->caps()->CompressedTexturesAreSupported();
	cloudsManager()->setFrameTextureFormat(compressed ? D3DFMT_DXT1 : D3DFMT_A1R5G5B5);
}

int CloudsController::currentBackInTimeOffset()
{
	unsigned int index = (unsigned int)_shownFrame <= 10000 ? _shownFrame : 0;
	CloudsManager *manager = cloudsManager();
	return manager ? manager->backInTimeOffsetForFrameAtIndex(index) : -1;
}

void CloudsController::setBackInTimeOffset(int hours)
{
	if (CloudsManager *manager = cloudsManager()) {
		int index = manager->frameIndexForBackInTimeOffset(hours);
		if (index >= 0)
			_frame = index;
	}
	_invalid = true;
}

C2DTexture *CloudsController::_createCloudsTexture()
{
	int size = 0;
	if (CloudsManager *manager = cloudsManager())
		size = (int)manager->frameTextureSize().width;
	if (!size || !_holder)
		return 0;
	CHolder *holder = _holder;
	holder->beginAccumulationBlock();
	C2DTexture *texture = C2DTexture::_alloc()->initWithHolderAndParams(
		_holder, size, size, cloudsManager()->_frameFormat, D3DUSAGE_WRITEONLY, 1);
	holder->endAccumulationBlock();
	return texture;
}

C2DTexture *CloudsController::textureForEastNext()
{
	if (!_eastNext)
		_eastNext = _createCloudsTexture();
	return _eastNext;
}

C2DTexture *CloudsController::textureForEast()
{
	if (!_east)
		_east = _createCloudsTexture();
	return _east;
}

C2DTexture *CloudsController::textureForWestNext()
{
	if (!_westNext)
		_westNext = _createCloudsTexture();
	return _westNext;
}

C2DTexture *CloudsController::textureForWest()
{
	if (!_west)
		_west = _createCloudsTexture();
	return _west;
}

intptr_t CloudsController::_contextWasAttached(char *)
{
	findCloudsTextureFormat();
	invalidateCloudsTextures();
	invalidateAnimationParams();
	return 0;
}

intptr_t CloudsController::_cloudsTextureSizeWasChanged(char *)
{
	invalidateCloudsTextures();
	invalidateAnimationParams();
	return 0;
}

intptr_t CloudsController::_cloudsEnableStateWasChanged(char *)
{
	invalidateAnimationParams();
	if (!Preferences::cloudsEnabled()) {
		invalidateCloudsTextures();
	} else {
		TCloudsAnimation quality = Preferences::cloudsAnimationQuality();
		if (quality == kCloudsAnimationNone || quality == kCloudsAnimationLow)
			invalidateCloudsTextures();
		_invalid = true;
	}
	return 0;
}

bool CloudsController::swapCloudsTextures()
{
	bool missing = !_east || !_west || !_eastNext || !_westNext;
	C2DTexture *texture = textureForEastNext();
	_eastNext = textureForEast();
	_east = texture;
	texture = textureForWestNext();
	_westNext = textureForWest();
	_west = texture;
	return missing;
}

bool CloudsController::_adjustCloudsToFrame(int frame, TCloudsAnimation animation)
{
	CloudsManager *manager = cloudsManager();
	int count;
	if (!manager || !(count = manager->framesCount()))
		return false;
	frame %= count;
	if (_shownFrame == frame && !_invalid)
		return true;
	switch (animation) {
	case kCloudsAnimationNone:
		if (!_invalid)
			return true;
		frame = count - 1;
		// fall through
	case kCloudsAnimationLow:
		manager->updateTexture(textureForEast(), frame, 1);
		manager->updateTexture(textureForWest(), frame, 0);
		break;
	case kCloudsAnimationHigh: {
		if (count < 2) {
			_frameStart = 0.0;
			return false;
		}
		// After a step to the next frame the swapped textures are current.
		bool missing = swapCloudsTextures();
		bool stepped = (frame == 0 && _shownFrame == count - 1) || frame == _shownFrame + 1;
		if (!stepped || missing) {
			int previous = frame - 1 < 0 ? count - 1 : frame - 1;
			manager->updateTexture(textureForEast(), previous, 1);
			manager->updateTexture(textureForWest(), previous, 0);
		}
		manager->updateTexture(textureForEastNext(), frame, 1);
		manager->updateTexture(textureForWestNext(), frame, 0);
		break;
	}
	default:
		break;
	}
	_invalid = false;
	_shownFrame = frame;
	LMap *info = LMap::mapWithObjectsAndKeys(LValue::valueWithInt(_shownFrame), kCloudsFrameIndexKey,
	                                         LValue::valueWithInt(currentBackInTimeOffset()),
	                                         kCloudsFrameBITOffsetKey, 0);
	LNotificationCenter::center()->postNotification(kCloudsFrameWasChanged, this, info);
	return true;
}

// Playing back a captured frame.
void CloudsController::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	CloudsManager *manager = cloudsManager();
	if (!manager || !manager->framesCount())
		return;
	BaseScene *baseScene = lobj_cast<BaseScene>(scene);
	if (baseScene && _adjustCloudsToFrame(_frame, _animation))
		_render(baseScene, baseScene->_frameDrawer);
}

void CloudsController::renderInScene(BaseScene *scene, CFrameDrawer *drawer)
{
	CloudsManager *manager = cloudsManager();
	unsigned int count;
	if (!manager || !(count = manager->framesCount()))
		return;
	double now = LTimer::currentTimeValue();
	double elapsed = now - _frameStart;
	float delta = Preferences::cloudsAnimationDeltaTime();
	_blendFactor = clamp01((float)(elapsed / delta));
	_visibility = clamp01(((1.0f - scene->_controller->scale()) - 0.3f) * 7.0f);
	if (delta < elapsed && !_paused) {
		_blendFactor = 0.0f;
		_frameStart = now;
		_frame = (_frame + 1) % count;
	}
	_animation = Preferences::cloudsAnimationQuality();
	if ((unsigned int)_animation < kCloudsAnimationHigh)
		_blendFactor = 0.0f;
	else if (_animation == kCloudsAnimationHigh && _paused)
		_blendFactor = 1.0f;
	if (_adjustCloudsToFrame(_frame, _animation)) {
		scene->storePropertyOwnerIfNeeded(this, 0);
		_render(scene, drawer);
	}
}
