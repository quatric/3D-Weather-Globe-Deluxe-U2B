// The globe's panel controller.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PerspPanelController, LF_CLASSOF(AtlasPanelController))

LF_SELECTOR(PerspPanelController, _demLoadCompleteForGroundElement);
LF_SELECTOR(PerspPanelController, _internetStateWasChanged);

static const float kPi = 3.1415927f;
// log(5 / altitude) for the scale 1.
static const float kScaleLogRange = 6.2f;

PerspPanelController::PerspPanelController()
    : _scene(0), _viewDirValid(false), _unused2A0(0), _unused2A4(0), _dragViewDir(0.0f, 0.0f, 1.0f),
      _dragEarthVec(1.0f, 0.0f, 0.0f), _dragAngle(0.0f), _weatherTracer(0)
{
}

void PerspPanelController::dealloc()
{
	if (_scene)
		_scene->invalidate();
	_scene = 0;
	if (_weatherTracer)
		_weatherTracer->invalidate();
	_weatherTracer = 0;
	AtlasPanelController::dealloc();
}

BaseScene *PerspPanelController::scene()
{
	return _scene;
}

void PerspPanelController::setPosition(LPoint mapPoint)
{
	AtlasPanelController::setPosition(mapPoint);
	_viewDirValid = false;
}

void PerspPanelController::setHorizAngle(float angle)
{
	if (angle < 0.0f)
		angle += (float)(int)(-angle + 1.0f);
	else if (angle > 1.0f)
		angle -= (float)(int)angle;
	_params.angle = angle;
	_viewDirValid = false;
}

void PerspPanelController::setVertAngle(float tilt)
{
	if (tilt < -0.996f)
		tilt = -0.996f;
	else if (tilt > 0.996f)
		tilt = 0.996f;
	_params.tilt = tilt;
	_viewDirValid = false;
}

void PerspPanelController::setTiltAngle(float spin)
{
	if (spin < -1.0f)
		spin = -1.0f;
	else if (spin > 1.0f)
		spin = 1.0f;
	_params.spin = spin;
	_viewDirValid = false;
}

float PerspPanelController::compassTrapetionFactor()
{
	float f = (_params.tilt + 1.0f) * 0.65f;
	if (f < 0.0f)
		return 0.0f;
	return f > 0.6f ? 0.6f : f;
}

bool PerspPanelController::canConvertNormalizedViewPointToGeoPoint(LPoint point, LPoint *geoPoint)
{
	if (!(point.x >= 0.0f && point.y >= 0.0f && point.x <= 1.0f && point.y <= 1.0f))
		return false;
	if (!_scene->geoPointFromNormalizedViewPoint(&point))
		return false;
	if (geoPoint)
		*geoPoint = point;
	return true;
}

// Low over the ground, the moves slow down more when the camera looks at the
// horizon.
float PerspPanelController::attenuationFactorFromScale(TAttenuationFactorType type)
{
	float f;
	if (type == kAttenuationMove)
		f = (float)(2.0 / exp((double)(scale() * kScaleLogRange * 0.6f)));
	else
		f = altitudeFromScale(scale());
	if (f < 0.04f) {
		float tilt = _params.tilt + 1.0f;
		if (tilt > 1.0f)
			tilt = 1.0f;
		if (!(tilt >= 0.0f))
			tilt = 0.0f;
		f = (float)pow((double)(f / 0.04f), (double)(tilt * -0.7f + 1.0f)) * 0.04f;
	}
	return f;
}

// In kilometers.
float PerspPanelController::currentAltitude()
{
	const Vector3 *position = scene()->camera()->position();
	return (position->length() - 1.0f) * 6371.1f;
}

float PerspPanelController::altitudeFromScale(float scale)
{
	return (float)(5.0 / exp((double)(scale * kScaleLogRange)));
}

// The grid is denser at the poles.
float PerspPanelController::_gridStepScaleFactor(unsigned char latitude)
{
	float f;
	if (!latitude)
		f = 0.8f;
	else
		f = (float)((sin((double)(position().y * kPi)) * 0.699999988079071 + 0.30000001192092896) *
		            0.800000011920929);
	return f * kScaleLogRange;
}

void PerspPanelController::_adjustToSlidingProgress(float t, const TParamsHolder *params)
{
	AtlasPanelController::_adjustToSlidingProgress(t, params);
	setHorizAngle(params->angle);
	setVertAngle(params->tilt);
}

void PerspPanelController::_fillFlatViewDir(Vector2 *out)
{
	if (out) {
		float a = (_params.angle + _params.angle) * kPi;
		out->x = sinf(a);
		out->y = cosf(a);
	}
}

void PerspPanelController::sceneWillBeInstalled()
{
	AtlasPanelController::sceneWillBeInstalled();
	LNotificationCenter *center = LNotificationCenter::center();
	center->addObserver(this, "_demLoadCompleteForGroundElement", &sDEMLoadCompleteNotification, 0);
	center->addObserver(this, "_internetStateWasChanged", Preferences::kInternetStateWasChanged, 0);
	center->addObserver(this, "_internetStateWasChanged", Preferences::kCloudsEnableStateWasChanged, 0);
	if (Preferences::usesInternet() && Preferences::cloudsEnabled())
		_weatherTracer->startTracing();
	_animFlags |= kAnimScaleTimer;
	_setupTimer();
}

void PerspPanelController::sceneWasUninstalled()
{
	AtlasPanelController::sceneWasUninstalled();
	LNotificationCenter *center = LNotificationCenter::center();
	center->removeObserver(this, Preferences::kCloudsEnableStateWasChanged, kLock);
	center->removeObserver(this, &sDEMLoadCompleteNotification, kLock);
	center->removeObserver(this, Preferences::kInternetStateWasChanged, kLock);
	_weatherTracer->stopTracing();
	_animFlags &= ~kAnimScaleTimer;
	_setupTimer();
}

void PerspPanelController::stopSounds()
{
	AtlasPanelController::stopSounds();
	_scene->natureParams()->stopSounds();
}

void PerspPanelController::doUninstallButtons()
{
	AtlasPanelController::doUninstallButtons();
	SharedContextObjectsContainer *objects = scene()->objectsContainer();
	scene()->removeControl(objects->lockNormalButton());
	scene()->removeControl(objects->lockTiltedButton());
}

void PerspPanelController::_tiltWithDelta(LPoint delta)
{
	setHorizAngle(delta.x * 0.2f + _params.angle);
	setVertAngle(delta.y * 0.8f + _params.tilt);
	refreshView();
}

void PerspPanelController::setLockMode(TEarthLockMode mode)
{
	AtlasPanelController::setLockMode(mode);
	bool normal = mode == kEarthLockNormal;
	bool tilted = mode == kEarthLockTilted;
	scene()->objectsContainer()->lockNormalButton()->setValue(normal);
	scene()->objectsContainer()->lockTiltedButton()->setValue(tilted);
	if (normal || tilted)
		resetView();
}

TMouseCursorType PerspPanelController::_mouseCursorType()
{
	return AtlasPanelController::_mouseCursorType();
}

intptr_t PerspPanelController::_demLoadCompleteForGroundElement(char *)
{
	_window->refreshAcceleratedView();
	return 0;
}

intptr_t PerspPanelController::_internetStateWasChanged(char *)
{
	if (Preferences::usesInternet() && Preferences::cloudsEnabled())
		_weatherTracer->startTracing();
	else
		_weatherTracer->stopTracing();
	return 0;
}

static float floatFromInfo(LMap *info, LString *key, float def)
{
	LValue *value = lobj_cast<LValue>(info->objectForKey(key));
	return value ? value->floatValue() : def;
}

void PerspPanelController::installSnapshotInfo(LMap *info)
{
	AtlasPanelController::installSnapshotInfo(info);
	if (LValue *value = lobj_cast<LValue>(info->objectForKey(kSnapshotInfoVerticalScaleFactor)))
		Preferences::setVerticalScaleFactor(value->floatValue());
	if (LValue *value = lobj_cast<LValue>(info->objectForKey(kSnapshotInfoOverrideWeather)))
		_weatherTracer->setUseCustomWeather((unsigned char)value->intValue());
	if (!_weatherTracer->_useCustomWeather)
		return;

	LValue *value = lobj_cast<LValue>(info->objectForKey(kSnapshotInfoCloudsType));
	TSkyCode sky = (TSkyCode)(value ? value->intValue() : 0);
	_weatherTracer->setCustomSkyState(sky, sky, 0.0f);
	value = lobj_cast<LValue>(info->objectForKey(kSnapshotInfoPrecipitationsType));
	TPrecipitationCode code = (TPrecipitationCode)(value ? value->intValue() : 0);
	SPrecipitationDescr precip(code, floatFromInfo(info, kSnapshotInfoPrecipitationsIntesity, 0.0f));
	_weatherTracer->setCustomPrecipitations(&precip, &precip, 0.0f);

	SWeatherInfo weather;
	weather.temperature = 0.0f;
	weather.pressure = 0.0f;
	weather.humidity = 0.0f;
	weather.windSpeed = floatFromInfo(info, kSnapshotInfoWindSpeed, 0.0f);
	weather.wind.x = floatFromInfo(info, kSnapshotInfoWindDirectionX, 0.0f);
	weather.wind.y = floatFromInfo(info, kSnapshotInfoWindDirectionY, 0.0f);
	_weatherTracer->setCustomWeatherInfo(&weather);
}

void PerspPanelController::zoomToSelectionRect(LRect rect)
{
	LSize viewSize = _window->acceleratedViewSize();
	float f = rect.size.height;
	if (viewSize.width / viewSize.height < rect.size.width / rect.size.height)
		f = rect.size.width;
	f = fabsf(f) * 0.5f;
	if (f < 0.01f)
		f = 0.01f;
	else if (f > 0.99f)
		f = 0.99f;
	float newScale = scale() - logf(f) / kScaleLogRange;
	if (newScale < 0.0f)
		newScale = 0.0f;
	else if (newScale > 1.0f)
		newScale = 1.0f;
	if (newScale == scale())
		return;
	LPoint center = {(rect.origin.x + rect.size.width * 0.5f) * 0.5f + 0.5f,
	                 (rect.origin.y + rect.size.height * 0.5f) * 0.5f + 0.5f};
	LPoint geoPoint;
	if (!canConvertNormalizedViewPointToGeoPoint(center, &geoPoint))
		geoPoint = geoPointFromMapPoint(position());
	_zoomWithParams(newScale, geoPoint, 0);
}

// The angle between the two positions, seen from the Earth's center.
float PerspPanelController::_interpolationDistance(const TParamsHolder *from, const TParamsHolder *to)
{
	LPoint a = {from->x, from->y};
	LPoint b = {to->x, to->y};
	Vector3 va = earthVecFromMapPoint(a);
	Vector3 vb = earthVecFromMapPoint(b);
	float dot = va.dot(vb);
	if (dot < -1.0f)
		dot = -1.0f;
	else if (dot > 1.0f)
		dot = 1.0f;
	double angle = acos((double)dot);
	return (float)(angle + angle);
}

// The globe turns with the mouse, slower when low.
void PerspPanelController::_directDragWithDiff(LPoint diff)
{
	AtlasPanelController::_directDragWithDiff(diff);
	float f = attenuationFactorFromScale((TAttenuationFactorType)0);
	LPoint last = _dragDiff;
	_dragDiff = diff;
	LPoint delta = {f * (last.x - diff.x), (last.y - diff.y) * f};
	_moveWithDelta(delta);
}

void PerspPanelController::doInstallButtons()
{
	AtlasPanelController::doInstallButtons();
	BaseScene *scene = this->scene();
	SharedContextObjectsContainer *objects = scene->objectsContainer();
	LPoint nrm = {0.5f, 1.0f};

	CSceneControl *button = objects->lockNormalButton();
	LPoint normalOffset = {-48.0f, -20.0f};
	button->setNrmPosWithPtOff(nrm, normalOffset);
	button->setTagTargetSelector(5, this, "_onUIButton");
	scene->addControl(button);

	button = objects->lockTiltedButton();
	LPoint tiltedOffset = {49.0f, -20.0f};
	button->setNrmPosWithPtOff(nrm, tiltedOffset);
	button->setTagTargetSelector(6, this, "_onUIButton");
	scene->addControl(button);
}

// The scale at which a sphere cap `geoRect.size` across in degrees fills the
// view.
float PerspPanelController::zoomScaleToFitGeoRect(LRect geoRect)
{
	float size = sqrtf(geoRect.size.width * geoRect.size.width + geoRect.size.height * geoRect.size.height);
	if (size < 1.0f)
		size = 1.0f;
	else if (size > 179.0f)
		size = 179.0f;
	float fov = scene()->camera()->fov();
	float a = ((size * kPi) / 180.0f) * 0.5f;
	float s = sinf(a);
	float t = tanf(fov * 0.5f);
	float c = cosf(a);
	float scale = (float)(log((double)(5.0f / ((s / t + c) - 1.0f))) / 6.199999809265137);
	if (scale > 1.0f)
		scale = 1.0f;
	if (!(scale >= 0.0f))
		scale = 0.0f;
	return scale;
}

LSize PerspPanelController::visibleParts()
{
	float latFrom, latTo, lonFrom, lonTo;
	CGlobeMapGridMarkers::calcRenderParams(scene()->camera()->position(), &latFrom, &latTo, &lonFrom, &lonTo);
	LSize size = {fabsf(lonTo - lonFrom), fabsf(latTo - latFrom)};
	return size;
}

Vector3 PerspPanelController::viewDirection(Vector3 *up)
{
	if (!_viewDirValid || _flying) {
		if (_flying) {
			// Turned along the route.
			Vector2 dir = scene()->objectsContainer()->routePointsContainer()->currentFlyDirection();
			float angle = (float)(acos((double)dir.y) / 6.2831854820251465);
			if (dir.x < 0.0f)
				angle = 1.0f - angle;
			setHorizAngle(angle);
		}
		// The camera above the position, looking down, north up; turned by
		// the angle, tilted, then moved to the position; then rolled.
		Vector3 pole(0.0f, 0.0f, 1.0f);
		Vector3 earthVec = earthVecFromMapPoint(position());
		Vector3 dir(pole.x - earthVec.x, pole.y - earthVec.y, 0.0f);
		FHVec3Normalize(&dir, &dir);
		_upVector = earthVec;
		Vector3 axis;
		FHVec3Cross(&axis, &pole, &earthVec);
		FHVec3Normalize(&axis, &axis);
		Vector3 tiltAxis;
		FHVec3Cross(&tiltAxis, &dir, &pole);
		FHVec3Normalize(&tiltAxis, &tiltAxis);
		float z = earthVec.z;
		if (z > 1.0f)
			z = 1.0f;
		if (!(z >= -1.0f))
			z = -1.0f;
		Matrix turn, tilt, turned, toPosition, m;
		FHMatrixRotationAxis(&turn, &pole, _params.angle * -2.0f * kPi);
		FHMatrixRotationAxis(&tilt, &tiltAxis, _params.tilt * 0.5f * kPi);
		FHMatrixMultiply(&turned, &tilt, &turn);
		FHMatrixRotationAxis(&toPosition, &axis, acosf(z));
		FHMatrixMultiply(&m, &turned, &toPosition);
		_SVector4 v;
		FHVec3Transform(&v, &dir, &m);
		_viewDir = Vector3(v.x, v.y, v.z);
		FHMatrixRotationAxis(&m, &_viewDir, _params.spin * 6.2831855f);
		FHVec3Transform(&v, &_upVector, &m);
		_upVector = Vector3(v.x, v.y, v.z);
		_viewDirValid = true;
	}
	if (up)
		*up = _upVector;
	return _viewDir;
}

void PerspPanelController::_directDragStarted()
{
	AtlasPanelController::_directDragStarted();
	_dragViewDir = viewDirection(0);
	_dragEarthVec = earthVecFromMapPoint(position());
	_dragAngle = _params.angle;
	_dragDiff = LZeroPoint;
}

void PerspPanelController::setupViewParams(const Vector3 *earthVec, const Vector3 *viewDir, LPoint delta)
{
	Vector3 e = *earthVec;
	float x = position().x;
	Vector3 side;
	FHVec3Cross(&side, viewDir, &e);
	FHVec3Normalize(&side, &side);
	Vector3 forward;
	FHVec3Cross(&forward, &e, &side);
	FHVec3Normalize(&forward, &forward);
	Vector3 moved = side * delta.x + forward * delta.y + e;
	FHVec3Normalize(&moved, &moved);
	// The original also built this rotation, unused.
	Vector3 axis;
	FHVec3Cross(&axis, &e, &moved);
	FHVec3Normalize(&axis, &axis);
	Matrix rotation;
	FHMatrixRotationAxis(&rotation, &axis, sqrtf(delta.x * delta.x + delta.y * delta.y));

	LPoint mapPoint = mapPointFromEarthVec(&moved);
	float dx = mapPoint.x - x;
	if (dx < -0.5f)
		dx += 1.0f;
	if (dx > 0.5f)
		dx -= 1.0f;
	if (dx > 0.01f)
		mapPoint.x = x + 0.01f;
	if (dx < -0.01f)
		mapPoint.x = x - 0.01f;
	setPosition(mapPoint);
}

void PerspPanelController::_moveWithDelta(LPoint delta)
{
	Vector3 earthVec = earthVecFromMapPoint(position());
	Vector3 viewDir = viewDirection(0);
	setupViewParams(&earthVec, &viewDir, delta);
	refreshView();
}

PerspPanelController *PerspPanelController::initWithMainWindowInterface(IWindowInterface *wnd)
{
	if (!AtlasPanelController::initWithMainWindowInterface(wnd))
		return 0;
	_weatherTracer = WeatherTracer::_alloc()->init();
	_scene = PerspectiveScene::_alloc()->initWithController(this, wnd->acceleratedView()->holder());
	if (!getDefaultCamParams(&_params)) {
		_params.x = 0.5f;
		_params.y = 0.5f;
		_params.angle = 0.0f;
		_params.tilt = -0.996f;
		_params.scale = 0.2f;
		_params.spin = 0.0f;
	}
	return this;
}

LMap *PerspPanelController::collectSnapshotInfo()
{
	LMap *info = AtlasPanelController::collectSnapshotInfo();
	info->setObjectForKey(LValue::valueWithFloat(Preferences::verticalScaleFactor()),
	                      kSnapshotInfoVerticalScaleFactor);
	info->setObjectForKey(LValue::valueWithBoolean(_weatherTracer->_useCustomWeather), kSnapshotInfoOverrideWeather);
	if (!_weatherTracer->_useCustomWeather)
		return info;

	TSkyCode sky, nextSky;
	float weight, nextWeight;
	_weatherTracer->getSkyState(&sky, &weight, &nextSky, &nextWeight);
	info->setObjectForKey(LValue::valueWithInt(sky), kSnapshotInfoCloudsType);
	SPrecipitationDescr precip, nextPrecip;
	_weatherTracer->getPrecipitations(&precip, &weight, &nextPrecip, &nextWeight);
	info->setObjectForKey(LValue::valueWithInt(precip.code), kSnapshotInfoPrecipitationsType);
	info->setObjectForKey(LValue::valueWithFloat(precip.intensity), kSnapshotInfoPrecipitationsIntesity);
	if (const SWeatherInfo *weather = _weatherTracer->currentWeatherInfo()) {
		info->setObjectForKey(LValue::valueWithFloat(weather->windSpeed), kSnapshotInfoWindSpeed);
		info->setObjectForKey(LValue::valueWithFloat(weather->wind.x), kSnapshotInfoWindDirectionX);
		info->setObjectForKey(LValue::valueWithFloat(weather->wind.y), kSnapshotInfoWindDirectionY);
	}
	return info;
}

LObject *PerspPanelController::detachPrintObjectWithFrame(LRect frame, unsigned int flags)
{
	return LAutorelease(SphereDrawing::_alloc()->initWithFrameSizeScene(frame.size, scene(), flags));
}
