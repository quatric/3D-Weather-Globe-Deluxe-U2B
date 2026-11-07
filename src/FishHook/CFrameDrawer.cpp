// CFrameDrawer and its draw queues.

#include "FHScene.h"

LF_IMPLEMENT_CLASS(CFrameDrawer, 0)

// ---------------------------------------------------------------------------
// TRedBlackTree

template <typename C>
void TRedBlackTree<C>::_simpleInsert(CBaseRBTreeEntry *entry)
{
	entry->_left = entry->_right = entry->_parent = 0;
	entry->_color = 0;
	CBaseRBTreeEntry *parent = 0;
	CBaseRBTreeEntry *node = _root;
	CDrawableModelData *data = C::dataForEntry(entry);
	while (node) {
		parent = node;
		node = _queue->_compare(data, C::dataForEntry(node), _queue) < 0 ? node->_left : node->_right;
	}
	entry->_parent = parent;
	if (parent == 0)
		_root = entry;
	else if (_queue->_compare(data, C::dataForEntry(parent), _queue) < 0)
		parent->_left = entry;
	else
		parent->_right = entry;
}

template <typename C>
void TRedBlackTree<C>::_rotateLeft(CBaseRBTreeEntry *x)
{
	CBaseRBTreeEntry *y = x->_right;
	x->_right = y->_left;
	if (y->_left)
		y->_left->_parent = x;
	y->_parent = x->_parent;
	if (x->_parent == 0)
		_root = y;
	else if (x == x->_parent->_left)
		x->_parent->_left = y;
	else
		x->_parent->_right = y;
	y->_left = x;
	x->_parent = y;
}

template <typename C>
void TRedBlackTree<C>::_rotateRight(CBaseRBTreeEntry *x)
{
	CBaseRBTreeEntry *y = x->_left;
	x->_left = y->_right;
	if (y->_right)
		y->_right->_parent = x;
	y->_parent = x->_parent;
	if (x->_parent == 0)
		_root = y;
	else if (x == x->_parent->_right)
		x->_parent->_right = y;
	else
		x->_parent->_left = y;
	y->_right = x;
	x->_parent = y;
}

// Color 0 is red, 1 black.
template <typename C>
void TRedBlackTree<C>::insertEntry(CBaseRBTreeEntry *x)
{
	_simpleInsert(x);
	while (x != _root && x->_parent->_color == 0) {
		CBaseRBTreeEntry *parent = x->_parent;
		CBaseRBTreeEntry *grand = parent->_parent;
		if (parent == grand->_left) {
			CBaseRBTreeEntry *uncle = grand->_right;
			if (uncle && uncle->_color == 0) {
				parent->_color = 1;
				uncle->_color = 1;
				grand->_color = 0;
				x = grand;
			} else {
				if (x == parent->_right) {
					x = parent;
					_rotateLeft(x);
				}
				x->_parent->_color = 1;
				x->_parent->_parent->_color = 0;
				_rotateRight(x->_parent->_parent);
			}
		} else {
			CBaseRBTreeEntry *uncle = grand->_left;
			if (uncle && uncle->_color == 0) {
				parent->_color = 1;
				uncle->_color = 1;
				grand->_color = 0;
				x = grand;
			} else {
				if (x == parent->_left) {
					x = parent;
					_rotateRight(x);
				}
				x->_parent->_color = 1;
				x->_parent->_parent->_color = 0;
				_rotateLeft(x->_parent->_parent);
			}
		}
	}
	_root->_color = 1;
}

template <typename C>
bool TRBTreeIterator<C>::next()
{
	if (_atEnd)
		return false;
	CBaseRBTreeEntry *e = _current;
	if (e == 0) {
		e = _tree->_root;
		if (e == 0)
			return false;
		while (e->_left)
			e = e->_left;
		_current = e;
		return true;
	}
	if (e->_right) {
		e = e->_right;
		while (e->_left)
			e = e->_left;
		_current = e;
		return true;
	}
	while (e->_parent) {
		if (e == e->_parent->_left) {
			_current = e->_parent;
			return true;
		}
		e = e->_parent;
	}
	_current = 0;
	_atEnd = true;
	return false;
}

template class TRedBlackTree<SDrawEntryCompare>;
template class TRBTreeIterator<SDrawEntryCompare>;

// ---------------------------------------------------------------------------
// CDrawQueue

CDrawQueue::~CDrawQueue()
{
	clearQueue();
	delete _tree;
	_tree = 0;
}

void CDrawQueue::createQueueArray()
{
	_tree = new TRedBlackTree<SDrawEntryCompare>(this);
	_compare = _compareCallback();
}

void CDrawQueue::executeQueue(CFrameDrawer *drawer)
{
	if (_tree == 0 || _tree->_root == 0)
		return;
	TRBTreeIterator<SDrawEntryCompare> it(_tree);
	while (it.next())
		it.data()->immediateDraw(drawer);
}

void CDrawQueue::clearQueue()
{
	if (_tree == 0)
		return;
	TRBTreeIterator<SDrawEntryCompare> it(_tree);
	while (it.next())
		it.data()->releaseWithModel();
	_tree->_root = 0;
}

void CDrawQueue::postDrawDataToQueue(CDrawableModelData *data)
{
	if (_tree == 0)
		createQueueArray();
	data->retainWithModel();
	_tree->insertEntry(data);
}

static int _compareDistances(float a, float b)
{
	float d = a - b;
	return d == 0 ? 0 : d < 0 ? 1 : -1;
}

int COpaqueObjectsDrawQueue::opaqueObjectCompare(const CDrawableModelData *a, const CDrawableModelData *b,
                                                 CDrawQueue *)
{
	if (a->_profile && b->_profile) {
		int r = a->_profile->_name->compareWith(b->_profile->_name);
		if (r != 0)
			return r;
	}
	return _compareDistances(a->_distance, b->_distance);
}

int CTranspObjectsDrawQueue::transpObjectCompare(const CDrawableModelData *a, const CDrawableModelData *b,
                                                 CDrawQueue *)
{
	return -_compareDistances(a->_distance, b->_distance);
}

int CGlowsDrawQueue::glowsCompare(const CDrawableModelData *a, const CDrawableModelData *b, CDrawQueue *)
{
	if (a->_profile && b->_profile)
		return a->_profile->_name->compareWith(b->_profile->_name);
	return 0;
}

// ---------------------------------------------------------------------------
// CFrameDrawer

CFrameDrawer::CFrameDrawer()
    : _usedModels(0), _drawnDataSet(0), _usedProfiles(0), _capacity(0x10), _context(0), _camera(0), _frameNumber(0),
      _properties(0)
{
	for (int i = 0; i < 5; i++) {
		_orders[i] = 0;
		_batches[i] = new TModelBatchesMap;
	}
	_drawnDatas = new TArray<CDrawableModelData *>;
}

void CFrameDrawer::dealloc()
{
	LRelease(_properties);
	_properties = 0;
	LRelease(_usedModels);
	_usedModels = 0;
	delete _drawnDatas;
	_drawnDatas = 0;
	delete _drawnDataSet;
	_drawnDataSet = 0;
	LRelease(_usedProfiles);
	_usedProfiles = 0;
	for (int i = 0; i < 5; i++) {
		LRelease(_orders[i]);
		_orders[i] = 0;
	}
	for (int i = 0; i < 5; i++) {
		delete _batches[i];
		_batches[i] = 0;
	}
	LRelease(_camera);
	_camera = 0;
	LObject::dealloc();
}

void CFrameDrawer::beginFrame(CScene *scene, CScenePropertiesSet *properties)
{
	LAssign(_properties, properties);
	_usedModels = LSet::_alloc()->init();
	_drawnDataSet = new CHashSet;
	_usedProfiles = LSet::_alloc()->init();
	_orders[0] = LArray::_alloc()->init();
	_orders[0]->preserveCapacity(_capacity);
	_context = scene->_holder->_context;
	_camera = LRetain(scene->_camera);
	CRenderEnvironment *env = _context->_renderEnvironment;
	StPostponeProfilesInvalidation postpone(env);
	env->removeAllLights();
	env->removeAllClipPlanes();
	LArray *lights = scene->_lights;
	int count = lights->count();
	for (int i = 0; i < count; i++)
		env->addLight((CContextLight *)lights->objectAtIndex(i), 0);
	LArray *planes = scene->_clipPlanes;
	count = planes->count();
	for (int i = 0; i < count; i++)
		env->addClipPlane((CClipPlane *)planes->objectAtIndex(i), 0);
}

// Order 0 goes to the sorted queues; the others are drawn in the order
// given, models that can draw arrays of their data once for all of them.
void CFrameDrawer::renderDrawableData(CDrawableModelData *data, TOrder order)
{
	CDrawableModel *model = data->_model;
	if (model && model->evalVisibilityParams(data, 0, 0)) {
		_usedModels->addObject(model);
		_drawnDatas->addObject(data);
		_drawnDataSet->addObject((intptr_t)data);
	}
	bool batch = model->canBeDrawnWithDataArray();
	if (model->hasRenderProfile()) {
		CRenderProfile *profile = model->renderProfile();
		_usedProfiles->addObject(profile);
		batch = batch && !profile->_isTransparent;
	}
	if (order >= 0 && order <= 4) {
		LArray *array;
		if (order == 0) {
			array = _orders[0];
		} else if (!batch) {
			if (_orders[order] == 0)
				_orders[order] = LArray::_alloc()->init();
			array = _orders[order];
		} else {
			array = _batches[order]->objectForKey((intptr_t)model);
			if (array == 0) {
				array = LArray::_alloc()->init();
				_batches[order]->setObjectForKey((TLArray<CDrawableModelData *> *)array, (intptr_t)model);
				array->release();
			}
		}
		array->addObject(data);
	}
	if (_properties && data->_storesState)
		_properties->storeDrawableData(data, order);
}

static void _drawOrder(CFrameDrawer *drawer, LArray *array, bool onlyDrawn)
{
	int count = array ? array->count() : 0;
	for (int i = 0; i < count; i++) {
		CDrawableModelData *data = (CDrawableModelData *)array->objectAtIndex(i);
		if (onlyDrawn && !drawer->_drawnDataSet->containsObject((intptr_t)data))
			continue;
		CDrawableModel *model = data->_model;
		if (model && model->evalVisibilityParams(data, &data->_center, &data->_distance))
			data->immediateDraw(drawer);
	}
}

static void _drawBatches(CFrameDrawer *drawer, TModelBatchesMap *batches)
{
	TModelBatchesMap::hashTableIterator it(batches);
	while (it.next()) {
		CDrawableModel *model = (CDrawableModel *)it.entry()->key;
		model->doImmediateDrawWithDataArray(it.entry()->value, drawer);
	}
}

// Order 1, the opaque queue, order 2, the transparent queue, order 3, the
// glows, order 4.
void CFrameDrawer::_immediateDrawForAllQueues(bool onlyDrawn)
{
	COpaqueObjectsDrawQueue opaque;
	CTranspObjectsDrawQueue transparent;
	CGlowsDrawQueue glows;
	LArray *queued = _orders[0];
	int count = queued->count();
	for (int i = 0; i < count; i++) {
		CDrawableModelData *data = (CDrawableModelData *)queued->objectAtIndex(i);
		CDrawableModel *model = data->_model;
		if (model == 0 || !model->evalVisibilityParams(data, &data->_center, &data->_distance))
			continue;
		switch (data->_model->drawableType()) {
		case 1:
			opaque.postDrawDataToQueue(data);
			break;
		case 2:
			transparent.postDrawDataToQueue(data);
			break;
		case 3:
			glows.postDrawDataToQueue(data);
			break;
		}
	}
	_drawOrder(this, _orders[1], onlyDrawn);
	_drawBatches(this, _batches[1]);
	opaque.executeQueue(this);
	_drawOrder(this, _orders[2], onlyDrawn);
	_drawBatches(this, _batches[2]);
	transparent.executeQueue(this);
	_drawOrder(this, _orders[3], onlyDrawn);
	_drawBatches(this, _batches[3]);
	glows.executeQueue(this);
	_drawOrder(this, _orders[4], onlyDrawn);
	_drawBatches(this, _batches[4]);
}

void CFrameDrawer::_updateEnvironmentMap(CEnvironmentMap *map)
{
	if (map->beginUpdate()) {
		_context->setDefaultDrawParameters();
		_context->clearCurrentRenderTarget();
		_immediateDrawForAllQueues(false);
		map->endUpdate();
	}
}

void CFrameDrawer::flushRenderingQueue()
{
	_context->_renderEnvironment->beginFrameWithUsedProfiles(_usedProfiles);
	_context->setDefaultDrawParameters();
	_immediateDrawForAllQueues(true);
}

void CFrameDrawer::endFrame()
{
	_context->_renderEnvironment->endFrame();
	LRelease(_usedModels);
	_usedModels = 0;
	_drawnDatas->removeAllObjects();
	delete _drawnDataSet;
	_drawnDataSet = 0;
	LRelease(_usedProfiles);
	_usedProfiles = 0;
	_capacity = _orders[0]->count() + 1;
	for (int i = 0; i < 5; i++) {
		LRelease(_orders[i]);
		_orders[i] = 0;
	}
	for (int i = 0; i < 5; i++)
		_batches[i]->removeAllObjects();
	LRelease(_camera);
	_camera = 0;
	_frameNumber++;
	_context = 0;
}
