// AtlasPanelController: the camera of a panel and its animations, the
// on-screen devices (navigation, info, minimap, images preview) and the
// mouse and keyboard handling. The globe and flat map panels derive from it.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(AtlasPanelController, 0)
LF_SELECTOR(AtlasPanelController, _onTimer);
LF_SELECTOR(AtlasPanelController, _delayedCurrentCitySelect);
LF_SELECTOR(AtlasPanelController, _timerOnFirstClickProcess);
LF_SELECTOR(AtlasPanelController, _timerOnShowPopupInfo);
LF_SELECTOR(AtlasPanelController, _timerOnHidePopupInfo);
LF_SELECTOR(AtlasPanelController, _accelViewFrameDidChange);
LF_SELECTOR(AtlasPanelController, _worldTimeWasChanged);
LF_SELECTOR(AtlasPanelController, _playSoundsStateWasChanged);
LF_SELECTOR(AtlasPanelController, _enabledBordersSetWasChanged);
LF_SELECTOR(AtlasPanelController, _imagesSelectionWasChanged);
LF_SELECTOR(AtlasPanelController, _onDBUpdateFinished);
LF_SELECTOR(AtlasPanelController, _onUIButton);
LF_SELECTOR(AtlasPanelController, _selectNextDemoSequence);

// The UI buttons' tags (_onUIButton).
enum { kButtonNav = 1, kButtonInfo = 2, kButtonMinimap = 3, kButtonFullScreen = 4, kButtonLockNormal = 5,
       kButtonLockTilted = 6 };

// _buttons: the mouse buttons down.
enum { kMouseLeft = 0x8, kMouseRight = 0x10, kMouseOther = 0x20 };

// _keys
enum {
	kKeyTiltUp = 0x1,
	kKeyTiltDown = 0x2,
	kKeyTurnLeft = 0x4,
	kKeyTurnRight = 0x8,
	kKeyZoomIn = 0x10,
	kKeyZoomOut = 0x20,
	kKeyUp = 0x40,
	kKeyDown = 0x80,
	kKeySpinLeft = 0x100,
	kKeySpinRight = 0x200,
};

// _mouseTrackBehaviour
enum { kTrackNone = 0, kTrackMove = 1, kTrackDirectDrag = 2, kTrackTilt = 3, kTrackZoomIn = 4, kTrackZoomOut = 5 };

// The center of the compass ring on the navigation device, in device units.
static const LPoint kNavCompassCenterPoint = {0.5f, 0.42f};

static const float kTwoPi = 6.2831855f;

// ---------------------------------------------------------------------------

void AtlasPanelController::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	releaseAndClear(_highlightedShapes);
	releaseAndClear(_shapesCaption);
	releaseAndClear(_staticHilitedShapes);
	releaseAndClear(_regionString);
	releaseAndClear(_coordsString);
	releaseAndClear(_capturer);
	releaseAndClear(_hoveredEntry);
	releaseAndClear(_flyingSound);
	LObject::dealloc();
}

AtlasPanelController *AtlasPanelController::initWithMainWindowInterface(IWindowInterface *wnd)
{
	if (LObject::init() == 0)
		return 0;
	_window = wnd;
	LNotificationCenter *center = LNotificationCenter::center();
	center->addObserver(this, "_onDBUpdateFinished", AtlasDatabase::kMainDatabaseDidEndUpdate, 0);
	center->addObserver(this, "_enabledBordersSetWasChanged", Preferences::kEnabledBordersSetWasChanged, 0);
	center->addObserver(this, "_imagesSelectionWasChanged", kEntryInfoImageWasSelectedNotification, 0);
	controlAction(LZeroPoint, Preferences::toggledNavigationControl());
	return this;
}

void AtlasPanelController::resetPosScaleTimer()
{
	_animFlags &= ~kAnimScaleTimer;
	if (_timer)
		_timer->invalidate();
	_timer = 0;
}

void AtlasPanelController::refreshView()
{
	if (_window)
		_window->refreshAcceleratedView();
}

void AtlasPanelController::_updateMinimapWeigths()
{
	_minimapSmallWeight = _minimapWeights.y;
	_minimapFullWeight = _minimapWeights.x;
	_minimapHiddenWeight = 1.0f - (_minimapWeights.x + _minimapWeights.y);
}

void AtlasPanelController::_resetKBFlags()
{
	_keys = 0;
	_zoomKey = false;
	_zoomOutKey = false;
	_tiltKey = false;
	if (_animFlags & kAnimKeyboard) {
		_animFlags &= ~kAnimKeyboard;
		_setupTimer();
	}
}

void AtlasPanelController::doUninstallButtons()
{
	BaseScene *scene = this->scene();
	SharedContextObjectsContainer *objects = scene->objectsContainer();
	CSceneControl *button = objects->navButton();
	scene->removeControl(button);
	button->setTagTargetSelector(0, 0, 0);
	button = objects->infoButton();
	scene->removeControl(button);
	button->setTagTargetSelector(0, 0, 0);
	button = objects->minimapButton();
	scene->removeControl(button);
	button->setTagTargetSelector(0, 0, 0);
	button = objects->fullScreenButton();
	scene->removeControl(button);
	button->setTagTargetSelector(0, 0, 0);
}

// The timer runs while something animates or a route is flown.
void AtlasPanelController::_setupTimer()
{
	if (_timer) {
		if (_animFlags != 0)
			return;
		if (!_flying) {
			_timer->invalidate();
			_timer = 0;
		}
	}
	if ((_animFlags != 0 || _flying) && _timer == 0)
		_timer = LTimer::timerWithParams(30, this, "_onTimer", true, 3);
}

void AtlasPanelController::startDemoSequence()
{
	_introRunning = false;
	invokeSelector("_selectNextDemoSequence");
}

void AtlasPanelController::startCountdown(int seconds)
{
	scene()->startCountdown(seconds);
}

void AtlasPanelController::stopCountdown()
{
	scene()->stopCountdown();
}

void AtlasPanelController::onCountdownFinished()
{
	if (_captureAfterCountdown && _capturer)
		_capturer->startCapturing();
	_captureAfterCountdown = false;
	LNotificationCenter::center()->postNotification(kCountdownFinishedNotification, this, 0);
}

void AtlasPanelController::startCapturing()
{
	if (_capturer) {
		_captureAfterCountdown = true;
		startCountdown(3);
	}
}

bool AtlasPanelController::isCapturing()
{
	if (_capturer == 0)
		return false;
	return _captureAfterCountdown || _capturer->isCapturing();
}

void AtlasPanelController::stopCapturing()
{
	_captureAfterCountdown = false;
	stopCountdown();
	if (_capturer)
		_capturer->stopCapturing();
}

void AtlasPanelController::playSequence()
{
	if (_capturer)
		_capturer->playInScene(0);
}

bool AtlasPanelController::isSequencePlaying()
{
	return _capturer && _capturer->isPlaying();
}

void AtlasPanelController::stopSequence()
{
	if (_capturer)
		_capturer->stopPlayback();
}

bool AtlasPanelController::sequenceWasRecorded()
{
	return _capturer && !_capturer->isEmpty();
}

void AtlasPanelController::stopFlying()
{
	_flying = false;
	_setupTimer();
	scene()->objectsContainer()->routePointsContainer()->stopFlying();
	if (_flyingSound)
		_flyingSound->stopPlaying();
	_window->stopAnyFlying();
}

void AtlasPanelController::setPosition(LPoint mapPoint)
{
	_params.x = mapPoint.x;
	if (mapPoint.x < 0.0f)
		_params.x = (float)(int)((0.0f - mapPoint.x) + 1.0f) + mapPoint.x;
	else if (mapPoint.x > 1.0f)
		_params.x = mapPoint.x - (float)(int)mapPoint.x;
	float y = mapPoint.y;
	if (_lockMode == kEarthLockNone) {
		if (y < 0.001f)
			y = 0.001f;
		else if (y > 0.999f)
			y = 0.999f;
	} else {
		if (y < 0.01f)
			y = 0.01f;
		else if (y > 0.99f)
			y = 0.99f;
	}
	_params.y = y;
	if (Preferences::screenSaverDisplayRegionNamesEnabled()) {
		_mouseGeoPoint = geoPointFromMapPoint(mapPoint);
		releaseAndClear(_coordsString);
		releaseAndClear(_regionString);
	}
}

void AtlasPanelController::setScaleFromUI(float scale)
{
	setScale(scale);
	_animFlags &= ~(kAnimSliding | kAnimInertia);
	_setupTimer();
}

void AtlasPanelController::setScale(float scale)
{
	float s = 0.0f;
	if (scale >= 0.0f)
		s = scale <= 1.0f ? scale : 1.0f;
	_params.scale = s;
	_requestedScale = scale;
	_window->setScaleValue(scale);
}

void AtlasPanelController::_showPopupInfoForEntry(GenericEntry *entry)
{
	_window->showPopupEntryInfo(entry);
	_popupTimer = LTimer::timerWithParams(60000, this, "_timerOnHidePopupInfo", false, 0);
}

intptr_t AtlasPanelController::_timerOnShowPopupInfo(char *)
{
	_popupTimer = 0;
	_showPopupInfoForEntry(_hoveredEntry);
	return 0;
}

intptr_t AtlasPanelController::_timerOnHidePopupInfo(char *)
{
	_popupTimer = 0;
	_window->hidePopupEntryInfo();
	return 0;
}

intptr_t AtlasPanelController::_accelViewFrameDidChange(char *)
{
	scene()->viewFrameChanged();
	updateCursor();
	return 0;
}

intptr_t AtlasPanelController::_worldTimeWasChanged(char *)
{
	scene()->worldTimeChanged();
	return 0;
}

intptr_t AtlasPanelController::_playSoundsStateWasChanged(char *)
{
	if (!Preferences::playSounds())
		stopSounds();
	return 0;
}

void AtlasPanelController::updateCursor()
{
	_window->setMouseCursorType(_mouseCursorType());
}

void AtlasPanelController::stopSounds()
{
	releaseAndClear(_flyingSound);
}

float AtlasPanelController::minimapVisibility()
{
	if (Preferences::screenSaverDisplayMiniMapEnabled())
		return Preferences::screenSaverPointOfViewMode() == 0 ? 1.0f : 0.0f;
	return _minimapFullWeight + _minimapSmallWeight;
}

float AtlasPanelController::infoDeviceVisibility()
{
	if (Preferences::screenSaverDisplayRegionNamesEnabled())
		return Preferences::screenSaverPointOfViewMode() == 0 ? 1.0f : 0.0f;
	return _infoVisibility;
}

float AtlasPanelController::_interpolationDistance(const TParamsHolder *from, const TParamsHolder *to)
{
	float dx = from->x - to->x;
	float dy = from->y - to->y;
	return sqrtf(dx * dx + dy * dy) * 9.0f;
}

// A slide climbs and comes down again: the altitude (1 - scale) follows a
// parabola through the two ends that tops out higher the farther apart they
// are, and the position moves with the distance flown at that altitude.
void AtlasPanelController::_interpolateSlidingParams(const TParamsHolder *from, const TParamsHolder *to,
                                                     TParamsHolder *out, float t)
{
	float distance = _interpolationDistance(from, to);
	float altFrom = 1.0f - from->scale;
	float altTo = 1.0f - to->scale;
	float altMax = altFrom > altTo ? altFrom : altTo;
	float climb = (distance * 0.5f + 0.001f) * 1.4f;
	float top = altMax;
	if (altMax <= climb) {
		top = altMax > 0.8f ? altMax : 0.8f;
		if (climb <= top)
			top = climb;
	}
	float p = top - altFrom;
	if (p < 0.0f)
		p = 0.0f;
	float q = top - altTo;
	if (q < 0.0f)
		q = 0.0f;
	// altitude(t) = altFrom + b t + a t^2
	float a = -(sqrtf(p) + sqrtf(q)) * (sqrtf(p) + sqrtf(q));
	float b = p + sqrtf(p * q);
	b = b + b;
	float at = a * t;
	float halfB = b * 0.5f;
	out->scale = 1.0f - (t * at + b * t + altFrom);
	float f = (((at / 3.0f + halfB) * t + altFrom) * t) / (altFrom + a / 3.0f + halfB);
	float g = 1.0f - f;
	out->x = g * from->x + f * to->x;
	out->y = g * from->y + f * to->y;
	out->angle = g * from->angle + f * to->angle;

	// The tilt eases off to the scale's in the middle of the slide.
	float tilt = _params.scale * 1.3f - 1.0f;
	if (tilt < -0.99f)
		tilt = -0.99f;
	else if (tilt > 0.99f)
		tilt = 0.99f;
	float tiltFrom = _slideFrom.tilt;
	float tiltTo = _slideTo.tilt;
	if (tiltFrom <= tilt)
		tilt = tiltFrom;
	if (tiltTo <= tilt)
		tilt = tiltTo;
	if (t >= 0.5f) {
		float s = t - 0.5f;
		out->tilt = tilt * (s * -2.0f + 1.0f) + tiltTo * (s + s);
	} else {
		out->tilt = tiltFrom * (t * -2.0f + 1.0f) + (t + t) * tilt;
	}
}

void AtlasPanelController::setCurrentEntry(GenericEntry *entry, unsigned char delayed)
{
	scene()->objectsContainer()->setCurrentEntry(entry);
	scene()->highlightEntry(entry, 0);
	if (_citySelectTimer)
		_citySelectTimer->invalidate();
	if (!delayed || entry == 0)
		_citySelectTimer = 0;
	else
		_citySelectTimer = LTimer::timerWithParams(350, this, "_delayedCurrentCitySelect", false, 0);
}

// The map part of the minimap texture.
LRect AtlasPanelController::minimapMapRect()
{
	LRect r = minimapTextureRect();
	float w = r.size.width;
	r.size.width = w * 972.0f * (1.0f / 1024.0f);
	r.origin.x = w * 26.0f * (1.0f / 1024.0f) + r.origin.x;
	float h = r.size.height;
	r.size.height = h * 486.0f * (1.0f / 512.0f);
	r.origin.y = h * 12.0f * (1.0f / 512.0f) + r.origin.y;
	return r;
}

void AtlasPanelController::controlDown(TNavControlID)
{
	_animFlags |= kAnimControlDown;
	_setupTimer();
}

void AtlasPanelController::controlUp(TNavControlID)
{
	_animFlags = (_animFlags & ~kAnimControlDown) | kAnimInertia;
	_setupTimer();
}

// A click on a control of the navigation device: the reset button resets
// the view; the move buttons toggle a continuous move, and the stop button
// ends it.
void AtlasPanelController::controlAction(LPoint, TNavControlID control)
{
	if (control == kNavReset) {
		resetView();
		return;
	}
	if (control < kNavCompass) {
		if ((unsigned int)(control - 1) > 3)
			return;
	} else if (control != kNavStop) {
		return;
	}
	if (!(_animFlags & kAnimControlDown) && control != kNavStop) {
		_animFlags |= kAnimControlDown;
		_toggledControl = control;
	} else {
		_animFlags = (_animFlags & ~kAnimControlDown) | kAnimInertia;
		_toggledControl = (TNavControlID)0;
	}
	Preferences::setToggledNavigationControl(_toggledControl);
	_setupTimer();
}

void AtlasPanelController::refreshCameraSettings()
{
	refreshView();
	if (_captionsDirty) {
		LNotificationCenter::center()->postNotification(kUpdateLayoutCaptionsNotification, this, 0);
		_captionsDirty = false;
	}
}

// A single click, once it is known not to be the first of a double click:
// a click on a city opens it, any other click acts on the map.
void AtlasPanelController::_firstClickProcess(LPoint point, unsigned char billboard)
{
	if (_firstClickTimer)
		_firstClickTimer->invalidate();
	_firstClickTimer = 0;
	TMapMouseMode behaviour = Preferences::mouseBehaviourOnMap();
	if (!_window->isInCoordinatesPickerMode() && behaviour == 1 && _hoveredEntry) {
		_window->entryWasClicked(_hoveredEntry, point, (TBillboardKind)(billboard != 0));
		return;
	}
	LSize size = _window->acceleratedViewSize();
	LPoint geoPoint;
	LPoint nrm = {point.x / size.width, point.y / size.height};
	if (canConvertNormalizedViewPointToGeoPoint(nrm, &geoPoint))
		singleActionOnGeoPoint(geoPoint);
}

intptr_t AtlasPanelController::_timerOnFirstClickProcess(char *)
{
	_firstClickProcess(_firstClickPoint, _firstClickBillboard);
	return 0;
}

void AtlasPanelController::hideImagesPreview()
{
	if (_previewVisibility >= 0.001f && _previewDirection >= -0.5f) {
		_minimapTracking = false;
		double now = LTimer::currentTimeValue();
		_animFlags |= kAnimPreviewFade;
		_previewDirection = -1.0f;
		_previewStartVisibility = _previewVisibility;
		_previewFadeStart = now;
		if (_sceneInstalled)
			_setupTimer();
	}
}

void AtlasPanelController::mouseUp(unsigned int flags, LPoint point)
{
	if (_pressedControl) {
		TNavControlID control = controlIDFromScreenPosition(point);
		controlUp(_pressedControl);
		if (control == _pressedControl)
			controlAction(point, control);
		_pressedControl = (TNavControlID)0;
		return;
	}
	if (!_minimapTracking) {
		if (_firstClickTimer) {
			_firstClickTimer->invalidate();
			_firstClickTimer = 0;
		}
		if (_hoveredKind & kBillboardPhotos) {
			_window->entryWasClicked(_hoveredEntry, point, kBillboardWeather);
		} else {
			// Wait for a second click before acting on the first.
			_firstClickPoint = point;
			_firstClickBillboard = _hoveredBillboard;
			_firstClickEntry = _hoveredEntry;
			if (!_dragging)
				_firstClickTimer = LTimer::timerWithParams(300, this, "_timerOnFirstClickProcess", false, 0);
		}
	}
	_minimapTracking = false;
	_buttons = flags;
	_directDragging = false;
	updateCursor();
}

void AtlasPanelController::onMouseLeave(unsigned int, LPoint)
{
	mouseLost();
}

void AtlasPanelController::dragStarted(unsigned int, LPoint point)
{
	_zoomMoves = false;
	_dragging = true;
	startTrackingAtViewPoint(point);
}

void AtlasPanelController::dragFinished(unsigned int, LPoint point)
{
	endTrackingAtViewPoint(point);
	_minimapTracking = false;
	refreshView();
}

void AtlasPanelController::startTrackingAtViewPoint(LPoint)
{
	_lastDevelopTime = LTimer::currentTimeValue();
	_animFlags = (_animFlags & ~(kAnimSliding | kAnimInertia)) | kAnimTracking;
	_setupTimer();
}

// The rectangle from `start` to `point`, in normalized view coordinates
// (-1..1).
LRect AtlasPanelController::_selectionRectForPoint(LPoint point, LPoint start)
{
	LRect r = LZeroRect;
	LSize size = _window->acceleratedViewSize();
	float halfH = size.height * 0.5f;
	float halfW = size.width * 0.5f;
	float x = (start.x - halfW) / halfW;
	r.origin.x = x;
	r.size.width = (point.x - halfW) / halfW - x;
	float y = (start.y - halfH) / halfH;
	r.origin.y = y;
	r.size.height = (point.y - halfH) / halfH - y;
	return r;
}

void AtlasPanelController::doubleActionOnGeoPoint(LPoint)
{
	zoomToCurrentShape();
}

void AtlasPanelController::mouseLost()
{
	_mouseWasLost = false;
	releaseAndClear(_highlightedShapes);
	releaseAndClear(_shapesCaption);
	releaseAndClear(_regionString);
	releaseAndClear(_coordsString);
	if (_popupTimer)
		_popupTimer->invalidate();
	_popupTimer = 0;
	_window->hidePopupEntryInfo();
	releaseAndClear(_hoveredEntry);
	scene()->highlightEntry(scene()->objectsContainer()->currentEntry(), 0);
	refreshView();
}

// The keys held, as forces on the camera parameters.
void AtlasPanelController::_addKeyboardForces(TParamsHolder *forces)
{
	if (_keys & kKeyTiltUp)
		forces->tilt -= 1.0f;
	if (_keys & kKeyTiltDown)
		forces->tilt += 1.0f;
	if (_keys & kKeyUp)
		forces->y += 1.0f;
	if (_keys & kKeyDown)
		forces->y -= 1.0f;
	if (_keys & kKeyTurnLeft)
		forces->angle -= 1.0f;
	if (_keys & kKeyTurnRight)
		forces->angle += 1.0f;
	if (_keys & kKeySpinLeft) {
		forces->angle -= 1.0f;
		forces->spin -= 1.0f;
	}
	if (_keys & kKeySpinRight) {
		forces->angle += 1.0f;
		forces->spin += 1.0f;
	}
	if (_keys & kKeyZoomOut)
		forces->scale -= 1.0f;
	if (_keys & kKeyZoomIn)
		forces->scale += 1.0f;
	// The spin fades out as the camera tilts.
	float f = _params.tilt * -3.0f * _params.tilt + 1.1f;
	if (f < 0.0f)
		f = 0.0f;
	else if (f > 1.0f)
		f = 1.0f;
	forces->spin = f * forces->spin;
}

void AtlasPanelController::_directDragStarted()
{
	_dragTime = LTimer::currentTimeValue() - (double)0.05f;
}

void AtlasPanelController::hideNavDevice()
{
	if (_navVisibility >= 0.001f && _navDirection >= -0.5f) {
		double now = LTimer::currentTimeValue();
		_animFlags |= kAnimNavFade;
		_navDirection = -1.0f;
		_navStartVisibility = _navVisibility;
		_navFadeStart = now;
		_setupTimer();
		Preferences::setShowNavDeviceEnabled(false);
	}
}

void AtlasPanelController::showNavDevice()
{
	if (_navDirection <= 0.5f) {
		double now = LTimer::currentTimeValue();
		_animFlags |= kAnimNavFade;
		_navDirection = 1.0f;
		_navStartVisibility = _navVisibility;
		_navFadeStart = now;
		_setupTimer();
		Preferences::setShowNavDeviceEnabled(true);
	}
}

void AtlasPanelController::toggleNavDevice()
{
	if (_navVisibility > 0.001f && _navDirection > 0.0f)
		hideNavDevice();
	else
		showNavDevice();
}

void AtlasPanelController::_fadeToMinimapMode(TMinimapMode mode)
{
	_minimapTargetMode = mode;
	if (!(_animFlags & kAnimMinimapFade))
		_minimapFadeStart = LTimer::currentTimeValue();
	_animFlags |= kAnimMinimapFade;
	_setupTimer();
}

void AtlasPanelController::_hideFullMinimap()
{
	_fullMinimap = false;
	_fadeToMinimapMode(_minimapMode);
}

void AtlasPanelController::_showFullMinimap()
{
	_fullMinimap = true;
	_fadeToMinimapMode((TMinimapMode)2);
}

// Between the small minimap and none.
void AtlasPanelController::cycleMinimapMode()
{
	TMinimapMode mode = (TMinimapMode)(_minimapMode == 0);
	_fadeToMinimapMode(mode);
	_minimapMode = mode;
	Preferences::setMinimapMode(mode);
}

void AtlasPanelController::showInfo()
{
	if (_infoDirection <= 0.5f) {
		double now = LTimer::currentTimeValue();
		_animFlags |= kAnimInfoFade;
		_infoDirection = 1.0f;
		_infoStartVisibility = _infoVisibility;
		_infoFadeStart = now;
		_setupTimer();
		Preferences::setShowInfoDeviceEnabled(true);
	}
}

void AtlasPanelController::hideInfo()
{
	if (_infoVisibility >= 0.001f && _infoDirection >= -0.5f) {
		_minimapTracking = false;
		double now = LTimer::currentTimeValue();
		_animFlags |= kAnimInfoFade;
		_infoDirection = -1.0f;
		_infoStartVisibility = _infoVisibility;
		_infoFadeStart = now;
		_setupTimer();
		Preferences::setShowInfoDeviceEnabled(false);
	}
}

void AtlasPanelController::toggleInfo()
{
	if (_infoVisibility > 0.001f && _infoDirection > 0.0f)
		hideInfo();
	else
		showInfo();
}

void AtlasPanelController::showImagesPreview()
{
	if (_previewDirection <= 0.5f) {
		double now = LTimer::currentTimeValue();
		_animFlags |= kAnimPreviewFade;
		_previewDirection = 1.0f;
		_previewStartVisibility = _previewVisibility;
		_previewFadeStart = now;
		if (_sceneInstalled)
			_setupTimer();
	}
}

void AtlasPanelController::toggleImagesPreview()
{
	if (_previewVisibility > 0.001f && _previewDirection > 0.0f)
		hideImagesPreview();
	else
		showImagesPreview();
}

// The user defaults key of the camera of this kind of panel.
LString *AtlasPanelController::_cameraParamsKey()
{
	return LString::stringWithFormat("%s_camParams", getClass()->name());
}

intptr_t AtlasPanelController::_enabledBordersSetWasChanged(char *)
{
	releaseAndClear(_staticHilitedShapes);
	return 0;
}

intptr_t AtlasPanelController::_onUIButton(char *args)
{
	CSceneControl *button = LF_ARG(args, 0, CSceneControl *);
	switch (button->tag()) {
	case kButtonNav:
		toggleNavDevice();
		break;
	case kButtonInfo:
		toggleInfo();
		break;
	case kButtonMinimap:
		cycleMinimapMode();
		break;
	case kButtonFullScreen:
		_window->toogleFullScreen();
		break;
	case kButtonLockNormal:
		setLockMode(scene()->objectsContainer()->lockNormalButton()->value() ? kEarthLockNormal : kEarthLockNone);
		break;
	case kButtonLockTilted:
		setLockMode(scene()->objectsContainer()->lockTiltedButton()->value() ? kEarthLockTilted : kEarthLockNone);
		break;
	}
	return 0;
}

// The buttons stack up from the bottom right corner, above the full screen
// one when in full screen.
void AtlasPanelController::doInstallButtons()
{
	BaseScene *scene = this->scene();
	SharedContextObjectsContainer *objects = scene->objectsContainer();
	const LPoint corner = {1.0f, 1.0f};
	float base;
	if (!_window->isInFullScreenMode()) {
		base = 16.0f;
	} else {
		CSceneControl *button = scene->objectsContainer()->fullScreenButton();
		const LPoint off = {-24.0f, -16.0f};
		button->setNrmPosWithPtOff(corner, off);
		button->setTagTargetSelector(kButtonFullScreen, this, "_onUIButton");
		scene->addControl(button);
		base = 55.0f;
	}
	CSceneControl *button = objects->minimapButton();
	LPoint off = {-24.0f, -base};
	button->setNrmPosWithPtOff(corner, off);
	button->setTagTargetSelector(kButtonMinimap, this, "_onUIButton");
	scene->addControl(button);
	button = objects->infoButton();
	off.y = -(base + 33.0f);
	button->setNrmPosWithPtOff(corner, off);
	button->setTagTargetSelector(kButtonInfo, this, "_onUIButton");
	scene->addControl(button);
	button = objects->navButton();
	off.y = -(base + 33.0f + 33.0f);
	button->setNrmPosWithPtOff(corner, off);
	button->setTagTargetSelector(kButtonNav, this, "_onUIButton");
	scene->addControl(button);
}

// The map point under `point` in the minimap `minimap` (y up).
LPoint AtlasPanelController::positionFromMinimap(LPoint point, LRect minimap)
{
	float x = point.x >= minimap.origin.x ? point.x : minimap.origin.x;
	if (x > minimap.origin.x + minimap.size.width)
		x = minimap.origin.x + minimap.size.width;
	float y = point.y >= minimap.origin.y ? point.y : minimap.origin.y;
	if (y > minimap.origin.y + minimap.size.height)
		y = minimap.origin.y + minimap.size.height;
	LPoint p = {(x - minimap.origin.x) / minimap.size.width, 1.0f - (y - minimap.origin.y) / minimap.size.height};
	return p;
}

LPoint AtlasPanelController::minimapPositionFromPosition(LPoint position, LRect minimap)
{
	float y;
	if (position.y < 0.0f)
		y = 1.0f;
	else if (position.y <= 1.0f)
		y = 1.0f - position.y;
	else
		y = 0.0f;
	float x = position.x <= 1.0f ? position.x : 1.0f;
	if (!(position.x >= 0.0f))
		x = 0.0f;
	LPoint p = {minimap.size.width * x + minimap.origin.x, y * minimap.size.height + minimap.origin.y};
	return p;
}

// In navigation device units: 0..1 across, 0..1 from the bottom.
LPoint AtlasPanelController::navPositionFromScreenPosition(LPoint point, LRect nav)
{
	LPoint p = {(point.x - nav.origin.x) / nav.size.width, 1.0f - (point.y - nav.origin.y) / nav.size.height};
	return p;
}

// The compass angle (0..-1, clockwise from the top) of `point` around the
// compass ring.
float AtlasPanelController::_compassAngleFromScreenPos(LPoint point)
{
	LPoint p = navPositionFromScreenPosition(point, navDeviceRect());
	Vector2 d;
	d.x = p.x - kNavCompassCenterPoint.x;
	d.y = p.y - kNavCompassCenterPoint.y;
	d.x = d.x + d.x;	// the device is twice as wide as high
	FHVec2Normalize(&d, &d);
	double c;
	if (d.y > 1.0f)
		c = -1.0;
	else if (d.y < -1.0f)
		c = 1.0;
	else
		c = (double)-d.y;
	float angle = (float)acos(c);
	if (d.x < 0.0f)
		angle = kTwoPi - angle;
	return -(angle / kTwoPi);
}

void AtlasPanelController::mouseDragged(unsigned int, LPoint point)
{
	if (_pressedControl == 0) {
		LSize size = _window->acceleratedViewSize();
		if (!_minimapTracking) {
			trackToViewPoint(point);
			return;
		}
		LRect minimap = minimapMapRect();
		LPoint p = {point.x, size.height - point.y};
		if (minimap.origin.x <= p.x && minimap.origin.y <= p.y && p.x <= minimap.origin.x + minimap.size.width &&
		    p.y <= minimap.origin.y + minimap.size.height) {
			_minimapTracking = true;
			setPosition(positionFromMinimap(p, minimap));
			_window->refreshAcceleratedView();
		}
	} else if (_pressedControl == kNavCompass) {
		setCompassAngle(_compassAngleFromScreenPos(point));
	} else {
		TNavControlID control = controlIDFromScreenPosition(point);
		if (!_pressedInside) {
			if (control == _pressedControl) {
				controlDown(control);
				_pressedInside = true;
			}
		} else if (control != _pressedControl) {
			controlUp(_pressedControl);
			_pressedInside = false;
		}
	}
}

int AtlasPanelController::_mouseTrackBehaviour(LPoint *point)
{
	TMapMouseMode behaviour = Preferences::mouseBehaviourOnMap();
	if (_buttons & kMouseLeft) {
		if (point) {
			point->x -= _buttonStart[0].x;
			point->y -= _buttonStart[0].y;
		}
		if (_zoomOutKey)
			return kTrackZoomOut;
		if (_tiltKey)
			return kTrackTilt;
		if (behaviour == 0)
			return _zoomKey ? kTrackDirectDrag : kTrackZoomIn;
		if (behaviour >= 0 && behaviour < 3)
			return _zoomKey ? kTrackZoomIn : kTrackDirectDrag;
	}
	if (_buttons & kMouseRight) {
		if (point) {
			point->x -= _buttonStart[1].x;
			point->y -= _buttonStart[1].y;
		}
		return kTrackTilt;
	}
	if (_buttons & kMouseOther) {
		if (point) {
			point->x -= _buttonStart[2].x;
			point->y -= _buttonStart[2].y;
		}
		return kTrackMove;
	}
	return kTrackNone;
}

TMouseCursorType AtlasPanelController::_mouseCursorType()
{
	if (_window->isInCoordinatesPickerMode())
		return kCursorCrosshair;
	// What a left drag would do.
	unsigned int buttons = _buttons;
	_buttons = buttons | kMouseLeft;
	int behaviour = _mouseTrackBehaviour(0);
	_buttons = buttons;
	if (behaviour == kTrackZoomIn)
		return kCursorZoomIn;
	if (behaviour == kTrackZoomOut)
		return kCursorZoomOut;
	if (behaviour != kTrackDirectDrag)
		return kCursorArrow;
	if (Preferences::mouseBehaviourOnMap() == 2)
		return kCursorPlaceEntry;
	return _buttons == 0 ? kCursorOpenHand : kCursorClosedHand;
}

void AtlasPanelController::mouseDblClick(unsigned int, LPoint point)
{
	TNavControlID control = controlIDFromScreenPosition(point);
	_pressedControl = control;
	if (control == 0) {
		if (_firstClickTimer)
			_firstClickTimer->invalidate();
		_firstClickTimer = 0;
		LSize size = _window->acceleratedViewSize();
		LPoint nrm = {point.x / size.width, point.y / size.height};
		LPoint geoPoint;
		if (canConvertNormalizedViewPointToGeoPoint(nrm, &geoPoint))
			doubleActionOnGeoPoint(geoPoint);
	} else {
		controlDown(control);
		_pressedInside = true;
	}
}

void AtlasPanelController::trackToViewPoint(LPoint point)
{
	_trackPoint = point;
	int behaviour = _mouseTrackBehaviour(&_trackPoint);
	LSize size = _window->acceleratedViewSize();
	bool direct = false;
	if (behaviour == kTrackDirectDrag) {
		if (!_directDragging) {
			_dragStart = point;
			_dragLast = point;
			_directDragStarted();
		}
		double now = LTimer::currentTimeValue();
		float dt = (float)(now - _dragTime);
		if (dt < 0.05f)
			dt = 0.05f;
		_dragTime = now;
		LPoint diff = {(point.x - _dragStart.x) / size.height, (point.y - _dragStart.y) / size.height};
		_directDragWithDiff(diff);
		// The inertia the camera keeps when the button goes up.
		float vx = ((point.x - _dragLast.x) * -3.0f) / (dt * size.height);
		float vy = ((point.y - _dragLast.y) * -3.0f) / (dt * size.height);
		vx = vx < -1.0f ? -1.0f : (vx > 1.0f ? 1.0f : vx);
		vy = vy < -1.0f ? -1.0f : (vy > 1.0f ? 1.0f : vy);
		_velocity.x = _velocity.x * (1.0f - 0.6f) + vx * 0.6f;
		_velocity.y = _velocity.y * (1.0f - 0.6f) + vy * 0.6f;
		_dragLast = point;
		if (!(_animFlags & kAnimInertia)) {
			_animFlags |= kAnimInertia;
			_lastDevelopTime = now;
			_setupTimer();
		}
		direct = true;
	} else if (behaviour == kTrackZoomIn || behaviour == kTrackZoomOut) {
		LPoint start = {point.x - _trackPoint.x, point.y - _trackPoint.y};
		_selectionRect = _selectionRectForPoint(point, start);
		scene()->setSelectionRect(_selectionRect);
	}
	_directDragging = direct;
}

float AtlasPanelController::gridStepFromScale(unsigned char latitude)
{
	float f = _gridStepScaleFactor(latitude);
	float v = 40.0f / expf(f * _params.scale);
	if (v < 0.5f)
		return 0.5f;
	if (v < 1.0f)
		return 1.0f;
	if (v < 2.0f)
		return 2.0f;
	if (v < 5.0f)
		return 5.0f;
	if (v < 10.0f)
		return 10.0f;
	return 20.0f;
}

// The minimap, in view pixels from the bottom left: hidden below the view,
// small in the bottom left corner, or full across the view; the weights mix
// the three.
LRect AtlasPanelController::minimapTextureRect()
{
	LSize size = _window->acceleratedViewSize();
	float w = size.width;
	float h = size.height;
	float fullW, fullH, fullX, fullY;
	if (w / h < 2.0f) {
		fullW = w;
		fullH = w * 0.5f;
		fullY = fullH * -0.95f + h;
		fullX = w * 0.025f;
	} else {
		fullX = w * 0.5f * (-2.0f / (w / h) + 1.0f) + w * 0.025f;
		fullY = 0.0f;
		fullW = h + h;
		fullH = h;
	}
	LRect r = LZeroRect;
	r.origin.x = fullX * _minimapFullWeight;
	r.origin.y = h * _minimapHiddenWeight + (h - 128.0f) * _minimapSmallWeight + fullY * _minimapFullWeight;
	r.size.width = _minimapSmallWeight * 256.0f + fullW * 0.95f * _minimapFullWeight;
	r.size.height = _minimapSmallWeight * 128.0f + fullH * 0.95f * _minimapFullWeight;
	return r;
}

LRect AtlasPanelController::navDeviceRect()
{
	LSize size = _window->acceleratedViewSize();
	LRect r = {{size.width * 0.5f - 128.0f, 10.0f}, {256.0f, 128.0f}};
	return r;
}

LRect AtlasPanelController::infoDeviceRect(unsigned char full)
{
	_window->acceleratedViewSize();
	LRect r = {{12.0f, 12.0f}, {512.0f, full ? 256.0f : 128.0f}};
	return r;
}

LString *AtlasPanelController::currentCoordsString()
{
	if (_coordsString == 0) {
		Utils::TGeolocFormat format =
		    Preferences::screenSaverDisplayRegionNamesEnabled() ? Utils::kGeolocFormatSimple : Utils::kGeolocFormatFull;
		_coordsString = LRetain(Utils::shortGeoCoordinatesDescription(_mouseGeoPoint, format));
	}
	return _coordsString;
}

void AtlasPanelController::installButtonsIfNeeded()
{
	if (scene()->_renderOptions->showOverlays())
		doInstallButtons();
}

void AtlasPanelController::uninstallButtonsIfNeeded()
{
	if (scene()->_renderOptions->showOverlays())
		doUninstallButtons();
}

intptr_t AtlasPanelController::_imagesSelectionWasChanged(char *args)
{
	LMap *info = LF_ARG(args, 0, LNotification *)->userInfo();
	LObject *index = info ? info->objectForKey(kEntryInfoImageIndexKey) : 0;
	if (index == 0) {
		hideImagesPreview();
		return 0;
	}
	showImagesPreview();
	if (!_sceneInstalled)
		return 0;
	info = LF_ARG(args, 0, LNotification *)->userInfo();
	GenericEntry *entry = info ? lobj_cast<GenericEntry>(info->objectForKey(kEntryInfoGenericEntryKey)) : 0;
	int i = UMapParsingUtils::loadIntFromObject(index);
	ImagesPreviewController *preview = scene()->objectsContainer()->imagesPreviewController();
	scene()->objectsContainer()->setImagesEntry(entry);
	preview->scrollToImageAtIndex(i);
	return 0;
}

int AtlasPanelController::shapesLODFromScale()
{
	BaseScene *scene = this->scene();
	float f = 0.0f;
	if (scene)
		f = scene->_holder->fullFrameSize().height / 6000.0f - 0.15f;
	f = f + _params.scale;
	if (f < 0.1f)
		return 0;
	if (f < 0.3f)
		return 1;
	return (f >= 0.5f) + 2;
}

intptr_t AtlasPanelController::_delayedCurrentCitySelect(char *)
{
	GenericEntry *entry = scene()->objectsContainer()->currentEntry();
	if (entry && !_flying)
		slideToEntry(entry);
	_flyAfterSlide = false;
	_citySelectTimer = 0;
	return 0;
}

void AtlasPanelController::mouseDown(unsigned int flags, LPoint point)
{
	_terminateCinematics();
	if (_toggledControl)
		controlAction(LZeroPoint, (TNavControlID)kNavStop);
	LSize size = _window->acceleratedViewSize();
	if (_previewVisibility <= 0.001f) {
		TNavControlID control = controlIDFromScreenPosition(point);
		_pressedControl = control;
		if (control == 0) {
			if (flags & kMouseLeft)
				stopFlying();
			_dragging = false;
			LRect minimap = minimapMapRect();
			LPoint p = {point.x, size.height - point.y};
			if (minimap.origin.x <= p.x && minimap.origin.y <= p.y && p.x <= minimap.size.width + minimap.origin.x &&
			    p.y <= minimap.size.height + minimap.origin.y) {
				_minimapTracking = true;
				setPosition(positionFromMinimap(p, minimap));
				_window->refreshAcceleratedView();
			}
			unsigned int changed = flags ^ _buttons;
			_buttons = flags;
			if (changed & kMouseLeft)
				_buttonStart[0] = point;
			if (changed & kMouseRight)
				_buttonStart[1] = point;
			if (changed & kMouseOther)
				_buttonStart[2] = point;
			updateCursor();
			return;
		}
		if (control == kNavCompass)
			_compassGrab = _compassAngleFromScreenPos(point) - compassAngle();
		controlDown(_pressedControl);
		_pressedInside = true;
	} else {
		// The images preview strip across the middle of the view: a click
		// to the sides scrolls by up to four images.
		float y = (point.y / size.height + point.y / size.height) - 1.0f;
		if (y < -0.72f || y > 0.72f) {
			hideImagesPreview();
			return;
		}
		float x = point.x / size.width;
		float f = (size.width / size.height) * ((x + x) - 1.0f);
		float a = fabsf(f);
		if (a > 0.7f) {
			int step = 4;
			ImagesPreviewController *preview = scene()->objectsContainer()->imagesPreviewController();
			if (a <= 1.37f) {
				step = 3;
				if (a <= 1.18f)
					step = (a > 1.0f) + 1;
			}
			if (f < 0.0f)
				step = -step;
			preview->scrollToImageAtIndex(preview->currentSelection() + step);
		}
	}
}

void AtlasPanelController::_terminateCinematics()
{
	CinematicSequence *sequence = scene()->currentCinematicSequence();
	bool demoAllowed = _demoAllowed;
	_demoAllowed = false;
	if (sequence) {
		sequence->stop();
		LNotificationCenter::center()->postNotification(kCinematicStoppedNotification, this, 0);
	}
	scene()->setControlsVisibility(1);
	_demoAllowed = demoAllowed;
}

void AtlasPanelController::sceneWillBeInstalled()
{
	releaseAndClear(_highlightedShapes);
	releaseAndClear(_shapesCaption);
	releaseAndClear(_regionString);
	releaseAndClear(_coordsString);
	releaseAndClear(_staticHilitedShapes);
	releaseAndClear(_hoveredEntry);
	_hoveredBillboard = false;
	_hoveredExact = false;
	_hoveredKind = kBillboardNone;
	C3DView *view = _window->acceleratedView();
	if (view->context()) {
		view->context()->contextRenderTarget()->setFogColor(scene()->fogColor());
		scene()->highlightEntry(scene()->objectsContainer()->currentEntry(), 0);
		LNotificationCenter *center = LNotificationCenter::center();
		center->addObserver(this, "_playSoundsStateWasChanged", Preferences::kPlaySoundsStateWasChanged, 0);
		center->addObserver(this, "_worldTimeWasChanged", TimeController::kWorldTimeWasChanged, 0);
		_worldTimeWasChanged(0);
		center->addObserver(this, "_accelViewFrameDidChange", IWindowInterface::frameWasChangedNotification,
		                    _window->acceleratedView());
		updateCursor();
		center->addObserver(this, "_selectNextDemoSequence", CinematicSequence::kPlaybackFinished, 0);
	}
	_sceneInstalled = true;
	BaseScene *scene = this->scene();
	if (_capturer == 0)
		_capturer = LRetain(scene->detachCapturer());
	scene->_holder->setObjectForKey(scene->_renderOptions, RenderOptions::kRenderOptionsContextKey);
	RenderOptions::setCurrentOptions(scene->_renderOptions);
	installButtonsIfNeeded();
	this->scene()->objectsContainer()->setSceneForInternalObjects(this->scene());
	scene->hostControllerWasInstalled();
	_minimapMode = Preferences::minimapMode();
	_minimapTargetMode = _minimapMode;
	_minimapWeights = _minimapModeWeights[_minimapMode];
	_updateMinimapWeigths();
	if (Preferences::showInfoDeviceEnabled()) {
		_infoVisibility = 1.0f;
		_infoDirection = 1.0f;
	} else {
		_infoVisibility = 0.0f;
		_infoDirection = -1.0f;
	}
	if (Preferences::showNavDeviceEnabled()) {
		_navVisibility = 1.0f;
		_navDirection = 1.0f;
	} else {
		_navVisibility = 0.0f;
		_navDirection = -1.0f;
	}
}

// Slides the camera to `to`, taking the shorter way around the map. With
// `keepTarget` and a slide under way, the slide keeps its scale and tilt.
void AtlasPanelController::_slideWithParams(const TParamsHolder &to, unsigned char keepTarget)
{
	_terminateCinematics();
	_slideFrom = _params;
	float scale, tilt;
	if (!keepTarget || !(_animFlags & kAnimSliding)) {
		scale = to.scale;
		tilt = to.tilt;
	} else {
		scale = _slideTo.scale;
		tilt = _slideTo.tilt;
	}
	_slideTo.x = to.x;
	_slideTo.y = to.y;
	_slideTo.angle = to.angle;
	_slideTo.tilt = tilt;
	_slideTo.scale = scale;
	_slideTo.spin = 0.0f;
	_slideStart = LTimer::currentTimeValue();
	float dx = _slideTo.x - _slideFrom.x;
	if (dx > 0.55f)
		_slideFrom.x = _slideFrom.x + 1.0f;
	else if (dx < -0.55f)
		_slideFrom.x = _slideFrom.x - 1.0f;
	float distance = _interpolationDistance(&_slideFrom, &_slideTo);
	double d = pow((double)((_params.scale + 0.01f) * distance), 0.5);
	_animFlags |= kAnimSliding;
	_slideDuration = (float)(d * 5.0 + (double)0.3f);
	_setupTimer();
}

// An entry with a size is framed; a point keeps the scale.
void AtlasPanelController::slideToEntry(GenericEntry *entry)
{
	if (entry == 0)
		return;
	LRect rect;
	if (!entry->getEntryGeoRect(&rect))
		return;
	bool point = fabsf(rect.size.width) < 0.0001f && fabsf(rect.size.height) < 0.0001f;
	if (_toggledControl)
		controlAction(LZeroPoint, (TNavControlID)kNavStop);
	TParamsHolder to = _params;
	to.scale = point ? _params.scale : zoomScaleToFitGeoRect(rect);
	LPoint center = {rect.size.width * 0.5f + rect.origin.x, rect.size.height * 0.5f + rect.origin.y};
	LPoint mapPoint = mapPointFromGeoPoint(center);
	to.x = mapPoint.x;
	to.y = mapPoint.y;
	_slideWithParams(to, point);
	_flyAfterSlide = false;
}

// Zooms to the most detailed shape under the mouse.
void AtlasPanelController::zoomToCurrentShape()
{
	if (_highlightedShapes == 0)
		return;
	TLArray<LString *> *ids = CShapeManager::registeredShapeIDs();
	for (int i = ids->count() - 1; i >= 0; i--) {
		CShapeContainer *shape = (CShapeContainer *)_highlightedShapes->objectForKey(ids->objectAtIndex(i));
		if (shape) {
			zoomToGeoRect(shape->allPathsBoundsGeoRect(), 1);
			return;
		}
	}
}

// The navigation device's controls (TNavControlID 1..11 and the three
// spare ones), in device units; the compass ring around them is 12.
static const LRect sHotRects[14] = {
    {{0.47f, 0.19f}, {0.06f, 0.16f}}, {{0.47f, 0.49f}, {0.06f, 0.16f}}, {{0.37f, 0.37f}, {0.10f, 0.11f}},
    {{0.53f, 0.37f}, {0.10f, 0.11f}}, {{0.10f, 0.13f}, {0.09f, 0.28f}}, {{0.10f, 0.47f}, {0.09f, 0.28f}},
    {{0.21f, 0.13f}, {0.07f, 0.62f}}, {{0.72f, 0.13f}, {0.07f, 0.62f}}, {{0.82f, 0.17f}, {0.08f, 0.15f}},
    {{0.82f, 0.55f}, {0.08f, 0.15f}}, {{0.39f, 0.88f}, {0.22f, 0.15f}},
};

TNavControlID AtlasPanelController::navValueFromScreenPosition(LPoint point)
{
	LPoint p = navPositionFromScreenPosition(point, navDeviceRect());
	float dx = (p.x - kNavCompassCenterPoint.x) + (p.x - kNavCompassCenterPoint.x);
	float dy = p.y - kNavCompassCenterPoint.y;
	float d = sqrtf(dx * dx + dy * dy);
	if (d > 0.315f && d < 0.415f)
		return (TNavControlID)kNavCompass;
	for (int i = 0; i < 14; i++) {
		const LRect &r = sHotRects[i];
		if (r.origin.x <= p.x && r.origin.y <= p.y && p.x <= r.origin.x + r.size.width &&
		    p.y <= r.origin.y + r.size.height)
			return (TNavControlID)(i + 1);
	}
	return (TNavControlID)0;
}

TNavControlID AtlasPanelController::controlIDFromScreenPosition(LPoint point)
{
	if (_navVisibility > 0.95f) {
		TNavControlID control = navValueFromScreenPosition(point);
		if (control != 0)
			return control;
	}
	return (TNavControlID)0;
}

// The index (of the database's points) among `indices` nearest to `point`
// in the view; `inRadius` tells whether it is within `radius`.
int AtlasPanelController::_nearestPointIndexForScreenPositionWithRadius(LData *indices, LPoint point, float radius,
                                                                        unsigned char *inRadius, float *distance)
{
	const SAtlasPoint *points = (const SAtlasPoint *)AtlasDatabase::instance()->allPointsArray()->bytes();
	unsigned int length = indices->length();
	const int *ids = (const int *)indices->bytes();
	drawIterator *iterator = detachDrawIterator(0);
	int best = -1;
	float bestDistance = 0.0f;
	while (iterator->next()) {
		for (int i = (int)(length >> 2) - 1; i >= 0; i--) {
			int index = ids[i];
			LPoint viewPoint;
			if (!scene()->viewPointFromGeoPoint(&viewPoint, &points[index].geoPoint, iterator, 0))
				continue;
			float d = (point.x - viewPoint.x) * (point.x - viewPoint.x) +
			          (point.y - viewPoint.y) * (point.y - viewPoint.y);
			if (d <= radius * radius) {
				if (distance)
					*distance = sqrtf(d);
				if (inRadius)
					*inRadius = 1;
				delete iterator;
				return index;
			}
			if (best == -1 || d < bestDistance) {
				best = index;
				bestDistance = d;
			}
		}
	}
	if (distance)
		*distance = sqrtf(bestDistance);
	if (inRadius)
		*inRadius = 0;
	delete iterator;
	return best;
}

// The forces on the camera parameters (keys, navigation controls, mouse
// tracking), and the ranges the velocities are kept in.
static const float kForceMagnitudes[6] = {3.0f, 3.0f, 3.0f, 3.0f, 5.0f, 3.0f};

void AtlasPanelController::_evalParameterForces(TParamsHolder *forces, TParamsHolder *min, TParamsHolder *max)
{
	TParamsHolder f = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	_addKeyboardForces(&f);
	// The spin comes back to rest.
	if (f.spin == 0.0f) {
		if (_params.spin > 0.0f)
			f.spin -= 1.0f;
		else if (_params.spin < 0.0f)
			f.spin += 1.0f;
	}
	int control = _pressedInside ? _pressedControl : 0;
	switch (control) {
	case kNavTiltUp:
		f.tilt += 1.0f;
		break;
	case kNavTiltDown:
		f.tilt -= 1.0f;
		break;
	case kNavTurnLeft:
		f.angle += 1.0f;
		break;
	case kNavTurnRight:
		f.angle -= 1.0f;
		break;
	case kNavZoomIn:
		f.scale += 1.5f;
		break;
	case kNavZoomOut:
		f.scale -= 1.5f;
		break;
	}
	// A move with the navigation device goes at the automove speed.
	float speed = Preferences::automoveSpeedUIValue() * 0.1f + 0.01f;
	int move = _toggledControl ? (int)_toggledControl : control;
	switch (move) {
	case kNavMoveUp:
		f.y += 1.0f;
		break;
	case kNavMoveDown:
		f.y -= 1.0f;
		break;
	case kNavMoveLeft:
		f.x -= 1.0f;
		break;
	case kNavMoveRight:
		f.x += 1.0f;
		break;
	default:
		speed = 1.0f;
		break;
	}
	int behaviour = _mouseTrackBehaviour(0);
	LSize size = _window->acceleratedViewSize();
	if (behaviour == kTrackMove) {
		f.x = (_trackPoint.x * 3.0f) / size.height + f.x;
		f.y = (_trackPoint.y * 3.0f) / size.height + f.y;
	} else if (behaviour == kTrackTilt) {
		f.angle = _trackPoint.x / size.height + f.angle;
		f.tilt = _trackPoint.y / size.height + f.tilt;
	}
	TParamsHolder lo, hi;
	if (f.x > 0.0f) {
		hi.x = speed * f.x;
		lo.x = -10.0f;
	} else {
		lo.x = f.x < 0.0f ? speed * f.x : -10.0f;
		hi.x = 10.0f;
	}
	if (f.y > 0.0f) {
		hi.y = speed * f.y;
		lo.y = -10.0f;
	} else {
		lo.y = f.y < 0.0f ? speed * f.y : -10.0f;
		hi.y = 10.0f;
	}
	if (f.angle > 0.0f) {
		lo.angle = -10.0f;
		hi.angle = f.angle;
	} else {
		lo.angle = f.angle < 0.0f ? f.angle : -10.0f;
		hi.angle = 10.0f;
	}
	if (f.tilt > 0.0f) {
		lo.tilt = -10.0f;
		hi.tilt = f.tilt;
	} else {
		lo.tilt = f.tilt < 0.0f ? f.tilt : -10.0f;
		hi.tilt = 10.0f;
	}
	if (f.scale > 0.0f) {
		lo.scale = -10.0f;
		hi.scale = f.scale;
	} else {
		lo.scale = f.scale < 0.0f ? f.scale : -10.0f;
		hi.scale = 10.0f;
	}
	if (f.spin > 0.0f) {
		lo.spin = -10.0f;
		hi.spin = f.spin;
	} else {
		lo.spin = f.spin < 0.0f ? f.spin : -10.0f;
		hi.spin = 10.0f;
	}
	if (behaviour == kTrackMove) {
		f.x = (_trackPoint.x > 0.0f ? 0.2f : -0.2f) + f.x;
		f.y = (_trackPoint.y > 0.0f ? 0.2f : -0.2f) + f.y;
	} else if (behaviour == kTrackTilt) {
		f.angle = (_trackPoint.x > 0.0f ? 0.2f : -0.2f) + f.angle;
		f.tilt = (_trackPoint.y > 0.0f ? 0.2f : -0.2f) + f.tilt;
	}
	// Friction: a moving parameter is pulled back to rest, but not past it.
	if (_velocity.x != 0.0f) {
		if (_velocity.x > 0.0f) {
			lo.x = 0.0f;
			f.x -= 0.2f;
		} else {
			hi.x = 0.0f;
			f.x += 0.2f;
		}
	}
	if (_velocity.y != 0.0f) {
		if (_velocity.y > 0.0f) {
			lo.y = 0.0f;
			f.y -= 0.2f;
		} else {
			hi.y = 0.0f;
			f.y += 0.2f;
		}
	}
	if (_velocity.angle != 0.0f) {
		if (_velocity.angle > 0.0f) {
			lo.angle = 0.0f;
			f.angle -= 0.2f;
		} else {
			hi.angle = 0.0f;
			f.angle += 0.2f;
		}
	}
	if (_velocity.tilt != 0.0f) {
		if (_velocity.tilt > 0.0f) {
			lo.tilt = 0.0f;
			f.tilt -= 0.2f;
		} else {
			hi.tilt = 0.0f;
			f.tilt += 0.2f;
		}
	}
	if (_velocity.scale != 0.0f) {
		if (_velocity.scale > 0.0f) {
			lo.scale = 0.0f;
			f.scale -= 0.2f;
		} else {
			hi.scale = 0.0f;
			f.scale += 0.2f;
		}
	}
	if (_velocity.spin != 0.0f) {
		if (_velocity.spin > 0.0f) {
			lo.spin = 0.0f;
			f.spin -= 0.2f;
		} else {
			hi.spin = 0.0f;
			f.spin += 0.2f;
		}
	}
	f.x *= kForceMagnitudes[0];
	f.y *= kForceMagnitudes[1];
	f.angle *= kForceMagnitudes[2];
	f.tilt *= kForceMagnitudes[3];
	f.scale *= kForceMagnitudes[4];
	f.spin *= kForceMagnitudes[5];
	if (forces)
		*forces = f;
	if (min)
		*min = lo;
	if (max)
		*max = hi;
}

// The camera parameters as a 6 element array, in user defaults and
// snapshots.
static bool paramsFromArray(LArray *array, AtlasPanelController::TParamsHolder *params)
{
	if (array == 0 || array->count() != 6)
		return false;
	params->x = UMapParsingUtils::loadFloatFromObject(array->objectAtIndex(0));
	params->y = UMapParsingUtils::loadFloatFromObject(array->objectAtIndex(1));
	params->angle = UMapParsingUtils::loadFloatFromObject(array->objectAtIndex(2));
	params->tilt = UMapParsingUtils::loadFloatFromObject(array->objectAtIndex(3));
	params->scale = UMapParsingUtils::loadFloatFromObject(array->objectAtIndex(4));
	params->spin = UMapParsingUtils::loadFloatFromObject(array->objectAtIndex(5));
	return true;
}

void AtlasPanelController::installSnapshotInfo(LMap *info)
{
	if (info == 0)
		return;
	LValue *time = lobj_cast<LValue>(info->objectForKey(kSnapshotInfoTime));
	if (time) {
		LDate *date = LDate::dateWithTimeIntervalSinceReferenceDate(time->doubleValue());
		TimeController *controller = TimeController::instance();
		controller->setUseCustomTime(true);
		controller->setDate(date);
	}
	TParamsHolder params;
	if (paramsFromArray(lobj_cast<LArray>(info->objectForKey(_cameraParamsKey())), &params)) {
		_slideWithParams(params, 0);
		_flyAfterSlide = false;
	}
	LMap *defaults = lobj_cast<LMap>(info->objectForKey(kSnapshotInfoLUserDefaults));
	LUserDefaults *userDefaults = LUserDefaults::instance();
	LMapIterator it(defaults, true);
	while (it.next())
		userDefaults->setObjectForKey(it.object(), it.key());
}

// The camera the panel was left with, if the preferences keep it.
bool AtlasPanelController::getDefaultCamParams(TParamsHolder *params)
{
	if (!Preferences::retainLastView())
		return false;
	LObject *value = LUserDefaults::instance()->objectForKey(_cameraParamsKey());
	return paramsFromArray(lobj_cast<LArray>(value), params);
}

// Flies the route: slides to its start, then follows it (_onTimer).
void AtlasPanelController::startFlying()
{
	_animFlags &= ~(kAnimSliding | kAnimInertia);
	CRoutePointsContainer *route = scene()->objectsContainer()->routePointsContainer();
	route->startFlying();
	if (!route->flyStepWithDeltaTime((double)0.01f))
		return;
	LPoint start = route->currentFlyMapPoint();
	Vector2 direction = route->currentFlyDirection();
	float dx = direction.x < -1.0f ? -1.0f : (direction.x > 1.0f ? 1.0f : direction.x);
	float angle = (float)(asin((double)dx) / (double)kTwoPi);
	if (direction.y <= 0.0f)
		angle = 0.5f - angle;
	_params.x = _params.x < 0.0f ? 0.0f : (_params.x > 1.0f ? 1.0f : _params.x);
	_params.y = _params.y < 0.0f ? 0.0f : (_params.y > 1.0f ? 1.0f : _params.y);
	_params.angle = _params.angle < 0.0f ? 0.0f : (_params.angle > 1.0f ? 1.0f : _params.angle);
	_params.tilt = _params.tilt < -1.0f ? -1.0f : (_params.tilt > 1.0f ? 1.0f : _params.tilt);
	_params.scale = _params.scale < 0.0f ? 0.0f : (_params.scale > 1.0f ? 1.0f : _params.scale);
	_params.spin = _params.spin < -1.0f ? -1.0f : (_params.spin > 1.0f ? 1.0f : _params.spin);
	TParamsHolder to = {start.x, start.y, angle, -0.1f, 0.85f, 0.0f};
	_slideWithParams(to, 0);
	_flyAfterSlide = true;
	if (Preferences::isSoundsAvailable() && Preferences::playSounds()) {
		if (_flyingSound == 0)
			_flyingSound = LRetain(SoundManager::manager()->detachSoundBuffer(&kRouteFlyingSound)->detachSoundSource());
		_flyingSound->playSimple(0);
	}
}

// Back to the whole map, north up, flat.
void AtlasPanelController::resetView()
{
	_params.x = _params.x < -1.0f ? -1.0f : (_params.x > 1.0f ? 1.0f : _params.x);
	_params.y = _params.y < 0.0f ? 0.0f : (_params.y > 1.0f ? 1.0f : _params.y);
	_params.angle = _params.angle < 0.0f ? 0.0f : (_params.angle > 1.0f ? 1.0f : _params.angle);
	_params.tilt = _params.tilt < -1.0f ? -1.0f : (_params.tilt > 1.0f ? 1.0f : _params.tilt);
	_params.scale = _params.scale < 0.0f ? 0.0f : (_params.scale > 1.0f ? 1.0f : _params.scale);
	float spin = _params.spin;
	if (spin < -0.5f)
		spin = -0.5f;
	else if (spin > 0.5f)
		spin = 0.5f;
	_params.spin = spin;
	TParamsHolder to;
	to.x = _params.x;
	to.y = _params.y;
	to.scale = _params.scale;
	// The nearer of north up, the way round the compass is turned.
	if (_lockMode != kEarthLockTilted) {
		to.angle = _params.angle > 0.5f ? 0.999f : 0.0f;
	} else {
		to.angle = _params.angle + 0.06527778f > 0.5f ? 0.999f : 0.0f;
		to.angle = to.angle - 0.06527778f;
	}
	to.tilt = -0.999f;
	to.spin = 0.0f;
	_slideWithParams(to, 0);
	_flyAfterSlide = false;
}

// Zooms to `scale` around `geoPoint`; with `tilt`, the compass turns to
// the nearer of north and south.
void AtlasPanelController::_zoomWithParams(float scale, LPoint geoPoint, unsigned char tilt)
{
	_terminateCinematics();
	_zoomGeoPoint = geoPoint;
	_zoomStartGeoPoint = geoPointFromMapPoint(position());
	float x = _zoomGeoPoint.x;
	if (x - _zoomStartGeoPoint.x > 180.0f) {
		x = x - 360.0f;
		_zoomGeoPoint.x = x;
	}
	if (x - _zoomStartGeoPoint.x < -180.0f)
		_zoomStartGeoPoint.x = _zoomStartGeoPoint.x - 360.0f;
	_zoomStart = LTimer::currentTimeValue();
	_zoomStartScale = _params.scale;
	_animFlags |= kAnimZooming;
	_zoomScale = scale;
	_zoomMoves = true;
	float angle = compassAngle();
	_zoomStartAngle = angle;
	if (tilt)
		angle = angle > 0.5f ? 0.9999f : 0.0f;
	_zoomAngle = angle;
	_setupTimer();
}

void AtlasPanelController::zoomToGeoRect(LRect rect, unsigned char tilt)
{
	LPoint center = {rect.size.width * 0.5f + rect.origin.x, rect.size.height * 0.5f + rect.origin.y};
	rect.size.width = fabsf(rect.size.width);
	rect.size.height = fabsf(rect.size.height);
	float scale = zoomScaleToFitGeoRect(rect);
	scale = scale >= 0.0f ? (scale < 1.0f ? scale : 1.0f) : 0.0f;
	_zoomWithParams(scale, center, tilt);
}

// `rect` in normalized view coordinates (-1..1).
void AtlasPanelController::zoomToSelectionRect(LRect rect)
{
	LPoint nrm = {(rect.size.width * 0.5f + rect.origin.x) * 0.5f + 0.5f,
	              (rect.size.height * 0.5f + rect.origin.y) * 0.5f + 0.5f};
	LPoint geoPoint;
	if (!canConvertNormalizedViewPointToGeoPoint(nrm, &geoPoint))
		return;
	float w = fabsf(rect.size.width) * 0.5f;
	w = w >= 0.0f ? (w <= 1.0f ? w : 1.0f) : 0.0f;
	float h = fabsf(rect.size.height) * 0.5f;
	h = h >= 0.0f ? (h <= 1.0f ? h : 1.0f) : 0.0f;
	if (w <= h)
		w = h;
	_zoomWithParams(zoomScaleFromBaseScale(w), geoPoint, 0);
}

void AtlasPanelController::zoomToEntry(GenericEntry *entry, unsigned char tilt)
{
	if (entry == 0)
		return;
	LRect rect;
	if (!entry->getEntryGeoRect(&rect))
		return;
	float scale;
	if (fabsf(rect.size.width) < 0.0001f && fabsf(rect.size.height) < 0.0001f)
		scale = _params.scale;
	else
		scale = zoomScaleToFitGeoRect(rect);
	LPoint center = {rect.size.width * 0.5f + rect.origin.x, rect.size.height * 0.5f + rect.origin.y};
	_zoomWithParams(scale, center, tilt);
}

// A click on the map: picks the location in the coordinates picker, adds an
// entry in the "place entry" mouse mode, or zooms in or out.
void AtlasPanelController::singleActionOnGeoPoint(LPoint geoPoint)
{
	TMapMouseMode behaviour = Preferences::mouseBehaviourOnMap();
	if (_window->isInCoordinatesPickerMode()) {
		_window->locationWasPicked(geoPoint);
		_window->setAnimatedMarkerPosition(geoPoint);
		return;
	}
	if (behaviour != 2) {
		float scale;
		TMouseCursorType cursor = _mouseCursorType();
		if (cursor == kCursorZoomIn) {
			scale = _params.scale + 0.15f;
			if (scale > 1.0f)
				scale = 1.0f;
		} else if (cursor == kCursorZoomOut) {
			scale = _params.scale - 0.15f;
			if (scale < 0.0f)
				scale = 0.0f;
		} else {
			return;
		}
		_zoomWithParams(scale, geoPoint, 0);
		return;
	}
	GenericEntry *entry = _firstClickEntry;
	if (entry == 0)
		entry = UserCustomEntry::entryWithGeoPointAndName(geoPoint, 0);
	if (entry)
		_window->appendEntryToSet(entry);
	refreshView();
}

void AtlasPanelController::endTrackingAtViewPoint(LPoint)
{
	int behaviour = _mouseTrackBehaviour(0);
	if (behaviour == kTrackZoomIn) {
		if (!_minimapTracking)
			zoomToSelectionRect(_selectionRect);
	} else if (behaviour == kTrackZoomOut) {
		if (_params.scale > 0.0f && !_minimapTracking) {
			float scale = _params.scale - 0.15f;
			if (scale < 0.0f)
				scale = 0.0f;
			_zoomWithParams(scale, geoPointFromMapPoint(position()), 0);
		}
	}
	_selectionRect = LZeroRect;
	bool tracking = (_animFlags & kAnimTracking) != 0;
	if (tracking)
		_animFlags &= ~kAnimTracking;
	if (!(_animFlags & kAnimInertia)) {
		_animFlags |= kAnimInertia;
		_lastDevelopTime = LTimer::currentTimeValue();
		_setupTimer();
	} else if (tracking) {
		_setupTimer();
	}
	updateCursor();
}

void AtlasPanelController::mouseWheel(short delta, unsigned int, LPoint)
{
	if (_previewVisibility <= 0.5f) {
		float scale = (float)(int)delta * -0.0003f + _requestedScale;
		if (scale < 0.0f)
			scale = 0.0f;
		else if (scale > 1.0f)
			scale = 1.0f;
		_requestedScale = scale;
		_zoomWithParams(_requestedScale, geoPointFromMapPoint(position()), 0);
		return;
	}
	ImagesPreviewController *preview = scene()->objectsContainer()->imagesPreviewController();
	preview->scrollToImageAtIndex(preview->currentSelection() + ((((unsigned short)delta >> 14) & 2) - 1));
}

void AtlasPanelController::_moveWithDelta(LPoint delta)
{
	LPoint p = {_params.x + delta.x, _params.y + delta.y};
	setPosition(p);
	refreshView();
}

// The key codes the views pass to onKeyDown and onKeyUp.
enum {
	kKeyCodeFullMinimap = 1,
	kKeyCodeHidePreview = 3,
	kKeyCodeNavDevice = 4,
	kKeyCodeZoomIn = 5,
	kKeyCodeZoomOut = 6,
	kKeyCodeTurnLeft = 7,
	kKeyCodeTurnRight = 8,
	kKeyCodeTiltUp = 9,
	kKeyCodeTiltDown = 10,
	kKeyCodeSpinLeft = 0x2b,
	kKeyCodeSpinRight = 0x2e,
	kKeyCodeDown = 0x3d,
	kKeyCodeUp = 0x41,
	kKeyCodeTiltModifier = 0x1000,
	kKeyCodeZoomOutModifier = 0x1002,
	kKeyCodeZoomModifier = 0x1003,
};

// The _keys bit of a movement key, or 0.
static unsigned int keyBitForKeyCode(unsigned int key)
{
	switch (key) {
	case kKeyCodeUp:
		return kKeyUp;
	case kKeyCodeDown:
		return kKeyDown;
	case kKeyCodeTiltUp:
		return kKeyTiltUp;
	case kKeyCodeTiltDown:
		return kKeyTiltDown;
	case kKeyCodeTurnLeft:
		return kKeyTurnLeft;
	case kKeyCodeTurnRight:
		return kKeyTurnRight;
	case kKeyCodeSpinLeft:
		return kKeySpinLeft;
	case kKeyCodeSpinRight:
		return kKeySpinRight;
	case kKeyCodeZoomIn:
		return kKeyZoomIn;
	case kKeyCodeZoomOut:
		return kKeyZoomOut;
	}
	return 0;
}

bool AtlasPanelController::onKeyUp(unsigned int, unsigned int key)
{
	double now = LTimer::currentTimeValue();
	switch (key) {
	case kKeyCodeFullMinimap:
		_fullMinimap = false;
		_fadeToMinimapMode(_minimapMode);
		return false;
	case kKeyCodeNavDevice:
		hideNavDevice();
		return false;
	case kKeyCodeTiltModifier:
		_tiltKey = false;
		updateCursor();
		return false;
	case kKeyCodeZoomOutModifier:
		_zoomOutKey = false;
		updateCursor();
		return false;
	case kKeyCodeZoomModifier:
		_zoomKey = false;
		updateCursor();
		return false;
	}
	unsigned int bit = keyBitForKeyCode(key);
	if (bit == 0)
		return false;
	_keys &= ~bit;
	bool stopped = false;
	if ((_animFlags & kAnimKeyboard) && _keys == 0) {
		_animFlags &= ~kAnimKeyboard;
		stopped = true;
	}
	if (!(_animFlags & kAnimInertia)) {
		_animFlags |= kAnimInertia;
		_lastDevelopTime = now;
	} else if (!stopped) {
		return false;
	}
	_setupTimer();
	return false;
}

AtlasPanelController::drawIterator *AtlasPanelController::detachDrawIterator(const LSize *)
{
	return new drawIteratorSimple;
}

AtlasPanelController::AtlasPanelController()
    : _window(0), _highlightedShapes(0), _staticHilitedShapes(0), _shapesCaption(0), _regionString(0),
      _coordsString(0), _unused2C(0), _requestedScale(0.0f), _lockMode(kEarthLockNone), _zoomScale(0.0f),
      _zoomStartScale(0.0f), _zoomStartAngle(0.0f), _zoomAngle(0.0f), _slideStart(0.0), _slideDuration(3.0f),
      _timer(0), _zoomStart(0.0), _animFlags(0), _zoomMoves(false), _flying(false), _lastFlyTime(0.0),
      _flyAfterSlide(false), _highlightedControl((TNavControlID)0), _pressedControl((TNavControlID)0),
      _toggledControl((TNavControlID)0), _pressedInside(false), _minimapMode((TMinimapMode)0),
      _minimapTargetMode((TMinimapMode)0), _minimapHiddenWeight(1.0f), _minimapSmallWeight(0.0f),
      _minimapFullWeight(0.0f), _minimapTracking(false), _fullMinimap(false), _minimapFadeStart(0.0),
      _navFadeStart(0.0), _navVisibility(0.0f), _navDirection(0.0f), _navStartVisibility(0.0f),
      _infoFadeStart(0.0), _infoVisibility(0.0f), _infoDirection(0.0f), _infoStartVisibility(0.0f),
      _previewFadeStart(0.0), _previewVisibility(0.0f), _previewDirection(0.0f), _previewStartVisibility(0.0f),
      _flyingPaused(false), _unused17C(0.0), _flyDistance(0.0), _hoveredEntry(0), _hoveredBillboard(false),
      _hoveredKind(kBillboardNone), _hoveredExact(false), _popupTimer(0), _mouseWasLost(false), _mouseInside(true),
      _citySelectTimer(0), _buttons(0), _directDragging(false), _dragTime(0.0), _keys(0), _zoomKey(false),
      _zoomOutKey(false), _tiltKey(false), _lastDevelopTime(0.0), _compassGrab(0.0f), _dragging(false),
      _firstClickEntry(0), _firstClickBillboard(false), _firstClickTimer(0), _flyingSound(0), _demoTimer(0),
      _demoAllowed(true), _introRunning(false), _sceneInstalled(false), _capturer(0), _captureAfterCountdown(false),
      _captionsDirty(true)
{
	const TParamsHolder zero = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	_params = zero;
	_params.y = 0.5f;
	_slideFrom = zero;
	_slideTo = zero;
	_velocity = zero;
	_zoomScale = _params.scale;
	_requestedScale = _params.scale;
	_zoomGeoPoint = _zoomStartGeoPoint = LZeroPoint;
	_selectionRect = LZeroRect;
	// hidden, small and full: (full, small, hidden) weights
	_minimapModeWeights[0].x = 0.0f;
	_minimapModeWeights[0].y = 0.0f;
	_minimapModeWeights[0].z = 0.0f;
	_minimapModeWeights[1].x = 0.0f;
	_minimapModeWeights[1].y = 1.0f;
	_minimapModeWeights[1].z = 0.0f;
	_minimapModeWeights[2].x = 1.0f;
	_minimapModeWeights[2].y = 0.0f;
	_minimapModeWeights[2].z = 0.0f;
	_minimapWeights = _minimapModeWeights[0];
	_buttonStart[0] = _buttonStart[1] = _buttonStart[2] = LZeroPoint;
	_trackPoint = LZeroPoint;
	_dragStart.x = _dragStart.y = -1.0f;
	_dragLast.x = _dragLast.y = -1.0f;
	_mouseGeoPoint = LZeroPoint;
	_firstClickPoint = LZeroPoint;
}

intptr_t AtlasPanelController::_onTimer(char *)
{
	bool changed = _flying && !_flyingPaused;
	unsigned int flags = _animFlags;
	if (flags != 0)
		changed = true;

	if (flags & kAnimZooming) {
		// The point under the zoom stays put on screen: the map moves
		// toward it as the scale grows exponentially.
		double t = (LTimer::currentTimeValue() - _zoomStart) / (double)0.35f;
		LPoint g;
		if (t < 1.0) {
			float s0 = _zoomStartScale;
			float s = (float)((double)s0 + (double)(_zoomScale - s0) * t);
			_params.scale = s;
			double e0 = exp((double)(s0 * 3.0f));
			double e1 = exp((double)(s * 3.0f));
			float k = (float)(e0 / e1);
			g.x = (float)((double)_zoomGeoPoint.x -
			              (double)((_zoomGeoPoint.x - _zoomStartGeoPoint.x) * k) * (1.0 - t));
			g.y = (float)((double)_zoomGeoPoint.y -
			              (double)(k * (_zoomGeoPoint.y - _zoomStartGeoPoint.y)) * (1.0 - t));
		} else {
			g.x = _zoomGeoPoint.x;
			_animFlags &= ~kAnimZooming;
			g.y = _zoomGeoPoint.y;
			_params.scale = _zoomScale;
			_requestedScale = _zoomScale;
			setCompassAngle(_zoomAngle);
			_setupTimer();
			_captionsDirty = true;
		}
		if (_zoomMoves)
			setPosition(mapPointFromGeoPoint(g));
		float scale = _params.scale;
		float requested = _requestedScale;
		_requestedScale = 0.0f;
		setScale(scale);
		setCompassAngle((float)((double)_zoomStartAngle * (1.0 - t) + (double)_zoomAngle * t));
		flags = _animFlags;
		_requestedScale = requested;
		changed = true;
	}

	if (flags & (kAnimKeyboard | kAnimTracking | kAnimControlDown)) {
		double now = LTimer::currentTimeValue();
		double last = _lastDevelopTime;
		_lastDevelopTime = now;
		double dt = now - last;
		if (dt > 0.1)
			dt = 0.1;
		_developParametersWithDeltaTime((float)dt);
		flags = _animFlags;
	}

	if ((flags & kAnimSliding) && !_flyingPaused) {
		double now = LTimer::currentTimeValue();
		float t = (float)((now - _slideStart) / (double)_slideDuration);
		if (t > 1.0f)
			t = 1.0f;
		TParamsHolder p = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
		_interpolateSlidingParams(&_slideFrom, &_slideTo, &p, t);
		LPoint position = {p.x, p.y};
		setPosition(position);
		setScale(p.scale);
		_adjustToSlidingProgress(t, &p);
		if (t >= 1.0f) {
			_animFlags &= ~kAnimSliding;
			recacheAll();
			_captionsDirty = true;
			if (_flyAfterSlide) {
				// At the route's start: fly it.
				double flyStart = LTimer::currentTimeValue();
				_flyDistance = 0.0;
				_flying = true;
				_lastFlyTime = flyStart;
			}
			_setupTimer();
		}
		flags = _animFlags;
		changed = true;
	}

	if (flags & kAnimInertia) {
		double now = LTimer::currentTimeValue();
		double last = _lastDevelopTime;
		_lastDevelopTime = now;
		float dt = (float)(now - last);
		if (dt > 0.1f)
			dt = 0.1f;
		_developParametersWithDeltaTime(dt);
		if (_velocity.x == 0.0f && _velocity.y == 0.0f && _velocity.angle == 0.0f && _velocity.tilt == 0.0f &&
		    _velocity.scale == 0.0f && _velocity.spin == 0.0f) {
			_animFlags &= ~kAnimInertia;
			_setupTimer();
			recacheAll();
			_captionsDirty = true;
		}
		changed = true;
	}

	if (_flying) {
		double now = LTimer::currentTimeValue();
		double dt = now - _lastFlyTime;
		if (dt > (double)0.1f)
			dt = (double)0.1f;
		float speed = Preferences::automoveSpeedUIValue();
		float attenuation = attenuationFactorFromScale(kAttenuationMove);
		_lastFlyTime = now;
		CRoutePointsContainer *route = scene()->objectsContainer()->routePointsContainer();
		if (!route->flyStepWithDeltaTime((double)(speed * attenuation) * dt))
			stopFlying();
		else
			setPosition(scene()->objectsContainer()->routePointsContainer()->currentFlyMapPoint());
		changed = true;
	}

	flags = _animFlags;
	if (flags & kAnimMinimapFade) {
		const Vector3 &target = _minimapModeWeights[_minimapTargetMode];
		Vector3 d;
		d.x = target.x - _minimapWeights.x;
		d.y = target.y - _minimapWeights.y;
		d.z = target.z - _minimapWeights.z;
		double now = LTimer::currentTimeValue();
		double step = (now - _minimapFadeStart) / (double)0.2f;
		_minimapFadeStart = now;
		if (step < (double)sqrtf(d.x * d.x + d.y * d.y + d.z * d.z)) {
			FHVec3Normalize(&d, &d);
			float s = (float)step;
			_minimapWeights.x = s * d.x + _minimapWeights.x;
			_minimapWeights.y = s * d.y + _minimapWeights.y;
			_minimapWeights.z = s * d.z + _minimapWeights.z;
		} else {
			_animFlags &= ~kAnimMinimapFade;
			_setupTimer();
			_minimapWeights = _minimapModeWeights[_minimapTargetMode];
		}
		_updateMinimapWeigths();
		flags = _animFlags;
		changed = true;
	}

	if (flags & kAnimNavFade) {
		double now = LTimer::currentTimeValue();
		float v = (float)((double)_navStartVisibility + (double)_navDirection * (now - _navFadeStart) * 4.0);
		_navVisibility = v;
		if ((v > 1.0f && _navDirection > 0.0f) || (v < 0.0f && _navDirection < 0.0f)) {
			_animFlags &= ~kAnimNavFade;
			_navVisibility = v >= 0.0f ? (v <= 1.0f ? v : 1.0f) : 0.0f;
			_setupTimer();
		}
		flags = _animFlags;
		changed = true;
	}

	if (flags & kAnimPreviewFade) {
		double now = LTimer::currentTimeValue();
		float v = (float)((double)_previewStartVisibility +
		                  (double)_previewDirection * (now - _previewFadeStart) * 4.0);
		_previewVisibility = v;
		if ((v > 1.0f && _previewDirection > 0.0f) || (v < 0.0f && _previewDirection < 0.0f)) {
			_animFlags &= ~kAnimPreviewFade;
			_previewVisibility = v >= 0.0f ? (v <= 1.0f ? v : 1.0f) : 0.0f;
			_setupTimer();
		}
		flags = _animFlags;
		changed = true;
	}

	if (flags & kAnimInfoFade) {
		double now = LTimer::currentTimeValue();
		float v = (float)((double)_infoStartVisibility + (double)_infoDirection * (now - _infoFadeStart) * 4.0);
		_infoVisibility = v;
		if ((v > 1.0f && _infoDirection > 0.0f) || (v < 0.0f && _infoDirection < 0.0f)) {
			_animFlags &= ~kAnimInfoFade;
			_infoVisibility = v >= 0.0f ? (v <= 1.0f ? v : 1.0f) : 0.0f;
			_setupTimer();
		}
	} else if (!changed) {
		return 0;
	}
	refreshCameraSettings();
	return 0;
}

// Moves the camera on by `dt` seconds of the forces: the velocities follow
// the forces within their ranges, and the parameters the velocities.
void AtlasPanelController::_developParametersWithDeltaTime(float dt)
{
	TParamsHolder forces = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	TParamsHolder min = forces;
	TParamsHolder max = forces;
	_evalParameterForces(&forces, &min, &max);
	_velocity.angle = dt * forces.angle + _velocity.angle;
	_velocity.tilt = dt * forces.tilt + _velocity.tilt;
	_velocity.scale = dt * forces.scale + _velocity.scale;
	_velocity.spin = dt * forces.spin + _velocity.spin;
	_velocity.y = dt * forces.y + _velocity.y;
	_velocity.x = dt * forces.x + _velocity.x;
	_velocity.clampSelf(&min, &max);
	float k = dt * 0.6f;
	float attenuation = attenuationFactorFromScale(kAttenuationMove);
	LPoint move = {attenuation * _velocity.x * k, attenuation * _velocity.y * k};
	_moveWithDelta(move);
	LPoint turn = {k * _velocity.angle * 0.7f, k * _velocity.tilt};
	_tiltWithDelta(turn);
	// Moving up or down a tilted view also changes the scale.
	float upDown = 0.0f;
	if (_keys & kKeyUp)
		upDown = 1.0f;
	if (_keys & kKeyDown)
		upDown = upDown - 1.0f;
	setScale(_velocity.scale * 0.1f * k + _params.scale + k * -0.4f * _params.tilt * upDown);
	// The spin slows down as it nears its limit.
	float spin = _params.spin;
	float damping = 1.0f;
	if (spin * _velocity.spin > 0.0f) {
		float a = spin < 0.0f ? -spin : spin;
		if (a / 0.1f > 0.3f) {
			damping = (a / 0.1f - 0.3f) / -0.7f + 1.0f;
			damping = damping * damping;
		}
	}
	float s = _velocity.spin * 0.5f * k * damping + spin;
	_params.spin = s;
	if (s < -0.1f)
		s = -0.1f;
	else if (s > 0.1f)
		s = 0.1f;
	_params.spin = s;
	if (spin * s < 0.0f && !(_keys & (kKeyTurnLeft | kKeyTurnRight))) {
		_params.spin = 0.0f;
		_velocity.spin = 0.0f;
	}
}

void AtlasPanelController::startIntro()
{
	_terminateCinematics();
	CinematicSequence *intro = (CinematicSequence *)LAutorelease(CinematicIntro::_alloc()->init());
	if (intro) {
		scene()->setControlsVisibility(0);
		_introRunning = true;
		intro->play(scene());
	}
}

// The shapes drawn with all their borders (the render options' "show all
// borders" and time zones), made once.
LArray *AtlasPanelController::allStaticHilitedShapes()
{
	StSharableLocker lock(&_shapesLock, true);
	RenderOptions *options = scene()->_renderOptions;
	if (!options->showAllBorders() && !options->showTimeZoneEnabled()) {
		releaseAndClear(_staticHilitedShapes);
		return 0;
	}
	if (_staticHilitedShapes == 0) {
		LSetIterator it(options->shapesIDsToDraw(), true);
		while (LObject *object = it.nextObject()) {
			LString *shapeID = lobj_cast<LString>(object);
			if (_staticHilitedShapes == 0)
				_staticHilitedShapes = CShapeManager::instance()->allShapesWithShapeID(shapeID);
			else
				_staticHilitedShapes->_addDataFromArray(CShapeManager::instance()->allShapesWithShapeID(shapeID));
		}
		LRetain(_staticHilitedShapes);
	}
	return _staticHilitedShapes;
}

// The shapes under the mouse, of the kinds the preferences highlight.
void AtlasPanelController::mousePositionChanged(LPoint geoPoint)
{
	_mouseWasLost = false;
	if (!Preferences::screenSaverDisplayRegionNamesEnabled()) {
		_mouseGeoPoint = geoPoint;
		releaseAndClear(_coordsString);
		releaseAndClear(_regionString);
	}
	bool changed = _infoVisibility > 0.01f;
	if (!Preferences::highlightBordersAnyway()) {
		if (changed)
			refreshView();
		return;
	}
	LSet *shapeIDs = Preferences::shapesIDsToHighlight();
	if (_highlightedShapes) {
		// Kinds no longer highlighted.
		LArray *gone = 0;
		LMapIterator it(_highlightedShapes, true);
		while (it.next()) {
			if (!shapeIDs->containsObject(it.key())) {
				if (gone == 0)
					gone = LArray::_alloc()->init();
				gone->addObject(it.key());
			}
		}
		if (gone) {
			int count = gone->count();
			for (int i = 0; i < count; i++)
				_highlightedShapes->removeObjectForKey((LComparable *)gone->objectAtIndex(i));
			gone->release();
			changed = true;
		}
	}
	LSetIterator it(shapeIDs, true);
	while (LObject *object = it.nextObject()) {
		LString *shapeID = lobj_cast<LString>(object);
		LString *uid = shapeID ? CShapeManager::instance()->objectUIDByHit(geoPoint, shapeID) : 0;
		LObject *shape = uid ? CShapeManager::instance()->shapeForObject(uid, shapeID) : 0;
		if (shape == 0 && (uid == 0 || _highlightedShapes)) {
			if (_highlightedShapes) {
				_highlightedShapes->removeObjectForKey(shapeID);
				changed = true;
			}
			continue;
		}
		if (_highlightedShapes == 0)
			_highlightedShapes = LMap::_alloc()->init();
		if (shape != _highlightedShapes->objectForKey(shapeID))
			changed = true;
		_highlightedShapes->setObjectForKey(shape, shapeID);
	}
	releaseAndClear(_shapesCaption);
	if (changed)
		refreshView();
}

LArray *AtlasPanelController::_cameraParamsVal()
{
	return LArray::arrayWithObjects(LValue::valueWithFloat(_params.x), LValue::valueWithFloat(_params.y),
	                                LValue::valueWithFloat(_params.angle), LValue::valueWithFloat(_params.tilt),
	                                LValue::valueWithFloat(_params.scale), LValue::valueWithFloat(_params.spin), 0);
}

void AtlasPanelController::_storeCurrentCamParams()
{
	LUserDefaults::instance()->setObjectForKey(_cameraParamsVal(), _cameraParamsKey());
}

void AtlasPanelController::invalidate()
{
	_storeCurrentCamParams();
	if (_timer)
		_timer->invalidate();
	_timer = 0;
	if (_citySelectTimer)
		_citySelectTimer->invalidate();
	_citySelectTimer = 0;
	if (_firstClickTimer)
		_firstClickTimer->invalidate();
	_firstClickTimer = 0;
	if (_demoTimer)
		_demoTimer->invalidate();
	_demoTimer = 0;
	if (_popupTimer)
		_popupTimer->invalidate();
	_popupTimer = 0;
	release();
}

// The user defaults the preferences name, the time and the camera.
LMap *AtlasPanelController::collectSnapshotInfo()
{
	LMap *defaults = LMap::map();
	LUserDefaults *userDefaults = LUserDefaults::instance();
	LArray *keys = Preferences::snapshotInfoKeys();
	int count = keys ? keys->count() : 0;
	defaults->preserveCapacity(count);
	for (int i = 0; i < count; i++) {
		LComparable *key = (LComparable *)keys->objectAtIndex(i);
		LObject *value = userDefaults->objectForKey(key);
		if (value)
			defaults->setObjectForKey(value, key);
	}
	LValue *time = LValue::valueWithDouble(TimeController::instance()->date()->timeIntervalSinceReferenceDate());
	return LMap::mapWithObjectsAndKeys(defaults, kSnapshotInfoLUserDefaults, _cameraParamsVal(), _cameraParamsKey(),
	                                   time, kSnapshotInfoTime, 0);
}

// The point entry (capital or Apple store) under `point`, or with the
// "nearest point" preference the nearest one; `exact` tells whether one was
// under the point.
GenericEntry *AtlasPanelController::_entryNearestToGeoPointWithScreenPosition(LPoint geoPoint, LPoint point,
                                                                             unsigned char *exact)
{
	unsigned char inRadius = 0;
	float distance = 0.0f;
	float storeDistance;
	float radius = _params.scale * 9.0f + 6.0f;
	AtlasDatabase *database = AtlasDatabase::instance();
	bool nearest = Preferences::showNearestPoint();
	int index = -1;
	if (Preferences::showCapitalPoints())
		index = _nearestPointIndexForScreenPositionWithRadius(database->allCapitalsIndices(), point, radius,
		                                                      &inRadius, &distance);
	if (!inRadius && Preferences::showAppleStorePoints()) {
		int store = _nearestPointIndexForScreenPositionWithRadius(database->allAppleStoresIndices(), point, radius,
		                                                          &inRadius, &storeDistance);
		if (inRadius || index == -1 || (store >= 0 && storeDistance < distance)) {
			distance = storeDistance;
			index = store;
		}
	}
	if (exact)
		*exact = inRadius;
	if (nearest && !inRadius && Preferences::showOtherPoints())
		return database->entryNearestToGeoPoint(geoPoint);
	if ((nearest || inRadius) && index != -1) {
		const char *uids = (const char *)database->allPointUIDsArray()->bytes();
		GenericEntry *entry = GenericEntry::entryWithUID(LString::stringWithCString(uids + index * kAtlasPointUIDSize));
		return lobj_cast<PointEntry>(entry);
	}
	return 0;
}

// Highlights the control and the entry under the mouse, and shows the
// entry's popup after a while.
void AtlasPanelController::mouseMove(unsigned int, LPoint point)
{
	if (_animFlags & kAnimZooming)
		return;
	if (_firstClickTimer) {
		_firstClickProcess(point, _firstClickBillboard);
		return;
	}
	TNavControlID control = controlIDFromScreenPosition(point);
	if (control != _highlightedControl) {
		if (_highlightedControl)
			unhighlightControlWithID(_highlightedControl);
		_highlightedControl = control;
		if (control)
			highlightControlWithID(control);
	}
	LSize size = _window->acceleratedViewSize();
	LPoint flipped = {point.x, size.height - point.y};
	_hoveredBillboard = false;
	_hoveredKind = kBillboardNone;
	unsigned char exact = 0;
	bool weather = Preferences::showWeatherOnMapEnabled();
	bool images = Preferences::showImagesOnMapEnabled();
	unsigned int mask = weather ? kBillboardWeather : 0;
	if (images)
		mask |= kBillboardPhotos;
	GenericEntry *entry = 0;
	if (mask) {
		EntryBillboard *billboard = scene()->objectsContainer()->billboardsHitTest(flipped, scene());
		if (billboard) {
			_hoveredKind = (TBillboardKind)billboard->kindFromViewPoint(flipped, mask);
			entry = billboard->entry();
			if (entry)
				_hoveredBillboard = true;
		}
	}
	LPoint nrm = {point.x / size.width, point.y / size.height};
	LPoint geoPoint;
	if (!canConvertNormalizedViewPointToGeoPoint(nrm, &geoPoint)) {
		if (_mouseInside) {
			mouseLost();
			_mouseInside = false;
		}
	} else {
		mousePositionChanged(geoPoint);
		_mouseInside = true;
		if (entry == 0 && !(_animFlags & kAnimSliding)) {
			if (Preferences::showAllPoints())
				entry = _entryNearestToGeoPointWithScreenPosition(geoPoint, flipped, &exact);
			else if (Preferences::showNearestPoint())
				entry = AtlasDatabase::instance()->entryNearestToGeoPoint(geoPoint);
		}
	}
	if (entry != _hoveredEntry || _hoveredExact != (exact != 0)) {
		scene()->highlightEntry(entry, exact);
		_hoveredExact = exact;
		LRetain(entry);
		releaseAndClear(_hoveredEntry);
		_hoveredEntry = entry;
		refreshView();
		if (_popupTimer)
			_popupTimer->invalidate();
		_popupTimer = 0;
		if (!_hoveredExact) {
			if (_window->isPopupEntryInfoVisible())
				_window->hidePopupEntryInfo();
		} else if (!_window->isPopupEntryInfoVisible()) {
			_popupTimer = LTimer::timerWithParams(1000, this, "_timerOnShowPopupInfo", false, 0);
		} else {
			_showPopupInfoForEntry(_hoveredEntry);
		}
	}
}

LString *AtlasPanelController::currentCaption()
{
	return LString::stringWithCString("");
}

bool AtlasPanelController::onKeyDown(unsigned int, unsigned int key)
{
	_terminateCinematics();
	double now = LTimer::currentTimeValue();
	switch (key) {
	case kKeyCodeFullMinimap:
		_fullMinimap = true;
		_fadeToMinimapMode((TMinimapMode)2);
		return false;
	case kKeyCodeHidePreview:
		hideImagesPreview();
		return false;
	case kKeyCodeNavDevice:
		showNavDevice();
		return false;
	case kKeyCodeTiltModifier:
		_tiltKey = true;
		updateCursor();
		return false;
	case kKeyCodeZoomOutModifier:
		_zoomOutKey = true;
		updateCursor();
		return false;
	case kKeyCodeZoomModifier:
		_zoomKey = true;
		updateCursor();
		return false;
	}
	unsigned int bit = keyBitForKeyCode(key);
	if (bit == 0)
		return false;
	if (_previewVisibility <= 0.5f) {
		if (_keys == 0)
			_lastDevelopTime = now;
		_keys |= bit;
		if (!(_animFlags & kAnimKeyboard)) {
			_animFlags |= kAnimKeyboard;
			_setupTimer();
		}
	} else {
		// The images preview takes the arrows.
		int step;
		if (key == kKeyCodeTurnRight || key == kKeyCodeZoomIn)
			step = 1;
		else if (key == kKeyCodeZoomOut || key == kKeyCodeTurnLeft)
			step = -1;
		else
			return false;
		ImagesPreviewController *preview = scene()->objectsContainer()->imagesPreviewController();
		preview->scrollToImageAtIndex(preview->currentSelection() + step);
	}
	return false;
}

static LString *shapeName(LMap *shapes, LString *shapeID)
{
	CShapeContainer *shape = (CShapeContainer *)shapes->objectForKey(shapeID);
	if (shape == 0)
		return 0;
	GenericEntry *entry = GenericEntry::entryWithUID(shape->_objectUID);
	return entry ? entry->ownName(0) : 0;
}

// "County (State) (Country), Time zone" of the shapes under the mouse.
LString *AtlasPanelController::shapesCaption()
{
	if (_shapesCaption)
		return _shapesCaption;
	if (_highlightedShapes == 0 || _highlightedShapes->isEmpty())
		return _shapesCaption;
	LString *country = shapeName(_highlightedShapes, CShapeManager::kShapeIDCountries);
	LString *state = shapeName(_highlightedShapes, CShapeManager::kShapeIDStates);
	LString *county = shapeName(_highlightedShapes, CShapeManager::kShapeIDCounties);
	LString *timezone = shapeName(_highlightedShapes, CShapeManager::kShapeIDTimezones);
	if (county)
		_shapesCaption = LString::stringWithString(county);
	if (state) {
		if (_shapesCaption == 0)
			_shapesCaption = LString::stringWithString(state);
		else
			_shapesCaption->appendFormat(" (%@)", state);
	}
	if (country) {
		if (_shapesCaption == 0)
			_shapesCaption = LString::stringWithString(country);
		else
			_shapesCaption->appendFormat(" (%@)", country);
	}
	if (timezone) {
		if (_shapesCaption == 0)
			_shapesCaption = LString::stringWithString(timezone);
		else
			_shapesCaption->appendFormat(", %@", timezone);
	}
	return LRetain(_shapesCaption);
}

// "State, Country" under the mouse.
LString *AtlasPanelController::currentRegionString()
{
	if (_regionString)
		return _regionString;
	CShapeManager *shapes = CShapeManager::instance();
	LString *countryUID = shapes->objectUIDByHit(_mouseGeoPoint, CShapeManager::kShapeIDCountries);
	LString *stateUID = shapes->objectUIDByHit(_mouseGeoPoint, CShapeManager::kShapeIDStates);
	GenericEntry *country = countryUID ? GenericEntry::entryWithUID(countryUID) : 0;
	GenericEntry *state = stateUID ? GenericEntry::entryWithUID(stateUID) : 0;
	if (country == 0 && state == 0)
		return _regionString;
	LString *first, *second = 0;
	const char *format = "%@";
	if (state == 0) {
		first = country->ownName(0);
	} else {
		first = state->ownName(0);
		if (country) {
			second = country->ownName(0);
			if (second)
				format = "%@, %@";
		}
	}
	_regionString = LString::_alloc()->initWithFormat(format, first, second);
	return _regionString;
}

intptr_t AtlasPanelController::_onDBUpdateFinished(char *)
{
	StSharableLocker lock(&_shapesLock, true);
	releaseAndClear(_staticHilitedShapes);
	return 0;
}

// The saver's demo: the globe demos in turn, then
// kDemoSequenceWasEndedNotification.
intptr_t AtlasPanelController::_selectNextDemoSequence(char *)
{
	static int idx = -1;
	_demoTimer = 0;
	_terminateCinematics();
	if (_introRunning) {
		DownloadManager::instance()->resumeQueue();
		return 0;
	}
	if (!_demoAllowed)
		return 0;
	idx++;
	if (idx > 4) {
		idx = -1;
		LNotificationCenter::center()->postNotification(kDemoSequenceWasEndedNotification, this, 0);
		return 0;
	}
	CinematicSequence *sequence;
	switch (idx) {
	case 1:
		sequence = PerspSceneDemo2::_alloc()->init();
		break;
	case 2:
		sequence = PerspSceneDemo0::_alloc()->init();
		break;
	case 3:
		sequence = PerspSceneDemo1::_alloc()->init();
		break;
	case 4:
		sequence = PerspSceneDemo4::_alloc()->init();
		break;
	default:
		sequence = PerspSceneDemo3::_alloc()->init();
		break;
	}
	sequence = (CinematicSequence *)LAutorelease(sequence);
	if (sequence) {
		scene()->setControlsVisibility(0);
		sequence->play(scene());
	}
	return 0;
}

void AtlasPanelController::sceneWasUninstalled()
{
	scene()->objectsContainer()->setSceneForInternalObjects(0);
	releaseAndClear(_staticHilitedShapes);
	uninstallButtonsIfNeeded();
	_resetKBFlags();
	_sceneInstalled = false;
	LNotificationCenter *center = LNotificationCenter::center();
	center->removeObserver(this, Preferences::kPlaySoundsStateWasChanged, kLock);
	center->removeObserver(this, TimeController::kWorldTimeWasChanged, kLock);
	center->removeObserver(this, IWindowInterface::frameWasChangedNotification, kLock);
	center->removeObserver(this, CinematicSequence::kPlaybackFinished, kLock);
	LNotificationCenter::center()->postNotification(kRecordingMustBeStoppedNotification, this, 0);
	if (_capturer)
		_capturer->stopCapturing();
	if (CShapeManager::classObject()->hasSingleton())
		CShapeManager::instance()->invalAllDrawables();
	stopSounds();
	_animFlags &= ~kAnimScaleTimer;
	_setupTimer();
}

bool AtlasPanelController::resignFocus()
{
	_resetKBFlags();
	updateCursor();
	return true;
}

// Out of range values go to the nearer end (NaN to `min`).
void AtlasPanelController::TParamsHolder::clampSelf(const TParamsHolder *min, const TParamsHolder *max)
{
	x = min->x <= x ? (x <= max->x ? x : max->x) : min->x;
	y = min->y <= y ? (y <= max->y ? y : max->y) : min->y;
	angle = min->angle <= angle ? (angle <= max->angle ? angle : max->angle) : min->angle;
	tilt = min->tilt <= tilt ? (tilt <= max->tilt ? tilt : max->tilt) : min->tilt;
	scale = min->scale <= scale ? (scale <= max->scale ? scale : max->scale) : min->scale;
	spin = min->spin <= spin ? (spin <= max->spin ? spin : max->spin) : min->spin;
}
