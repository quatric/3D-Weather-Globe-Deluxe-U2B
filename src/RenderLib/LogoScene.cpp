// The logo scene.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(LogoScene, LF_CLASSOF(BaseScene))

// The script, in seconds.
static const float kScriptDuration = 24.0f;
static const float kScriptFadeInTime = 1.7f;
static const float kScriptMoveTime = 12.0f;
static const float kScriptTitleLogoShowDurationTime = 6.0f;
static const float kScriptNormalLogoShowDurationTime = 0.8f;
static const float kScriptFadeoutDurationTime = 1.5f;
static float kScriptTitleShowStartTime = 9.0f;
static float kScriptNormalLogoShowStartTime = 11.7f;
static float kScriptFadeoutStartTime = 22.5f;

static const float kPi = 3.1415927f;
// The big Earth's radius.
static const float kEarthRadius = 30.2f;

static inline float clampUnit(float f)
{
	if (!(f >= 0.0f))
		return 0.0f;
	return f <= 1.0f ? f : 1.0f;
}

static inline float randomUnit()
{
	return (float)rand() * 4.656613e-10f;
}

LogoScene::LogoScene()
    : _ended(false), _startTime(-1.0), _sunLight(0), _titleLight(0), _lensFlares(0), _sunGlow(0), _fade(1.0f),
      _logoRectAlpha(0.0f), _stars(0), _logoSystem(0), _billboardModel(0), _logoRect(0), _earth(0), _earthGlow(0),
      _title(0)
{
}

void LogoScene::dealloc()
{
	releaseAndClear(_sunLight);
	releaseAndClear(_titleLight);
	releaseAndClear(_lensFlares);
	releaseAndClear(_sunGlow);
	releaseAndClear(_stars);
	releaseAndClear(_logoSystem);
	releaseAndClear(_title);
	releaseAndClear(_billboardModel);
	releaseAndClear(_logoRect);
	releaseAndClear(_earth);
	releaseAndClear(_earthGlow);
	BaseScene::dealloc();
}

void LogoScene::rewind()
{
	_ended = false;
	_startTime = -1.0;
}

void LogoScene::stopDemo()
{
	_startTime = 0.1;
	_ended = true;
	LNotificationCenter::center()->postNotification(kLogoSceneWasEndedNotification, this, LMap::map());
}

CBillboardModelData *LogoScene::detachBillboardData()
{
	if (!_billboardModel) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(&BillboardRenderProfile, _holder);
		_billboardModel = CBillboardModel::_alloc()->initWithRenderProfile(profile);
		if (holder)
			holder->endAccumulationBlock();
	}
	return static_cast<CBillboardModelData *>(_billboardModel->detachModelDataInstance());
}

CBillboardModelData *LogoScene::logoRectDrawableData()
{
	if (!_logoRect) {
		_logoRect = LRetain(detachBillboardData());
		C2DTexture *texture =
		    C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\Logo_nrm.dds"), _holder);
		_logoRect->setTextureForSource(texture, 0);
	}
	return _logoRect;
}

void LogoScene::_renderEarthMap(CFrameDrawer *drawer)
{
	LSize frame = _holder->virtualFrameSize();
	if (_stars)
		drawer->renderDrawableData(_stars, 1);
	drawer->renderDrawableData(_logoSystem->modelData(), 4);
	if (_earth)
		drawer->renderDrawableData(_earth, 4);
	if (_earthGlow)
		drawer->renderDrawableData(_earthGlow, 4);
	if (_logoRectAlpha > 0.01f) {
		if (CBillboardModelData *rect = logoRectDrawableData()) {
			CBillboardSingleData *billboard = rect->billboardAtIndex(0, 1);
			float height = 1.09f / _logoSystem->aspect();
			float width = 1.065f / (frame.width / frame.height);
			LRect r;
			r.origin.x = width * -0.5f;
			r.origin.y = height + 0.12f;
			r.size.width = width;
			r.size.height = height;
			billboard->setViewportRect(r);
			Color color(1.0f, 1.0f, 1.0f, _logoRectAlpha);
			billboard->setDiffuseColor(&color);
			drawer->renderDrawableData(rect, 4);
		}
	}
	if (_title)
		drawer->renderDrawableData(_title, 4);
	if (_lensFlares && _flaresVisibility > 0.01f)
		drawer->renderDrawableData(_lensFlares, 4);
	if (_sunGlow && _flaresVisibility > 0.01f)
		drawer->renderDrawableData(_sunGlow, 4);
	if (_fade < 0.999f) {
		// Black over everything.
		if (CBillboardModelData *white = objectsContainer()->whiteRectDrawableData()) {
			CBillboardSingleData *billboard = white->billboardAtIndex(0, 1);
			LRect r;
			r.origin.x = -1.003f;
			r.origin.y = -1.003f;
			r.size.width = 2.006f;
			r.size.height = 2.006f;
			billboard->setViewportRect(r);
			Color color(0.0f, 0.0f, 0.0f, 1.0f - _fade);
			billboard->setDiffuseColor(&color);
			drawer->renderDrawableData(white, 4);
		}
	}
}

GlowDrawableData *LogoScene::sunGlowDrawableData()
{
	if (_sunGlow)
		return _sunGlow;
	CRenderProfile *profile =
	    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\GlobeGlow.yaf"), _holder);
	CHolder *holder = _holder;
	if (holder)
		holder->beginAccumulationBlock();
	SphereGlowModel *model = SphereGlowModel::_alloc();
	float radii[3] = {0.0f, 1.0f, 50.0f};
	float texParams[3] = {0.0f, 0.125f, 1.0f};
	model->initWithParams(0x20, 3, radii, texParams, SphereGlowModel::kTexGenRadial, profile);
	_sunGlow = LRetain(lobj_cast<GlowDrawableData>(model->detachModelDataInstance()));
	model->release();
	C2DTexture *beams =
	    C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\sun_beams.dds"), _holder);
	_sunGlow->setTextureForSource(beams, 0);
	_sunGlow->setTextureForSource(beams, 1);
	Color color(0.7f, 0.6f, 0.45f, 1.0f);
	_sunGlow->setGlowColors(color, color);
	_sunGlow->setDynamicProlongFactor(0.0f);
	_sunGlow->setBlendingMode(GlowDrawableData::kBlendAdd);
	if (holder)
		holder->endAccumulationBlock();
	return _sunGlow;
}

// The script: the camera rises from below the Earth while it turns under
// the sun; the title lights up, then the logo; the end fades out.
void LogoScene::_adjustCameraToParameters(LPoint, float)
{
	_holder->context()->contextRenderTarget()->setClearColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
	double now = LTimer::currentTimeValue();
	double turns = floor(now * 0.004);
	if (_startTime < 0.0)
		_startTime = now;
	float t = (float)((now - _startTime) / (double)kScriptDuration);
	if (t > 1.0f) {
		t = 1.0f;
		if (!_ended) {
			_ended = true;
			LNotificationCenter::center()->postNotification(kLogoSceneWasEndedNotification, this, 0);
			t = 1.0f;
		}
	}

	Matrix *m = &_earth->_matrix;
	Vector3 earthPosition(m->_41, m->_42, m->_43);
	FHMatrixRotationY(m, (t * 0.05f + 1.5707964f) - 0.04f);
	m->_41 = earthPosition.x;
	m->_42 = earthPosition.y;
	m->_43 = earthPosition.z;
	CContextCamera *camera = _camera;
	_earthGlow->adjustParameters(&earthPosition, kEarthRadius, camera);

	_sunLight->setDirection(Vector3(t - 0.4f, 1.0f, t - 0.7f));
	float title = clampUnit((t - kScriptTitleShowStartTime / kScriptDuration) *
	                        (kScriptDuration / kScriptTitleLogoShowDurationTime));
	_titleLight->setDirection(Vector3(cosf(title * 6.0f), title * title * -3.0f + 0.1f, sinf(title * 6.0f)));

	float move = clampUnit(t + t);	// kScriptMoveTime
	_fade = clampUnit(t / (kScriptFadeInTime / kScriptDuration)) *
	        (1.0f - clampUnit((t - kScriptFadeoutStartTime / kScriptDuration) *
	                          (kScriptDuration / kScriptFadeoutDurationTime)));
	_logoRectAlpha = clampUnit((t - kScriptNormalLogoShowStartTime / kScriptDuration) /
	                           (kScriptNormalLogoShowDurationTime / kScriptDuration));

	float ease = (cosf(move * kPi) + 1.0f) * 0.5f;
	if (ease > 0.5f)
		ease = (1.0f - ease) * (1.0f - ease) * -2.0f + 1.0f;
	else
		ease = (ease + ease) * ease;
	Vector3 cameraPosition(0.0f, ease * 8.0f - 1.0f, 0.0f);
	camera->setPosition(&cameraPosition);
	Vector3 direction(0.0f, 1.0f, 0.0f);
	camera->setViewDirection(&direction);
	Vector3 up(0.0f, 0.0f, 1.0f);
	camera->setViewUpVector(&up);

	if (!_lensFlares)
		return;
	// The flares show as the sun comes out from behind the Earth.
	Vector3 toEarth = earthPosition - cameraPosition;
	float distance = toEarth.length();
	const D3DVECTOR &sun = _sunLight->_params.Direction;
	Vector3 sunDirection(sun.x, sun.y, sun.z);
	float c = toEarth.dot(sunDirection) / distance;
	double angle = acos(c < -1.0f ? -1.0 : (c > 1.0f ? 1.0 : (double)c)) - asin((double)(kEarthRadius / distance));
	_flaresVisibility = clampUnit((((float)angle * 180.0f) / kPi + 1.5f) * 0.3f);
	Vector3 flarePosition = sunDirection * camera->farClip() + cameraPosition;
	_lensFlares->setPosition(&flarePosition);
	_lensFlares->setGlowBrightness(_flaresVisibility);
	if (_flaresVisibility > 0.01f) {
		sunGlowDrawableData();
		_sunGlow->adjustParameters(&flarePosition, 0.5f, camera);
		Color color(_flaresVisibility, _flaresVisibility, _flaresVisibility, _flaresVisibility);
		_sunGlow->setGlowColors(color, color);
		float rotation = (float)(turns - now * 0.004);
		_sunGlow->setTextureRotation(rotation, 0);
		_sunGlow->setTextureRotation(-rotation, 1);
	}
}

LogoScene *LogoScene::initWithController(AtlasPanelController *controller, CHolder *holder)
{
	if (!BaseScene::initWithController(controller, holder))
		return 0;
	_recordsState = false;
	setControlsVisibility(0);
	CContextCamera *camera = CContextCamera::_alloc()->init();
	camera->setFOV(70.0f);
	camera->setClipDistances(0.1f, 25.0f);
	camera->_stateOwnerName = kStateObjNameSceneCamera;
	setCamera(camera);
	camera->release();

	_logoSystem =
	    LogoSystem::_alloc()->initWithParams(_holder, LString::stringWithCString("TEXTURES\\Logo_nrm.dds"), 0x80);
	CHolder *accumulating = _holder;
	if (accumulating)
		accumulating->beginAccumulationBlock();

	// The big Earth: a sphere part.
	SStdGeometry geometry;
	geometry.type = 1;
	geometry.cols = 0x120;
	geometry.rows = 0xa0;
	geometry.radius = kEarthRadius;
	geometry.partsX = 0;
	geometry.partsY = 5;
	geometry.partX = 4;
	geometry.partY = 2;
	CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(
	    LString::stringWithCString("RENDERPROFILES\\LogoBigSphere.yaf"), _holder);
	CMesh *mesh = CMesh::_alloc()->initWithStdGeometry(&geometry, profile);
	_earth = LRetain(lobj_cast<ModelDataWithMatrix>(mesh->detachModelDataInstance()));
	mesh->release();
	C2DTexture *texture =
	    C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\large_earth_txt.dds"), _holder);
	_earth->setTextureForSource(texture, 0);
	FHMatrixRotationY(&_earth->_matrix, 1.5707964f);
	_earth->_matrix._42 = 11.55f;
	_earth->_matrix._43 = -31.0f;

	// The title: a plane as high as its texture.
	profile = CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\LogoTitle.yaf"),
	                                                  _holder);
	texture = C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\TitleNormals.dds"), _holder);
	geometry.type = 3;
	geometry.cols = 4;
	geometry.rows = 4;
	geometry.radius = 10.0f;
	geometry.height = ((float)texture->_info.Height * 10.0f) / (float)texture->_info.Width;
	mesh = CMesh::_alloc()->initWithStdGeometry(&geometry, profile);
	_title = LRetain(lobj_cast<ModelDataWithMatrix>(mesh->detachModelDataInstance()));
	mesh->release();
	_title->setTextureForSource(texture, 0);
	texture =
	    C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\specular_table.dds"), _holder);
	_title->setTextureForSource(texture, 1);
	FHMatrixRotationX(&_title->_matrix, 1.5707964f);
	_title->_matrix._42 = 9.0f;
	_title->_matrix._43 = -0.3f;
	_title->_matrix._41 = geometry.radius * -0.5f;

	// The stars: a band of 2048 in front of the camera.
	profile = CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\LogoStars.yaf"),
	                                                  _holder);
	CParticleSystem *system = CParticleSystem::_alloc();
	system->initWithParams(0, 0x800, kPartSysQuads, profile, 0, 0);
	_stars = LRetain(lobj_cast<CParticleSystemData>(system->detachModelDataInstance()));
	CParticleSystemData::SStoredParticle *p = (CParticleSystemData::SStoredParticle *)_stars->particles();
	int count = _stars->particlesCount();
	for (int i = 0; i < count; i++, p++) {
		float distance = (float)rand() * 8.0f * 4.656613e-10f + 12.0f;
		float latitude = (float)rand() * 0.85f * 4.656613e-10f - 0.3f;
		float longitude = (float)rand() * 1.6f * 4.656613e-10f - 0.8f;
		float b = (float)rand() * 0.9f * 4.656613e-10f + 0.1f;
		float brightness = (float)pow((double)b, (double)(b * 5.0f + 0.2f));
		int green = rand();
		int red = rand();
		Vector3 rgb((float)red * 0.7f * 4.656613e-10f + 0.1f, (float)green * 0.100000024f * 4.656613e-10f + 0.5f, 0.0f);
		rgb.z = 1.0f - rgb.x;
		FHVec3Normalize(&rgb, &rgb);
		float brightest = rgb.x;
		if (brightest <= rgb.y)
			brightest = rgb.y;
		if (brightest <= rgb.z)
			brightest = rgb.z;
		rgb *= brightness / brightest;
		p->position = Vector3(cosf(latitude) * distance * sinf(longitude),
		                      distance * cosf(longitude) * cosf(latitude), distance * sinf(latitude));
		p->size = brightness * 4.0f + 5.0f;
		p->color = (unsigned int)Color(rgb.x, rgb.y, rgb.z, 1.0f);
	}
	texture = C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\star_max.dds"), _holder);
	system->setTextureForSource(texture, 0);
	system->release();

	// The Earth's glow.
	profile = CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\GlobeGlow.yaf"),
	                                                  _holder);
	SphereGlowModel *glow = SphereGlowModel::_alloc();
	float radii[3] = {0.996f, 1.0f, 1.007f};
	float texParams[3] = {1.0f, 0.0f, 1.0f};
	glow->initWithParams(0x200, 3, radii, texParams, SphereGlowModel::kTexGenCylindric, profile);
	_earthGlow = LRetain(lobj_cast<GlowDrawableData>(glow->detachModelDataInstance()));
	glow->release();
	if (_earthGlow) {
		texture = C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\glow.dds"), _holder);
		_earthGlow->setTextureForSource(texture, 0);
		_earthGlow->setGlowColors(Color(0.4f, 0.7f, 1.0f, 1.0f), Color(0.05f, 0.15f, 0.2f, 1.0f));
		_earthGlow->setDynamicProlongFactor(0.2f);
	}

	// The sun and the title's light.
	_sunLight = CContextLight::_alloc()->initWithLightType(D3DLIGHT_DIRECTIONAL);
	_sunLight->setDirection(Vector3(-0.1f, 1.0f, 0.01f));
	_sunLight->_intensity = 10.0f;
	addLight(_sunLight);
	_titleLight = CContextLight::_alloc()->initWithLightType(D3DLIGHT_DIRECTIONAL);
	_titleLight->setDirection(Vector3(-0.1f, -1.0f, 0.01f));
	_titleLight->_intensity = 2.0f;
	addLight(_titleLight);

	_lensFlares = LRetain(objectsContainer()->lensFlares());
	_stars->invalidateParticles();
	if (accumulating)
		accumulating->endAccumulationBlock();
	return this;
}
