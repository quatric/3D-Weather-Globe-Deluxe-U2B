// GroundElementsManager: the ground elements of the current holder (cached
// up to 1.5 times a frame's count, 120 at least), their DEM loading queue,
// the buffers every element shares and the water and city light colors.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(GroundElementsManager, 0)
LF_SELECTOR(GroundElementsManager, _contextWillBeDestroyed);
LF_SELECTOR(GroundElementsManager, _lodWasChanged);
LF_SELECTOR(GroundElementsManager, _reliefStateChanged);

static const Color kDarkWaterColor(0.02f, 0.07f, 0.12f, 1.0f);
static const Color kLightWaterColor(0.3f, 0.4f, 0.7f, 1.0f);
static const Color kConstNightGlowsColor(0.5f, 1.0f, 0.2f, 0.0f);

bool GroundElementsManager::mDrawWithRelief = true;
float GroundElementsManager::mAnimatedWaterVisibility = 1.0f;
bool GroundElementsManager::mAnimatedWaterIsEnabled = false;
float GroundElementsManager::mAnimatedWaterSpecsVisibility = 1.0f;
float GroundElementsManager::mWaterTextureScale = 3.0f;
Color GroundElementsManager::mNightCityGlowsColor = kConstNightGlowsColor;
Color GroundElementsManager::mLightWaterConstColor = kLightWaterColor;
Color GroundElementsManager::mDarkWaterColor = kDarkWaterColor;
Color GroundElementsManager::mLightWaterColor = kLightWaterColor;
Color GroundElementsManager::mLightDarkDiffColor = kLightWaterColor - kDarkWaterColor;
Color GroundElementsManager::mWaterSpecularColor(1.0f, 1.0f, 1.0f, 0.0f);

static int sNumCanceled;	// for profiling

static inline STextureID textureIDFromKey(unsigned int key)
{
	STextureID id;
	id.x = (short)(key & 0xfff);
	id.y = (short)((key >> 12) & 0xfff);
	id.level = (char)(key >> 24);
	return id;
}

GroundElementsManager::GroundElementsManager()
	: _context(0), _holder(0), _lodBiasApplied(false), _data(0), _minCachedElements(120), _waterParts(0)
{
	_lodBias = Preferences::terrainLODBiasValue();
	mDrawWithRelief = Preferences::drawWithReliefEnabled();
	LNotificationCenter *center = LNotificationCenter::center();
	center->addObserver(this, "_lodWasChanged", Preferences::kTerrainLODWasChanged, 0);
	center->addObserver(this, "_reliefStateChanged", Preferences::kRenderWith3DStateWasChanged, 0);
}

void GroundElementsManager::dealloc()
{
	LRelease(_waterParts);
	_waterParts = 0;
	LNotificationCenter::center()->removeObserver(this);
	LObject::dealloc();
}

intptr_t GroundElementsManager::_contextWillBeDestroyed(char *)
{
	_context = 0;
	return 0;
}

intptr_t GroundElementsManager::_lodWasChanged(char *)
{
	_lodBiasApplied = false;
	return 0;
}

intptr_t GroundElementsManager::_reliefStateChanged(char *)
{
	mDrawWithRelief = Preferences::drawWithReliefEnabled();
	return 0;
}

int GroundElementsManager::_compareGroundElementsByTime(const void *a, const void *b, const void *)
{
	return (int)((*(GroundElement *const *)b)->_lastRenderTime - (*(GroundElement *const *)a)->_lastRenderTime);
}

void GroundElementsManager::setLODBias(float bias)
{
	_lodBias = bias;
	TLIntPtrMap<GroundElement *>::hashTableIterator it(&_data->_elements);
	while (it.next())
		it.entry()->value->_setLODBias(_lodBias);
}

void GroundElementsManager::setAnimatedWaterDayFactor(float factor)
{
	float k = factor * 0.4f + 0.6f;
	mDarkWaterColor = kDarkWaterColor * k;
	mLightWaterColor = mLightWaterConstColor * k;
	mLightDarkDiffColor = mLightWaterConstColor * k - kDarkWaterColor * k;
}

// Forgets `count` of the elements not drawn this frame.
void GroundElementsManager::_releaseOldestElements(int count)
{
	TArray<GroundElement *> elements;
	elements.setComparator(_compareGroundElementsByTime);
	elements.setKeepsSorted(true);
	TLIntPtrMap<GroundElement *>::hashTableIterator it(&_data->_elements);
	while (it.next())
		if (!_data->_drawn || !_data->_drawn->containsObject(it.entry()->key))
			elements.addObject(it.entry()->value);
	int n = (int)elements.count() < count ? (int)elements.count() : count;
	for (int i = 0; i < n; i++)
		_data->_elements.removeObjectForKey(CTextureLoader::keyForID(elements[i]->_id));
}

// ---------------------------------------------------------------------------
// Shared resources.

LTaskQueue *GroundElementsManager::loadingQueue()
{
	if (!_data->_queue) {
		// The queue keeps itself while it runs.
		_data->_queue = LTaskQueue::detachNewQueue();
		_data->_queue->setTaskQueueComparator(LoadDEMTask::compare, &_data->_cameraKey);
		_data->_queue->setThreadsCount(4);
		_data->_queue->runQueue();
	}
	return _data->_queue;
}

CVertexBuffer *GroundElementsManager::sharedEmptyZNrmBufferForLOD(int lod)
{
	if (!_data->_zNrmBuffers[lod]) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_data->_zNrmBuffers[lod] = CVertexBuffer::_alloc()->initWithDataSource(this, _holder, 0, 0);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _data->_zNrmBuffers[lod];
}

CVertexBuffer *GroundElementsManager::sharedXYTBufferForLOD(int lod)
{
	if (!_data->_xyBuffers[lod]) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_data->_xyBuffers[lod] = CVertexBuffer::_alloc()->initWithDataSource(this, _holder, 0, 0);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _data->_xyBuffers[lod];
}

CRenderProfile *GroundElementsManager::sharedRenderProfile()
{
	if (!_data->_profile)
		_data->_profile = LRetain(CRenderProfile::renderProfileByFileDesc(
			LString::stringWithCString("RENDERPROFILES\\GroundElement.yaf"), _holder));
	return _data->_profile;
}

CBaseTexture *GroundElementsManager::darkWaterColorTexture()
{
	if (!_data->_darkWaterTexture)
		_data->_darkWaterTexture =
			LRetain(C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\water_dark.dds"), _holder));
	return _data->_darkWaterTexture;
}

// ---------------------------------------------------------------------------
// The shared buffers.

unsigned int GroundElementsManager::streamNumberForBuffer(CVertexBuffer *vb)
{
	for (int i = 0; i < 15; i++)
		if (_data->_zNrmBuffers[i] == vb)
			return 1;
	return 0;
}

// The buffer's LOD, -1 if it isn't one of the shared ones.
static int sharedBufferLOD(GroundElementsManagerData *data, CVertexBuffer *vb)
{
	for (int i = 0; i < 15; i++)
		if (data->_zNrmBuffers[i] == vb)
			return i;
	for (int i = 0; i < 15; i++)
		if (data->_xyBuffers[i] == vb)
			return i;
	return -1;
}

unsigned int GroundElementsManager::vertCountForBuffer(CVertexBuffer *vb)
{
	int lod = sharedBufferLOD(_data, vb);
	if (lod < 0)
		return 0;
	int size = 1 << (lod >> 1);
	int count = (size + 1) * (size + 1);
	if (lod & 1)
		count += size * size;
	return count + size * 4;
}

CShader *GroundElementsManager::renderShaderForBuffer(CVertexBuffer *)
{
	CRenderProfile *profile = sharedRenderProfile();
	return profile ? profile->shader() : 0;
}

void GroundElementsManager::_fillSharedXYBufferForLOD(int lod, unsigned char bounds)
{
	CVertexBuffer *vb = _data->_xyBuffers[lod];
	if (!vb)
		return;
	CRenderProfile *profile = sharedRenderProfile();
	long stride = profile->vertexStructSize(streamNumberForBuffer(vb));
	StVertexBufferLocker *locker = vb->detachLocker(0);
	char *data = (char *)locker->data();
	int uvOffset = (int)profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
	if (uvOffset >= 0) {
		TAdaptiveVBFiller<0, 0, 0, 1>::fillVertices_LOD(data, stride, lod, 0, 0, 0, uvOffset,
		                                                GroundElement::borderZValue());
		if ((lod & 1) == 0) {
			TAdaptiveVBDegradedFiller<0, 0, 0, 1>::fillVertices_LOD_2n(lod, stride, 0, 0, 0, data + uvOffset);
			if (bounds && lod == 14) {
				int n = IAdaptiveMeshDataSource::boundsArrayElementsCount(7);
				if (!_data->_mins)
					_data->_mins = new Vector2[n];
				if (!_data->_maxs)
					_data->_maxs = new Vector2[n];
				updateBoundsWhole(lod, data, stride, uvOffset, -1, _data->_mins, _data->_maxs, 0, 0);
			}
		} else {
			TAdaptiveVBDegradedFiller<0, 0, 0, 1>::fillVertices_LOD_2n_plus_1(lod, stride, 0, 0, 0, data + uvOffset);
		}
	}
	delete locker;
}

void GroundElementsManager::fillBuffer(CVertexBuffer *vb)
{
	CRenderProfile *profile = sharedRenderProfile();
	long stride = profile->vertexStructSize(streamNumberForBuffer(vb));
	int lod = sharedBufferLOD(_data, vb);
	if (lod < 0)
		return;
	if (_data->_zNrmBuffers[lod] != vb) {
		_fillSharedXYBufferForLOD(lod, 1);
		return;
	}
	StVertexBufferLocker *locker = vb->detachLocker(0);
	char *data = (char *)locker->data();
	int zOffset = (int)profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int colorOffset = (int)profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	if (zOffset >= 0 && colorOffset >= 0) {
		TAdaptiveVBFiller<0, 1, 1, 0>::fillVertices_LOD(data, stride, lod, 0, zOffset, colorOffset, 0,
		                                                GroundElement::borderZValue());
		if ((lod & 1) == 0)
			TAdaptiveVBDegradedFiller<0, 1, 1, 0>::fillVertices_LOD_2n(lod, stride, 0, data + zOffset,
			                                                           data + colorOffset, 0);
		else
			TAdaptiveVBDegradedFiller<0, 1, 1, 0>::fillVertices_LOD_2n_plus_1(lod, stride, 0, data + zOffset,
			                                                                 data + colorOffset, 0);
	}
	delete locker;
}

// ---------------------------------------------------------------------------
// The elements.

GroundElementsManagerData *GroundElementsManager::_specificDataForHolder(CHolder *holder)
{
	LF_CHECK_PTR(holder);
	GroundElementsManagerData *data =
		(GroundElementsManagerData *)holder->objectForKey(kGroundElementsManagerDataKey);
	if (!data) {
		data = GroundElementsManagerData::_alloc();
		data->init();
		holder->setObjectForKey(data, kGroundElementsManagerDataKey);
		data->release();
	}
	return data;
}

GroundElement *GroundElementsManager::_createElement(const STextureID &id)
{
	GroundElement *element = GroundElement::_alloc()->initWithTextureID(id, _holder);
	if (!_waterParts)
		_waterParts = LData::_alloc()->initWithResource(LString::stringWithCString("waterParts"),
		                                               LString::stringWithCString("arr"), getRenderLibModule());
	const float *parts = _waterParts ? (const float *)_waterParts->bytes() : 0;
	if (element && parts && id.level == 5)
		element->_hasWater = parts[id.y * 324 + id.x] > 0.0001f;
	return element ? LAutorelease(element) : 0;
}

GroundElement *GroundElementsManager::elementWithID(const STextureID &id, unsigned char create)
{
	intptr_t key = CTextureLoader::keyForID(id);
	GroundElement *element = _data->_elements.objectForKey(key);
	if (create && !element) {
		element = _createElement(id);
		if (element)
			_data->_elements.setObjectForKey(element, key);
	}
	return element;
}

GroundElementModel *GroundElementsManager::groundMesh(const STextureID &id, unsigned char create)
{
	GroundElement *element = elementWithID(id, create);
	if (!element)
		return 0;
	GlobeAdaptiveMeshData *data = element->_drawableData();
	CDrawableModel *model = data ? data->_model : 0;
	if (model && model->getClass()->isSubclassOf(LF_CLASSOF(GroundElementModel)))
		return (GroundElementModel *)model;
	return 0;
}

void GroundElementsManager::markElementAsDEMLoading(GroundElement *element)
{
	if (element->_holder)
		_specificDataForHolder(element->_holder)->_loading.setValueForKey(0, CTextureLoader::keyForID(element->_id));
}

void GroundElementsManager::markElementAsDEMLoaded(GroundElement *element)
{
	if (element->_holder)
		_specificDataForHolder(element->_holder)->_loading.removeValueForKey(CTextureLoader::keyForID(element->_id));
}

// ---------------------------------------------------------------------------
// Frames.

void GroundElementsManager::startFrame(CFrameDrawer *drawer, CBaseTexture *waterTexture, LPoint cameraMapPoint)
{
	CContext *context = drawer->context();
	if (_context != context) {
		_context = context;
		if (context && _holder != context->holder()) {
			LNotificationCenter::center()->removeObserver(this, &ContextWillBeDetachedNotification, kLock);
			_holder = context->holder();
			_data = _specificDataForHolder(_holder);
			LNotificationCenter::center()->addObserver(this, "_contextWillBeDestroyed",
			                                           &ContextWillBeDetachedNotification, _holder);
			sharedXYTBufferForLOD(14);
		}
	}
	LTaskQueue *queue = loadingQueue();
	if (queue->queuedTasksCount() != 0) {
		// The camera's level 5 column and row, with the columns from the
		// date line as the patches are numbered.
		int column = (int)(cameraMapPoint.x * 324.0f + 0.1f);
		column = column < 162 ? column + 162 : column - 162;
		_data->_cameraKey = (column & 0xffff) | (int)(cameraMapPoint.y * 162.0f + 0.1f) << 16;
		queue->sortQueue();
	}
	if (!_lodBiasApplied) {
		setLODBias(Preferences::terrainLODBiasValue());
		_lodBiasApplied = true;
	}
	unsigned int keep = (unsigned int)(_data->_postedCount * 3) >> 1;
	if ((int)keep < (int)_minCachedElements)
		keep = _minCachedElements;
	if (keep < _data->_elements.count())
		_releaseOldestElements(_data->_elements.count() - keep);
	delete _data->_drawn;
	_data->_drawn = new CHashSet;
	_data->_postedCount = 0;
	_data->_toDraw.removeAllObjects();
	LAssign(_data->_waterTexture, waterTexture);
}

void GroundElementsManager::postElementToQueue(const STextureID &id, int textureLevel, float farClip)
{
	GroundElement *element = elementWithID(id, 1);
	if (!element)
		return;
	_data->_postedCount++;
	if (GlobeAdaptiveMeshData *data = element->tryToRender(textureLevel, farClip)) {
		_data->_toDraw.addObject(data);
		_data->_drawn->addObject(CTextureLoader::keyForID(id));
	}
}

void GroundElementsManager::endFrame(CFrameDrawer *drawer)
{
	for (unsigned int i = 0; i < _data->_toDraw.count(); i++) {
		GlobeAdaptiveMeshData *data = _data->_toDraw[i];
		data->setTextureForSource(_data->_waterTexture, 2);
		drawer->renderDrawableData(data, 1);
	}
	_data->_toDraw.removeAllObjects();
	// A load waits one frame for its element to be drawn, then is canceled.
	TArray<intptr_t> waited, canceled;
	CHashTableIterator it(&_data->_loading);
	while (it.next()) {
		intptr_t key = it.key();
		intptr_t frames = it.value();
		if (frames < 1) {
			if (!_data->_drawn->containsObject(key))
				waited.addObject(key);
		} else if (_data->_queue) {
			canceled.addObject(key);
		}
	}
	for (unsigned int i = 0; i < waited.count(); i++) {
		intptr_t frames = 0;
		_data->_loading.getValueForKey(&frames, waited[i]);
		_data->_loading.setValueForKey(frames + 1, waited[i]);
	}
	for (unsigned int i = 0; i < canceled.count(); i++) {
		unsigned int key = (unsigned int)canceled[i];
		if (GroundElement *element = elementWithID(textureIDFromKey(key), 0))
			element->_demLoading = false;
		sNumCanceled++;
		_data->_queue->removeTaskWithID((int)key, 0);
		_data->_loading.removeValueForKey(key);
	}
}
