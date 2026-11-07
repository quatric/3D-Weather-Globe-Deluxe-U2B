// NatureParams: the light, the sky, the precipitations, the lightning and
// the sounds of the weather around the camera.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(NatureParams, 0)
LF_IMPLEMENT_CLASS(_PrecipsStateHolder, 0)

// 1/2^31: rand() scaled to 0..1.
static const float kRandScale = 4.656613e-10f;

// ---------------------------------------------------------------------------
// _PrecipsStateHolder

const void *_PrecipsStateHolder::restoreSceneStateFromData(const void *data, unsigned int)
{
	_params->_addPrecipsFromStreamedParams();
	return data;
}

void _PrecipsStateHolder::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	_params->drawPrecipitations(scene);
}

// ---------------------------------------------------------------------------
// NatureParams

NatureParams::NatureParams()
    : _holder(0), _lightingData(0), _lightingTextures(0), _lightingDir(1.0f, 0.0f, 0.0f),
      _lightingColor(0.0f, 0.0f, 0.0f, 0.0f), _dayFactor(1.0f), _atmosphereFactor(0.0f), _sunDot(0.0f),
      _envColor(1.0f, 1.0f, 1.0f, 1.0f), _glowColor(1.0f, 1.0f, 1.0f, 1.0f), _sunElevation(0.0f),
      _sunDiskColor(0.9f, 0.8f, 0.5f, 1.0f), _sunDiskOuterColor(0.8f, 0.75f, 0.7f, 1.0f),
      _sunHaloColor(0.9f, 0.8f, 0.5f, 0.02f), _sunHaloOuterColor(0.8f, 0.75f, 0.7f, 0.01f), _skyTexture(0),
      _nextSkyTexture(0), _skyNormals(0), _nextSkyNormals(0), _skyMaskPath(0), _nextSkyMaskPath(0),
      _unusedSky(0), _nextUnusedSky(0), _skyWeight(0.0f), _nextSkyWeight(0.0f), _skyCode(kSkyClean),
      _nextSkyCode(kSkyClean), _cloudiness(0.0f), _skyTexturesLoaded(false), _soundBuffers(0), _thunder(0),
      _thunderLow(0), _wind(0), _windVolume(0.0f), _soundsVolume(0.0f), _precipWeight(0.0f),
      _nextPrecipWeight(0.0f), _hasWeather(false), _timeMs(0.0f), _groundFactor(1.0f), _lowAltitudeFactor(0.0f),
      _sunsetLight(0.0f), _sunAboveHorizon(true), _sunVisibility(0.0f), _sunGlowVisibility(0.0f),
      _spaceGlowVisibility(0.0f), _eclipseFactor(0.0f), _eclipseDistance(1.0f), _sunHeight(1.0f),
      _horizonFactor(0.0f), _sunsetGlow(0.0f), _forceLighting(false), _lightingStart(-1.0),
      _lastLightingCheck(-1.0), _lightingTexture(0), _weatherMap(0), _precipsByName(0)
{
	memset(&_params, 0, sizeof(_params));
	for (int i = 0; i < 4; i++) {
		_precipSounds[i] = 0;
		_precipSoundNames[i] = 0;
	}
	_precipDatas = LArray::_alloc()->init();
	_precipsHolder = _PrecipsStateHolder::_alloc();
	_precipsHolder->_params = this;
}

NatureParams *NatureParams::initWithHolder(CHolder *holder)
{
	if (LObject::init() == 0)
		return 0;
	_holder = holder;
	CRenderProfile *profile =
	    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\LightingSprite.yaf"), holder);
	if (profile) {
		if (holder)
			holder->beginAccumulationBlock();
		SStdGeometry geometry;
		memset(&geometry, 0, sizeof(geometry));
		geometry.type = 3;
		geometry.cols = geometry.rows = 4;
		geometry.radius = 1.0f;
		geometry.height = 1.0f;
		LightingMesh *mesh = (LightingMesh *)LightingMesh::_alloc()->initWithStdGeometry(&geometry, profile);
		_lightingData = LRetain(lobj_cast<LightingSpriteData>(mesh->detachModelDataInstance()));
		mesh->release();
		for (int i = 0; i != 7; i++) {
			CBaseTexture *texture = CBaseTexture::textureWithContentsOfFile(
			    LString::stringWithFormat("TEXTURES\\Lighting%d.dds", i), holder, LF_CLASSOF(C2DTexture));
			if (!_lightingTextures)
				_lightingTextures = LArray::_alloc()->init();
			if (texture)
				_lightingTextures->addObject(texture);
		}
		if (_lightingData)
			_lightingData->_framesCount = 4;
		if (holder)
			holder->endAccumulationBlock();
	}
	_stateOwnerName = kStateObjNameNatureParams;
	if (_precipsHolder)
		_precipsHolder->_stateOwnerName = kObjNamePrecipitationsHolder;
	if (_lightingData)
		_lightingData->_stateOwnerName = kObjNameLightingSpriteData;
	return this;
}

void NatureParams::stopSounds()
{
	for (int i = 0; i < 4; i++) {
		releaseAndClear(_precipSounds[i]);
		releaseAndClear(_precipSoundNames[i]);
	}
	releaseAndClear(_thunder);
	releaseAndClear(_thunderLow);
	releaseAndClear(_soundBuffers);
	releaseAndClear(_wind);
	_windVolume = 0.0f;
}

void NatureParams::dealloc()
{
	stopSounds();
	releaseAndClear(_precipsHolder);
	releaseAndClear(_skyTexture);
	releaseAndClear(_nextSkyTexture);
	releaseAndClear(_skyNormals);
	releaseAndClear(_nextSkyNormals);
	releaseAndClear(_skyMaskPath);
	releaseAndClear(_nextSkyMaskPath);
	releaseAndClear(_unusedSky);
	releaseAndClear(_nextUnusedSky);
	releaseAndClear(_lightingData);
	releaseAndClear(_lightingTextures);
	releaseAndClear(_precipDatas);
	releaseAndClear(_weatherMap);
	releaseAndClear(_precipsByName);
	LObject::dealloc();
}

void NatureParams::registerDrawDatas(CScene *scene)
{
	scene->registerStateOwner(this);
	scene->registerStateOwner(_precipsHolder);
	scene->registerStateOwner(_lightingData);
}

void NatureParams::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeIntInStream(_skyCode, stream);
	storeIntInStream(_nextSkyCode, stream);
	storeIntInStream(_skyTexturesLoaded, stream);
	storeIntInStream(_hasWeather, stream);
	storeFloatInStream(_atmosphereFactor, stream);
	storeFloatInStream(_timeMs, stream);
	storeFloatInStream(_precipWeight, stream);
	storeFloatInStream(_nextPrecipWeight, stream);
	storeFloatInStream(_skyWeight, stream);
	storeFloatInStream(_nextSkyWeight, stream);
	storeFloatInStream(_weather.temperature, stream);
	storeFloatInStream(_weather.humidity, stream);
	storeFloatInStream(_weather.pressure, stream);
	storeVectorInStream(&_weather.wind, stream);
	storeFloatInStream(_weather.windSpeed, stream);
	storeIntInStream(_precip.code, stream);
	storeFloatInStream(_precip.intensity, stream);
	storeIntInStream(_nextPrecip.code, stream);
	storeFloatInStream(_nextPrecip.intensity, stream);
	storeColorInStream(&_params.sunDiffuse, stream);
	storeColorInStream(&_params.sunAmbient, stream);
	storeColorInStream(&_params.waterLight, stream);
	storeColorInStream(&_params.moonDiffuse, stream);
	storeColorInStream(&_params.moonAmbient, stream);
	storeColorInStream(&_params.fog, stream);
	storeColorInStream(&_params.env, stream);
	storeColorInStream(&_envColor, stream);
	storeIntInStream(_lightingTexture, stream);
}

const void *NatureParams::restoreSceneStateFromData(const void *data, unsigned int)
{
	int sky, next, loaded, i;
	data = loadIntFromStream(data, &sky);
	data = loadIntFromStream(data, &next);
	data = loadIntFromStream(data, &loaded);
	if (loaded)
		_loadSkyTextures((TSkyCode)sky, (TSkyCode)next);
	data = loadIntFromStream(data, &i);
	_hasWeather = i != 0;
	data = loadFloatFromStream(data, &_atmosphereFactor);
	data = loadFloatFromStream(data, &_timeMs);
	data = loadFloatFromStream(data, &_precipWeight);
	data = loadFloatFromStream(data, &_nextPrecipWeight);
	data = loadFloatFromStream(data, &_skyWeight);
	data = loadFloatFromStream(data, &_nextSkyWeight);
	data = loadFloatFromStream(data, &_weather.temperature);
	data = loadFloatFromStream(data, &_weather.humidity);
	data = loadFloatFromStream(data, &_weather.pressure);
	data = loadVectorFromStream(data, &_weather.wind);
	data = loadFloatFromStream(data, &_weather.windSpeed);
	data = loadIntFromStream(data, &i);
	_precip.code = (TPrecipitationCode)i;
	data = loadFloatFromStream(data, &_precip.intensity);
	data = loadIntFromStream(data, &i);
	_nextPrecip.code = (TPrecipitationCode)i;
	data = loadFloatFromStream(data, &_nextPrecip.intensity);
	data = loadColorFromStream(data, &_params.sunDiffuse);
	data = loadColorFromStream(data, &_params.sunAmbient);
	data = loadColorFromStream(data, &_params.waterLight);
	data = loadColorFromStream(data, &_params.moonDiffuse);
	data = loadColorFromStream(data, &_params.moonAmbient);
	data = loadColorFromStream(data, &_params.fog);
	data = loadColorFromStream(data, &_params.env);
	data = loadColorFromStream(data, &_envColor);
	data = loadIntFromStream(data, &_lightingTexture);
	CBaseTexture *texture = 0;
	if (_lightingTextures && (unsigned)_lightingTexture < _lightingTextures->count())
		texture = (CBaseTexture *)_lightingTextures->objectAtIndex(_lightingTexture);
	if (_lightingData)
		_lightingData->setTextureForSource(texture, 0);
	return data;
}

void NatureParams::drawLighting(CFrameDrawer *drawer)
{
	if (_lightingStart > 0.0 && _lightingData)
		drawer->renderDrawableData(_lightingData, 4);
}

void NatureParams::drawPrecipitations(CScene *scene)
{
	int count = _precipDatas ? _precipDatas->count() : 0;
	for (int i = 0; i < count; i++)
		scene->frameDrawer()->renderDrawableData((CDrawableModelData *)_precipDatas->objectAtIndex(i), 4);
	scene->storePropertyOwnerIfNeeded(_precipsHolder, 0);
}

// The eclipse factor grows as the moon covers the sun, seen from the camera.
void NatureParams::_evalSolarEclipseFactor(const Vector3 &sun, const Vector3 &moon, const Vector3 &camera)
{
	_SDVector3 toSun = {(double)sun.x - camera.x, (double)sun.y - camera.y, (double)sun.z - camera.z};
	_SDVector3 toMoon = {(double)moon.x - camera.x, (double)moon.y - camera.y, (double)moon.z - camera.z};
	FHDVec3Normalize(&toSun, &toSun);
	FHDVec3Normalize(&toMoon, &toMoon);
	double dot = toSun.x * toMoon.x + toSun.y * toMoon.y + toSun.z * toMoon.z;
	double c = dot < 0.0 ? 0.0 : (dot <= 1.0 ? dot : 1.0);
	// The angle between them, in units of 3 degrees.
	double distance = acos(c) * 180.0 / (double)3.14159265f / 3.0;
	_eclipseDistance = (float)distance;
	double f = 1.015 - distance;
	f = f >= 0.0 ? (f <= 1.0 ? f : 1.0) : 0.0;
	_eclipseFactor = (float)pow(f, 30.0);
	float d = _eclipseDistance * 0.2f;
	_eclipseDistance = d < 0.0f ? 0.0f : (d <= 1.0f ? d : 1.0f);
}

C2DTexture *NatureParams::_textureForSkyCode(TSkyCode code, unsigned char normals, CHolder *holder)
{
	LString *path = cloudsTextureFromCode(code, normals);
	if (path == 0)
		return 0;
	if (holder)
		holder->beginAccumulationBlock();
	C2DTexture *texture = C2DTexture::_alloc()->initWithHolderAndPath(holder, path);
	LAutorelease(texture);
	if (holder)
		holder->endAccumulationBlock();
	return texture;
}

void NatureParams::_loadSkyTextures(TSkyCode sky, TSkyCode next)
{
	if (_skyCode != sky) {
		releaseAndClear(_skyTexture);
		releaseAndClear(_skyNormals);
		releaseAndClear(_skyMaskPath);
		releaseAndClear(_unusedSky);
		// The sky changed to the next one: its textures are loaded.
		if (_nextSkyCode == sky) {
			_skyTexture = _nextSkyTexture;
			_skyNormals = _nextSkyNormals;
			_skyMaskPath = _nextSkyMaskPath;
			_unusedSky = _nextUnusedSky;
		} else {
			_skyTexture = _textureForSkyCode(sky, 0, _holder);
			_skyNormals = _textureForSkyCode(sky, 1, _holder);
			_skyMaskPath = cloudsTextureMaskPathFromCode(sky);
		}
		LRetain(_skyTexture);
		LRetain(_skyNormals);
		LRetain(_skyMaskPath);
		LRetain(_unusedSky);
		_skyCode = sky;
	}
	if (_nextSkyCode != next) {
		releaseAndClear(_nextSkyTexture);
		releaseAndClear(_nextSkyNormals);
		releaseAndClear(_nextSkyMaskPath);
		releaseAndClear(_nextUnusedSky);
		_nextSkyTexture = LRetain(_textureForSkyCode(next, 0, _holder));
		_nextSkyNormals = LRetain(_textureForSkyCode(next, 1, _holder));
		_nextSkyMaskPath = LRetain(cloudsTextureMaskPathFromCode(next));
		_nextSkyCode = next;
	}
}

void NatureParams::_evalSkyTextures(WeatherTracer *tracer)
{
	if (tracer == 0 || !tracer->weatherInfoIsAvailable()) {
		_skyCode = _nextSkyCode = kSkyClean;
		_skyWeight = _nextSkyWeight = 0.0f;
		releaseAndClear(_skyTexture);
		releaseAndClear(_skyNormals);
		releaseAndClear(_skyMaskPath);
		releaseAndClear(_unusedSky);
		releaseAndClear(_nextSkyTexture);
		releaseAndClear(_nextSkyNormals);
		releaseAndClear(_nextSkyMaskPath);
		releaseAndClear(_nextUnusedSky);
		_skyTexturesLoaded = false;
		_cloudiness = 0.0f;
	} else {
		TSkyCode sky, next;
		tracer->getSkyState(&sky, &_skyWeight, &next, &_nextSkyWeight);
		float visibility = sunFlaresVisibilityFromCode(sky);
		float nextVisibility = sunFlaresVisibilityFromCode(next);
		_cloudiness = 1.0f - (nextVisibility * _nextSkyWeight + visibility * _skyWeight);
		_loadSkyTextures(sky, next);
		_skyTexturesLoaded = true;
	}
}

// ---------------------------------------------------------------------------
// Sounds

SoundBuffer *NatureParams::_soundBuffer(LString *name)
{
	if (_soundBuffers == 0)
		_soundBuffers = LMap::_alloc()->init();
	SoundBuffer *buffer = (SoundBuffer *)_soundBuffers->objectForKey(name);
	if (buffer == 0) {
		buffer = SoundManager::manager()->detachSoundBuffer(name);
		if (buffer)
			_soundBuffers->setObjectForKey(buffer, name);
	}
	return buffer;
}

// Plays `name` in *source, looped and silent, unless it plays already.
void NatureParams::_genSoundSource(LString *name, LString **current, SoundSource **source)
{
	if (*current == 0 && name == 0)
		return;
	if (name && *current && name->isEqualTo(*current))
		return;
	if (source && *source) {
		(*source)->stopPlaying();
		releaseAndClear(*source);
	}
	if (name == 0 || _soundsVolume <= 0.01f) {
		releaseAndClear(*current);
		return;
	}
	SoundBuffer *buffer = _soundBuffer(name);
	LAssign(*current, name);
	if (source && buffer) {
		*source = LRetain(buffer->detachSoundSource());
		if (*source) {
			(*source)->setVolume(0.0f);
			(*source)->playSimple(true);
		}
	}
}

void NatureParams::_adjustSoundsToCurrPrecips()
{
	LString *day = precipSoundFromCode(_precip.code, true);
	LString *night = precipSoundFromCode(_precip.code, false);
	LString *nextDay = precipSoundFromCode(_nextPrecip.code, true);
	LString *nextNight = precipSoundFromCode(_nextPrecip.code, false);
	float volume = _soundsVolume;
	_genSoundSource(day, &_precipSoundNames[0], &_precipSounds[0]);
	if (_precipSounds[0])
		_precipSounds[0]->setVolume(volume * _precipWeight * _dayFactor);
	_genSoundSource(night, &_precipSoundNames[1], &_precipSounds[1]);
	if (_precipSounds[1])
		_precipSounds[1]->setVolume(volume * _precipWeight * (1.0f - _dayFactor));
	_genSoundSource(nextDay, &_precipSoundNames[2], &_precipSounds[2]);
	if (_precipSounds[2])
		_precipSounds[2]->setVolume(volume * _nextPrecipWeight * _dayFactor);
	_genSoundSource(nextNight, &_precipSoundNames[3], &_precipSounds[3]);
	if (_precipSounds[3])
		_precipSounds[3]->setVolume(volume * _nextPrecipWeight * (1.0f - _dayFactor));

	if (volume * _windVolume > 0.01f && _wind == 0) {
		if (_soundsVolume <= 0.01f)
			return;
		SoundBuffer *buffer = _soundBuffer(&kWindSound);
		_wind = buffer ? LRetain(buffer->detachSoundSource()) : 0;
		if (_wind == 0)
			return;
		_wind->setPitch(1.2f);
		_wind->playSimple(true);
	}
	if (_wind)
		_wind->setVolume(_soundsVolume * _windVolume);
}

// ---------------------------------------------------------------------------
// Lightning

float NatureParams::_developLighting(double time, const Vector3 &cameraDir, const LPoint &geoPoint, float farClip)
{
	if (_lastLightingCheck < 0.0)
		_lastLightingCheck = time - 0.1;
	float probability = _params.lightingProbability;
	// A new lightning comes `probability` times a second on average.
	if (_lightingStart < 0.0 && probability > 1e-06f &&
	    ((float)rand() * (15.0f / (probability * (float)(time - _lastLightingCheck))) * kRandScale + 0.0f < 1.0f ||
	     _forceLighting)) {
		_lightingStart = time;
		int r1 = rand(), r2 = rand();
		Vector2 offset(((float)r2 + (float)r2) * kRandScale - 1.0f, ((float)r1 + (float)r1) * kRandScale - 1.0f);
		FHVec2Normalize(&offset, &offset);
		float strength = (float)rand() * 0.6f * kRandScale + 0.4f;
		int texture = (int)((float)rand() * 7.0f * kRandScale + 0.0f);
		if (texture > 7)
			texture = 7;
		if (texture < 0)
			texture = 0;
		_lightingTexture = texture;
		CBaseTexture *tex = 0;
		if (_lightingTextures && (unsigned)texture < _lightingTextures->count())
			tex = (CBaseTexture *)_lightingTextures->objectAtIndex(texture);
		bool large = _lightingTexture > 3;
		if (_lightingData) {
			_lightingData->setTextureForSource(tex, 0);
			_lightingData->_large = large;
		}
		LPoint p = {geoPoint.x + offset.x, geoPoint.y + offset.y};
		Vector3 point = earthVecFromGeoPoint(p);
		_lightingDir = point * 1.001f - cameraDir;
		FHVec3Normalize(&_lightingDir, &_lightingDir);
		float distance = farClip * 0.95f;
		if (distance >= 0.055f)
			distance = 0.055f;
		Vector3 dir = point - cameraDir;
		FHVec3Normalize(&dir, &dir);
		Vector3 position = dir * distance * strength + cameraDir;
		FHVec3Normalize(&position, &position);
		if (large)
			position *= 1.01f;
		if (_lightingData) {
			// A sprite facing the camera, stretched.
			Matrix m;
			FHMatrixLookAtRH(&m, &position, &cameraDir, &position);
			float det = 0.0f;
			FHMatrixInverse(&_lightingData->_matrix, &det, &m);
			if (large)
				FHMatrixScaling(&m, 0.05f, 0.004f, 1.0f);
			else
				FHMatrixScaling(&m, 0.003f, 0.012f, 1.0f);
			FHMatrixMultiply(&_lightingData->_matrix, &m, &_lightingData->_matrix);
		}
		if (_soundsVolume > 0.01f && Preferences::isSoundsAvailable() && Preferences::playSounds()) {
			if (_thunder == 0 || _thunderLow == 0) {
				SoundBuffer *buffer = _soundBuffer(&kThunderSound);
				LAssign(_thunder, buffer ? buffer->detachSoundSource() : 0);
				buffer = _soundBuffer(&kThunderSoundLow);
				LAssign(_thunderLow, buffer ? buffer->detachSoundSource() : 0);
			}
			// The weak ones are near.
			SoundSource *sound = strength <= 0.6f ? _thunder : _thunderLow;
			if (sound) {
				sound->stopPlaying();
				sound->setPitch((float)rand() * 0.49999994f * kRandScale + 0.8f);
				double volume = (double)((1.02f - strength) / 0.62f);
				sound->setVolume((float)((double)_soundsVolume * volume * volume));
				LLog("lVolume is %.02f\n", volume);
				int a = rand(), b = rand();
				Vector3 at(((float)b + (float)b) * kRandScale - 1.0f, ((float)a + (float)a) * kRandScale - 1.0f, 0.0f);
				sound->setPosition(&at);
				sound->playSimple(false);
			}
		}
		LNotificationCenter::center()->postNotification(kLightingWasStarted, this, 0);
	}
	_forceLighting = false;
	float brightness = 0.0f;
	if (_lightingStart > 0.0) {
		float t = (float)(time - _lightingStart) / 0.8f;
		if (t <= 1.0f) {
			// A flash, then a second one.
			double flash = pow((double)(1.0f - t), 30.0);
			float b = 0.2f - t;
			b = b <= -b ? b + 1.0f : 1.0f - b;
			double second = pow((double)b, 20.0);
			brightness = (float)(flash * 0.699999988079071) + (float)(second * 3.0);
			_lightingColor = Color(brightness * 0.3f, brightness * 0.6f, brightness * 1.1f, 0.0f);
			if (_lightingData) {
				int frame = (int)(t * 50.0f);
				unsigned int f = frame < 5 ? (frame < 0 ? 0 : frame) : 4;
				unsigned int last = _lightingData->_framesCount - 1;
				_lightingData->_frame = f <= last ? f : last;
				float c = (float)((double)_atmosphereFactor * pow((double)(1.1f - t), 6.0));
				_lightingData->_color = Color(c * 0.6f, c * 0.8f, c, c);
			}
		} else {
			_lightingStart = -1.0;
			_lightingColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
			if (_lightingData) {
				_lightingData->_frame = 0;
				_lightingData->_color = Color(0.4f, 0.7f, 1.1f, 0.0f);
			}
		}
	}
	_lastLightingCheck = time;
	return brightness;
}

// ---------------------------------------------------------------------------
// Precipitations

LArray *NatureParams::_precipitationsByName(LString *name)
{
	if (_precipsByName) {
		LArray *models = (LArray *)_precipsByName->objectForKey(name);
		if (models)
			return models;
	}
	LArray *entries = 0;
	if (_weatherMap == 0)
		_weatherMap = LMap::_alloc()->initWithContentsOfFile(LString::stringWithCString("MAPS\\Weather.map"));
	if (_weatherMap) {
		LObject *entry = _weatherMap->objectForKey(name);
		if (entry) {
			// One entry or a list of them.
			entries = lobj_cast<LArray>(entry);
			if (entries == 0) {
				LMap *map = lobj_cast<LMap>(entry);
				if (map)
					entries = LArray::arrayWithObjects(map, (LObject *)0);
			}
		}
	}
	int count = entries ? entries->count() : 0;
	LArray *result = 0;
	for (int i = 0; i < count; i++) {
		LMap *params = lobj_cast<LMap>(entries->objectAtIndex(i));
		if (params == 0)
			continue;
		PrecipitationsModel *model = PrecipitationsModel::modelWithParams(params, _holder);
		if (model == 0)
			continue;
		PrecipitationsModelData *data = lobj_cast<PrecipitationsModelData>(model->detachModelDataInstance());
		if (data == 0)
			continue;
		data->postInitialize(params);
		if (result == 0)
			result = LArray::array();
		data->_storesState = false;
		result->addObject(data);
	}
	if (result) {
		if (_precipsByName == 0)
			_precipsByName = LMap::_alloc()->init();
		_precipsByName->setObjectForKey(result, name);
	}
	return result;
}

void NatureParams::_addPrecipDrawableDataWithParams(const SWeatherInfo *weather, const SPrecipitationDescr *precip,
                                                    float alpha, float time)
{
	LString *name = precipNameFromCode(precip->code);
	if (name == 0)
		return;
	LArray *datas = _precipitationsByName(name);
	int count = datas ? datas->count() : 0;
	for (int i = 0; i < count; i++) {
		PrecipitationsModelData *data = (PrecipitationsModelData *)datas->objectAtIndex(i);
		PrecipitationsModelData::SWeatherParameters params;
		params.color = _envColor;
		params.wind = weather->wind;
		params.time = time;
		params.alpha = alpha;
		params.intensity = precip->intensity;
		data->adjustWithParameters(&params);
		if (_precipDatas)
			_precipDatas->addObject(data);
	}
}

void NatureParams::_addPrecipDatas()
{
	if (_precip.code == _nextPrecip.code) {
		if (_precip.code != kPrecipNone)
			_addPrecipDrawableDataWithParams(&_weather, &_precip, _atmosphereFactor, _timeMs);
		return;
	}
	if (_precip.code != kPrecipNone)
		_addPrecipDrawableDataWithParams(&_weather, &_precip, _atmosphereFactor * _precipWeight, _timeMs);
	if (_nextPrecip.code != kPrecipNone)
		_addPrecipDrawableDataWithParams(&_weather, &_nextPrecip, _atmosphereFactor * _nextPrecipWeight, _timeMs);
}

void NatureParams::_addPrecipsFromStreamedParams()
{
	_precipDatas->removeAllObjects();
	if (_hasWeather)
		_addPrecipDatas();
}

// ---------------------------------------------------------------------------
// The adjustment

void NatureParams::adjustParameters(WeatherTracer *tracer, CContextCamera *camera, CinematicSequence *sequence)
{
	Vector3 position = *camera->position();
	Vector3 viewDir = *camera->viewDirection();
	Vector3 cameraDir;
	FHVec3Normalize(&cameraDir, &position);
	TimeController *timeController = TimeController::instance();
	Vector3 sunDir = *timeController->sunDirection();
	// The sun in Earth radii, from the moon's side.
	Vector3 sunPos = *timeController->sunPosition() * -23480.697f;
	Vector3 moonPos = *timeController->moonPosition() / 6371.1f;
	double now = LTimer::currentTimeValue();
	float farClip = camera->farClip();
	bool dayNight = Preferences::dayNightIsEnabled();
	_timeMs = (float)(floor(now * 0.001) * -1000.0 + now);
	float distance = sqrtf(position.x * position.x + position.y * position.y + position.z * position.z);
	_groundFactor = clamp01((distance - 1.0f) * 7.0f + 0.2f);
	_atmosphereFactor = Atmosphere::transformFactorFromDistance(distance);
	_sunDot = -sunDir.x * cameraDir.x + -sunDir.y * cameraDir.y + -sunDir.z * cameraDir.z;
	_dayFactor = clamp01(_sunDot * 6.0f + 0.1f);
	if (!dayNight)
		_dayFactor = 1.0f;
	LPoint geoPoint = geoPointFromEarthVec(&cameraDir);
	_lowAltitudeFactor = 1.0f - clamp01(((distance - 1.0f) - 0.015f) / 0.015f);

	// The sounds fade towards the poles and away from the ground.
	float volume = (geoPoint.y + 65.0f) / 145.0f;
	_soundsVolume = volume;
	if (volume > 0.5f)
		_soundsVolume = 1.0f - volume;
	_soundsVolume = clamp01(_soundsVolume * 20.0f);
	_soundsVolume = (Preferences::soundVolume() <= 0.05f ? 0.0f : 1.0f) * _soundsVolume * _atmosphereFactor;
	if (sequence)
		_soundsVolume *= sequence->_fade;

	_evalSolarEclipseFactor(sunPos, moonPos, position);
	_dayFactor = (1.0f - _eclipseFactor) * _dayFactor;
	_hasWeather = false;
	if (tracer && tracer->weatherInfoIsAvailable()) {
		_hasWeather = true;
		tracer->getPrecipitations(&_precip, &_precipWeight, &_nextPrecip, &_nextPrecipWeight);
		SNatureParams params, nextParams;
		_precip.intensity *= _atmosphereFactor;
		_nextPrecip.intensity *= _atmosphereFactor;
		TSkyCode sky, nextSky;
		float skyWeight, nextSkyWeight;
		tracer->getSkyState(&sky, &skyWeight, &nextSky, &nextSkyWeight);
		fillNatureParams(&params, sky, &_precip);
		fillNatureParams(&nextParams, nextSky, &_nextPrecip);
		SNatureParams::fillParams(&params, skyWeight, &nextParams, nextSkyWeight, _dayFactor, &_params);
	} else {
		SNatureParams::fillDefaultParams(_dayFactor, &_params);
		_soundsVolume = 0.0f;
	}
	// The eclipse darkens the sun.
	if (_atmosphereFactor > 0.01f && _eclipseFactor > 0.01f) {
		float f = 1.0f - _eclipseFactor * _atmosphereFactor * _eclipseFactor;
		_params.sunDiffuse = _params.sunDiffuse * f;
	}
	float lighting = _developLighting(now, cameraDir, geoPoint, farClip);
	lighting = lighting + lighting + 1.0f;
	_envColor = _params.env * lighting;
	_evalSkyTextures(tracer);

	// The sun's elevation over the camera's horizon.
	double c = _sunDot <= 1.0f ? (_sunDot >= -1.0f ? (double)-_sunDot : 1.0) : -1.0;
	double angle = acos(c);
	double horizon = asin((double)(-1.0f / distance));
	_sunElevation = ((float)(horizon + angle) * 180.0f) / 9.424778f;
	float h = _sunElevation <= 0.0f ? _sunElevation * 0.5f : _sunElevation * 0.3f;
	_sunHeight = h >= -1.0f ? (h <= 1.0f ? h : 1.0f) : -1.0f;
	_horizonFactor = 1.0f - _sunHeight * _sunHeight;
	_horizonFactor = (float)pow((double)_horizonFactor, 4.0);
	float e = _sunElevation;
	_sunVisibility = clamp01(e + 0.5f + e + 0.5f) * (1.0f - _eclipseFactor) * (1.0f - _cloudiness);
	_sunAboveHorizon = e > -0.8f;
	_sunGlowVisibility =
	    clamp01((e + 0.7f) * 1.5f) * (1.0f - _eclipseFactor) * (1.0f - _cloudiness) * _atmosphereFactor;
	_spaceGlowVisibility = clamp01((e + 0.25f) * 2.5f) * (1.0f - _eclipseFactor) * (1.0f - _atmosphereFactor);
	_sunsetLight = clamp01((e + 0.1f) * 2.5f) * (1.0f - _cloudiness);
	_glowColor = Color(1.0f, 1.0f, 1.0f, 1.0f);

	// The sun's colors redden towards the horizon.
	float height = _sunHeight;
	double horizonFactor;
	if (!dayNight) {
		_sunDiskOuterColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
		_sunDiskColor = _sunDiskOuterColor;
		_sunsetGlow = 0.0f;
		_sunHaloOuterColor = Color(0.8f, 0.85f, 0.9f, 0.015f);
		_sunHaloColor = _sunHaloOuterColor;
		horizonFactor = _horizonFactor;
	} else {
		float a = 1.0f - (height + 0.1f) * (height + 0.1f);
		a = a * a;
		float b = (float)pow((double)(1.0f - (height - 0.1f) * (height - 0.1f)), 8.0);
		float up = (height + 1.0f) * 0.5f;
		float down = (height + 1.0f) * -0.5f + 1.0f;
		float blue = down * 0.35f + up;
		float green = up + down * 0.3f;
		float red = down * 0.25f + up;
		float hz = _horizonFactor;
		float nhz = 1.0f - hz;
		_sunDiskColor.a = (up + down) * nhz + hz;
		_sunDiskColor.b = hz * 0.5f + blue * nhz;
		_sunDiskColor.g = hz * 0.8f + nhz * green;
		float na = 1.0f - a;
		_sunDiskColor.a = na * _sunDiskColor.a + a;
		_sunDiskColor.b = a * 0.2f + na * _sunDiskColor.b;
		_sunDiskColor.g = a * 0.25f + na * _sunDiskColor.g;
		_sunDiskColor.r = (hz * 0.9f + red * nhz) * na + a * 0.3f;
		float nb = 1.0f - b;
		_sunDiskOuterColor.a = (up + down) * nb + b;
		_sunDiskOuterColor.b = b * 0.35f + blue * nb;
		_sunDiskOuterColor.g = b * 0.4f + green * nb;
		_sunDiskOuterColor.r = b * 0.45f + red * nb;
		horizonFactor = _horizonFactor;
		_sunsetGlow = (float)((double)(1.5f - a) * pow(horizonFactor, (double)0.6f));
		float k = up * 0.8f;
		_sunHaloColor.a = a * 0.015f + (up * 0.015f + down * 0.15f) * na;
		_sunHaloColor.b = (down * 0.3f + k) * na + a * 0.3f;
		_sunHaloColor.g = a * 0.8f + (down * 0.5f + k) * na;
		_sunHaloColor.r = a * 0.7f + na * (k + down * 0.8f);
		_sunHaloOuterColor = Color(0.7f, 0.75f, 0.8f, 0.01f);
	}
	_sunsetGlow = (1.0f - _cloudiness) * _sunsetGlow;

	// The fog and the sunlight redden looking at a low sun.
	double toSun = (double)((viewDir.x * -sunDir.x + viewDir.y * -sunDir.y + viewDir.z * -sunDir.z) * 0.5f + 0.5f);
	float f = (float)(toSun * toSun * (double)_atmosphereFactor * horizonFactor);
	float red = height * -0.2f + 0.8f;
	float nf = 1.0f - f;
	_params.fog.a = f + nf * _params.fog.a;
	_params.fog.b = f * 0.15f + nf * _params.fog.b;
	_params.fog.g = f * 0.5f + nf * _params.fog.g;
	_params.fog.r = f * red + nf * _params.fog.r;
	f = _atmosphereFactor * 0.9f * _horizonFactor + 0.1f;
	nf = 1.0f - f;
	_params.sunDiffuse.a = f + nf * _params.sunDiffuse.a;
	_params.sunDiffuse.b = f * 0.15f + nf * _params.sunDiffuse.b;
	_params.sunDiffuse.g = nf * _params.sunDiffuse.g + f * 0.5f;
	_params.sunDiffuse.r = red * f + nf * _params.sunDiffuse.r;
	f = _atmosphereFactor * 0.2f * _horizonFactor;
	nf = 1.0f - f;
	_glowColor.b = f * 0.19500001f + nf * _glowColor.b;
	_glowColor.g = f * 0.65f + nf * _glowColor.g;
	_glowColor.r = f * red * 1.3f + nf * _glowColor.r;
	_glowColor.a = _sunGlowVisibility;

	_precipDatas->removeAllObjects();
	if (tracer && _hasWeather) {
		const SWeatherInfo *weather = tracer->currentWeatherInfo();
		if (weather)
			_weather = *weather;
		_addPrecipDatas();
	}
	_windVolume = clamp01((_weather.windSpeed - 10.0f) / 20.0f);
	_windVolume = (float)pow((double)_windVolume, (double)1.8f) * 0.4f;
	if (Preferences::isSoundsAvailable() && Preferences::playSounds())
		_adjustSoundsToCurrPrecips();
}
