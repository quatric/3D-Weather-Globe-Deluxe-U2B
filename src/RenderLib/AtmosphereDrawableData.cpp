// The atmosphere dome's parameters: the sky, or a cloud layer drifting with the wind.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(AtmosphereDrawableData, LF_CLASSOF(ModelDataWithMatrix))

AtmosphereDrawableData::AtmosphereDrawableData()
	: _atmosphereFactor(0.0f), _sunParam1(1.0f), _sunParam2(1.0f), _horizonAngle(0.0f), _horizonParam(0.0f),
	  _cloudsOffset(0.0f, 0.0f), _cloudsDrift(0.0f, 0.0f),
	  _sunColor(0.9f, 0.8f, 0.5f, 1.0f), _oppositeColor(0.8f, 0.75f, 0.7f, 1.0f),
	  _sunsetGlow(0.0f), _cloudsAlpha(0.0f), _mode(kModeSky), _visible(true), _lastTime(0.0)
{
}

void AtmosphereDrawableData::dealloc()
{
	CDrawableModelData::dealloc();
}

bool AtmosphereDrawableData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	if (stage->_objectID == 1)
		*on = _mode == kModeSky;
	else if (stage->_objectID == 2)
		*on = _mode == kModeClouds;
	else
		return CShaderStageStateProvider::_isStageOn(stage, on);
	return true;
}

void AtmosphereDrawableData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	ModelDataWithMatrix::storeSceneStateInStream(stream, flags);
	storeMatrixInStream(&_rotation, stream);
	storeVectorInStream(&_cloudsOffset, stream);
	storeFloatInStream(_atmosphereFactor, stream);
	storeFloatInStream(_horizonAngle, stream);
	storeFloatInStream(_horizonParam, stream);
	storeFloatInStream(_sunParam1, stream);
	storeFloatInStream(_sunParam2, stream);
	storeColorInStream(&_sunColor, stream);
	storeColorInStream(&_oppositeColor, stream);
	storeFloatInStream(_sunsetGlow, stream);
	storeFloatInStream(_cloudsAlpha, stream);
	storeIntInStream(_mode, stream);
	storeIntInStream(_visible, stream);
}

const void *AtmosphereDrawableData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	int i = 0;
	data = ModelDataWithMatrix::restoreSceneStateFromData(data, flags);
	data = loadMatrixFromStream(data, &_rotation);
	data = loadVectorFromStream(data, &_cloudsOffset);
	data = loadFloatFromStream(data, &_atmosphereFactor);
	data = loadFloatFromStream(data, &_horizonAngle);
	data = loadFloatFromStream(data, &_horizonParam);
	data = loadFloatFromStream(data, &_sunParam1);
	data = loadFloatFromStream(data, &_sunParam2);
	data = loadColorFromStream(data, &_sunColor);
	data = loadColorFromStream(data, &_oppositeColor);
	data = loadFloatFromStream(data, &_sunsetGlow);
	data = loadFloatFromStream(data, &_cloudsAlpha);
	data = loadIntFromStream(data, &i);
	_mode = i;
	data = loadIntFromStream(data, &i);
	_visible = i != 0;
	return data;
}

void AtmosphereDrawableData::adjustWithNatureParams(const NatureParams *params, unsigned char next,
                                                    unsigned char high)
{
	_atmosphereFactor = params->_atmosphereFactor;
	float e = 1.0f - params->_eclipseFactor;
	_sunParam1 = e * clamp01((params->_sunDot * 8.0f + 2.2f) * e);
	_sunParam2 = e * clamp01((params->_sunDot * 10.0f + 1.4f) * e);
	float h = clamp01(1.15f - 0.1f * fabsf(params->_sunElevation - 0.04f));
	_horizonParam = h * h;
	_sunColor = params->_sunDiskColor;
	_oppositeColor = params->_sunDiskOuterColor;
	_sunsetGlow = params->_sunsetGlow;

	double now = LTimer::currentTimeValue();
	if (_mode != kModeSky) {
		float dt = (float)(now - _lastTime);
		if (!(dt <= 0.2f))
			dt = 0.0f;
		if (high)
			dt *= 0.35f;
		_cloudsDrift.x -= dt * params->_weather.wind.x * 0.0015f;
		_cloudsDrift.y -= dt * params->_weather.wind.y * 0.0015f;
		if (!high) {
			C2DTexture *texture, *normals;
			if (!next) {
				texture = params->_skyTexture;
				normals = params->_skyNormals;
				_cloudsAlpha = params->_skyWeight;
			} else {
				texture = params->_nextSkyTexture;
				normals = params->_nextSkyNormals;
				_cloudsAlpha = params->_nextSkyWeight;
			}
			setTextureForSource(texture, 0);
			setTextureForSource(normals, 1);
			_visible = texture && _cloudsAlpha > 0.01f;
		} else {
			// The high layer shows the thin cloud skies only.
			int code;
			if (params->_nextSkyWeight <= 0.01f) {
				_cloudsAlpha = 1.0f;
				code = params->_skyCode;
			} else {
				_cloudsAlpha = params->_nextSkyWeight;
				code = params->_nextSkyCode;
			}
			_cloudsAlpha *= 0.8f;
			_visible = (unsigned)(code - kSkyBrokenCloudsLight) < 2 && _cloudsAlpha > 0.01f;
		}
	}
	_lastTime = now;
}

void AtmosphereDrawableData::adjustToCamera(const CContextCamera *camera)
{
	SphereLOD::SCameraParams p(camera->position());
	float f = camera->_far >= 0.0f ? camera->_far : 0.0f;

	// The dome's pole to the camera, turned with its longitude.
	Vector3 z(0.0f, 0.0f, 1.0f);
	Quater q1, q2, q;
	FHQuaternionRotationAxis(&q1, &z, p.longitude + 1.5707964f);
	Vector3 axis;
	FHVec3Cross(&axis, &z, &p.direction);
	FHVec3Normalize(&axis, &axis);
	FHQuaternionRotationAxis(&q2, &axis, acosf(z.dot(p.direction)));
	FHQuaternionMultiply(&q, &q1, &q2);
	FHMatrixRotationQuaternion(&_rotation, &q);
	_matrix = _rotation;
	FHMatrixTranspose(&_rotation, &_rotation);

	// Scaled to the far plane around the camera.
	Vector3 t = p.position * (1.0f - f);
	_matrix._11 *= f; _matrix._12 *= f; _matrix._13 *= f;
	_matrix._21 *= f; _matrix._22 *= f; _matrix._23 *= f;
	_matrix._31 *= f; _matrix._32 *= f; _matrix._33 *= f;
	_matrix._41 = t.x;
	_matrix._42 = t.y;
	_matrix._43 = t.z;

	_horizonAngle = (float)(acos(1.0 / (double)1.05f) + acos((double)(1.0f / p.distance)));

	// Where the view ray toward the sun leaves the atmosphere.
	const Vector3 *sun = TimeController::instance()->sunDirection();
	float c = sun->dot(p.direction);
	if (c > 1.0f)
		c = 1.0f;
	if (!(c >= -1.0f))
		c = -1.0f;
	float s = sqrtf(1.0f - c * c);
	float r = 1.05f / s;
	float d2 = r * r - p.distance * p.distance;
	if (!(d2 >= 0.0f))
		d2 = 0.0f;
	float k = s * sqrtf(d2) + c * p.distance;
	_sunDirIntersection = p.position - *sun * k;

	_cloudsOffset.x = (p.longitude * 8.0f) / 3.1415927f + _cloudsDrift.x;
	_cloudsOffset.y = (p.latitude * 8.0f) / 3.1415927f + _cloudsDrift.y;
	_cloudsOffset.x -= floorf(_cloudsOffset.x);
	_cloudsOffset.y -= floorf(_cloudsOffset.y);
	_cloudsDrift.x -= floorf(_cloudsDrift.x);
	_cloudsDrift.y -= floorf(_cloudsDrift.y);
}

const void *AtmosphereDrawableData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	float *v = (float *)buffer;
	switch (descr->id) {
	case 'AtRt':
		return &_rotation;
	case 'AtRu':
		v[0] = _mode != kModeSky ? _atmosphereFactor * _cloudsAlpha : _atmosphereFactor;
		v[1] = _sunParam1;
		v[2] = _sunParam2;
		v[3] = _horizonAngle;
		return buffer;
	case 'AtRv':
		v[0] = _horizonParam;
		v[1] = v[2] = v[3] = 0.0f;
		return buffer;
	case 'AtRw':
		FHVec3Transform((_SVector4 *)buffer, &_sunDirIntersection, &_rotation);
		return buffer;
	case 'AtRx':
		v[0] = v[1] = 1.3f;
		v[2] = _cloudsOffset.x;
		v[3] = _cloudsOffset.y;
		return buffer;
	case 'AtRy':
		return &_sunColor;
	case 'AtRz':
		return &_oppositeColor;
	case 'AtR{':
		v[0] = _sunsetGlow;
		v[1] = v[2] = v[3] = 0.0f;
		return buffer;
	}
	return CDrawableModelData::_dataPtrForConstant(descr, buffer);
}
