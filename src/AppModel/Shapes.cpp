// Shapes: shape containers, their drawables and model datas, and the shape
// manager that loads them from the atlas.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(ShapeModelData, LF_CLASSOF(ModelDataWithMatrix))
LF_IMPLEMENT_CLASS(CShapeDrawable, 0)
LF_IMPLEMENT_CLASS(CShapeContainer, 0)
LF_IMPLEMENT_CLASS(CShapeManager, 0)

// The border profiles' color constant, answered by ShapeModelData.
static ShaderConstantDef _shaderConstDef_ShapeHighlightColor(ShapeModelData::kHighlightColorConstID,
                                                              "ShapeHighlightColor", 1, kShaderConstDynamic, 0);

// ---------------------------------------------------------------------------
// ShapeModelData

ShapeModelData::ShapeModelData() : _color(1, 1, 1, 1), _drawable(0), _buffer(0)
{
}

const void *ShapeModelData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != kHighlightColorConstID)
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	memcpy(buffer, &_color, sizeof(_color));
	return buffer;
}

void ShapeModelData::dealloc()
{
	if (_buffer) {
		delete[] (char *)_buffer;
		_buffer = 0;
	}
	CDrawableModelData::dealloc();
}

// ---------------------------------------------------------------------------
// CShapeDrawable

CShapeDrawable::CShapeDrawable()
    : _container(0), _projection(0), _mode(0), _models(0), _datasByUsage(0), _profile(0)
{
}

LClass *CShapeDrawable::_classForModel(unsigned int)
{
	return LF_CLASSOF(CBuffersOwner);
}

CShapeDrawable *CShapeDrawable::initWithParams(int mode, CShapeContainer *container, CProjection *projection)
{
	_container = container;
	_projection = projection;
	_mode = mode;
	LRetain(_container);
	LRetain(_projection);
	return this;
}

void CShapeDrawable::dealloc()
{
	if (_container) {
		_container->release();
		_container = 0;
	}
	if (_projection) {
		_projection->release();
		_projection = 0;
	}
	if (_datasByUsage) {
		_datasByUsage->release();
		_datasByUsage = 0;
	}
	int count = _models ? _models->count() : 0;
	for (int i = 0; i < count; i++)
		((CBuffersOwner *)_models->objectAtIndex(i))->dataSourceWillDie();
	if (_models) {
		_models->release();
		_models = 0;
	}
	if (_profile) {
		_profile->release();
		_profile = 0;
	}
	LObject::dealloc();
}

// The render profile, shared through the holder under the class name.
CRenderProfile *CShapeDrawable::renderProfile(CHolder *holder)
{
	if (_profile == 0) {
		LString *key = LString::stringWithCString(getClass()->name());
		_profile = (CRenderProfile *)holder->objectForKey(key);
		if (_profile == 0) {
			_profile = CRenderProfile::renderProfileByFileDesc(_renderProfileFile(), holder);
			if (_profile) {
				_profile->retain();
				holder->setObjectForKey(_profile, key);
			}
		} else {
			_profile->retain();
		}
	}
	return _profile;
}

LArray *CShapeDrawable::drawableDatas(int index, CHolder *holder, const LString *usageKey, unsigned char *created)
{
	if (_datasByUsage == 0)
		_datasByUsage = LMap::_alloc()->init();
	LArray *byIndex = (LArray *)_datasByUsage->objectForKey(usageKey);
	if (byIndex == 0) {
		byIndex = LArray::_alloc()->init();
		_datasByUsage->setObjectForKey(byIndex, usageKey);
		byIndex->release();
	}
	LArray *datas = index < (int)byIndex->count() ? (LArray *)byIndex->objectAtIndex(index) : 0;
	if (datas) {
		if (created)
			*created = 0;
		return datas;
	}
	Color color(1, 1, 1, 1);
	CShapeManager::highlightColorForShapeID(_container->_shapeID, &color);
	if (created)
		*created = 1;
	datas = LArray::_alloc()->init();
	byIndex->addObject(datas);
	datas->release();
	if (holder)
		holder->beginAccumulationBlock();
	CRenderProfile *profile = renderProfile(holder);
	int count = _modelsCount();
	for (int i = 0; i < count; i++) {
		if (_models == 0)
			_models = LArray::_alloc()->init();
		CBuffersOwner *model;
		if (i < (int)_models->count()) {
			model = (CBuffersOwner *)_models->objectAtIndex(i);
		} else {
			LObject *object = _classForModel(i)->alloc();
			if (!object->getClass()->isSubclassOf(LF_CLASSOF(CBuffersOwner))) {
				object->release();
				if (holder)
					holder->endAccumulationBlock();
				return 0;
			}
			model = ((CBuffersOwner *)object)->initWithDataSource(this, profile, 0);
			if (model == 0)
				continue;
			_modelWasCreated(model, i);
			_models->addObject(model);
			model->release();
		}
		CDrawableModelData *data = model->detachModelDataInstance();
		if (data && data->getClass()->isSubclassOf(LF_CLASSOF(ShapeModelData))) {
			ShapeModelData *shapeData = (ShapeModelData *)data;
			shapeData->_color = color;
			shapeData->_drawable = this;
			datas->addObject(shapeData);
		}
	}
	if (holder)
		holder->endAccumulationBlock();
	return datas;
}

// ---------------------------------------------------------------------------
// CShapeContainer

CShapeContainer::CShapeContainer() : _indexes(0), _objectUID(0), _shapeID(0), _drawable(0), _drawableMode(-1), _loaded(false)
{
}

void CShapeContainer::dealloc()
{
	LObject **fields[] = { (LObject **)&_indexes, (LObject **)&_objectUID, (LObject **)&_shapeID,
		               (LObject **)&_drawable };
	for (LObject **field : fields)
		if (*field) {
			(*field)->release();
			*field = 0;
		}
	LObject::dealloc();
}

QuadTreeContainer *CShapeContainer::quadTree(unsigned char east)
{
	LookupGridBLOB *grid = CShapeManager::instance()->lookupGridByShapeID(_shapeID);
	QuadTreeContainer *tree = grid ? grid->treeForObjectWithUID(_objectUID, east) : 0;
	if (tree)
		tree->downsampleIfNeeded();
	return tree;
}

void CShapeContainer::_loadShapesIfNeeded()
{
	if (_loaded)
		return;
	LF_CHECK_PTR(_indexes);
	_loaded = _loadShapes();
}

bool CShapeContainer::_loadShapes()
{
	LLog("_loadShapes() not overriden for %s\n", getClass()->name());
	return false;
}

void CShapeContainer::invalidateDrawables()
{
	if (_drawable) {
		_drawable->release();
		_drawable = 0;
	}
	_drawableMode = -1;
}

// The drawable of drawableClass() for the mode, made again when the mode
// changes.
CShapeDrawable *CShapeContainer::drawableForProjection(CProjection *projection, int mode, float)
{
	if (_drawableMode != mode) {
		_drawableMode = mode;
		if (_drawable) {
			_drawable->release();
			_drawable = 0;
		}
	}
	if (_drawable)
		return _drawable;
	LClass *cls = drawableClass();
	if (cls == 0)
		return 0;
	LObject *object = cls->alloc();
	if (!object->getClass()->isSubclassOf(LF_CLASSOF(CShapeDrawable))) {
		object->release();
		return 0;
	}
	_drawable = (CShapeDrawable *)object;
	_drawable->initWithParams(mode, this, projection);
	return _drawable;
}

LClass *CShapeContainer::drawableClass()
{
	LLog("drawableClass() is notdefined for %s\n", getClass()->name());
	return 0;
}

bool CShapeContainer::hitTest(LPoint)
{
	LLog("hitTest(LPoint geoPt) not overriden for %s\n", getClass()->name());
	return false;
}

LRect CShapeContainer::allPathsBoundsGeoRect()
{
	LLog("allPathsBoundsGeoRect() not overriden for %s\n", getClass()->name());
	return LZeroRect;
}

CShapeContainer *CShapeContainer::initWithIndexWrapper(IndexBuffersBLOB *indexes, const LString *objectUID,
                                                       const LString *shapeID)
{
	if (!LObject::init())
		return 0;
	_indexes = LRetain(indexes);
	_objectUID = LRetain(const_cast<LString *>(objectUID));
	_shapeID = LRetain(const_cast<LString *>(shapeID));
	return this;
}

// ---------------------------------------------------------------------------
// CShapeManager

TLArray<LString *> *CShapeManager::__registeredShapes = 0;
LMap *CShapeManager::__shapeIDToPublicNameRemap = 0;
LMap *CShapeManager::__shapeIDToColorRemap = 0;
LMap *CShapeManager::__shapeIDToFillColorRemap = 0;
LMap *CShapeManager::__shapeIDToGridNameRemap = 0;
LMap *CShapeManager::__shapeIDToClassRemap = 0;
LMap *CShapeManager::__shapeIDToGeotypeNamesRemap = 0;

class _shapeNgrCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		LObject **maps[] = { (LObject **)&CShapeManager::__registeredShapes,
			             (LObject **)&CShapeManager::__shapeIDToPublicNameRemap,
			             (LObject **)&CShapeManager::__shapeIDToColorRemap,
			             (LObject **)&CShapeManager::__shapeIDToFillColorRemap,
			             (LObject **)&CShapeManager::__shapeIDToGridNameRemap,
			             (LObject **)&CShapeManager::__shapeIDToClassRemap,
			             (LObject **)&CShapeManager::__shapeIDToGeotypeNamesRemap };
		for (LObject **map : maps)
			if (*map) {
				(*map)->release();
				*map = 0;
			}
	}
};

static _shapeNgrCleanup _sShapeNgrCleanup;

CShapeManager::CShapeManager() : _lookupGrids(0), _shapes(0)
{
}

void CShapeManager::initAsSingleton(void *)
{
	LNotificationCenter::center()->addObserver(this, "_onDBUpdateFinished", AtlasDatabase::kMainDatabaseDidEndUpdate,
	                                           0);
}

void CShapeManager::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	if (_lookupGrids) {
		_lookupGrids->release();
		_lookupGrids = 0;
	}
	if (_shapes) {
		_shapes->release();
		_shapes = 0;
	}
	LObject::dealloc();
}

void CShapeManager::invalAllDrawables()
{
	if (_shapes == 0)
		return;
	LMapIterator byShape(_shapes, true);
	while (byShape.next()) {
		LMapIterator byObject((LMap *)byShape.object(), true);
		while (byObject.next())
			((CShapeContainer *)byObject.object())->invalidateDrawables();
	}
}

static LValue *_colorValue(float r, float g, float b, float a)
{
	Color color(r, g, b, a);
	return LValue::valueWithBytes(&color, sizeof(color));
}

static bool _colorForShapeID(LMap *colors, const LString *shapeID, Color *color)
{
	LValue *value = shapeID ? (LValue *)colors->objectForKey(shapeID) : 0;
	if (color && value)
		value->getValue(color);
	return value != 0;
}

bool CShapeManager::highlightColorForShapeID(const LString *shapeID, Color *color)
{
	if (__shapeIDToColorRemap == 0)
		__shapeIDToColorRemap = LRetain(LMap::mapWithObjectsAndKeys(
		    _colorValue(0.5f, 0.8f, 1, 1), kShapeIDCounties, _colorValue(1, 0.8f, 0.8f, 1), kShapeIDStates,
		    _colorValue(1, 1, 1, 1), kShapeIDCountries, _colorValue(1, 0.4f, 1, 1), kShapeIDTimezones, 0));
	return _colorForShapeID(__shapeIDToColorRemap, shapeID, color);
}

bool CShapeManager::highlightFillColorForShapeID(const LString *shapeID, Color *color)
{
	if (__shapeIDToFillColorRemap == 0)
		__shapeIDToFillColorRemap = LRetain(LMap::mapWithObjectsAndKeys(
		    _colorValue(0.5f, 0.8f, 1, 1), kShapeIDCounties, _colorValue(0.4f, 0.08f, 0.08f, 0.4f), kShapeIDStates,
		    _colorValue(0.08f, 0.4f, 0.08f, 0.4f), kShapeIDCountries, _colorValue(0.2f, 0.2f, 1, 0.4f),
		    kShapeIDTimezones, 0));
	return _colorForShapeID(__shapeIDToFillColorRemap, shapeID, color);
}

LString *CShapeManager::_dbGridNameForShapeWithID(const LString *shapeID)
{
	if (__shapeIDToGridNameRemap == 0)
		__shapeIDToGridNameRemap = LRetain(LMap::mapWithObjectsAndKeys(
		    kRepTypeLookupGridCountries, kShapeIDCountries, kRepTypeLookupGridStates, kShapeIDStates,
		    kRepTypeLookupGridCounties, kShapeIDCounties, kRepTypeLookupGridTimezones, kShapeIDTimezones, 0));
	return shapeID ? (LString *)__shapeIDToGridNameRemap->objectForKey(shapeID) : 0;
}

LString *CShapeManager::publicNameFromShapeID(const LString *shapeID)
{
	if (__shapeIDToPublicNameRemap == 0)
		__shapeIDToPublicNameRemap = LRetain(LMap::mapWithObjectsAndKeys(
		    &_sShapeNameCountries, kShapeIDCountries, &_sShapeNameStates, kShapeIDStates, &_sShapeNameCounties,
		    kShapeIDCounties, &_sShapeNameTimezones, kShapeIDTimezones, 0));
	return shapeID ? (LString *)__shapeIDToPublicNameRemap->objectForKey(shapeID) : 0;
}

LClass *CShapeManager::_shapeClassForShapeID(const LString *shapeID)
{
	if (__shapeIDToClassRemap == 0) {
		LString *polys = LString::stringWithCString("CPolysContainer");
		__shapeIDToClassRemap = LRetain(LMap::mapWithObjectsAndKeys(
		    polys, kShapeIDCountries, polys, kShapeIDStates, polys, kShapeIDCounties, polys, kShapeIDTimezones, 0));
	}
	return shapeID ? LClass::classByName((LString *)__shapeIDToClassRemap->objectForKey(shapeID)) : 0;
}

CShapeContainer *CShapeManager::_createShapeContainer(const LString *objectUID, const LString *shapeID)
{
	BLOBWrapper *blob =
	    BLOBWrapper::wrapperForObjectWithUID(const_cast<LString *>(objectUID), kRepTypeShapeIndices);
	if (blob == 0 || !blob->getClass()->isSubclassOf(LF_CLASSOF(IndexBuffersBLOB)))
		return 0;
	LClass *cls = _shapeClassForShapeID(shapeID);
	if (cls == 0)
		return 0;
	LObject *object = cls->alloc();
	if (!object->getClass()->isSubclassOf(LF_CLASSOF(CShapeContainer))) {
		LLog("wrong class returned:%s\n", cls->name());
		return 0;
	}
	return ((CShapeContainer *)object)->initWithIndexWrapper((IndexBuffersBLOB *)blob, objectUID, shapeID);
}

LookupGridBLOB *CShapeManager::lookupGridByShapeID(const LString *shapeID)
{
	StSharableLocker lock(&_lock);
	if (_lookupGrids == 0)
		_lookupGrids = LMap::_alloc()->init();
	LookupGridBLOB *grid = (LookupGridBLOB *)_lookupGrids->objectForKey(shapeID);
	if (grid)
		return grid;
	LArray *rows = AtlasDatabase::instance()->simpleSearchWithRequest(
	    SQL_DB_Strings::selectBLOBUIDForGrid(_dbGridNameForShapeWithID(shapeID)));
	if (rows && rows->count() == 1) {
		LString *uid = lobj_cast<LString>(((LMap *)rows->objectAtIndex(0))->objectForKey(kColumnNameBLOBUid));
		BLOBWrapper *blob = uid ? BLOBWrapper::wrapperWithBLOBUid(uid) : 0;
		if (blob && blob->getClass()->isSubclassOf(LF_CLASSOF(LookupGridBLOB))) {
			_lookupGrids->setObjectForKey(blob, shapeID);
			return (LookupGridBLOB *)blob;
		}
	}
	return 0;
}

CShapeContainer *CShapeManager::shapeForObject(const LString *objectUID, const LString *shapeID)
{
	StSharableLocker lock(&_lock);
	if (_shapes == 0)
		_shapes = LMap::_alloc()->init();
	LMap *containers = (LMap *)_shapes->objectForKey(shapeID);
	if (containers == 0) {
		containers = LMap::_alloc()->init();
		_shapes->setObjectForKey(containers, shapeID);
		containers->release();
	}
	CShapeContainer *container = (CShapeContainer *)containers->objectForKey(objectUID);
	if (container == 0) {
		StAutoreleasePool pool;
		container = _createShapeContainer(objectUID, shapeID);
		if (container) {
			containers->setObjectForKey(container, objectUID);
			container->release();
		}
	}
	return container;
}

LString *CShapeManager::objectUIDByHit(LPoint geoPoint, const LString *shapeID)
{
	LookupGridBLOB *grid = lookupGridByShapeID(shapeID);
	if (grid == 0)
		return 0;
	LArray *uids = grid->objectUIDsBelowGeoPoint(geoPoint);
	int count = uids ? uids->count() : 0;
	float smallest = 64800.0f;
	LString *found = 0;
	for (int i = 0; i < count; i++) {
		LString *uid = (LString *)uids->objectAtIndex(i);
		CShapeContainer *shape = shapeForObject(uid, shapeID);
		if (shape == 0)
			continue;
		LRect bounds = shape->allPathsBoundsGeoRect();
		float area = bounds.size.width * bounds.size.height;
		if (area < smallest && shape->hitTest(geoPoint)) {
			smallest = area;
			found = uid;
		}
	}
	return found;
}

TLArray<LString *> *CShapeManager::registeredShapeIDs()
{
	if (__registeredShapes == 0)
		__registeredShapes = (TLArray<LString *> *)LArray::_alloc()->initWithObjects(
		    kShapeIDCounties, kShapeIDStates, kShapeIDCountries, kShapeIDTimezones, 0);
	return __registeredShapes;
}

TLArray<LString *> *CShapeManager::geotypesForShapeID(const LString *shapeID)
{
	if (__shapeIDToGeotypeNamesRemap == 0)
		__shapeIDToGeotypeNamesRemap = LRetain(LMap::mapWithObjectsAndKeys(
		    LArray::arrayWithObjects(kGeoTypeCountry, (LObject *)0), kShapeIDCountries,
		    LArray::arrayWithObjects(kGeoTypeState, (LObject *)0), kShapeIDStates,
		    LArray::arrayWithObjects(kGeoTypeCounty, (LObject *)0), kShapeIDCounties,
		    LArray::arrayWithObjects(kGeoTypeTimezone, (LObject *)0), kShapeIDTimezones, (LObject *)0));
	return (TLArray<LString *> *)__shapeIDToGeotypeNamesRemap->objectForKey(shapeID);
}

LArray *CShapeManager::allShapesWithShapeID(const LString *shapeID)
{
	LArray *rows = AtlasDatabase::instance()->simpleSearchWithRequest(
	    SQL_DB_Strings::selectAllObjectUIDsWithGeotypes(geotypesForShapeID(shapeID), kRepTypeShapeIndices));
	int count = rows ? rows->count() : 0;
	LArray *shapes = LArray::array();
	for (int i = 0; i < count; i++) {
		LMap *row = lobj_cast<LMap>(rows->objectAtIndex(i));
		LString *uid = row ? lobj_cast<LString>(row->objectForKey(kColumnNameObjectUID)) : 0;
		if (uid)
			if (CShapeContainer *shape = shapeForObject(uid, shapeID))
				shapes->addObject(shape);
	}
	return shapes;
}

intptr_t CShapeManager::_onDBUpdateFinished(char *)
{
	StSharableLocker lock(&_lock);
	if (_lookupGrids) {
		_lookupGrids->release();
		_lookupGrids = 0;
	}
	if (_shapes) {
		_shapes->release();
		_shapes = 0;
	}
	return 0;
}
