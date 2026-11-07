// The precipitations: particle systems from MAPS\Weather.map, drawn in a
// box of cells around the camera.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PrecipitationsModel, LF_CLASSOF(CParticleSystem))
LF_IMPLEMENT_CLASS(PrecipitationsModelData, LF_CLASSOF(CParticleSystemData))
LF_IMPLEMENT_CLASS(PrecipitationsSubsystem, LF_CLASSOF(CParticleSubsystem))
LF_IMPLEMENT_CLASS(FarRainModel, LF_CLASSOF(PrecipitationsModel))
LF_IMPLEMENT_CLASS(FarRainModelData, LF_CLASSOF(PrecipitationsModelData))
LF_IMPLEMENT_CLASS(RainDropsModel, LF_CLASSOF(PrecipitationsModel))
LF_IMPLEMENT_CLASS(RainDropsModelData, LF_CLASSOF(PrecipitationsModelData))

// 1/2^31: rand() scaled to 0..1.
static const float kRandScale = 4.656613e-10f;

// The fraction of v, in 0..1.
static inline float wrap01(float v)
{
	if (v >= 0.0f) {
		if (v > 1.0f)
			v = v - (float)(int)v;
	} else {
		v = v + (float)(int)((0.0f - v) + 1.0f);
	}
	return v;
}

// ---------------------------------------------------------------------------
// PrecipitationsModel

LClass *PrecipitationsModel::modelDataClass()
{
	return LF_CLASSOF(PrecipitationsModelData);
}

PrecipitationsModel *PrecipitationsModel::initWithParams(LMap *params, CHolder *holder)
{
	unsigned int count = UMapParsingUtils::loadIntFromObject(params->objectForKey(&sAmountOfParticles));
	CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc((LString *)params->objectForKey(&sShaderName), holder);
	if (profile == 0) {
		release();
		return 0;
	}
	return (PrecipitationsModel *)CParticleSystem::initWithParams(false, count, (TPartSysRepresentationMode)2,
	                                                              profile, 0, LF_CLASSOF(PrecipitationsSubsystem));
}

PrecipitationsModel *PrecipitationsModel::modelWithParams(LMap *params, CHolder *holder)
{
	if (holder)
		holder->beginAccumulationBlock();
	LString *className = lobj_cast<LString>(params->objectForKey(&sParticleModelClass));
	LClass *cls = LClass::classByName(className);
	if (cls == 0)
		cls = LF_CLASSOF(PrecipitationsModel);
	LObject *object = cls->alloc();
	PrecipitationsModel *model = 0;
	if (!object->getClass()->isSubclassOf(LF_CLASSOF(PrecipitationsModel)))
		object->release();
	else
		model = LAutorelease(((PrecipitationsModel *)object)->initWithParams(params, holder));
	if (holder)
		holder->endAccumulationBlock();
	return model;
}

LClass *FarRainModel::modelDataClass()
{
	return LF_CLASSOF(FarRainModelData);
}

LClass *RainDropsModel::modelDataClass()
{
	return LF_CLASSOF(RainDropsModelData);
}

// ---------------------------------------------------------------------------
// PrecipitationsModelData

PrecipitationsModelData::PrecipitationsModelData()
    : _phase(0.0f), _speedFactor(0.0f), _windOffsetFactor(0.0f), _sizeScale(1.0f), _wind(0.0f, 0.0f),
      _windOffset(0.0f, 0.0f), _alpha(1.0f), _cameraOffset(0.0f, 0.0f, 0.0f), _unused(0.0f),
      _boxSize(0.015f, 0.015f, 0.006f), _lastTime(-1.0)
{
}

const void *PrecipitationsModelData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	float *v = (float *)buffer;
	switch (descr->id) {
	case 'PrPT':
		v[0] = _phase;
		v[1] = v[2] = v[3] = 0.0f;
		return buffer;
	case 'PrCO':
		return &_cameraOffset;
	case 'PrDf':
		*(Color *)v = _color;
		v[3] *= _alpha;
		return buffer;
	case 'PrSM':
		v[0] = _sizeScale * 0.001f;
		v[1] = v[2] = v[3] = 0.0f;
		return buffer;
	case 'PrWD':
		v[0] = _wind.x;
		v[1] = _wind.y;
		v[2] = v[3] = 0.0f;
		return buffer;
	default:
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	}
}

void PrecipitationsModelData::postInitialize(LMap *params)
{
	CHolder *holder = _model->holder();
	CBaseTexture *texture = CBaseTexture::textureWithContentsOfFile((LString *)params->objectForKey(&sTextureName),
	                                                                holder, LF_CLASSOF(C2DTexture));
	setTextureForSource(texture, 0);
	_speedFactor = UMapParsingUtils::loadFloatFromObject(params->objectForKey(&sSpeedFactor));
	_windOffsetFactor = UMapParsingUtils::loadFloatFromObject(params->objectForKey(&sWindOffsetFactor));
	float minSize = UMapParsingUtils::loadFloatFromObject(params->objectForKey(&sParticleMinSize));
	float maxSize = UMapParsingUtils::loadFloatFromObject(params->objectForKey(&sParticleMaxSize));
	float minAlpha = UMapParsingUtils::loadFloatFromObject(params->objectForKey(&sParticleMinAlpha));
	float maxAlpha = UMapParsingUtils::loadFloatFromObject(params->objectForKey(&sParticleMaxAlpha));
	Vector3 rgb(0.0f, 0.0f, 0.0f);
	UMapParsingUtilsFH::loadVectorFromObject(&rgb, params->objectForKey(&sParticleColor));
	Color color(rgb.x, rgb.y, rgb.z, 1.0f);
	LObject *box = params->objectForKey(&sParticleBoxSize);
	if (box)
		UMapParsingUtilsFH::loadVectorFromObject(&_boxSize, box);

	// Random particles in the unit cell.
	char *p = (char *)_particles;
	unsigned int size = particleStructSize();
	int count = _count;
	for (int i = 0; i < count; i++) {
		SStoredParticle *particle = (SStoredParticle *)p;
		int r1 = rand(), r2 = rand(), r3 = rand();
		particle->position.z = (float)r1 * kRandScale + 0.0f;
		particle->position.y = (float)r2 * kRandScale + 0.0f;
		particle->position.x = (float)r3 * kRandScale + 0.0f;
		particle->size = (float)rand() * (maxSize - minSize) * kRandScale + minSize;
		color.a = (float)rand() * (maxAlpha - minAlpha) * kRandScale + minAlpha;
		particle->color = (unsigned int)color;
		_fillParticleStruct(i, particle);
		p += size;
	}
	_vbsDirty = true;
	_instancingDirty = true;
}

void PrecipitationsModelData::adjustWithParameters(SWeatherParameters *params)
{
	_phase = (float)((double)_speedFactor * params->time);
	_wind = params->wind;
	_color = params->color;
	_alpha = params->alpha;
	float dt;
	if (_lastTime < 0.0) {
		_lastTime = params->time;
		dt = 0.0f;
	} else {
		dt = (float)(params->time - _lastTime);
		_lastTime = params->time;
	}
	float count = (float)maxParticlesCount() * params->intensity;
	if (count <= 0.0f)
		count = 0.0f;
	setParticlesCount((unsigned int)count);

	// The particles move with the camera's geographic point and altitude.
	CContextCamera *camera = _model->context()->mainCamera();
	Vector3 position = *camera->position();
	Vector3 dir;
	FHVec3Normalize(&dir, &position);
	LPoint geoPoint = geoPointFromEarthVec(&dir);
	_cameraOffset.x = -((((geoPoint.x + 360.0f) * 3.1415927f) / 180.0f) * 80.0f);
	_cameraOffset.y = -((((geoPoint.y + 360.0f) * 3.1415927f) / 180.0f) * 80.0f);
	_cameraOffset.z =
	    -(sqrtf(position.x * position.x + position.y * position.y + position.z * position.z) / _boxSize.z);
	_cameraOffset.x = wrap01(_cameraOffset.x);
	_cameraOffset.y = wrap01(_cameraOffset.y);
	_cameraOffset.z = wrap01(_cameraOffset.z);
	// And with the wind.
	_windOffset.x = _windOffsetFactor * dt * _wind.x + _windOffset.x;
	_windOffset.y = dt * _wind.y * _windOffsetFactor + _windOffset.y;
	_windOffset.x = wrap01(_windOffset.x);
	_windOffset.y = wrap01(_windOffset.y);
	_cameraOffset.x += _windOffset.x;
	_cameraOffset.y += _windOffset.y;
}

void FarRainModelData::adjustWithParameters(SWeatherParameters *params)
{
	PrecipitationsModelData::adjustWithParameters(params);
	setParticlesCount(maxParticlesCount());
	_alpha = (params->intensity * 0.6f + 0.4f) * _alpha;
}

void RainDropsModelData::adjustWithParameters(SWeatherParameters *params)
{
	PrecipitationsModelData::adjustWithParameters(params);
	setParticlesCount(maxParticlesCount());
	_sizeScale = params->intensity * 0.4f + 0.6f;
	if (_baseSpeed < 0.0f)
		_baseSpeed = _speedFactor;
	_speedFactor = (params->intensity * 0.4f + 0.6f) * _baseSpeed;
}

void RainDropsModelData::_fillParticleStruct(unsigned int, SStoredParticle *particle)
{
	Color color(particle->color);
	int image = (int)((float)rand() * 8.0f * kRandScale + 0.0f);
	if (image > 7)
		image = 7;
	if (image < 0)
		image = 0;
	color.r = (float)image * 0.125f;
	particle->color = (unsigned int)color;
}

// ---------------------------------------------------------------------------
// PrecipitationsSubsystem

PrecipitationsSubsystem::PrecipitationsSubsystem() : _gridHalf(2), _cellOffset(0.0f, 0.0f, 0.0f, 1.0f)
{
}

const void *PrecipitationsSubsystem::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'PrFD')
		return CParticleSubsystem::_dataPtrForConstant(descr, buffer);
	return &_cellOffset;
}

void PrecipitationsSubsystem::drawSubset(CDrawableModelData *data, long subset, long index)
{
	static int frame_counter = 0;
	frame_counter++;
	PrecipitationsModelData *precips = (PrecipitationsModelData *)((CParticleSubsystemData *)data)->_systemData;
	const Vector3 &box = precips->_boxSize;
	Matrix scaling;
	FHMatrixScaling(&scaling, box.x, box.y, box.z);
	Vector3 eye = *context()->mainCamera()->position();
	Vector3 up(0.0f, 0.0f, 1.0f), at(0.0f, 0.0f, 0.0f);
	Matrix look;
	FHMatrixLookAtRH(&look, &eye, &at, &up);
	FHMatrixInverse(&look, 0, &look);
	Matrix m;
	FHMatrixMultiply(&m, &scaling, &look);
	// The cells' axes, facing the camera.
	Vector3 zAxis, xAxis, yAxis, north(0.0f, 0.0f, 1.0f);
	FHVec3Normalize(&zAxis, &eye);
	FHVec3Cross(&xAxis, &north, &zAxis);
	FHVec3Normalize(&xAxis, &xAxis);
	FHVec3Cross(&yAxis, &zAxis, &xAxis);
	xAxis *= box.x;
	yAxis *= box.y;
	zAxis *= box.z;
	Vector3 boxMin(0.0f, 0.0f, 0.0f), boxMax(1.0f, 1.0f, 1.0f);
	for (int i = -_gridHalf; i < _gridHalf; i++) {
		for (int j = -_gridHalf; j < _gridHalf; j++) {
			for (int k = -_gridHalf; k < _gridHalf; k++) {
				float fi = (float)i, fj = (float)j, fk = (float)k;
				m._41 = fi * xAxis.x + eye.x + fj * yAxis.x + fk * zAxis.x;
				m._42 = fi * xAxis.y + eye.y + fj * yAxis.y + fk * zAxis.y;
				m._43 = fi * xAxis.z + eye.z + fj * yAxis.z + fk * zAxis.z;
				_cellOffset = Vector4(fi, fj, fk, 0.5f);
				if (context()->boxIntersectionTest(&boxMin, &boxMax, &m, false)) {
					context()->setWorldTransform(&m);
					CRenderProfile *profile = renderProfile();
					profile->reactivateDrawStage(profile->_activeStage);
					CParticleSubsystem::drawSubset(data, subset, index);
				}
			}
		}
	}
	if (frame_counter > 20)
		frame_counter = 0;
}
