// BaseScene: what the globe and the flat map share. It forwards the view's
// events to its panel controller (with y flipped to the controller's
// bottom-up view), draws the shapes, points, grid, routes and highlighted
// entry over the earth, then the overlays: billboards, the status line, the
// minimap, the navigation device and the information panel, the fades and
// the countdown clock.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(BaseScene, LF_CLASSOF(CScene))

// ---------------------------------------------------------------------------
// _BaseSceneVisTester: a quad tree cell is visible when the sphere around its
// center passes the view's frustum. The context says which half of the map
// the tree covers (non-zero: the east one).

class _BaseSceneVisTester : public QuadTreeContainer::IEntryVisibilityTester {
public:
	_BaseSceneVisTester(BaseScene *scene, PanelDrawIterator *iterator) : _scene(scene), _iterator(iterator) {}
	virtual bool isEntryVisible(const QuadTreeContainer::EntryID &entry, const void *context);

	BaseScene *_scene;
	PanelDrawIterator *_iterator;
};

bool _BaseSceneVisTester::isEntryVisible(const QuadTreeContainer::EntryID &entry, const void *context)
{
	float size = 180.0f / (float)(1 << (entry.packed & 0x1f));
	LPoint geo;
	geo.y = ((float)entry.y() + 0.5f) * size - 90.0f;
	geo.x = _iterator->_mapOffset * 360.0f + (((float)entry.x() + 0.5f) * size - (context ? 0.0f : 180.0f));
	Vector3 center;
	_scene->scenePointFromGeoPoint(&center, &geo);
	return _scene->holder()->context()->sphereProbe(&center, size * _scene->_premulAngleFactor(), 0, 0, 0);
}

// ---------------------------------------------------------------------------
// Life

BaseScene::BaseScene()
    : _controller(0), _projection(0), _cinematic(0), _renderOptions(0), _gridMesh(0), _gridMarkers(0),
      _cloudsMesh(0), _cloudsDataSource(0), _allPoints(0), _highlightedEntry(0), _entryCaptionFont(0),
      _highlighted(false), _zoomRectData(0), _zoomRectMesh(0), _countdownStart(0), _unusedDC(0), _statusFont(0),
      _statusBillboard(0), _boldInfoFont(0), _normalInfoFont(0), _straightedgeMesh(0), _straightedgeData(0),
      _sceneState(0), _shapeContainerState(0), _shapesCaptionState(0), _flags(0), _flag130(true),
      _routePointsProfile(0), _timeFrozen(false), _refreshOnTimeChange(true), _point13C(LZeroPoint),
      _cachedScale(-1.0f), _cachedCompassAngle(-1.0f), _shapesCaptionFont(0)
{
	_cachedPosition.x = _cachedPosition.y = -1.0f;
	for (int i = 0; i < 9; i++)
		_unusedF0[i] = 0;
}

BaseScene::StDisableRefreshOnTimeChange::StDisableRefreshOnTimeChange(BaseScene &scene)
    : _scene(scene), _saved(scene._refreshOnTimeChange)
{
	scene._refreshOnTimeChange = false;
}

BaseScene *BaseScene::initWithController(AtlasPanelController *controller, CHolder *holder)
{
	initWithHolder(holder);
	_recordsState = true;
	_controller = controller;
	_projection = _createProjection();
	if (holder)
		holder->beginAccumulationBlock();
	if (_gridMarkers) {
		CRenderProfile *profile =
		    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\Grid.yaf"), holder);
		_gridMesh = (GridLineMesh *)((CMesh *)GridLineMesh::_alloc())
		                ->initWithDataSource(static_cast<CMeshDataSource *>(_gridMarkers), profile, 0);
		_gridMarkers->setGridMesh(_gridMesh);
	}
	if (_flags & kSceneHasStraightedge) {
		CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(
		    LString::stringWithCString("RENDERPROFILES\\Straightedge.yaf"), holder);
		_straightedgeMesh = LAutorelease(((CMesh *)CMesh::_alloc())->initWithDataSource(this, profile, 0));
		_straightedgeData = LRetain(_straightedgeMesh->detachModelDataInstance());
	}
	if (_flags & kSceneHasClouds) {
		CRenderProfile *profile =
		    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\Clouds.yaf"), holder);
		_cloudsMesh = (CCloudsMesh *)((CMesh *)CCloudsMesh::_alloc())->initWithDataSource(_cloudsDataSource, profile, 0);
		_cloudsMesh->_typeName = "BaseScene::mCloudsMesh";
	}
	SStdGeometry plane;
	memset(&plane, 0, sizeof(plane));
	plane.type = kStdGeometryPlaneXY;
	plane.cols = plane.rows = 1;
	plane.radius = 1.0f;
	plane.height = 1.0f;
	CRenderProfile *profile =
	    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\ZoomRect.yaf"), holder);
	_zoomRectMesh = LAutorelease(((CMesh *)CMesh::_alloc())->initWithStdGeometry(&plane, profile));
	_zoomRectMesh->setPerformBoxVisibilityTest(0);
	_zoomRectData = (ModelDataWithMatrix *)LRetain(_zoomRectMesh->detachModelDataInstance());

	_sceneState = SceneState::_alloc()->initWithScene(this);
	_sceneState->_stateOwnerName = kObjNameSceneState;
	_shapeContainerState = ShapeContainerStateHolder::_alloc()->initWithScene(this);
	_shapeContainerState->_stateOwnerName = kObjNameShapeContainerState;
	_shapesCaptionState = ShapesCaptionStateHolder::_alloc()->initWithScene(this);
	_shapesCaptionState->_stateOwnerName = kObjNameCaptionState;
	_allPoints = CAllPointsContainer::_alloc()->initWithScene(this);
	if (_gridMarkers)
		registerStateOwner(_gridMarkers);
	if (_allPoints)
		registerStateOwner(_allPoints);
	if (_sceneState)
		registerStateOwner(_sceneState);
	if (_shapeContainerState)
		registerStateOwner(_shapeContainerState);
	if (_shapesCaptionState)
		registerStateOwner(_shapesCaptionState);
	SharedContextObjectsContainer *objects = objectsContainer();
	MarkerModelData *marker = objects->cityMarker();
	registerStateOwner(marker ? static_cast<CSceneStatePropertyOwner *>(marker) : 0);
	CloudsController *clouds = objects->cloudsController();
	registerStateOwner(clouds ? static_cast<CSceneStatePropertyOwner *>(clouds) : 0);
	CRoutePointsContainer *routes = objects->routePointsContainer();
	registerStateOwner(routes ? static_cast<CSceneStatePropertyOwner *>(routes) : 0);
	setRenderOptions(objects->defaultRenderOptions());
	RenderOptions *options = objects->defaultRenderOptions();
	registerStateOwner(options ? static_cast<CSceneStatePropertyOwner *>(options) : 0);
	if (holder)
		holder->endAccumulationBlock();
	return this;
}

void BaseScene::invalidate()
{
	_controller = 0;
	if (_cinematic) {
		_cinematic->willDetachFromScene();
		releaseAndClear(_cinematic);
	}
	release();
}

void BaseScene::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	releaseAndClear(_sceneState);
	releaseAndClear(_shapeContainerState);
	releaseAndClear(_shapesCaptionState);
	releaseAndClear(_renderOptions);
	if (_cinematic) {
		_cinematic->willDetachFromScene();
		releaseAndClear(_cinematic);
	}
	releaseAndClear(_highlightedEntry);
	releaseAndClear(_shapesCaptionFont);
	releaseAndClear(_allPoints);
	if (_gridMesh) {
		_gridMesh->dataSourceWillDie();
		releaseAndClear(_gridMesh);
	}
	releaseAndClear(_gridMarkers);
	releaseAndClear(_straightedgeData);
	if (_cloudsMesh) {
		_cloudsMesh->dataSourceWillDie();
		releaseAndClear(_cloudsMesh);
	}
	if (_cloudsDataSource) {
		LObject *source = dynamic_cast<LObject *>(_cloudsDataSource);
		if (source)
			source->release();
	}
	_cloudsDataSource = 0;
	releaseAndClear(_zoomRectData);
	releaseAndClear(_projection);
	releaseAndClear(_unusedDC);
	releaseAndClear(_statusFont);
	releaseAndClear(_statusBillboard);
	releaseAndClear(_boldInfoFont);
	releaseAndClear(_normalInfoFont);
	for (int i = 0; i < 9; i++)
		releaseAndClear(_unusedF0[i]);
	releaseAndClear(_routePointsProfile);
	releaseAndClear(_entryCaptionFont);
	CScene::dealloc();
}

void BaseScene::hostControllerWasInstalled()
{
	_recacheCameraDependedObjects();
}

CSceneStatePropertyOwner *BaseScene::stateOwnerWithName(const char *name)
{
	return CScene::stateOwnerWithName(name);
}

SharedContextObjectsContainer *BaseScene::objectsContainer()
{
	return (SharedContextObjectsContainer *)_holder->objectForKey(kContextObjectsContainerKey);
}

void BaseScene::setRenderOptions(RenderOptions *options)
{
	LRetain(options);
	releaseAndClear(_renderOptions);
	_renderOptions = options;
	_holder->setObjectForKey(options, RenderOptions::kRenderOptionsContextKey);
	RenderOptions::setCurrentOptions(options);
}

void BaseScene::installCinematicSequence(CinematicSequence *sequence)
{
	if (_cinematic == sequence)
		return;
	if (_cinematic) {
		_cinematic->willDetachFromScene();
		releaseAndClear(_cinematic);
	}
	_cinematic = LRetain(sequence);
	if (_cinematic)
		_cinematic->wasAttachedToScene(this);
}

// ---------------------------------------------------------------------------
// Events: to the controller, in its bottom-up view coordinates.

static inline LPoint _flipped(CHolder *holder, LPoint point)
{
	LSize frame = holder->systemFrameSize();
	point.y = frame.height - point.y;
	return point;
}

bool BaseScene::processMessage(unsigned int message, unsigned int wParam, unsigned int lParam)
{
	return _controller ? _controller->processMessage(message, wParam, lParam) : false;
}

void BaseScene::mouseDown(unsigned int flags, LPoint point)
{
	CScene::mouseDown(flags, point);
	if (_pressedControl == 0 && _controller)
		_controller->mouseDown(flags, _flipped(_holder, point));
}

void BaseScene::mouseMove(unsigned int flags, LPoint point)
{
	CScene::mouseMove(flags, point);
	if (_hoveredControl == 0 && _controller)
		_controller->mouseMove(flags, _flipped(_holder, point));
}

void BaseScene::mouseUp(unsigned int flags, LPoint point)
{
	if (_pressedControl) {
		CScene::mouseUp(flags, point);
		return;
	}
	CScene::mouseUp(flags, point);
	if (_controller)
		_controller->mouseUp(flags, _flipped(_holder, point));
}

void BaseScene::onMouseLeave(unsigned int flags, LPoint point)
{
	if (_controller)
		_controller->onMouseLeave(flags, _flipped(_holder, point));
}

void BaseScene::mouseDblClick(unsigned int flags, LPoint point)
{
	if (_controller)
		_controller->mouseDblClick(flags, _flipped(_holder, point));
}

void BaseScene::mouseWheel(short delta, unsigned int flags, LPoint point)
{
	if (_controller)
		_controller->mouseWheel(delta, flags, point);
}

void BaseScene::mouseDragged(unsigned int flags, LPoint point)
{
	CScene::mouseDragged(flags, point);
	if (_pressedControl == 0 && _controller)
		_controller->mouseDragged(flags, _flipped(_holder, point));
}

void BaseScene::dragStarted(unsigned int flags, LPoint point)
{
	if (_controller)
		_controller->dragStarted(flags, _flipped(_holder, point));
}

void BaseScene::dragFinished(unsigned int flags, LPoint point)
{
	if (_controller)
		_controller->dragFinished(flags, _flipped(_holder, point));
}

bool BaseScene::becameInFocus()
{
	return _controller ? _controller->becameInFocus() : true;
}

bool BaseScene::resignFocus()
{
	return _controller ? _controller->resignFocus() : true;
}

bool BaseScene::onKeyDown(unsigned int key, unsigned int flags)
{
	return _controller ? _controller->onKeyDown(key, flags) : false;
}

bool BaseScene::onKeyUp(unsigned int key, unsigned int flags)
{
	return _controller ? _controller->onKeyUp(key, flags) : false;
}

// ---------------------------------------------------------------------------
// Camera and time

void BaseScene::_updateWorldTimeSettings()
{
	if (!_timeFrozen)
		_timeFrozen = true;
}

void BaseScene::_updateCamera()
{
	if (!_timeFrozen)
		_updateWorldTimeSettings();
	_adjustCameraToParameters(_controller->position(), _controller->scale());
}

void BaseScene::worldTimeChanged()
{
	_timeFrozen = false;
	if (_refreshOnTimeChange)
		_holder->refreshViewFromScene(this);
}

void BaseScene::_recacheCameraDependedObjects()
{
	SharedContextObjectsContainer *objects = objectsContainer();
	if (objects == 0)
		return;
	CRoutePointsContainer *routes = objects->routePointsContainer();
	if (routes)
		routes->invalCaptionsLayout();
	routes = objects->routePointsPreviewContainer();
	if (routes)
		routes->invalCaptionsLayout();
}

void BaseScene::viewProjMatrixForViewWithSize(Matrix *out, LSize size)
{
	Matrix projection;
	_camera->projectionMatrixForAspect(size.width / size.height, &projection);
	FHMatrixMultiply(out, _camera->viewMatrix(), &projection);
}

void BaseScene::startCountdown(int seconds)
{
	_countdownStart = LTimer::currentTimeValue();
	_countdownSeconds = seconds < 1 ? 1 : (seconds > 3 ? 3 : seconds);
}

void BaseScene::stopCountdown()
{
	_countdownStart = 0;
}

void BaseScene::storeSceneStateInStream(LData *stream, unsigned int)
{
	LPoint position = _controller->position();
	float scale = _controller->scale();
	CSceneStatePropertyOwner::storeVectorInStream((const Vector2 *)&position, stream);
	CSceneStatePropertyOwner::storeFloatInStream(scale, stream);
}

const void *BaseScene::loadSceneStateFromStream(const void *data, unsigned int)
{
	Vector2 position;
	float scale;
	data = CSceneStatePropertyOwner::loadVectorFromStream(data, &position);
	data = CSceneStatePropertyOwner::loadFloatFromStream(data, &scale);
	LPoint point = {position.x, position.y};
	_controller->setPosition(point);
	_controller->setScale(scale);
	return data;
}

// The route shown while capturing goes with the session.
void BaseScene::storeCaptureSessionConstants(CCaptureSession *session)
{
	RouteObject *route = objectsContainer()->routePointsContainer()->currentRoute();
	if (route == 0)
		return;
	LData *data = LCoder::archiveRootObject(route->dictionaryRepresentation(), true);
	if (data)
		session->setDataForObjectWithID(data, kScenePerpetumRouteObject);
}

void BaseScene::setUseConstantsFromSession(CCaptureSession *session)
{
	RouteObject *route = 0;
	if (session) {
		LData *data = session->dataForObjectWithID(kScenePerpetumRouteObject);
		LMap *rep = data ? lobj_cast<LMap>(LCoder::unarchiveRootObject(data)) : 0;
		if (rep) {
			route = RouteObject::_alloc()->initWithDictionaryRep(rep, 0);
			if (route)
				LAutorelease(route);
		}
	}
	objectsContainer()->routePointsContainer()->setRouteReplicant(route);
}

void BaseScene::highlightEntry(GenericEntry *entry, unsigned char highlight)
{
	LRetain(entry);
	releaseAndClear(_highlightedEntry);
	_highlightedEntry = entry;
	_highlighted = highlight;
	RouteObjectEntry *routeEntry = lobj_cast<RouteObjectEntry>(entry);
	if (routeEntry) {
		CRoutePointsContainer *preview = objectsContainer()->routePointsPreviewContainer();
		if (preview)
			preview->setCurrentRoute(routeEntry->routeObject());
	}
	_controller->refreshView();
}

// ---------------------------------------------------------------------------
// Projections

void BaseScene::scenePointFromGeoPoint(Vector3 *out, const LPoint *geoPoint)
{
	if (_projection) {
		_projection->scenePointFromGeoPoint(out, geoPoint);
		return;
	}
	out->x = out->y = out->z = 0.0f;
}

// In view when inside the frustum, widened by `viewSize` (the size of what
// is drawn at the point).
bool BaseScene::viewPointFromGeoPoint(LPoint *out, const LPoint *geoPoint, PanelDrawIterator *iterator,
                                      const LSize *viewSize) const
{
	if (_projection == 0)
		return false;
	LPoint geo = *geoPoint;
	geo.x += iterator ? iterator->_mapOffset * 360.0f : 0.0f;
	Vector3 scenePoint;
	_projection->scenePointFromGeoPoint(&scenePoint, &geo);
	CContext *context = _holder->context();
	CRenderTarget *target = context ? context->contextRenderTarget() : 0;
	LSize frame = _holder->virtualFrameSize();
	Vector3 projected;
	FHVec3TransformCoord(&projected, &scenePoint, target->viewProjMatrix());
	out->y = (1.0f - projected.y) * frame.height * 0.5f;
	out->x = (projected.x + 1.0f) * frame.width * 0.5f;
	float dx = 0.0f, dy = 0.0f;
	if (viewSize) {
		dx = (viewSize->width + viewSize->width) / frame.width;
		dy = (viewSize->height + viewSize->height) / frame.height;
	}
	float left, right, bottom, top;
	if (dx > 0.0f) {
		left = -1.0f - dx;
		right = 1.0f;
	} else {
		left = -1.0f;
		right = 1.0f - dx;
	}
	if (dy > 0.0f) {
		bottom = -1.0f;
		top = dy + 1.0f;
	} else {
		bottom = dy - 1.0f;
		top = 1.0f;
	}
	return projected.z >= -1.0f && projected.z <= 1.0f && projected.x >= left && projected.x <= right &&
	       projected.y >= bottom && projected.y <= top;
}

// Like viewPointFromGeoPoint, dividing by |w| so points behind the camera
// still land on the right side of the view.
bool BaseScene::viewPointFromGeoPointSmartProject(LPoint *out, const LPoint *geoPoint,
                                                  PanelDrawIterator *iterator) const
{
	if (_projection == 0)
		return false;
	LPoint geo = *geoPoint;
	geo.x += iterator ? iterator->_mapOffset * 360.0f : 0.0f;
	Vector3 scenePoint;
	_projection->scenePointFromGeoPoint(&scenePoint, &geo);
	CContext *context = _holder->context();
	CRenderTarget *target = context ? context->contextRenderTarget() : 0;
	if (target == 0)
		return false;
	Vector4 projected;
	FHVec3Transform(&projected, &scenePoint, target->viewProjMatrix());
	float w = fabsf(projected.w);
	projected.x /= w;
	projected.y /= w;
	projected.z /= w;
	LSize frame = _holder->virtualFrameSize();
	out->x = (projected.x + 1.0f) * frame.width * 0.5f;
	out->y = (1.0f - projected.y) * frame.height * 0.5f;
	return projected.z >= -1.0f && projected.z <= 1.0f && projected.x >= -1.0f && projected.x <= 1.0f &&
	       projected.y >= -1.0f && projected.y <= 1.0f;
}

LPoint BaseScene::_viewMapCaptionPositionForPolysContainer(const LSize &, const LRect &, const LRect &,
                                                           PanelDrawIterator *)
{
	LPoint none = {-10000.0f, -10000.0f};
	return none;
}

static inline bool _isEmptyRect(const LRect &r)
{
	return r.size.width == r.size.height && r.size.height == 0.0f;
}

LPoint BaseScene::_viewMapCaptionPosForQuadTrees(const LSize &captionSize, QuadTreeContainer *east,
                                                 QuadTreeContainer *west, const LRect &geoRect,
                                                 const LRect &labelGeoRect, PanelDrawIterator *iterator)
{
	LSize frame = _holder->virtualFrameSize();
	LPoint position = {-10000.0f, -10000.0f};
	LRect labelMapRect = mapRectFromGeoRect(labelGeoRect);
	float x = labelMapRect.size.width * 0.5f + labelMapRect.origin.x;
	float y = labelMapRect.size.height * 0.5f + labelMapRect.origin.y;
	bool useEast = east != 0 && x >= 0.5f;
	int level;
	if (west == 0) {
		useEast = true;
		x = x + x - 1.0f;
		level = east->logSize();
	} else if (!useEast) {
		x = x + x;
		level = west->logSize();
	} else {
		x = x + x - 1.0f;
		level = east->logSize();
	}
	level &= 0xff;
	float cells = (float)(1 << (level & 0x1f));
	x = x < 0.0f ? 0.0f : (x > 0.99999f ? 0.99999f : x);
	y = y < 0.0f ? 0.0f : (y > 0.99999f ? 0.99999f : y);
	QuadTreeContainer::EntryID target(level, (int)(x * cells), (int)(cells * y));
	QuadTreeContainer::EntryID area;
	_BaseSceneVisTester tester(this, iterator);
	QuadTreeContainer *tree = useEast ? east : west;
	// The original asks for the nearest cell as the west tree's.
	if (tree->findNearestVisibleElemID(target, area, &tester, 0) < 0)
		return position;

	LRect visible = tree->findLargestVisibleRect(area, &tester, useEast ? (const void *)1 : 0);
	LRect mapRect = visible;
	mapRect.origin.x = visible.origin.x * 0.5f + (useEast ? 0.5f : 0.0f);
	mapRect.size.width = visible.size.width * 0.5f;
	LRect visibleGeo = geoRectFromMapRect(mapRect);
	LRect part = LIntersectionRect(visibleGeo, geoRect);
	if (_isEmptyRect(part)) {
		LRect shifted = geoRect;
		shifted.origin.x += 360.0f;
		part = LIntersectionRect(visibleGeo, shifted);
		if (_isEmptyRect(part)) {
			shifted.origin.x -= 720.0f;
			part = LIntersectionRect(visibleGeo, shifted);
			if (_isEmptyRect(part))
				part = labelGeoRect;
		}
	}
	LPoint center = {part.size.width * 0.5f + part.origin.x, part.size.height * 0.5f + part.origin.y};
	viewPointFromGeoPointSmartProject(&position, &center, iterator);
	float maxX = frame.width - captionSize.width;
	float maxY = frame.height - captionSize.height;
	if (maxX < 0.0f)
		maxX = 0.0f;
	if (maxY < 0.0f)
		maxY = 0.0f;
	position.x += captionSize.width * -0.5f;
	position.y += captionSize.height * -0.5f;
	if (position.x <= -150.0f || position.y <= -70.0f || position.x >= maxX + 150.0f || position.y >= maxY + 70.0f) {
		position.x = position.y = -10000.0f;
	} else {
		position.x = position.x < 0.0f ? 0.0f : (position.x > maxX ? maxX : position.x);
		position.y = position.y < 0.0f ? 0.0f : (position.y > maxY ? maxY : position.y);
	}
	return position;
}

// ---------------------------------------------------------------------------
// The selection rectangle and the straightedge

// The zoom rectangle's quad, in clip space.
void BaseScene::setSelectionRect(LRect rect)
{
	StVertexBufferLocker *locker = _zoomRectMesh->detachVBLocker(0, 0);
	CRenderProfile *profile = _zoomRectMesh->renderProfile();
	if (profile == 0 || profile->_shader == 0) {
		delete locker;
		return;
	}
	int position = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	unsigned int stride = profile->vertexStructSize(0);
	char *vertex = (char *)locker->data() + position;
	const float corners[4][2] = {{rect.origin.x, rect.origin.y},
	                             {rect.size.width + rect.origin.x, rect.origin.y},
	                             {rect.origin.x, rect.size.height + rect.origin.y},
	                             {rect.size.width + rect.origin.x, rect.size.height + rect.origin.y}};
	for (int i = 0; i < 4; i++, vertex += stride) {
		float *p = (float *)vertex;
		p[0] = corners[i][0];
		p[1] = corners[i][1];
		p[2] = -1.0f;
	}
	delete locker;
	_controller->refreshView();
}

D3DPRIMITIVETYPE BaseScene::primitiveTypeForMesh(CMesh *mesh)
{
	return _straightedgeMesh == mesh ? D3DPT_LINELIST : D3DPT_TRIANGLELIST;
}

unsigned int BaseScene::numPrimitivesForMesh(CMesh *mesh)
{
	return _straightedgeMesh == mesh ? 23 : 0;
}

unsigned int BaseScene::numVerticesForMesh(CMesh *mesh)
{
	return _straightedgeMesh == mesh ? 46 : 0;
}

void BaseScene::meshWasInstantiated(CMesh *mesh)
{
	if (_straightedgeMesh == mesh)
		_fillStraightedgeMesh(_straightedgeMesh, 1.0f);
}

// The straightedge's lines: {color, x step, y} in pixels from the view's
// right and bottom, x steps `length` * 5 pixels apart.
static const struct {
	unsigned int color;
	int x, y;
} sStraightedge[46] = {
    {0xff000000, 20, 0},  {0xff000000, -20, 0}, {0xff000000, 20, 1},  {0xff000000, -20, 1}, {0xff000000, 20, 2},
    {0xff000000, -20, 2}, {0xffffffff, 10, 1},  {0xffffffff, 0, 1},   {0xffffffff, 10, 2},  {0xffffffff, 0, 2},
    {0xffffffff, -10, 1}, {0xffffffff, -20, 1}, {0xffffffff, -10, 2}, {0xffffffff, -20, 2}, {0xffffffff, 20, 1},
    {0xffffffff, 19, 1},  {0xffffffff, 20, 2},  {0xffffffff, 19, 2},  {0xffffffff, 18, 1},  {0xffffffff, 17, 1},
    {0xffffffff, 18, 2},  {0xffffffff, 17, 2},  {0xffffffff, 16, 1},  {0xffffffff, 15, 1},  {0xffffffff, 16, 2},
    {0xffffffff, 15, 2},  {0xffffffff, 14, 1},  {0xffffffff, 13, 1},  {0xffffffff, 14, 2},  {0xffffffff, 13, 2},
    {0xffffffff, 12, 1},  {0xffffffff, 11, 1},  {0xffffffff, 12, 2},  {0xffffffff, 11, 2},  {0xff000000, 15, 5},
    {0xff000000, 15, 0},  {0xff000000, 20, 5},  {0xff000000, 20, 0},  {0xff000000, 10, 5},  {0xff000000, 10, 0},
    {0xff000000, 0, 5},   {0xff000000, 0, 0},   {0xff000000, -10, 5}, {0xff000000, -10, 0}, {0xff000000, -20, 5},
    {0xff000000, -20, 0},
};

void BaseScene::_fillStraightedgeMesh(CMesh *mesh, float length)
{
	LSize frame = _holder->virtualFrameSize();
	StVertexBufferLocker *vertices = mesh->detachVBLocker(0, 0);
	CRenderProfile *profile = mesh->renderProfile();
	if (profile->_shader == 0)
		LF_RAISE(WrongConditionException());
	int position = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int color = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	unsigned int stride = profile->vertexStructSize(0);
	char *vertex = (char *)vertices->data();
	int count = numVerticesForMesh(mesh);
	for (int i = 0; i < count; i++, vertex += stride) {
		float *p = (float *)(vertex + position);
		p[2] = 0.0f;
		p[1] = ((float)sStraightedge[i].y + 27.0f) * (2.0f / frame.height) - 1.0f;
		p[0] = 1.0f - ((float)sStraightedge[i].x * length * 5.0f + 200.0f) * (2.0f / frame.width);
		*(unsigned int *)(vertex + color) = sStraightedge[i].color;
	}
	delete vertices;
	StIndexBufferLocker *indices = mesh->detachIBLocker(0);
	if (indices->_desc.Format == D3DFMT_INDEX32)
		LF_RAISE(WrongConditionException());
	// The original counted the index buffer's bytes / 6: one line each.
	uint16_t *index = (uint16_t *)indices->data();
	int lines = numPrimitivesForMesh(mesh);
	for (int i = 0; i < lines; i++) {
		index[i * 2] = (uint16_t)(i * 2);
		index[i * 2 + 1] = (uint16_t)(i * 2 + 1);
	}
	delete indices;
}

// ---------------------------------------------------------------------------
// Drawing

// The clear color: white when the sky is skipped (printing), else black.
void BaseScene::frameWillBeRendered()
{
	CRenderTarget *target = _holder->context()->currentRenderTarget();
	float c = _renderOptions && _renderOptions->skipSpaceAndSky() ? 1.0f : 0.0f;
	target->setClearColor(Color(c, c, c, 0.0f));
}

Color BaseScene::fogColor()
{
	return Color(0.0f, 0.0f, 0.0f, 0.0f);
}

void BaseScene::drawScene()
{
	float volume = (float)pow(Preferences::soundVolume(), 1.5);
	if (volume > 0.01f || LF_CLASSOF(SoundManager)->hasSingleton())
		SoundManager::manager()->setListenerVolume(volume);
	releaseAndClear(_shapesCaptionFont);
	if (_boldInfoFont)
		_boldInfoFont->removeAllStrings();
	if (_entryCaptionFont)
		_entryCaptionFont->removeAllStrings();
	if (_normalInfoFont)
		_normalInfoFont->removeAllStrings();
	if (_statusFont)
		_statusFont->removeAllStrings();
	if (_cinematic)
		_cinematic->adjustHostScene();
	objectsContainer()->textureLoader()->startNextFrame();
	_updateCamera();
	float scale = _controller->scale();
	LPoint position = _controller->position();
	float compass = _controller->compassAngle();
	if (fabsf(scale - _cachedScale) > 0.001f || fabsf(compass - _cachedCompassAngle) > 0.001f ||
	    fabsf(position.x - _cachedPosition.x) > 0.0003f || fabsf(position.y - _cachedPosition.y) > 0.0003f) {
		_cachedScale = scale;
		_cachedCompassAngle = compass;
		_cachedPosition = position;
		_recacheCameraDependedObjects();
	}
	if (_holder && _holder->context())
		_holder->context()->contextRenderTarget()->setFogColor(fogColor());
	CScene::drawScene();
}

void BaseScene::_renderEntriesBillboards(CFrameDrawer *drawer)
{
	unsigned int kinds = _renderOptions->showWeatherOnMap() ? kBillboardWeather : 0;
	if (_renderOptions->showPhotosOnMap())
		kinds |= kBillboardPhotos;
	else if (kinds == 0)
		return;
	EntriesBillboardsHolder *billboards = objectsContainer()->entriesBillboards();
	if (billboards)
		billboards->render(this, drawer, kinds);
}

// The billboards, the scale, the status line at the top right ("Authorizing"
// or the clouds being downloaded), then the devices.
void BaseScene::_renderOverlays(CFrameDrawer *drawer)
{
	_renderEntriesBillboards(drawer);
	if (_renderOptions->showScale())
		_renderScale(drawer);
	int clouds = CWManager::instance()->cloudMapsCountInDownloadQueue();
	DownloadManager *downloads = DownloadManager::instance();
	LString *status = 0;
	if (downloads->isAuthorizing())
		status = LString::stringFromTable("Authorizing", "RenderLib", getRenderLibModule());
	else if (!downloads->authorizationFailed(0) && clouds > 0)
		status = LString::stringWithFormat(
		    LString::stringFromTable("Downloading Clouds", "RenderLib", getRenderLibModule()), clouds);
	if (status) {
		C2DTexture *background = objectsContainer()->statusBackgroundTexture();
		float width = (float)background->_info.Width;
		float height = (float)background->_info.Height;
		LRect info = _controller->infoDeviceRect(0);
		LSize frame = _holder->virtualFrameSize();
		LRect rect = {{frame.width - info.origin.x - width, info.origin.y}, {width, height}};
		if (_statusBillboard == 0) {
			_statusBillboard = objectsContainer()->detachBillboardData();
			_statusBillboard->setTextureForSource(background, 0);
			LRetain(_statusBillboard);
			_statusBillboard->_storesState = false;
		}
		if (_statusFont == 0) {
			_statusFont = LRetain(objectsContainer()->detachDataForCaptionFont());
			_statusFont->_storesState = false;
		}
		if (_statusBillboard) {
			_statusBillboard->billboardAtIndex(0, 1)->setScreenRect(rect);
			drawer->renderDrawableData(_statusBillboard, 4);
		}
		if (_statusFont) {
			_statusFont->addCaptionInRect(status, rect, 0);
			_statusFont->setDrawFlags(0xc);
			drawer->renderDrawableData(_statusFont, 4);
		}
	}
	_renderMinimap(drawer);
	_renderNavDevice(drawer);
	_renderInfoDevice(drawer);
}

// The arrows of a pressed or toggled control show its pressed texture, of a
// highlighted one its highlighted texture (one less).
void BaseScene::_renderNavDevice(CFrameDrawer *drawer)
{
	float visibility = _controller->navDevicesVisibility();
	objectsContainer()->setLockButtonsTransparency(visibility);
	if (visibility < 0.001f)
		return;
	int highlighted = _controller->highlightedControlID();
	int control = _controller->pressedControlID();
	if (control == 0)
		control = _controller->toogledControlID();
	bool hover = control == 0;
	if (hover)
		control = highlighted;
	Color color(1.0f, 1.0f, 1.0f, visibility * 0.9f);
	LRect rect = _controller->navDeviceRect();
	LSize frame = _holder->virtualFrameSize();
	rect.origin.y = frame.height - rect.origin.y - rect.size.height;
	LRect compassRect = {{rect.size.width * 0.5f + rect.origin.x - 64.0f, rect.origin.y - 11.0f}, {128.0f, 128.0f}};
	SharedContextObjectsContainer *objects = objectsContainer();

	CBillboardModelData *circle = objects->compassCircleDeviceDrawableData();
	if (circle) {
		CBillboardSingleData *billboard = circle->billboardAtIndex(0, 1);
		billboard->setDiffuseColor(&color);
		billboard->setScreenRect(rect);
		int texture;
		switch (control) {
		case 5:
			texture = 0x11 - hover;
			break;
		case 6:
			texture = 0x13 - hover;
			break;
		case 9:
			texture = 0xd - hover;
			break;
		case 10:
			texture = 0xf - hover;
			break;
		case 11:
			texture = 2 - hover;
			break;
		default:
			texture = 0;
			break;
		}
		circle->setTextureForSource(objects->compassDeviceTexture((TCompassTextureID)texture), 0);
		drawer->renderDrawableData(circle, 4);
	}

	CBillboardModelData *center = objects->compassCenterDeviceDrawableData();
	if (center) {
		CBillboardSingleData *billboard = center->billboardAtIndex(0, 1);
		billboard->setDiffuseColor(&color);
		billboard->setScreenRect(rect);
		billboard->setTrapetionFactor(_controller->compassTrapetionFactor());
		billboard->setFlipHorizontally(0);
		int texture;
		switch (control) {
		case 1:
			texture = 9 - hover;
			break;
		case 2:
			texture = 5 - hover;
			break;
		case 4:
			billboard->setFlipHorizontally(1);
			// fall through
		case 3:
			texture = 7 - hover;
			break;
		case 8:
			billboard->setFlipHorizontally(1);
			// fall through
		case 7:
			texture = 0xb - hover;
			break;
		default:
			texture = 3;
			break;
		}
		center->setTextureForSource(objects->compassDeviceTexture((TCompassTextureID)texture), 0);
		drawer->renderDrawableData(center, 4);
	}

	CBillboardModelData *compass = objects->compassDeviceDrawableData();
	if (compass) {
		float angle = _controller->compassAngle();
		CBillboardSingleData *billboard = compass->billboardAtIndex(0, 1);
		billboard->setDiffuseColor(&color);
		billboard->setScreenRect(compassRect);
		billboard->setTrapetionFactor(_controller->compassTrapetionFactor());
		billboard->setFlipHorizontally(0);
		billboard->setRotationAngle((angle + angle) * 3.1415927f);
		int texture = control == 0xc ? 0x16 - hover : 0x14;
		compass->setTextureForSource(objects->compassDeviceTexture((TCompassTextureID)texture), 0);
		drawer->renderDrawableData(compass, 4);
	}
}

// The minimap, and over it the part of the map in view (three copies across
// the date line unless it shows the whole width).
void BaseScene::_renderMinimap(CFrameDrawer *drawer)
{
	float visibility = _controller->minimapVisibility();
	if (visibility <= 0.001f)
		return;
	Color color(1.0f, 1.0f, 1.0f, visibility * 0.9f);
	CBillboardModelData *minimap = objectsContainer()->minimapDrawableData();
	LRect textureRect = _controller->minimapTextureRect();
	if (minimap) {
		CBillboardSingleData *billboard = minimap->billboardAtIndex(0, 1);
		billboard->setDiffuseColor(&color);
		billboard->setScreenRect(textureRect);
		drawer->renderDrawableData(minimap, 4);
	}
	if (visibility <= 0.99f)
		return;
	LPoint position = _controller->position();
	LSize parts = _controller->visibleParts();
	float widthPart = parts.width / 360.0f;
	LRect mapRect = _controller->minimapMapRect();
	float height = (parts.height / 180.0f) * mapRect.size.height;
	float width = widthPart * mapRect.size.width;
	float y = height * -0.5f + mapRect.size.height * (1.0f - position.y) + mapRect.origin.y;
	for (int copy = -1; copy != 2; copy++) {
		LRect view = {{((float)copy + position.x) * mapRect.size.width + mapRect.origin.x + width * -0.5f, y},
		              {width, height}};
		LRect part = LIntersectionRect(view, mapRect);
		if (_isEmptyRect(part))
			continue;
		if (widthPart > 0.995f) {
			part.size.width = mapRect.size.width;
			part.origin.x = mapRect.origin.x;
		}
		LSize frame = _holder->virtualFrameSize();
		ModelDataWithMatrix *selection = objectsContainer()->detachMapSelectionDrawableData();
		selection->_storesState = false;
		selection->_matrix._11 = (part.size.width + part.size.width) / frame.width;
		selection->_matrix._22 = (part.size.height + part.size.height) / frame.height;
		selection->_matrix._41 = (part.origin.x + part.origin.x) / frame.width - 1.0f;
		selection->_matrix._42 = ((part.origin.y + part.size.height) * -2.0f) / frame.height + 1.0f;
		drawer->renderDrawableData(selection, 4);
		if (widthPart > 0.995f)
			return;
	}
}

// "Information", and beside it the region, the coordinates and the altitude.
void BaseScene::_renderInfoDevice(CFrameDrawer *drawer)
{
	float visibility = _controller->infoDeviceVisibility();
	if (visibility <= 0.001f)
		return;
	Color color(1.0f, 1.0f, 1.0f, visibility);
	SharedContextObjectsContainer *objects = objectsContainer();
	CBillboardModelData *device = objects->infoDeviceDrawableData();
	LRect rect = _controller->infoDeviceRect(0);
	device->setTextureForSource(objects->infoDeviceTexture(0), 0);
	if (device) {
		CBillboardSingleData *billboard = device->billboardAtIndex(0, 1);
		billboard->setDiffuseColor(&color);
		billboard->setScreenRect(rect);
		drawer->renderDrawableData(device, 4);
	}
	LString *title = LString::stringFromTable("Information", "RenderLib", getRenderLibModule());
	if (_boldInfoFont == 0) {
		_boldInfoFont = LRetain(objectsContainer()->detachDataForBoldInfoFont());
		_boldInfoFont->_storesState = false;
		_boldInfoFont->setDrawFlags(0x10);
		_boldInfoFont->setLineSpacing(3.0f);
		Color titleColor((unsigned int)Color(0.5f, 0.6f, 1.0f, 1.0f));
		_boldInfoFont->setColor(titleColor);
	}
	float titleWidth = _boldInfoFont->calcTextSize(title).width + 4.0f;
	LRect titleRect = {{rect.origin.x + 2.0f, rect.origin.y + 8.0f}, {titleWidth, rect.size.height}};
	_boldInfoFont->addCaptionInRect(title, titleRect, 0);
	drawer->renderDrawableData(_boldInfoFont, 4);

	LString *region = _controller->currentRegionString();
	LString *coords = _controller->currentCoordsString();
	LString *altitude = Utils::shortHeightDesription(_controller->currentAltitude());
	if (region == 0)
		region = &sBlankInfoString;
	if (coords == 0)
		coords = &sBlankInfoString;
	if (altitude == 0)
		altitude = &sBlankInfoString;
	LString *text = LString::stringWithFormat("%@\n%@\n%@", region, coords, altitude);
	if (_normalInfoFont == 0) {
		_normalInfoFont = LRetain(objectsContainer()->detachDataForNormalInfoFont());
		_normalInfoFont->_storesState = false;
		_normalInfoFont->setLineSpacing(3.0f);
		_normalInfoFont->setDrawFlags(0x40);
	}
	_normalInfoFont->setColor(color);
	LRect textRect = {{titleRect.origin.x + titleWidth + 4.0f, titleRect.origin.y}, {320.0f - titleWidth, rect.size.height}};
	_normalInfoFont->addCaptionInRect(text, textRect, 0);
	drawer->renderDrawableData(_normalInfoFont, 4);
}

// The animated marker: two rings, the outer one turning back and forth.
void BaseScene::_renderAnimatedMarker(LPoint geoPoint, CFrameDrawer *drawer)
{
	PanelDrawIterator *it = _controller->detachDrawIterator(0);
	double now = LTimer::currentTimeValue();
	LPoint view = LZeroPoint;
	CBillboardModelData *inner = objectsContainer()->location0DData();
	CBillboardModelData *outer = objectsContainer()->location1DData();
	inner->removeAllBillboards();
	outer->removeAllBillboards();
	int count = 0;
	while (it->next()) {
		if (!viewPointFromGeoPoint(&view, &geoPoint, it, 0))
			continue;
		LRect rect = {{view.x - 16.5f, view.y - 16.5f}, {33.0f, 33.0f}};
		CBillboardSingleData *billboard = inner->billboardAtIndex(count, 1);
		billboard->setScreenRect(rect);
		billboard->setRotationAngle(0.0);
		billboard = outer->billboardAtIndex(count, 1);
		billboard->setScreenRect(rect);
		billboard->setRotationAngle(sin(now) * -3.1415927410125732);
		count++;
	}
	if (count) {
		drawer->renderDrawableData(inner, 4);
		drawer->renderDrawableData(outer, 4);
	}
	delete it;
}

void BaseScene::_renderShapeContainer(CFrameDrawer *drawer, CShapeContainer *container, const LString *usage,
                                      unsigned char highlighted)
{
	if (container == 0)
		return;
	if (!geoRectIsVisible(container->allPathsBoundsGeoRect()))
		return;
	if (_capturedProperties && highlighted && _shapeContainerState) {
		_shapeContainerState->grabInfoFromShapeContainer(container, usage);
		storePropertyOwnerIfNeeded(_shapeContainerState, 0);
	}
	CShapeDrawable *drawable =
	    container->drawableForProjection(_projection, _controller->shapesLODFromScale(), triangleDetailing());
	if (drawable == 0)
		return;
	PanelDrawIterator *it = _controller->detachDrawIterator(0);
	for (int copy = 0; it->next(); copy++) {
		unsigned char created = 0;
		LArray *datas = drawable->drawableDatas(copy, _holder, usage, &created);
		int count = datas->count();
		for (int i = 0; i < count; i++) {
			ShapeModelData *data = (ShapeModelData *)datas->objectAtIndex(i);
			if (created) {
				data->setupWithUsageKey(usage);
				data->_storesState = false;
			}
			_adjustShapeData(data, it);
			drawer->renderDrawableData(data, 3);
		}
	}
	delete it;
}

// The caption at the shape's largest visible part, in each copy of the map.
void BaseScene::_renderShapesCaption(CFrameDrawer *drawer, LString *caption, CShapeContainer *container,
                                     unsigned char highlighted)
{
	if (caption == 0 || container == 0)
		return;
	if (highlighted && _capturedProperties) {
		_shapesCaptionState->setShapesInfo(container, caption);
		storePropertyOwnerIfNeeded(_shapesCaptionState, 0);
	}
	if (_shapesCaptionFont == 0) {
		_shapesCaptionFont = LRetain(objectsContainer()->detachDataForLargeFont());
		Color white(1.0f, 1.0f, 1.0f, 1.0f);
		_shapesCaptionFont->setColor(white);
		_shapesCaptionFont->_storesState = false;
		drawer->renderDrawableData(_shapesCaptionFont, 4);
	}
	LSize size = _shapesCaptionFont->calcTextSize(caption);
	LRect bounds = container->allPathsBoundsGeoRect();
	LRect label = bounds;
	CPolysContainer *polys = lobj_cast<CPolysContainer>(container);
	if (polys)
		label = polys->largestPathBoundGeoRect();
	QuadTreeContainer *east = container->quadTree(1);
	QuadTreeContainer *west = container->quadTree(0);
	PanelDrawIterator *it = _controller->detachDrawIterator(0);
	while (it->next()) {
		LPoint position;
		if (east == 0 && west == 0)
			position = _viewMapCaptionPositionForPolysContainer(size, bounds, label, it);
		else
			position = _viewMapCaptionPosForQuadTrees(size, east, west, bounds, label, it);
		if (position.x > -200.0f && position.y > -200.0f)
			_shapesCaptionFont->addCaptionAtPosition(caption, position, 0);
	}
	delete it;
}

void BaseScene::_adjustAllCitiesData(CAllCitiesMeshData *data, PanelDrawIterator *iterator)
{
	data->_matrix._41 = iterator->_mapOffset;
}

void BaseScene::_adjustAllPointsData(ModelDataWithMatrix *data, PanelDrawIterator *iterator)
{
	data->_matrix._41 = iterator->_mapOffset;
}

void BaseScene::_adjustGridData(ModelDataWithMatrix *data, PanelDrawIterator *iterator)
{
	data->_matrix._41 = iterator->_mapOffset;
}

void BaseScene::_adjustRouteData(ModelDataWithMatrix *data, PanelDrawIterator *iterator)
{
	data->_matrix._41 = iterator->_mapOffset;
}

void BaseScene::_adjustShapeData(ModelDataWithMatrix *data, PanelDrawIterator *iterator)
{
	data->_matrix._41 = iterator->_mapOffset;
}

void BaseScene::_adjustRoutePointsProxy(CParticleSystemDrawProxy *proxy, PanelDrawIterator *iterator)
{
	proxy->_matrix._41 = iterator->_mapOffset;
}

// The highlighted shapes, from the timezones up to the counties, with the
// caption of the finest of them.
static void _renderHighlightedShapes(BaseScene *scene, CFrameDrawer *drawer)
{
	float detailing = scene->triangleDetailing();
	LMap *shapes = scene->controller()->shapesToHighlight();
	if (shapes == 0 || shapes->isEmpty())
		return;
	CShapeContainer *countries = (CShapeContainer *)shapes->objectForKey(CShapeManager::kShapeIDCountries);
	CShapeContainer *states = (CShapeContainer *)shapes->objectForKey(CShapeManager::kShapeIDStates);
	CShapeContainer *counties = (CShapeContainer *)shapes->objectForKey(CShapeManager::kShapeIDCounties);
	CShapeContainer *timezones = (CShapeContainer *)shapes->objectForKey(CShapeManager::kShapeIDTimezones);
	int lod = scene->controller()->shapesLODFromScale();
	CShapeContainer *ordered[3] = {timezones, countries, states};
	for (int i = 0; i < 3; i++) {
		if (ordered[i] && ordered[i]->drawableForProjection(scene->projection(), lod, detailing))
			scene->_renderShapeContainer(drawer, ordered[i], ShapeModelData::kUsageHitShapeHighlight, 1);
	}
	CShapeContainer *captioned;
	if (counties) {
		if (counties->drawableForProjection(scene->projection(), lod, detailing))
			scene->_renderShapeContainer(drawer, counties, ShapeModelData::kUsageHitShapeHighlight, 1);
		captioned = counties;
	} else if (states) {
		captioned = states;
	} else if (countries) {
		captioned = countries;
	} else {
		captioned = timezones && lobj_cast<CPolysContainer>(timezones) ? timezones : 0;
	}
	if (scene->renderOptions()->showCaptions())
		scene->_renderShapesCaption(drawer, scene->controller()->shapesCaption(), captioned, 1);
}

// The highlighted entry: a point's marker (in its kind's texture and size
// when highlighted) with its name, a shape with its caption, or a route.
static void _renderHighlightedEntry(BaseScene *scene, CFrameDrawer *drawer, float pointSize)
{
	GenericEntry *entry = scene->_highlightedEntry;
	SharedContextObjectsContainer *objects = scene->objectsContainer();
	CRoutePointsContainer *routes = objects->routePointsContainer();
	bool inRoute = routes && routes->currentRoute() && routes->currentRoute()->containsItem(entry);
	RenderOptions *options = scene->renderOptions();
	AtlasPanelController *controller = scene->controller();

	PointEntry *point = lobj_cast<PointEntry>(entry);
	if (point) {
		bool marked = controller->windowInterface()->entryIsMarked(entry);
		LPoint geoPoint = point->geoPoint(0);
		Vector3 position;
		scene->scenePointFromGeoPoint(&position, &geoPoint);
		C2DTexture *texture = objects->markerTexture();
		MarkerModelData *marker = objects->cityMarker();
		float size = 12.0f;
		if (scene->_highlighted) {
			bool kind = true;
			if (lobj_cast<AppleStoreEntry>(entry)) {
				texture = objects->markerTextureApple();
			} else if (kAltTypeCapital->isEqualTo(entry->altType(0)) ||
			           kAltTypeCountryStateCapital->isEqualTo(entry->altType(0))) {
				texture = objects->markerTextureCapital();
			} else if (kAltTypeStateCapital->isEqualTo(entry->altType(0))) {
				texture = objects->markerTextureCounty();
			} else {
				kind = false;
			}
			if (kind)
				size = CAllPointsContainer::texturedPointsScaleFromPointSize(pointSize);
		}
		marker->_params = Vector4(size, 0.0f, 0.0f, 0.0f);
		marker->_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
		marker->setParticleTexture(texture);
		marker->_matrix._41 = position.x;
		marker->_matrix._42 = position.y;
		marker->_matrix._43 = position.z;
		drawer->renderDrawableData(marker, 3);
		if (!inRoute && !controller->windowInterface()->isPopupEntryInfoVisible()) {
			if (scene->_entryCaptionFont == 0) {
				scene->_entryCaptionFont = LRetain(objects->detachDataForNormalFont());
				Color white(1.0f, 1.0f, 1.0f, 1.0f);
				scene->_entryCaptionFont->setColor(white);
				scene->_entryCaptionFont->_storesState = false;
			}
			if (!options->showLocationNamesInSet() || !marked) {
				PanelDrawIterator *it = controller->detachDrawIterator(0);
				while (it->next()) {
					LPoint view;
					if (scene->viewPointFromGeoPoint(&view, &geoPoint, it, 0)) {
						view.x += 8.0f;
						view.y -= 8.0f;
						scene->_entryCaptionFont->addCaptionAtPosition(entry->shortCaption(0), view, 0);
					}
				}
				drawer->renderDrawableData(scene->_entryCaptionFont, 4);
				delete it;
			}
		}
	}

	ShapeEntry *shapeEntry = lobj_cast<ShapeEntry>(entry);
	if (shapeEntry) {
		CShapeContainer *container = shapeEntry->shapeContainer();
		scene->_renderShapeContainer(drawer, container, ShapeModelData::kUsageAllShapesHighlight, 1);
		if (!inRoute && !options->showLocationNamesInSet())
			scene->_renderShapesCaption(drawer, shapeEntry->shortCaption(0), container, 1);
	}

	if (lobj_cast<RouteObjectEntry>(entry)) {
		CRoutePointsContainer *preview = objects->routePointsPreviewContainer();
		if (preview)
			preview->renderRoutes(drawer);
	}
}

void BaseScene::renderAllDrawables(CFrameDrawer *drawer)
{
	storePropertyOwnerIfNeeded(_sceneState, 0);
	RenderOptions *defaults = objectsContainer()->defaultRenderOptions();
	if (defaults == _renderOptions)
		storePropertyOwnerIfNeeded(defaults, 0);
	CClipingShaderStage::setFarClipValue(_farClipValue);
	_renderEnvironment(drawer);
	if (_renderOptions->renderEarth())
		_renderEarthMap(drawer);
	LSize frame = _holder->virtualFrameSize();
	SharedContextObjectsContainer *objects = objectsContainer();
	if (_renderOptions->showClouds() && _cloudsMesh)
		objects->cloudsController()->renderInScene(this, drawer);

	if (!_renderOptions->isPrinting() && _renderOptions->showAllBorders()) {
		LArray *shapes = _controller->allStaticHilitedShapes();
		int count = shapes ? shapes->count() : 0;
		for (int i = 0; i < count; i++)
			_renderShapeContainer(drawer, (CShapeContainer *)shapes->objectAtIndex(i),
			                      ShapeModelData::kUsageAllShapesHighlight, 1);
	}
	if (!_renderOptions->isPrinting() && _renderOptions->highlightBordersAnyway())
		_renderHighlightedShapes(this, drawer);

	float pointSize = _controller->scale() * 6.0f + 1.0f;
	if (_renderOptions->showAllPoints() && _allPoints) {
		_allPoints->drawWithDrawer(drawer, pointSize);
		storePropertyOwnerIfNeeded(_allPoints, 0);
	}
	if (_renderOptions->showGrid() && _gridMarkers) {
		_gridMarkers->setGridSteps(_controller->gridStepFromScale(1), _controller->gridStepFromScale(0));
		_gridMarkers->render(drawer);
		storePropertyOwnerIfNeeded(_gridMarkers, 0);
	}
	if (!_renderOptions->isPrinting()) {
		CRoutePointsContainer *routes = objects ? objects->routePointsContainer() : 0;
		if (routes) {
			storePropertyOwnerIfNeeded(routes, 0);
			routes->renderRoutes(drawer);
		}
		LRect selection = _controller->_selectionRect;
		if (fabsf(selection.size.width) > 1e-06f && fabsf(selection.size.height) > 1e-06f)
			drawer->renderDrawableData(_zoomRectData, 4);
	}
	if (_highlightedEntry && !_renderOptions->isPrinting())
		_renderHighlightedEntry(this, drawer, pointSize);

	unsigned char markerVisible = 0;
	LPoint marker = _controller->windowInterface()->animatedMarkerPosition(&markerVisible);
	if (markerVisible)
		_renderAnimatedMarker(marker, drawer);
	if (_renderOptions->showOverlays() && _controlsVisible) {
		_renderOverlays(drawer);
	} else if (_holder->virtualFrameSize().width >= 400.0f) {
		if (Preferences::screenSaverBillboardsEnabled())
			_renderEntriesBillboards(drawer);
		if (Preferences::screenSaverDisplayRegionNamesEnabled())
			_renderInfoDevice(drawer);
		if (Preferences::screenSaverDisplayMiniMapEnabled())
			_renderMinimap(drawer);
	}

	// The countdown: the view dims and a clock counts the seconds down.
	bool counting = false;
	double elapsed = 0.0;
	if (_countdownStart > 1.0) {
		elapsed = LTimer::currentTimeValue() - _countdownStart;
		if (elapsed > (double)(unsigned int)_countdownSeconds) {
			_countdownStart = 0;
			if (_controller)
				_controller->onCountdownFinished();
		} else {
			counting = true;
		}
	}
	float preview = _controller->imagesPreviewVisibility();
	float brightness = _cinematic ? _cinematic->_fade : 1.0f;
	brightness *= (1.0f - preview) + preview * 0.4f;
	if (counting)
		brightness *= 0.4f;
	if (brightness < 0.999f) {
		CBillboardModelData *dim = objects->whiteRectDrawableData();
		if (dim) {
			CBillboardSingleData *billboard = dim->billboardAtIndex(0, 1);
			LRect all = {{-1.003f, -1.003f}, {2.003f, 2.003f}};
			billboard->setViewportRect(all);
			Color black(0.0f, 0.0f, 0.0f, 1.0f - brightness);
			billboard->setDiffuseColor(&black);
			drawer->renderDrawableData(dim, 4);
		}
	}
	if (preview > 0.01f)
		objects->imagesPreviewController()->render(drawer, preview);
	if (counting) {
		double seconds = floor(elapsed);
		float aspect = frame.height / frame.width;
		LRect clockRect = {{aspect * -0.5f, -0.5f}, {aspect, 1.0f}};
		CBillboardModelData *clock = objects->clockDrawableData();
		clock->billboardAtIndex(0, 1)->setViewportRect(clockRect);
		drawer->renderDrawableData(clock, 4);
		CBillboardModelData *arrow = objects->clockArrowDrawableData();
		CBillboardSingleData *billboard = arrow->billboardAtIndex(0, 1);
		billboard->setViewportRect(clockRect);
		billboard->setRotationAngle(-((float)(elapsed - seconds) * 6.2831855f));
		drawer->renderDrawableData(arrow, 4);
		CBillboardModelData *digits = objects->clockDigitsDrawableData();
		billboard = digits->billboardAtIndex(0, 1);
		LRect digitRect = {{aspect * -0.125f, -0.5f}, {aspect * 0.25f, 1.0f}};
		billboard->setViewportRect(digitRect);
		Vector2 scale(0.25f, 1.0f);
		billboard->setTextureScale(&scale);
		Vector2 offset((float)(_countdownSeconds - (int)seconds) * 0.25f, 0.0f);
		billboard->setTextureOffset(&offset);
		drawer->renderDrawableData(digits, 4);
	}
}
