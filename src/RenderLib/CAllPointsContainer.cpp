// CAllPointsContainer: draws the atlas's points over the scene: the other
// cities as a point-list mesh, the capitals and Apple stores as point sprite
// systems, each with one data per copy of the map. Also parseASEFile, the
// pushpin model's loader.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CAllPointsContainer, 0)
LF_SELECTOR(CAllPointsContainer, _onDBChanged);
LF_IMPLEMENT_CLASS(CAllCitiesMesh, LF_CLASSOF(CMesh))
LF_IMPLEMENT_CLASS(CAllCitiesMeshData, LF_CLASSOF(ModelDataWithMatrix))
LF_IMPLEMENT_CLASS(PointsParticleSystem, LF_CLASSOF(CParticleSystem))
LF_IMPLEMENT_CLASS(PointsModelsDrawableData, LF_CLASSOF(CParticleSystemData))

static inline unsigned int _floatBits(float f)
{
	unsigned int u;
	memcpy(&u, &f, 4);
	return u;
}

// ---------------------------------------------------------------------------
// CAllCitiesMesh, CAllCitiesMeshData

LClass *CAllCitiesMesh::modelDataClass()
{
	return LF_CLASSOF(CAllCitiesMeshData);
}

// The points' size follows the scale, a pixel smaller for each subset.
void CAllCitiesMesh::drawSubset(CDrawableModelData *data, long subset, long index)
{
	CAllCitiesMeshData *cities = lobj_cast<CAllCitiesMeshData>(data);
	if (cities == 0)
		return;
	CDrawCache *cache = _context->_drawCache;
	cache->setRenderState(D3DRS_POINTSIZE, _floatBits(floorf(cities->_scale + 2.5f) - (float)(index * 2)));
	CMesh::drawSubset(data, subset, index);
	cache->setRenderState(D3DRS_POINTSIZE, _floatBits(1.0f));
}

void CAllCitiesMeshData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	ModelDataWithMatrix::storeSceneStateInStream(stream, flags);
	storeFloatInStream(_scale, stream);
}

const void *CAllCitiesMeshData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = ModelDataWithMatrix::restoreSceneStateFromData(data, flags);
	return loadFloatFromStream(data, &_scale);
}

// ---------------------------------------------------------------------------
// PointsParticleSystem, PointsModelsDrawableData

LClass *PointsParticleSystem::modelDataClass()
{
	return LF_CLASSOF(PointsModelsDrawableData);
}

const void *PointsModelsDrawableData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'CnSf')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	return &_params;
}

// ---------------------------------------------------------------------------
// CAllPointsContainer

CAllPointsContainer::CAllPointsContainer()
    : _scene(0), _pointSize(1.0f), _citiesMesh(0), _citiesDatas(0), _allPoints(0), _otherIndices(0), _capitals(0),
      _capitalsDatas(0), _appleStores(0), _appleStoresDatas(0)
{
}

static void _releaseAll(CAllPointsContainer *c)
{
	if (c->_citiesMesh) {
		c->_citiesMesh->dataSourceWillDie();
		releaseAndClear(c->_citiesMesh);
	}
	releaseAndClear(c->_citiesDatas);
	releaseAndClear(c->_allPoints);
	releaseAndClear(c->_otherIndices);
	releaseAndClear(c->_capitals);
	releaseAndClear(c->_capitalsDatas);
	releaseAndClear(c->_appleStores);
	releaseAndClear(c->_appleStoresDatas);
}

void CAllPointsContainer::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	_releaseAll(this);
	LObject::dealloc();
}

CAllPointsContainer *CAllPointsContainer::initWithScene(BaseScene *scene)
{
	if (!LObject::init())
		return 0;
	_scene = scene;
	LNotificationCenter::center()->addObserver(this, "_onDBChanged", AtlasDatabase::kMainDatabaseChangedNotification,
	                                           0);
	_stateOwnerName = kObjNameAllPointsContainer;
	return this;
}

void CAllPointsContainer::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeFloatInStream(_pointSize, stream);
}

const void *CAllPointsContainer::restoreSceneStateFromData(const void *data, unsigned int)
{
	return loadFloatFromStream(data, &_pointSize);
}

D3DPRIMITIVETYPE CAllPointsContainer::primitiveTypeForMesh(CMesh *mesh)
{
	return _citiesMesh == mesh ? D3DPT_POINTLIST : D3DPT_TRIANGLELIST;
}

unsigned int CAllPointsContainer::numPrimitivesForMesh(CMesh *mesh)
{
	return _citiesMesh == mesh;
}

unsigned int CAllPointsContainer::numVerticesForMesh(CMesh *mesh)
{
	if (_citiesMesh == mesh && _otherIndices)
		return _otherIndices->length() / 4;
	return 0;
}

// Each particle at its point, sized by kind (capitals 2 and 3, stores 8 and
// 10; the others not drawn).
void CAllPointsContainer::_fillPointsDrawableData(PointsModelsDrawableData *data, LData *indices)
{
	if (data == 0)
		return;
	int count = data->_count;
	CParticleSystemData::SStoredParticle *particle = (CParticleSystemData::SStoredParticle *)data->_particles;
	const uint32_t *index = (const uint32_t *)indices->bytes();
	const SAtlasPoint *points = (const SAtlasPoint *)_allPoints->bytes();
	for (int i = 0; i < count; i++, particle++) {
		const SAtlasPoint &point = points[index[i]];
		_scene->scenePointFromGeoPoint(&particle->position, &point.geoPoint);
		switch (point.kind) {
		case 2:
		case 8:
			particle->size = 2.0f;
			break;
		case 3:
		case 10:
			particle->size = 3.0f;
			break;
		default:
			particle->size = 0.0f;
			break;
		}
	}
	data->_vbsDirty = true;
	data->_instancingDirty = true;
}

float CAllPointsContainer::texturedPointsScaleFromPointSize(float size)
{
	return (powf((size - 1.0f) / 12.0f + 1.0f, 3.0f) + 0.7f) * 7.0f;
}

// The other cities' vertices: their scene points in their kind's color.
void CAllPointsContainer::meshWasInstantiated(CMesh *mesh)
{
	if (_citiesMesh != mesh || _otherIndices == 0)
		return;
	unsigned int count = _otherIndices->length() / 4;
	if (count == 0)
		return;
	const uint32_t *index = (const uint32_t *)_otherIndices->bytes();
	const SAtlasPoint *points = (const SAtlasPoint *)_allPoints->bytes();
	StVertexBufferLocker *locker = mesh->detachVBLocker(0, 0);
	CRenderProfile *profile = mesh->renderProfile();
	if (profile->_shader == 0)
		LF_RAISE(WrongConditionException());
	int position = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int color = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	char *vertex = (char *)locker->data();
	unsigned int stride = profile->vertexStructSize(0);
	for (unsigned int i = 0; i < count; i++, vertex += stride) {
		const SAtlasPoint &point = points[index[i]];
		_scene->scenePointFromGeoPoint((Vector3 *)(vertex + position), &point.geoPoint);
		if (color >= 0)
			*(unsigned int *)(vertex + color) = sPointColors[(signed char)point.kind];
	}
	delete locker;
}

// The database changed: everything is made again.
intptr_t CAllPointsContainer::_onDBChanged(char *)
{
	StSharableLocker lock(&_lock, true);
	_releaseAll(this);
	return 0;
}

PointsParticleSystem *CAllPointsContainer::_createPointsParticleSystem(unsigned int count, LString *texturePath)
{
	CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(
	    LString::stringWithCString("RENDERPROFILES\\TexturedPoints.yaf"), _scene->holder());
	C2DTexture *texture = LAutorelease(C2DTexture::_alloc()->initWithHolderAndPath(_scene->holder(), texturePath));
	PointsParticleSystem *system =
	    (PointsParticleSystem *)PointsParticleSystem::_alloc()->initWithParams(0, count, kPartSysQuads, profile, 0, 0);
	system->setTextureForSource(texture, 0);
	system->setTextureForSource(texture, 1);
	return system;
}

static PointsModelsDrawableData *_pointsData(LArray *datas, int index, PointsParticleSystem *system,
                                             CAllPointsContainer *container, LData *indices)
{
	if (index < (int)datas->count())
		return (PointsModelsDrawableData *)datas->objectAtIndex(index);
	PointsModelsDrawableData *data = lobj_cast<PointsModelsDrawableData>(system->detachModelDataInstance());
	if (data) {
		container->_fillPointsDrawableData(data, indices);
		datas->addObject(data);
		data->_storesState = false;
	}
	return data;
}

void CAllPointsContainer::drawWithDrawer(CFrameDrawer *drawer, float pointSize)
{
	StSharableLocker lock(&_lock, true);
	_pointSize = pointSize;
	AtlasDatabase *db = AtlasDatabase::instance();
	if (_allPoints == 0)
		_allPoints = LRetain(db->allPointsArray());
	bool others = Preferences::showOtherPoints();
	bool capitals = Preferences::showCapitalPoints();
	bool stores = Preferences::showAppleStorePoints();
	if (_citiesMesh == 0 || _capitals == 0 || _appleStores == 0) {
		CHolder *holder = _scene->holder();
		if (holder)
			holder->beginAccumulationBlock();
		if (others) {
			if (_citiesMesh == 0) {
				CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(
				    LString::stringWithCString("RENDERPROFILES\\AllCitiesGlobe.yaf"), _scene->holder());
				_citiesMesh = (CAllCitiesMesh *)CAllCitiesMesh::_alloc()->initWithDataSource(this, profile, 0);
				_citiesMesh->_typeName = "BaseScene::_allCitiesPointsMesh";
			}
			if (_citiesDatas == 0)
				_citiesDatas = LArray::_alloc()->init();
			if (_otherIndices == 0)
				_otherIndices = LRetain(db->allOtherPointsIndices());
		}
		if (capitals) {
			if (_capitals == 0)
				_capitals = _createPointsParticleSystem(db->allCapitalsIndices()->length() / 4,
				                                        LString::stringWithCString("TEXTURES\\cities.dds"));
			if (_capitalsDatas == 0)
				_capitalsDatas = LArray::_alloc()->init();
		}
		if (stores) {
			if (_appleStores == 0)
				_appleStores = _createPointsParticleSystem(db->allAppleStoresIndices()->length() / 4,
				                                           LString::stringWithCString("TEXTURES\\app_store.dds"));
			if (_appleStoresDatas == 0)
				_appleStoresDatas = LArray::_alloc()->init();
		}
		if (holder)
			holder->endAccumulationBlock();
	}
	PanelDrawIterator *it = _scene->controller()->detachDrawIterator(0);
	for (int copy = 0; it->next(); copy++) {
		if (others) {
			CAllCitiesMeshData *data;
			if (copy < (int)_citiesDatas->count()) {
				data = (CAllCitiesMeshData *)_citiesDatas->objectAtIndex(copy);
			} else {
				data = lobj_cast<CAllCitiesMeshData>(_citiesMesh->detachModelDataInstance());
				if (data) {
					_citiesDatas->addObject(data);
					data->_storesState = false;
				}
			}
			if (data) {
				_scene->_adjustAllCitiesData(data, it);
				data->_scale = pointSize;
				drawer->renderDrawableData(data, 3);
			}
		}
		PointsModelsDrawableData *data;
		if (stores && (data = _pointsData(_appleStoresDatas, copy, _appleStores, this, db->allAppleStoresIndices()))) {
			_scene->_adjustAllPointsData(data, it);
			data->_params = Vector4(texturedPointsScaleFromPointSize(pointSize), 0, 0, 0);
			drawer->renderDrawableData(data, 3);
		}
		if (capitals && (data = _pointsData(_capitalsDatas, copy, _capitals, this, db->allCapitalsIndices()))) {
			_scene->_adjustAllPointsData(data, it);
			data->_params = Vector4(texturedPointsScaleFromPointSize(pointSize), 0, 0, 0);
			drawer->renderDrawableData(data, 3);
		}
	}
	delete it;
}

void CAllPointsContainer::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	drawWithDrawer(scene->frameDrawer(), _pointSize);
}

// The .ase plists were written big-endian.
static void swapWords(LData *data)
{
	uint32_t *p = (uint32_t *)data->bytes();
	unsigned int count = data->length() / 4;
	for (unsigned int i = 0; i < count; i++)
		p[i] = __builtin_bswap32(p[i]);
}

bool parseASEFile(LString *path, unsigned char, unsigned char joinFaces, LArray **vertices, LArray **faces)
{
	if (path == 0 || vertices == 0 || faces == 0)
		return false;
	*vertices = 0;
	*faces = 0;
	LMap *file = LMap::_alloc()->initWithContentsOfFile(path);
	if (file == 0)
		return false;
	LAutorelease(file);
	LArray *objects = (LArray *)file->objectForKey(kObjectsKey);
	LMap *mesh = (LMap *)((LMap *)objects->objectAtIndex(0))->objectForKey(kObjectMeshKey);
	if (mesh == 0)
		return false;
	LArray *vertexArray = LArray::_alloc()->initWithArray((LArray *)mesh->objectForKey(kMeshVerticesKey));
	for (unsigned int i = 0; i < vertexArray->count(); i++) {
		LData *vertex = lobj_cast<LData>(vertexArray->objectAtIndex(i));
		if (vertex == 0) {
			vertexArray->release();
			return false;
		}
		swapWords(vertex);
	}
	LMap *indices = lobj_cast<LMap>(mesh->objectForKey(kMeshIndicesKey));
	LArray *faceArray = (LArray *)LArray::_alloc()->init();
	for (LMapIterator it(indices, false); it.next();)
		faceArray->addObject(it.object());
	unsigned int count = faceArray->count();
	if (count > 1 && joinFaces) {
		LData *joined = LData::dataWithLength(0);
		for (unsigned int i = 0; i < count; i++) {
			LData *part = lobj_cast<LData>(faceArray->objectAtIndex(i));
			if (part == 0) {
				vertexArray->release();
				faceArray->release();
				return false;
			}
			joined->appendBytes(part->bytes(), part->length());
		}
		// The original kept the autoreleased array (and leaked the parts).
		faceArray->release();
		faceArray = LRetain(LArray::arrayWithObjects(joined, (LObject *)0));
	}
	LData *first = lobj_cast<LData>(faceArray->objectAtIndex(0));
	if (first == 0) {
		vertexArray->release();
		faceArray->release();
		return false;
	}
	swapWords(first);
	*vertices = vertexArray;
	*faces = faceArray;
	return true;
}
