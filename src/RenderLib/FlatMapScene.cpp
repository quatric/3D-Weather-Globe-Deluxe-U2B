// FlatMapScene: the flat map, 2 x 1 in scene space, seen from above through
// an orthographic camera; copies of it repeat across the view.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(FlatMapScene, LF_CLASSOF(BaseScene))

FlatMapScene::FlatMapScene() : _renderer(0), _eclipseSource(0)
{
	_farClipValue = 1.0f;
}

void FlatMapScene::dealloc()
{
	releaseAndClear(_renderer);
	releaseAndClear(_eclipseSource);
	BaseScene::dealloc();
}

// Also shifted by a map width either way.
bool FlatMapScene::geoRectIsVisible(LRect rect)
{
	LRect visible = ((FlatMapController *)_controller)->visibleGeoRect();
	if (LIntersectsRect(visible, rect))
		return true;
	visible.origin.x -= 360.0f;
	if (LIntersectsRect(visible, rect))
		return true;
	visible.origin.x += 720.0f;
	return LIntersectsRect(visible, rect);
}

void FlatMapScene::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	BaseScene::storeSceneStateInStream(stream, flags);
}

const void *FlatMapScene::loadSceneStateFromStream(const void *data, unsigned int flags)
{
	const void *next = BaseScene::loadSceneStateFromStream(data, flags);
	_adjustCameraToParameters(_controller->position(), _controller->scale());
	return next;
}

void FlatMapScene::invalidateDayNightSeparator()
{
	_renderer->_separatorValid = false;
}

void FlatMapScene::viewFrameChanged()
{
	BaseScene::viewFrameChanged();
	_renderer->_separatorValid = false;
}

void FlatMapScene::_updateWorldTimeSettings()
{
	BaseScene::_updateWorldTimeSettings();
	_renderer->_separatorValid = false;
}

// The camera looks down at the map point; the view shows `visiblePart` of
// the map's height.
void FlatMapScene::_adjustCameraToParameters(LPoint position, float)
{
	CContextCamera *camera = _camera;
	Vector3 direction(0.0f, 0.0f, -1.0f);
	Vector3 up(0.0f, 1.0f, 0.0f);
	camera->setViewDirection(&direction);
	camera->setViewUpVector(&up);
	camera->setClipDistances(0.5f, 1.5f);
	Vector3 eye(position.x, position.y, 1.0f);
	camera->setPosition(&eye);
	float visiblePart = ((FlatMapController *)_controller)->visiblePartFromCtrlScale();
	LSize frame = _holder->virtualFrameSize();
	LSize frustum = {(frame.width / frame.height) * 0.5f * visiblePart, visiblePart};
	((COrthoCamera *)camera)->setFrustumSize(frustum);
}

// The clouds texture covers one map (the high resolution one, half of it),
// moved to the copy.
void FlatMapScene::_adjustCloudsMeshData(CCloudsMeshData *data, unsigned char high, PanelDrawIterator *iterator)
{
	BaseScene::_adjustCloudsMeshData(data, high, iterator);
	Matrix m;
	m.setIdentity();
	m._11 = 0.5f;
	m._41 = (high ? 0.5f : 0.0f) + iterator->_mapOffset;
	data->_matrix = m;
}

CProjection *FlatMapScene::_createProjection()
{
	return (CProjectionFlatScene *)CProjectionFlatScene::_alloc()->init();
}

CParticleSystem *FlatMapScene::routePointsParticleSystem(int count)
{
	if (count == 0)
		return 0;
	if (_routePointsProfile == 0) {
		_routePointsProfile = LRetain(CRenderProfile::renderProfileByFileDesc(
		    LString::stringWithCString("RENDERPROFILES\\Billboard.yaf"), _holder));
		if (_routePointsProfile == 0)
			return 0;
	}
	CHolder *holder = _holder;
	if (holder)
		holder->beginAccumulationBlock();
	CParticleSystem *system =
	    LAutorelease(CityMarkerModel::_alloc()->initWithParams(0, count, kPartSysQuads, _routePointsProfile, 0, 0));
	SharedContextObjectsContainer *objects = objectsContainer();
	if (objects) {
		C2DTexture *marker = objects->markerTexture();
		if (marker)
			system->setTextureForSource(marker, 0);
	}
	if (holder)
		holder->endAccumulationBlock();
	return system;
}

void FlatMapScene::_renderEarthMap(CFrameDrawer *drawer)
{
	_renderer->render(drawer);
}

FlatMapScene *FlatMapScene::initWithController(AtlasPanelController *controller, CHolder *holder)
{
	_flags |= 7;
	// The clouds: a plane the size of the map.
	SStdGeometry geometry;
	geometry.type = 3;
	geometry.cols = 3;
	geometry.rows = 3;
	geometry.radius = 1.0f;
	geometry.height = 1.0f;
	_cloudsDataSource = LRetain(LStdGeometryDataSource::sourceWithStdGeometry(&geometry));
	_gridMarkers = CFlatMapGridMarkers::_alloc()->initWithScene(this);
	if (BaseScene::initWithController(controller, holder) == 0)
		return 0;
	COrthoCamera *camera = COrthoCamera::_alloc()->init();
	if (camera) {
		LSize size = {1.0f, 1.0f};
		camera->setFrustumSize(size);
		camera->_stateOwnerName = kStateObjNameSceneCamera;
		setCamera(camera);
		camera->release();
	}
	_eclipseSource = (_EclipseFlatDataSource *)_EclipseFlatDataSource::_alloc()->init();
	_renderer = FlatMapRenderer::_alloc()->initWithScene(this);
	if (_renderer) {
		_renderer->_stateOwnerName = kObjNameFlatMapRenderer;
		registerStateOwner(_renderer);
	}
	return this;
}
