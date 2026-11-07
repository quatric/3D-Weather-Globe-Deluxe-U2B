// The screen saver's flight.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CinematicSaver, LF_CLASSOF(CinematicSequence))

static const float kPi = 3.1415927f;

static inline float randomUnit()
{
	return (float)rand() * 4.656613e-10f + 0.0f;
}

// Into 0..1.
static inline float wrapUnit(float f)
{
	if (f < 0.0f)
		return f + (float)(int)(-f + 1.0f);
	if (f > 1.0f)
		return f - (float)(int)f;
	return f;
}

CinematicSaver::CinematicSaver()
{
	_duration = 600.0f;
}

void CinematicSaver::_onPlaybackStarted()
{
	_orbitAngle = randomUnit();
	_phase = randomUnit();
	_spinDirection = 0.0f;
	_scale = 0.7f;
	_spin = randomUnit();
	_doSaverAnimation(0.0f, 1);
}

// Never ends: the progress stays away from the fades.
void CinematicSaver::_onPlayback(float dt)
{
	if (_progress > 0.7f)
		_progress = 0.3f;
	_doSaverAnimation(dt, 0);
}

void CinematicSaver::_doSaverAnimation(float dt, unsigned char reset)
{
	AtlasPanelController *controller = _scene->_controller;
	TScreenSaverPointOfViewMode pointOfView = Preferences::screenSaverPointOfViewMode();
	bool moved = reset || pointOfView != _pointOfView;
	float custom = Preferences::screenSaverCustomAltitudeValue();
	_pointOfView = pointOfView;
	bool globe = controller->getClass()->isSubclassOf(PerspPanelController::classObject());

	if (pointOfView != 0) {
		// Along the equator.
		controller->setScale((1.0f - custom) * 0.0f + custom * 0.4f);
		float speed = controller->attenuationFactorFromScale(AtlasPanelController::kAttenuationMove);
		_phase = wrapUnit(speed * dt * Preferences::automoveSpeedUIValue() * -0.006f + _phase);
		if (globe) {
			PerspPanelController *persp = static_cast<PerspPanelController *>(controller);
			float angle = Preferences::screenSaverTiltModeEnabled() ? 0.935f : 0.0f;
			persp->setVertAngle(-0.995f);
			persp->setHorizAngle(angle);
			persp->setTiltAngle(0.0f);
			LPoint position = {_phase, 0.5f};
			persp->setPosition(position);
		}
		return;
	}

	if (Preferences::screenSaverAltitudeMode() == 1)
		controller->setScale((1.0f - custom) * 0.6f + custom * 0.95f);
	else
		controller->setScale(_scale);
	float speed = dt * controller->attenuationFactorFromScale(AtlasPanelController::kAttenuationMove) *
	              Preferences::automoveSpeedUIValue();
	_orbitAngle = wrapUnit(speed * 0.00321f + _orbitAngle);
	_phase = wrapUnit(speed * 0.023f + _phase);

	// The camera rolls while high; low, it comes back level.
	if (_scale <= 0.775f) {
		if (_spinDirection == 0.0f)
			_spinDirection = randomUnit() > 0.5f ? 1.0f : -1.0f;
		_spin = dt * 0.004f * _spinDirection + _spin;
	} else {
		float spin = _spin;
		float step = spin >= 0.0f ? -0.015f : 0.015f;
		float next = dt * step * (((_scale - 0.6f) / 0.34999996f - 0.5f) * -2.0f + 1.05f) + spin;
		if (next > 0.5f)
			next -= 1.0f;
		_spinDirection = 0.0f;
		_spin = next * spin > 0.0f ? next : 0.0f;
	}
	if (_spin < -0.5f)
		_spin += (float)(int)((-0.5f - _spin) + 1.0f);
	else if (_spin > 0.5f)
		_spin -= (float)(int)(_spin + 0.5f);

	// The orbit: a great circle tilted by 50 degrees, turned around the
	// axis.
	Vector3 xAxis(1.0f, 0.0f, 0.0f);
	Vector3 zAxis(0.0f, 0.0f, 1.0f);
	Matrix tilt, turn, orbit;
	FHMatrixRotationAxis(&tilt, &xAxis, 0.87266463f);
	FHMatrixRotationAxis(&turn, &zAxis, (_orbitAngle + _orbitAngle) * kPi);
	FHMatrixMultiply(&orbit, &tilt, &turn);
	float a = _phase * kPi + _phase * kPi;
	Vector3 here(sinf(a), cosf(a), 0.0f);
	Vector3 next(sinf(a + 0.001f), cosf(a + 0.001f), 0.0f);
	_SVector4 p, q;
	FHVec3Transform(&p, &here, &orbit);
	FHVec3Transform(&q, &next, &orbit);
	Vector3 earthVec(p.x, p.y, p.z);
	Vector3 dir = earthVec - Vector3(q.x, q.y, q.z);
	FHVec3Normalize(&dir, &dir);

	// Up over land, down over the deep sea.
	LPoint mapPoint = mapPointFromEarthVec(&earthVec);
	int height = HeightsManager::quickHeightForMapPoint(mapPoint);
	float scale = _scale;
	if (height >= -9) {
		scale = _scale + (0.95f - _scale) * dt * 0.05f;
		_scale = scale;
	} else if (height < -50) {
		scale = _scale + (_scale - 0.6f) * dt * -0.02f;
		_scale = scale;
	}
	_scale = scale < 0.6f ? 0.6f : (scale <= 0.95f ? scale : 0.95f);

	if (globe) {
		PerspPanelController *persp = static_cast<PerspPanelController *>(controller);
		persp->setVertAngle(-0.2f);
		if (moved)
			persp->setPosition(mapPoint);
		else
			persp->setupViewParams(&earthVec, &dir, LZeroPoint);
		persp->setTiltAngle(_spin);
	}
}
