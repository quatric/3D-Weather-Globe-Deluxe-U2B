// PerspectiveScene: the globe, a unit sphere seen through a perspective
// camera, with the stars, the atmosphere, the weather and the sun, moon and
// lightning lights.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PerspectiveScene, LF_CLASSOF(BaseScene))

PerspectiveScene::PerspectiveScene()
    : _stars(0), _natureParams(0), _atmosphere(0), _sphere(0), _sunLight(0), _light1(0), _light2(0), _eclipseData(0),
      _eclipseMesh(0), _eclipseSource(0), _aseVertices(0), _aseFaces(0), _unused19C(0), _unused1A0(0),
      _routePointsSystem(0), _pushpinModel(0), _aboutData(0), _aboutStartTime(-1.0)
{
	_fogColor = Color(0.4f, 0.6f, 0.8f, 1.0f);
	_farClipValue = 1.0f;
}

void PerspectiveScene::dealloc()
{
	releaseAndClear(_unused1A0);
	releaseAndClear(_unused19C);
	releaseAndClear(_sunLight);
	releaseAndClear(_light1);
	releaseAndClear(_light2);
	releaseAndClear(_sphere);
	releaseAndClear(_aseVertices);
	releaseAndClear(_aseFaces);
	releaseAndClear(_aboutData);
	releaseAndClear(_routePointsSystem);
	releaseAndClear(_pushpinModel);
	if (_eclipseMesh) {
		_eclipseMesh->dataSourceWillDie();
		releaseAndClear(_eclipseMesh);
	}
	releaseAndClear(_eclipseData);
	releaseAndClear(_eclipseSource);
	releaseAndClear(_atmosphere);
	releaseAndClear(_stars);
	releaseAndClear(_natureParams);
	BaseScene::dealloc();
}

void PerspectiveScene::renderAllDrawables(CFrameDrawer *drawer)
{
	storePropertyOwnerIfNeeded(_natureParams, 0);
	storePropertyOwnerIfNeeded(_atmosphere, 0);
	BaseScene::renderAllDrawables(drawer);
	if (_renderOptions->showAbout())
		_renderAbout(drawer);
}

CSceneStatePropertyOwner *PerspectiveScene::stateOwnerWithName(const char *name)
{
	return BaseScene::stateOwnerWithName(name);
}

void PerspectiveScene::dragBringsEntries(TLArray<LMap *> *)
{
	objectsContainer()->routePointsContainer();
	_flag130 = true;
}

void PerspectiveScene::dragExitedWithAllEntries() {}

void PerspectiveScene::_renderEarthMap(CFrameDrawer *drawer)
{
	_sphere->setNightGlowsVisibility(_natureParams->_groundFactor);
	_sphere->setAnimatedWaterVisibility(_natureParams->_lowAltitudeFactor);
	_sphere->setAnimatedWaterSpecularsVisibility(_natureParams->_sunsetLight);
	_sphere->setAnimatedWaterDayFactor(_natureParams->_dayFactor);
	_sphere->performDraw(drawer, _controller->scale());
	storePropertyOwnerIfNeeded(_sphere, 0);
	_natureParams->drawLighting(drawer);
	if (_eclipseData) {
		_eclipseData->setVisibility((1.0f - _natureParams->_atmosphereFactor) * 0.5f + 0.5f);
		drawer->renderDrawableData(_eclipseData, (TOrder)1);
	}
	if (_aboutData)
		drawer->renderDrawableData(_aboutData, (TOrder)4);
}

// The logo plays once, from the first frame the about box shows.
void PerspectiveScene::_renderAbout(CFrameDrawer *drawer)
{
	double now = LTimer::currentTimeValue();
	if (_aboutStartTime < 0.0)
		_aboutStartTime = now;
	double time = (now - _aboutStartTime) * 0.029999999329447746 - 1.0;
	if (time < 2.4) {
		AboutMeshData *data = objectsContainer()->detachAboutMeshData();
		data->_time = (float)time;
		drawer->renderDrawableData(data, (TOrder)4);
	}
}

// The low resolution clouds texture starts at the back of the globe.
void PerspectiveScene::_adjustCloudsMeshData(CCloudsMeshData *data, unsigned char high, PanelDrawIterator *iterator)
{
	BaseScene::_adjustCloudsMeshData(data, high, iterator);
	FHMatrixRotationZ(&data->_matrix, high ? 0.0f : 3.1415927f);
}

// The pushpins grow with the markers scale and fade in as the camera nears.
void PerspectiveScene::_adjustRoutePointsProxy(CParticleSystemDrawProxy *proxy, PanelDrawIterator *iterator)
{
	BaseScene::_adjustRoutePointsProxy(proxy, iterator);
	CPushpinParticleSystemData *data = lobj_cast<CPushpinParticleSystemData>(proxy->_systemData);
	if (data == 0)
		return;
	float attenuation = _controller->attenuationFactorFromScale(AtlasPanelController::kAttenuationMove);
	data->setScale(attenuation * 0.0015f * (_renderOptions->markersScale() + 0.3f));
	float fog = _controller->scale() * 5.0f - 2.0f;
	if (fog < 0.0f)
		fog = 0.0f;
	else if (fog > 1.0f)
		fog = 1.0f;
	data->setFogVisibility(fog);
}

// Above the centre of the label's path, kept 20 points inside the view;
// (-10000, -10000) when it is behind the globe.
LPoint PerspectiveScene::_viewMapCaptionPositionForPolysContainer(const LSize &captionSize, const LRect &,
                                                                  const LRect &labelGeoRect, PanelDrawIterator *)
{
	LPoint result = {-10000.0f, -10000.0f};
	LSize frame = _holder->virtualFrameSize();
	LPoint center = {labelGeoRect.origin.x + labelGeoRect.size.width * 0.5f,
	                 labelGeoRect.origin.y + labelGeoRect.size.height * 0.5f};
	LPoint view;
	if (!viewPointFromGeoPoint(&view, &center, 0, 0))
		return result;
	result.x = view.x - captionSize.width * 0.5f;
	float y = (view.y - captionSize.height * 0.5f) - 15.0f;
	if (view.x + captionSize.width * 0.5f > frame.width)
		result.x = (frame.width - captionSize.width) - 20.0f;
	else if (-frame.width < result.x && result.x < 0.0f)
		result.x = 20.0f;
	if (view.y + captionSize.height * 0.5f > frame.height)
		result.y = (frame.height - captionSize.height) - 20.0f;
	else
		result.y = y < 0.0f ? 20.0f : y;
	return result;
}

void PerspectiveScene::_renderEnvironment(CFrameDrawer *drawer)
{
	if (_renderOptions->spaceEnabled() && _stars) {
		Vector3 position = *_camera->position();
		_stars->drawWithDrawer(5.0f, _starsScale, &position, drawer);
	}
	_atmosphere->drawWithDrawer(drawer);
	if (_renderOptions->renderEarth())
		_natureParams->drawPrecipitations(this);
}

void PerspectiveScene::playbackStoppedWithCapturer(CSceneCapturer *capturer)
{
	CScene::playbackStoppedWithCapturer(capturer);
	LNotificationCenter::center()->postNotification(TimeController::kPlanetsParamsWereChanged,
	                                                TimeController::instance(), 0);
}

// The sun light shines from the sun, the moon light from the moon.
void PerspectiveScene::_updateWorldTimeSettings()
{
	TimeController *time = TimeController::instance();
	Vector3 direction;
	FHVec3Normalize(&direction, time->sunDirection());
	_sunLight->setDirection(-direction);
	FHVec3Normalize(&direction, time->moonDirection());
	_light1->setDirection(direction);
	if (_eclipseData)
		_eclipseData->adjustToSunMoonParams(time->sunDirection(), time->moonPosition());
}

// The ray through the view point from the near to the far plane, met with
// the unit sphere at its nearer hit.
bool PerspectiveScene::geoPointFromNormalizedViewPoint(LPoint *point)
{
	CContext *context = _holder->context();
	if (context == 0)
		return false;
	CRenderTarget *target = context->contextRenderTarget();
	if (target == 0)
		return false;
	Matrix viewProjection, inverse;
	FHMatrixMultiply(&viewProjection, target->viewMatrix(), target->projectionMatrix());
	FHMatrixInverse(&inverse, 0, &viewProjection);
	float x = point->x + point->x - 1.0f, y = point->y + point->y - 1.0f;
	Vector4 nearPoint(x, y, -1.0f, 1.0f), farPoint(x, y, 1.0f, 1.0f);
	Vector4 a, b;
	FHVec4Transform(&a, &nearPoint, &inverse);
	FHVec4Transform(&b, &farPoint, &inverse);
	a /= a.w;
	b /= b.w;
	Vector3 origin(a.x, a.y, a.z);
	Vector3 ray(b.x - a.x, b.y - a.y, b.z - a.z);
	float rr = ray.x * ray.x + ray.y * ray.y + ray.z * ray.z;
	float ro = ray.x * origin.x + ray.y * origin.y + ray.z * origin.z;
	float disc = ro * ro - ((origin.x * origin.x + origin.y * origin.y + origin.z * origin.z) - 1.0f) * rr;
	if (disc < 0.0f)
		return false;
	disc = sqrtf(disc);
	float t1 = (disc - ro) / rr;
	float t2 = (-ro - disc) / rr;
	if (!((t1 >= 0.0f && t1 <= 1.0f) || (t2 >= 0.0f && t2 <= 1.0f)))
		return false;
	float t = t2 <= t1 ? t2 : t1;
	Vector3 hit(origin.x + ray.x * t, origin.y + ray.y * t, origin.z + ray.z * t);
	*point = geoPointFromEarthVec(&hit);
	return true;
}

static void setLightColor(D3DCOLORVALUE *value, const Color &color)
{
	value->r = color.r;
	value->g = color.g;
	value->b = color.b;
	value->a = 1.0f;
}

// The camera hangs `altitude` above the map point, looking along the
// controller's direction, tilted no lower than the horizon and kept above
// the ground; then the weather, the sky and the lights follow it. (The
// original marked each changed light dirty; here the lights are read each
// frame.)
void PerspectiveScene::_adjustCameraToParameters(LPoint position, float scale)
{
	PerspPanelController *controller = (PerspPanelController *)_controller;
	LPoint geo = geoPointFromMapPoint(position);
	float altitude = controller->altitudeFromScale(scale);
	Vector3 earth = earthVecFromGeoPoint(geo);
	Vector3 up = earth;
	Vector3 direction = controller->viewDirection(&up);
	Vector3 flat = direction;
	float d = earth.x * direction.x + earth.y * direction.y + earth.z * direction.z;
	if (d > 0.0f) {
		flat = Vector3(direction.x - earth.x * d, direction.y - earth.y * d, direction.z - earth.z * d);
		FHVec3Normalize(&flat, &flat);
	}
	Vector3 eye(earth.x - altitude * flat.x, earth.y - altitude * flat.y, earth.z - altitude * flat.z);
	float lowest = controller->altitudeFromScale(scale + 0.15f);
	lowest = lowest > 0.0051515996f ? lowest + 1.0f : 1.0051516f;
	float distance = sqrtf(eye.x * eye.x + eye.y * eye.y + eye.z * eye.z);
	if (distance < lowest) {
		FHVec3Normalize(&eye, &eye);
		eye = Vector3(eye.x * lowest, eye.y * lowest, eye.z * lowest);
		distance = lowest;
	}
	Vector3 eyeDirection;
	FHVec3Normalize(&eyeDirection, &eye);
	float farClip = SphereLOD::farClipValueForCameraParams(&direction, &eyeDirection, distance - 1.0f);
	if (farClip <= 0.1f)
		farClip = 0.1f;
	CContextCamera *camera = _camera;
	camera->setViewUpVector(&up);
	camera->setPosition(&eye);
	camera->setViewDirection(&direction);
	camera->setClipDistances(farClip * 0.0002f, farClip);
	_starsScale = camera->farClip() * 0.95f;
	NatureParams *nature = _natureParams;
	if (_grabbedFrameSource == 0) {
		WeatherTracer *tracer = controller->weatherTracer();
		tracer->tracePosition(geo, distance - 1.0f);
		nature->adjustParameters(tracer, camera, _cinematic);
		_atmosphere->adjustParameters(nature, camera);
	}
	_fogColor = nature->_params.fog;
	setLightColor(&_sunLight->_params.Diffuse, nature->_params.sunDiffuse);
	setLightColor(&_sunLight->_params.Ambient, nature->_params.sunAmbient);
	setLightColor(&_light1->_params.Diffuse, nature->_params.moonDiffuse);
	setLightColor(&_light1->_params.Ambient, nature->_params.moonAmbient);
	GroundElement::setAnimatedWaterLightColor(&nature->_params.waterLight);
	_light2->setDirection(nature->_lightingDir);
	setLightColor(&_light2->_params.Diffuse, nature->_lightingColor);
}

// The system is the scene's until the next call; the original returned it
// unretained too.
CParticleSystem *PerspectiveScene::routePointsParticleSystem(int count)
{
	releaseAndClear(_routePointsSystem);
	if (count == 0)
		return 0;
	if (_routePointsProfile == 0) {
		_routePointsProfile = LRetain(CRenderProfile::renderProfileByFileDesc(
		    LString::stringWithCString("RENDERPROFILES\\CityMarker.yaf"), _holder));
		if (_routePointsProfile == 0)
			return 0;
	}
	CHolder *holder = _holder;
	if (holder)
		holder->beginAccumulationBlock();
	if (_pushpinModel == 0) {
		CPushpinDataSource *source = CPushpinDataSource::_alloc()->initWithASEFileAndRenderProfile(
		    LString::stringWithCString("pushpin.plist"), _routePointsProfile);
		if (source) {
			_pushpinModel = (CPushpinModel *)CPushpinModel::_alloc()->initWithDataSource(source, _routePointsProfile, 1);
			source->release();
		}
	}
	if (_pushpinModel)
		_routePointsSystem = CPushpinParticleSystem::_alloc()->initWithParams(
		    0, count, (TPartSysRepresentationMode)3, _routePointsProfile, _pushpinModel, 0);
	if (holder)
		holder->endAccumulationBlock();
	return _routePointsSystem;
}

PerspectiveScene *PerspectiveScene::initWithController(AtlasPanelController *controller, CHolder *holder)
{
	_flags |= 2;
	// The clouds: a sphere a little above the ground.
	SStdGeometry geometry;
	geometry.type = 1;
	geometry.cols = 0x40;
	geometry.rows = 0x20;
	geometry.radius = 1.002f;
	geometry.partsX = 2;
	geometry.partsY = 1;
	geometry.partX = 0;
	geometry.partY = 0;
	_cloudsDataSource = LRetain(LStdGeometryDataSource::sourceWithStdGeometry(&geometry));
	_gridMarkers = CGlobeMapGridMarkers::_alloc()->initWithScene(this);
	if (BaseScene::initWithController(controller, holder) == 0)
		return 0;
	CContextCamera *camera = CContextCamera::_alloc()->init();
	if (camera) {
		camera->setFOV(0.87266463f);
		camera->_stateOwnerName = kStateObjNameSceneCamera;
		setCamera(camera);
		camera->release();
	}
	_sphere = AdaptiveLODSphere::_alloc()->initWithParams(holder);
	_natureParams = NatureParams::_alloc()->initWithHolder(holder);
	_atmosphere = Atmosphere::_alloc()->initWithScene(this);
	_atmosphere->_stateOwnerName = kStateObjNameAtmosphere;
	CRenderProfile *eclipseProfile =
	    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\Eclipse.yaf"), holder);
	if (eclipseProfile) {
		_eclipseSource = (_EclipseHemisphereDataSource *)_EclipseHemisphereDataSource::_alloc()->init();
		CHolder *sceneHolder = _holder;
		if (sceneHolder)
			sceneHolder->beginAccumulationBlock();
		_eclipseMesh = (CEclipseMesh *)CEclipseMesh::_alloc()->initWithDataSource(_eclipseSource, eclipseProfile, 1);
		_eclipseMesh->_typeName = "PerspectiveScene::_eclipseMesh";
		_eclipseData = LRetain((CEclipseMeshData *)_eclipseMesh->detachModelDataInstance());
		C2DTexture *texture = objectsContainer()->eclipseTexture();
		_eclipseData->setTextureForSource(texture, 0);
		_eclipseData->_stateOwnerName = kObjNameEclipseShadowGlobe;
		if (sceneHolder)
			sceneHolder->endAccumulationBlock();
	}
	// The sun.
	_sunLight = CContextLight::_alloc()->initWithLightType(D3DLIGHT_DIRECTIONAL);
	_sunLight->_params.Ambient = {0.25f, 0.25f, 0.25f, 1.0f};
	_sunLight->_params.Diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
	_sunLight->_params.Specular = {0.29f, 0.29f, 0.29f, 1.0f};
	_sunLight->_intensity = 10.0f;
	_sunLight->_stateOwnerName = kStateObjNameSceneLight0;
	addLight(_sunLight);
	// The moon.
	_light1 = CContextLight::_alloc()->initWithLightType(D3DLIGHT_DIRECTIONAL);
	_light1->_params.Ambient = {0.0f, 0.0f, 0.1f, 1.0f};
	_light1->_params.Diffuse = {0.2f, 0.3f, 0.4f, 1.0f};
	_light1->_params.Specular = {0.29f, 0.29f, 0.29f, 1.0f};
	_light1->_intensity = 2.0f;
	_light1->_stateOwnerName = kStateObjNameSceneLight1;
	addLight(_light1);
	// The lightning, dark until NatureParams flashes it.
	_light2 = CContextLight::_alloc()->initWithLightType(D3DLIGHT_DIRECTIONAL);
	_light2->_params.Ambient = {0.0f, 0.0f, 0.0f, 1.0f};
	_light2->_params.Diffuse = {0.0f, 0.0f, 0.0f, 1.0f};
	_light2->_params.Specular = {0.0f, 0.0f, 0.0f, 1.0f};
	_light2->_intensity = 8.0f;
	_light2->_stateOwnerName = kStateObjNameSceneLight2;
	addLight(_light2);
	_stars = Stars::_alloc()->initWithHolder(_holder, this);
	if (_atmosphere)
		_atmosphere->registerDrawDatas(this);
	if (_eclipseData)
		registerStateOwner(_eclipseData);
	if (_sphere)
		registerStateOwner(_sphere);
	if (_natureParams)
		_natureParams->registerDrawDatas(this);
	if (_atmosphere)
		registerStateOwner(_atmosphere);
	return this;
}

// The original read the file itself and raised on a malformed one; it also
// kept the joined faces' array unretained.
void PerspectiveScene::_parseASEFile(LString *path, unsigned char unused, unsigned char joinFaces)
{
	releaseAndClear(_aseVertices);
	releaseAndClear(_aseFaces);
	if (!parseASEFile(path, unused, joinFaces, &_aseVertices, &_aseFaces))
		LF_RAISE(NilPtrException());
}

CProjection *PerspectiveScene::_createProjection()
{
	return (ProjectionPerspScene *)ProjectionPerspScene::_alloc()->init();
}
