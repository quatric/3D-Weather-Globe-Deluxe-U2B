// Weather: the sky and precipitation codes; WeatherTracer, which follows the
// weather reports around the camera's geographic point; NatureParams, which
// turns the weather, the sun and the camera into light colors, fog, sky
// textures, precipitation particles, lightning and sounds; and the models
// drawing the precipitations and the lightning.

#ifndef RLWEATHER_H
#define RLWEATHER_H

#include "AppModel.h"
#include "FHMeshes.h"
#include "FHParticles.h"
#include "FHSound.h"

class CinematicSequence;
class NatureParams;

enum TSkyCode {
	kSkyClean = 0,
	kSkyAlmostClean = 1,
	kSkyBrokenCloudsLight = 2,
	kSkyBrokenCloudsMedium = 3,
	kSkyBrokenCloudsDense = 4,
	kSkyHighClouds = 5,
	kSkyDenseClouds = 6,
	kSkyOvercastClouds = 7,
	kSkyFoggyClouds = 8,
	kSkyStormyClouds = 9,
	kSkyCodesCount = 10,
};

enum TPrecipitationCode {
	kPrecipNone = 0,
	kPrecipRain = 1,
	kPrecipTStorm = 2,
	kPrecipRainWithSnow = 3,
	kPrecipSnow = 4,
	kPrecipHail = 5,
	kPrecipDustStorm = 6,
	kPrecipCodesCount = 7,
};

// The weather values; the operators besides * work on all of them.
struct SWeatherInfo {
	SWeatherInfo() { reset(); }
	void reset()
	{
		temperature = 20.0f;
		pressure = 1000.0f;
		humidity = 50.0f;
		windSpeed = 0.0f;
		wind = Vector2(1.0f, 0.0f);
	}
	SWeatherInfo &operator+=(const SWeatherInfo &w);
	SWeatherInfo &operator-=(const SWeatherInfo &w);
	SWeatherInfo operator+(const SWeatherInfo &w) const;
	SWeatherInfo operator-(const SWeatherInfo &w) const;
	// Scales the four scalars; the wind is copied.
	SWeatherInfo operator*(float f) const;

	float temperature;	// Celsius
	float pressure;
	float humidity;
	float windSpeed;
	Vector2 wind;	// direction times speed
};

struct SPrecipitationDescr {
	SPrecipitationDescr() : code(kPrecipNone), intensity(0.0f) {}
	SPrecipitationDescr(TPrecipitationCode c, float i) : code(c), intensity(i) {}

	TPrecipitationCode code;
	float intensity;	// 0..1
};

struct SEnvironmentInfo {
	SEnvironmentInfo() { values[0] = values[1] = values[2] = values[3] = 0; }
	int values[4];
};

// The light of a weather kind, from MAPS\NatureEnv.map.
struct SNatureParams {
	// What a scene is lit with: the day and night pairs blended.
	struct SParams {
		Color sunDiffuse;
		Color sunAmbient;
		Color waterLight;
		Color moonDiffuse;
		Color moonAmbient;
		Color fog;
		Color env;
		float lightingProbability;
	};

	SNatureParams() : lightingProbability(0.0f) {}

	// wa * a + wb * b; the fog follows pow(dayFactor, 0.6) and the
	// environment dayFactor from the night colors to the day ones.
	static void fillParams(const SNatureParams *a, float wa, const SNatureParams *b, float wb, float dayFactor,
	                       SParams *out);
	// The clean sky's.
	static void fillDefaultParams(float dayFactor, SParams *out);

	Color sunDiffuse;
	Color sunAmbient;
	Color waterLight;
	Color moonDiffuse;
	Color moonAmbient;
	Color fogDay;
	Color fogNight;
	Color envDay;
	Color envNight;
	float lightingProbability;	// lightnings a second
};

// The report codes mapped to ours.
TSkyCode skyCodeFromReportCode(int code);
const SPrecipitationDescr *precipCodeFromReportCode(int code);
// The Weather.map entry of the precipitation, or 0.
LString *precipNameFromCode(TPrecipitationCode code);
// Its sound for the day or the night, or 0.
LString *precipSoundFromCode(TPrecipitationCode code, bool day);
// How much the sun flares show through the sky.
float sunFlaresVisibilityFromCode(TSkyCode code);
// The localized names.
LString *skyTypeNameFromSkyCode(TSkyCode code);
LString *precipTypeNameFromPrecipCode(TPrecipitationCode code);
// The sky's cloud texture paths, autoreleased, or 0.
LString *cloudsTextureFromCode(TSkyCode code, bool normals);
LString *cloudsTextureMaskPathFromCode(TSkyCode code);
// The light of the weather, or 0 when NatureEnv.map lacks it.
SNatureParams *natureParamsForSkyPrecip(TSkyCode sky, TPrecipitationCode precip);
// The clean sky's light moved towards the weather's by the precipitation's
// intensity; zeroes when either is missing.
void fillNatureParams(SNatureParams *out, TSkyCode sky, const SPrecipitationDescr *precip);

// ---------------------------------------------------------------------------
// WeatherTracer: the weather where the camera looks, from the reports of the
// stations around it (or set by hand), with the changes blended in time.

class WeatherTracer : public LObject, public DownloadManager::IDownloadDelegate {
	LF_DECLARE_CLASS(WeatherTracer)
public:
	WeatherTracer();
	virtual WeatherTracer *init();
	virtual void dealloc();
	virtual WeatherTracer *getOwnerLObject() { return this; }
	virtual void startTracing();
	virtual void stopTracing();
	// `altitude` is the camera's distance from the ground in Earth radii.
	virtual void tracePosition(LPoint geoPoint, float altitude);
	virtual intptr_t _onReportDownloadTimeout(char *args);
	virtual void _startRealTracing(LPoint geoPoint);
	virtual void _cancelRealTracing();
	virtual void _traceGeoPoint(LPoint geoPoint);
	virtual void _loadReportForGeoPoint(LPoint geoPoint);
	virtual void _switchToNewReport(LMap *report, LPoint geoPoint);
	virtual void downloadFinishedForTask(LMap *params, LData *data, int status);

	// Stops the timer and releases.
	void invalidate();
	void setUseCustomWeather(unsigned char use) { _useCustomWeather = use; }
	void setCustomWeatherInfo(const SWeatherInfo *info) { _current = *info; }
	void setCustomSkyState(TSkyCode sky, TSkyCode next, float mix)
	{
		_skyCode = sky;
		_nextSkyCode = next;
		_customSkyMix = mix;
	}
	void setCustomPrecipitations(const SPrecipitationDescr *precip, const SPrecipitationDescr *next, float mix)
	{
		_precip = *precip;
		_nextPrecip = *next;
		_customPrecipMix = mix;
	}
	// 0 unless the weather is custom or traced.
	const SWeatherInfo *currentWeatherInfo() const
	{
		return (_useCustomWeather || _realTracing) ? &_current : 0;
	}
	// The weather and the one it changes to, with their weights.
	void getSkyState(TSkyCode *sky, float *weight, TSkyCode *next, float *nextWeight) const;
	void getPrecipitations(SPrecipitationDescr *precip, float *weight, SPrecipitationDescr *next,
	                       float *nextWeight) const;
	bool isTracingOnline() { return _tracing && _realTracing && !_downloadFailed; }
	bool weatherInfoIsAvailable();

	static DownloadManager::TASK_ACTION _enumDownloadTasks(LMap *params, void *tracer);

	bool _useCustomWeather;
	bool _tracing;	// asked to
	bool _realTracing;	// following the reports
	bool _hasReportData;
	bool _weatherVisible;	// the camera is low enough
	float _atmoFactor;
	LPoint _point;	// traced
	LPoint _reportPoint;	// the report's
	bool _taskFound;	// by _enumDownloadTasks
	int _retries;
	float _reportRadius;	// the next report is loaded farther
	bool _loading;
	LTimer *_timeoutTimer;
	LMap *_report;	// retained
	TArray<Vector2> _stations;	// (longitude, latitude) of the report's stations nearby
	bool _hasStations;
	SWeatherInfo _current;
	bool _downloadFailed;
	double _retryTime;
	double _lastTraceTime;
	SWeatherInfo _target;
	SWeatherInfo _delta;
	double _skyChangeTime;	// -1 when not changing
	TSkyCode _skyCode;
	TSkyCode _nextSkyCode;
	float _customSkyMix;
	double _precipChangeTime;	// -1 when not changing
	SPrecipitationDescr _precip;
	SPrecipitationDescr _nextPrecip;
	float _customPrecipMix;
};

// ---------------------------------------------------------------------------
// The lightning: a sprite flashing frames of a lightning texture.

class LightingMesh : public CMesh {
	LF_DECLARE_CLASS(LightingMesh)
public:
	virtual LClass *modelDataClass();
	// Hidden while its data is dark.
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual LightingMesh *getOwnerLObject() { return this; }
};

class LightingSpriteData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(LightingSpriteData)
public:
	LightingSpriteData();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	// 'LgDC' the color; 'LgIn' the frame's place in the texture.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual LightingSpriteData *getOwnerLObject() { return this; }

	unsigned int _frame;
	unsigned int _framesCount;
	Color _color;
	bool _large;	// the texture's frames are in rows
};

// ---------------------------------------------------------------------------
// The precipitations: particles in a box of cells around the camera.

class PrecipitationsModel : public CParticleSystem {
	LF_DECLARE_CLASS(PrecipitationsModel)
public:
	virtual LClass *modelDataClass();
	virtual PrecipitationsModel *getOwnerLObject() { return this; }

	// From a Weather.map entry: AmountOfParticles and ShaderName.
	PrecipitationsModel *initWithParams(LMap *params, CHolder *holder);
	// Of the entry's ParticleModelClass, PrecipitationsModel by default; autoreleased.
	static PrecipitationsModel *modelWithParams(LMap *params, CHolder *holder);
};

class PrecipitationsModelData : public CParticleSystemData {
	LF_DECLARE_CLASS(PrecipitationsModelData)
public:
	struct SWeatherParameters {
		SWeatherParameters() {}
		Color color;
		Vector2 wind;
		double time;
		float alpha;
		float intensity;
	};

	PrecipitationsModelData();
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	// Fills the particles from a Weather.map entry.
	virtual void postInitialize(LMap *params);
	virtual void adjustWithParameters(SWeatherParameters *params);
	virtual void _fillParticleStruct(unsigned int, SStoredParticle *) {}
	virtual PrecipitationsModelData *getOwnerLObject() { return this; }

	float _phase;
	float _speedFactor;
	float _windOffsetFactor;
	float _sizeScale;
	Vector2 _wind;
	Vector2 _windOffset;	// 0..1
	Color _color;
	float _alpha;
	Vector3 _cameraOffset;	// 0..1
	float _unused;
	Vector3 _boxSize;	// of a cell
	double _lastTime;
};

class PrecipitationsSubsystem : public CParticleSubsystem {
	LF_DECLARE_CLASS(PrecipitationsSubsystem)
public:
	PrecipitationsSubsystem();
	// 'PrFD' the cell drawn.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	// Draws the cells of the box around the camera that are in view.
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual PrecipitationsSubsystem *getOwnerLObject() { return this; }

	short _gridHalf;	// cells each way
	Vector4 _cellOffset;
};

class FarRainModel : public PrecipitationsModel {
	LF_DECLARE_CLASS(FarRainModel)
public:
	virtual LClass *modelDataClass();
	virtual FarRainModel *getOwnerLObject() { return this; }
};

// All its particles, fainter with less rain.
class FarRainModelData : public PrecipitationsModelData {
	LF_DECLARE_CLASS(FarRainModelData)
public:
	virtual void adjustWithParameters(SWeatherParameters *params);
	virtual FarRainModelData *getOwnerLObject() { return this; }
};

class RainDropsModel : public PrecipitationsModel {
	LF_DECLARE_CLASS(RainDropsModel)
public:
	virtual LClass *modelDataClass();
	virtual RainDropsModel *getOwnerLObject() { return this; }
};

// All its particles, larger and faster with more rain.
class RainDropsModelData : public PrecipitationsModelData {
	LF_DECLARE_CLASS(RainDropsModelData)
public:
	RainDropsModelData() : _baseSpeed(-1.0f) {}
	virtual void adjustWithParameters(SWeatherParameters *params);
	// Picks one of the 8 drop images.
	virtual void _fillParticleStruct(unsigned int index, SStoredParticle *particle);
	virtual RainDropsModelData *getOwnerLObject() { return this; }

	float _baseSpeed;	// the speed factor before scaling, -1 until known
};

// ---------------------------------------------------------------------------
// NatureParams

// Draws the precipitations in the scene's place for them.
class _PrecipsStateHolder : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(_PrecipsStateHolder)
public:
	_PrecipsStateHolder() : _params(0) {}
	virtual _PrecipsStateHolder *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *, unsigned int) {}
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int order);

	NatureParams *_params;	// not retained
};

class NatureParams : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(NatureParams)
public:
	static LString *kLightingWasStarted;

	NatureParams();
	virtual void dealloc();
	virtual NatureParams *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);

	NatureParams *initWithHolder(CHolder *holder);
	void registerDrawDatas(CScene *scene);
	// Follows the weather, the camera and the time; `sequence` scales the sounds.
	void adjustParameters(WeatherTracer *tracer, CContextCamera *camera, CinematicSequence *sequence);
	void drawLighting(CFrameDrawer *drawer);
	void drawPrecipitations(CScene *scene);
	void stopSounds();
	// A lightning on the next adjustment.
	void createRandomLighting() { _forceLighting = true; }
	const SNatureParams::SParams *params() const { return &_params; }

	void _evalSolarEclipseFactor(const Vector3 &sun, const Vector3 &moon, const Vector3 &camera);
	void _evalSkyTextures(WeatherTracer *tracer);
	void _loadSkyTextures(TSkyCode sky, TSkyCode next);
	// The lightning's brightness now.
	float _developLighting(double time, const Vector3 &viewDir, const LPoint &geoPoint, float far);
	void _genSoundSource(LString *name, LString **current, SoundSource **source);
	void _adjustSoundsToCurrPrecips();
	// The models of a Weather.map entry, made once.
	LArray *_precipitationsByName(LString *name);
	void _addPrecipDrawableDataWithParams(const SWeatherInfo *weather, const SPrecipitationDescr *precip,
	                                      float alpha, float time);
	void _addPrecipDatas();
	void _addPrecipsFromStreamedParams();
	SoundBuffer *_soundBuffer(LString *name);

	static C2DTexture *_textureForSkyCode(TSkyCode code, unsigned char normals, CHolder *holder);

	CHolder *_holder;
	LArray *_precipDatas;	// drawn
	LightingSpriteData *_lightingData;
	LArray *_lightingTextures;
	Vector3 _lightingDir;
	Color _lightingColor;
	SNatureParams::SParams _params;
	float _dayFactor;	// 1 by day, 0 by night
	float _atmosphereFactor;	// 1 in the atmosphere, 0 in space
	float _sunDot;	// of the sun's direction and the view's
	Color _envColor;	// lit by the lightning
	Color _glowColor;
	float _sunElevation;	// degrees over the horizon / 3
	Color _sunDiskColor;
	Color _sunDiskOuterColor;
	Color _sunHaloColor;
	Color _sunHaloOuterColor;
	C2DTexture *_skyTexture;
	C2DTexture *_nextSkyTexture;
	C2DTexture *_skyNormals;
	C2DTexture *_nextSkyNormals;
	LString *_skyMaskPath;
	LString *_nextSkyMaskPath;
	LObject *_unusedSky;
	LObject *_nextUnusedSky;
	float _skyWeight;
	float _nextSkyWeight;
	TSkyCode _skyCode;
	TSkyCode _nextSkyCode;
	float _cloudiness;	// how much the clouds hide the sun
	bool _skyTexturesLoaded;
	// The precipitation's by day and by night, then the next one's.
	SoundSource *_precipSounds[4];
	LString *_precipSoundNames[4];
	LMap *_soundBuffers;	// by file
	SoundSource *_thunder;
	SoundSource *_thunderLow;
	SoundSource *_wind;
	float _windVolume;
	float _soundsVolume;
	SPrecipitationDescr _precip;
	SPrecipitationDescr _nextPrecip;
	float _precipWeight;
	float _nextPrecipWeight;
	SWeatherInfo _weather;
	bool _hasWeather;
	float _timeMs;	// within the second
	_PrecipsStateHolder *_precipsHolder;
	float _groundFactor;	// 1 near the ground
	float _lowAltitudeFactor;
	float _sunsetLight;
	bool _sunAboveHorizon;
	float _sunVisibility;
	float _sunGlowVisibility;
	float _spaceGlowVisibility;
	float _eclipseFactor;	// 1 in a total solar eclipse
	float _eclipseDistance;	// 0..1 from the eclipse's center
	float _sunHeight;	// -1..1
	float _horizonFactor;	// 1 with the sun on the horizon
	float _sunsetGlow;
	bool _forceLighting;
	double _lightingStart;	// -1 without lightning
	double _lastLightingCheck;
	int _lightingTexture;
	LMap *_weatherMap;	// MAPS\Weather.map
	LMap *_precipsByName;	// the models
};

#endif
