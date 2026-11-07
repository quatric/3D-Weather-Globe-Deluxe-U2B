// GroundElement: a level 5 patch drawn as an adaptive mesh with its terrain.
// Its DEM (131 x 131 16-bit samples, the outer ones only for the normals) is
// read into heights and packed normals for each LOD; a LOD's data is freed
// once it is in its vertex buffer and read again if the buffer is refilled.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(GroundElement, 0)
LF_SELECTOR(GroundElement, _demLoadComplete);
LF_SELECTOR(GroundElement, _mainThreadReleaser);

// Where the border vertices (the skirts) go, below the ground.
static const float sBorderZValue = -0.002f;

// For profiling.
static int sNumNotifications;

GroundElement::GroundElement()
	: _holder(0), _unused28(0), _demMissing(true), _unused2D(false), _lastRenderTime(0), _heights(0), _data(0),
	  _demDisabled(false), _demLoading(false), _demAvailable(false), _minDrawnLOD(0), _maxDrawnLOD(0),
	  _hasWater(true), _computeZBounds(true), _minZ(0), _maxZ(0)
{
	_id.x = 0;
	_id.y = 0;
	_id.level = 0;
	memset(_heightVBs, 0, sizeof(_heightVBs));
}

GroundElement *GroundElement::initWithTextureID(const STextureID &id, CHolder *holder)
{
	if (LObject::init() == 0)
		return 0;
	_holder = holder;
	_id = id;
	int n = IAdaptiveMeshDataSource::boundsArrayElementsCount(7);
	_minZ = new float[n];
	_maxZ = new float[n];
	memset(_minZ, 0, n * sizeof(float));
	memset(_maxZ, 0, n * sizeof(float));
	return this;
}

void GroundElement::releaseMemoryResources()
{
	StSharableLocker lock(&_heightsLock);
	LRelease(_heights);
	_heights = 0;
	_demAvailable = false;
}

void GroundElement::dealloc()
{
	LRelease(_data);
	_data = 0;
	_releaseHeightVBs();
	delete[] _minZ;
	_minZ = 0;
	delete[] _maxZ;
	_maxZ = 0;
	releaseMemoryResources();
	LObject::dealloc();
}

void GroundElement::_releaseHeightVBs()
{
	for (int i = 0; i < 15; i++) {
		LRelease(_heightVBs[i]);
		_heightVBs[i] = 0;
	}
}

float GroundElement::borderZValue()
{
	return sBorderZValue;
}

bool GroundElement::requiresWaterRendering()
{
	return _hasWater && GroundElementsManager::mAnimatedWaterIsEnabled;
}

void GroundElement::finalize()
{
	if (GroundElementsManager::classObject()->hasSingleton())
		GroundElementsManager::classObject()->killSingleton();
}

// ---------------------------------------------------------------------------
// The frame-wide water and city lights settings.

void GroundElement::setAnimatedWaterVisibility(float visibility)
{
	GroundElementsManager::mAnimatedWaterVisibility = visibility;
	GroundElementsManager::mAnimatedWaterIsEnabled =
		Preferences::animatedWaterEnabled() && GroundElementsManager::mAnimatedWaterVisibility > 0.01f;
}

void GroundElement::setWaterTextureScale(float scale)
{
	GroundElementsManager::mWaterTextureScale = scale;
}

void GroundElement::setAnimatedWaterLightColor(const Color *color)
{
	GroundElementsManager::mLightWaterConstColor = *color;
}

void GroundElement::setAnimatedWaterSpecularsVisibility(float visibility)
{
	GroundElementsManager::mAnimatedWaterSpecsVisibility = visibility;
	GroundElementsManager::mWaterSpecularColor = Color(visibility, visibility, visibility, 0.0f);
}

void GroundElement::setAnimatedWaterDayFactor(float factor)
{
	GroundElementsManager::setAnimatedWaterDayFactor(factor);
}

void GroundElement::setNightGlowsVisibility(float visibility)
{
	GroundElementsManager::mNightCityGlowsColor = Color(0.5f, 1.0f, 0.2f, 0.0f) * visibility;
}

void GroundElement::setLODBias(float bias)
{
	GroundElementsManager::instance()->setLODBias(bias);
}

float GroundElement::LODBias()
{
	return GroundElementsManager::instance()->_lodBias;
}

void GroundElement::_setLODBias(float bias)
{
	if (_data) {
		CDrawableModel *model = _data->_model;
		if (model && model->getClass()->isSubclassOf(LF_CLASSOF(GroundElementModel)))
			((GroundElementModel *)model)->setLODBias(bias);
	}
}

// ---------------------------------------------------------------------------
// The manager's elements.

bool GroundElement::getZRange(const STextureID &id, float *minZ, float *maxZ)
{
	GroundElement *element = GroundElementsManager::instance()->elementWithID(id, 0);
	if (!element) {
		*minZ = 0.0f;
		*maxZ = 0.0f;
		return false;
	}
	Vector2 *mins, *maxs;
	float *minZs, *maxZs;
	element->getMinMaxCorners(&mins, &maxs, &minZs, &maxZs);
	*minZ = minZs[0];
	*maxZ = maxZs[0];
	return true;
}

void GroundElement::startFrame(CFrameDrawer *drawer, CBaseTexture *waterTexture, LPoint cameraMapPoint)
{
	GroundElementsManager::instance()->startFrame(drawer, waterTexture, cameraMapPoint);
}

void GroundElement::endFrame(CFrameDrawer *drawer)
{
	GroundElementsManager::instance()->endFrame(drawer);
}

GroundElement *GroundElement::elementWithID(const STextureID &id, unsigned char create)
{
	return GroundElementsManager::instance()->elementWithID(id, create);
}

void GroundElement::postElementToQueue(const STextureID &id, int textureLevel, float farClip)
{
	GroundElementsManager::instance()->postElementToQueue(id, textureLevel, farClip);
}

GroundElementModel *GroundElement::groundMesh(const STextureID &id, unsigned char create)
{
	return GroundElementsManager::instance()->groundMesh(id, create);
}

// ---------------------------------------------------------------------------
// The adaptive mesh data source.

char GroundElement::logGridFullSize()
{
	return 7;
}

CRenderProfile *GroundElement::renderProfile()
{
	return GroundElementsManager::instance()->sharedRenderProfile();
}

CVertexBuffer *GroundElement::vertexBufferForLOD(int stream, int lod)
{
	GroundElementsManager *manager = GroundElementsManager::instance();
	if (stream == 0)
		return manager->sharedXYTBufferForLOD(lod);
	if (stream != 1)
		return 0;
	if (!_demAvailable || !GroundElementsManager::mDrawWithRelief)
		return manager->sharedEmptyZNrmBufferForLOD(lod);
	if (!_heightVBs[lod]) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_heightVBs[lod] = CVertexBuffer::_alloc()->initWithDataSource(this, _holder, 0, 0);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _heightVBs[lod];
}

void GroundElement::getMinMaxCorners(Vector2 **mins, Vector2 **maxs, float **minZ, float **maxZ)
{
	GroundElementsManagerData *data = GroundElementsManager::instance()->_data;
	*mins = data->_mins;
	*maxs = data->_maxs;
	*minZ = _minZ;
	*maxZ = _maxZ;
}

CSharableResource *GroundElement::boundsMutex()
{
	return &_boundsLock;
}

// ---------------------------------------------------------------------------
// The height buffers.

int GroundElement::_lodFromBuffer(CVertexBuffer *vb)
{
	for (int i = 0; i < 15; i++)
		if (_heightVBs[i] == vb)
			return i;
	return -1;
}

unsigned int GroundElement::streamNumberForBuffer(CVertexBuffer *)
{
	return 1;
}

CShader *GroundElement::renderShaderForBuffer(CVertexBuffer *)
{
	if (!_data)
		return 0;
	return _data->_model->renderProfile()->shader();
}

unsigned int GroundElement::vertCountForBuffer(CVertexBuffer *vb)
{
	int lod = _lodFromBuffer(vb);
	if (lod < 0)
		return 0;
	int size = 1 << (lod >> 1);
	int count = (size + 1) * (size + 1);
	if (lod & 1)
		count += size * size;
	return count + size * 4;
}

void GroundElement::fillBuffer(CVertexBuffer *vb)
{
	int lod = _lodFromBuffer(vb);
	if (lod < 0)
		return;
	StVertexBufferLocker *locker = vb->detachLocker(0);
	CRenderProfile *profile = _data->_model->renderProfile();
	int zOffset = (int)profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int colorOffset = (int)profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	int degradedColorOffset = (int)profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 1, 0);
	if (zOffset >= 0 && colorOffset >= 0 && degradedColorOffset >= 0) {
		long stride = profile->vertexStructSize(streamNumberForBuffer(vb));
		char *data = (char *)locker->data();
		int index = lod - 4;
		SZFillerContext context;
		context.zOffset = zOffset;
		context.colorOffset = colorOffset;
		context.degradedColorOffset = degradedColorOffset;
		{
			StSharableLocker lock(&_heightsLock);
			if (!_heights || _heights->objectAtIndex(index) == LNull::null)
				loadDEMToMemory(this);
			context.heights = _heights ? (const float *)((LData *)_heights->objectAtIndex(index))->bytes() : 0;
			unsigned char centers = lod & 1;
			int size = 1 << (lod >> 1);
			TAdaptiveVBCustomFiller<ZFillerExecutor, SZFillerContext &> filler;
			filler.fillAdaptiveVertexBufferCustom(size, centers, data, stride, context);
			CBordersFiller::copyGeometryToBorders(size, centers, data, stride, zOffset, sBorderZValue);
			if (_heights)
				_heights->replaceObjectAtIndex(LNull::null, index);
		}
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
// The DEM.

LString *GroundElement::heightMapFilePath(const STextureID &id)
{
	if (id.level != 5)
		return 0;
	int x = id.x, y = id.y;
	int column = x / 9;
	bool west = column > 17;
	if (west)
		column = 36 - column;
	int row = y / 9;
	int latitude = row < 9 ? 9 - row : row - 9;
	LString *square = LString::stringWithFormat("%s%d%s%d", west ? "w" : "e", column * 10, row < 9 ? "s" : "n",
	                                            latitude * 10);
	LString *file = LString::stringWithFormat("%d.raw", x % 9 + 1 + (8 - y % 9) * 9);
	LString *path = LString::stringWithCString("DEMs");
	path->appendPathComponent(square);
	path->appendPathComponent(file);
	return path;
}

// A DEM sample's height (0 below sea level) in globe radii.
static inline float demHeight(unsigned short sample)
{
	int h = sample - 0x8000;
	return (float)(h < 0 ? 0 : h) * 4.7699996e-07f;
}

// The packed normal at a sample: the x and y slopes biased by 128, then the
// complement of their mean magnitude, then 255.
static inline float demNormal(const unsigned short *sample)
{
	unsigned int right = sample[1] < 0x8000 ? 0x8000 : sample[1];
	unsigned int left = sample[-1] < 0x8000 ? 0x8000 : sample[-1];
	unsigned int up = sample[-0x83] < 0x8000 ? 0x8000 : sample[-0x83];
	unsigned int down = sample[0x83] < 0x8000 ? 0x8000 : sample[0x83];
	int dx = (int)(left - right) >> 3;
	int dy = (int)(down - up) >> 3;
	dx = dx < -128 ? -128 : (dx > 127 ? 127 : dx);
	dy = dy < -128 ? -128 : (dy > 127 ? 127 : dy);
	unsigned int packed = ((dx + 0x80) & 0xff) | ((dy + 0x80) & 0xff) << 8 |
	                      (unsigned int)(unsigned char)~(unsigned char)((abs(dx) + abs(dy)) >> 1) << 16 |
	                      0xffu << 24;
	float f;
	memcpy(&f, &packed, sizeof(f));
	return f;
}

void GroundElement::loadDEMToMemory(GroundElement *element)
{
	LString *path = heightMapFilePath(element->_id);
	LData *file = path ? LData::dataWithContentsOfFile(path) : 0;
	if (!file) {
		element->_demMissing = true;
		return;
	}
	const unsigned short *grid = (const unsigned short *)((const char *)file->bytes() + 0x108);
	float *fullZ = element->_computeZBounds ? new float[129 * 129] : 0;
	LArray *lods = LArray::_alloc()->init();
	int y = element->_id.y;
	// The rows next to the poles' cut-offs fade to the sphere.
	bool polar = (unsigned short)(y - 10) > 0x8d;
	bool north = polar && y > 0x97;
	bool fadeRow = polar && (unsigned short)(y - 9) < 0x90;
	for (int lod = 4; lod < 15; lod++) {
		int half = lod >> 1;
		int step = 1 << (7 - half);
		int size = 1 << half;
		int count = (size + 1) * (size + 1);
		if (lod & 1)
			count += size * size;
		LData *data = LData::_alloc();
		data->setLength(count * 8);
		lods->addObject(data);
		data->release();
		float *out = (float *)data->bytes();
		float *fullRow = lod == 14 ? fullZ : 0;
		for (int r = 0; r <= size; r++) {
			float fade = 1.0f;
			if (fadeRow) {
				fade = (float)r / (float)size;
				if (north)
					fade = 1.0f - fade;
			}
			int rowStart = (size - r) * step * 0x83;
			float *full = fullRow;
			for (int c = 0; c <= size; c++) {
				const unsigned short *sample = grid + rowStart + c * step;
				float z = demHeight(*sample);
				out[0] = z;
				if (full)
					*full++ = z;
				out[1] = demNormal(sample);
				if (polar)
					out[0] = fade * out[0];
				out += 2;
			}
			if ((lod & 1) && r < size) {
				if (fadeRow) {
					fade = ((float)r + 0.5f) / (float)size;
					if (north)
						fade = 1.0f - fade;
				}
				int center = rowStart + ((step >> 1) - ((step * 0x83) >> 1));
				for (int c = 0; c < size; c++, center += step) {
					const unsigned short *sample = grid + center;
					out[0] = demHeight(*sample);
					out[1] = demNormal(sample);
					if (polar)
						out[0] = fade * out[0];
					out += 2;
				}
			}
			if (fullRow)
				fullRow += 129;
		}
	}
	{
		StSharableLocker lock(&element->_heightsLock);
		LRelease(element->_heights);
		element->_heights = lods;
	}
	if (element->_computeZBounds) {
		StSharableLocker lock(element->boundsMutex());
		updateBoundsWhole(14, (char *)fullZ, 4, -1, 0, 0, 0, element->_minZ, element->_maxZ);
	}
	delete[] fullZ;
}

intptr_t GroundElement::_demLoadComplete(char *)
{
	_demLoading = false;
	_demAvailable = !_demDisabled && _heights != 0;
	GroundElementsManager::instance()->markElementAsDEMLoaded(this);
	LNotificationCenter::center()->postNotification(kDEMLoadCompleteNotification, this, 0);
	sNumNotifications++;
	return 0;
}

intptr_t GroundElement::_mainThreadReleaser(char *)
{
	return 0;
}

// ---------------------------------------------------------------------------
// Drawing.

GlobeAdaptiveMeshData *GroundElement::_drawableData()
{
	if (!_data) {
		GroundElementsManager *manager = GroundElementsManager::instance();
		CRenderProfile *profile = manager->sharedRenderProfile();
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		GroundElementModel *model = GroundElementModel::_alloc()->initWithGroundElement(this, profile);
		if (!model) {
			if (holder)
				holder->endAccumulationBlock();
			return 0;
		}
		model->setLODBias(manager->_lodBias);
		CDrawableModelData *data = LRetain(model->detachModelDataInstance());
		_data = lobj_cast<GlobeAdaptiveMeshData>(data);
		model->setTransformCallback(GlobeAdaptiveMeshData::transformBoundingBox);
		model->release();
		_data->setTransforms(_id, 0.0f);
		CBaseTexture *water = GroundElementsManager::instance()->darkWaterColorTexture();
		_data->setTextureForSource(water, 3);
		_data->_storesState = false;
		if (holder)
			holder->endAccumulationBlock();
	}
	return _data;
}

GlobeAdaptiveMeshData *GroundElement::tryToRender(int textureLevel, float farClip)
{
	GlobeAdaptiveMeshData *data = _drawableData();
	if (data) {
		data->_fogClip = AdaptiveLODSphere::calcFogClip(farClip);
		data->setVerticalScaleFactor(Preferences::verticalScaleFactor());
		CTextureLoader *loader = (CTextureLoader *)_holder->objectForKey(kTextureLoaderKey);
		data->_drawInfo.adjustWithLoader(textureLevel, loader);
	}
	return data;
}
