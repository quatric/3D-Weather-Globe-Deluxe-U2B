// The perspective scene's sky: the atmosphere dome with its cloud layers, the
// horizon strip, the globe glow, the sun's glows, the moon and the lens flares.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(Atmosphere, 0)

// The dome's rings, as angles from the camera's zenith: from beyond the
// horizon seen from the highest point up to the pole.
static float lerp(float a, float b, float t)
{
	return a * (1.0f - t) + b * t;
}

static const float kHorizonCos = 1.0f / 1.05f;
static const float kPhiMin = (float)(acos((double)(1.0f / 1.01f)) + acos((double)kHorizonCos));
static const float kPhiMax = (float)(acos((double)(1.0f / 1.02f)) + acos((double)kHorizonCos));
static const float kPhi0 = kPhiMin * 0.7f;
static const float kYAngles[31] = {
	lerp(kPhiMax * 1.05f, kPhiMin, 0.0f),
	lerp(kPhiMax * 1.03f, kPhiMin, 0.1f),
	lerp(kPhiMax * 1.01f, kPhiMin, 0.2f),
	lerp(kPhiMax, kPhiMin, 0.3f),
	lerp(kPhiMax, kPhiMin, 0.4f),
	lerp(kPhiMax, kPhiMin, 0.5f),
	lerp(kPhiMax, kPhiMin, 0.6f),
	lerp(kPhiMax, kPhiMin, 0.7f),
	lerp(kPhiMax, kPhiMin, 0.8f),
	lerp(kPhiMax, kPhiMin, 0.9f),
	lerp(kPhiMin, kPhi0, 0.0f),
	lerp(kPhiMin, kPhi0, 0.1f),
	lerp(kPhiMin, kPhi0, 0.2f),
	lerp(kPhiMin, kPhi0, 0.3f),
	lerp(kPhiMin, kPhi0, 0.4f),
	lerp(kPhiMin, kPhi0, 0.5f),
	lerp(kPhiMin, kPhi0, 0.6f),
	lerp(kPhiMin, kPhi0, 0.7f),
	lerp(kPhiMin, kPhi0, 0.8f),
	lerp(kPhiMin, kPhi0, 0.9f),
	kPhi0,
	kPhi0 * 0.7f,
	kPhi0 * 0.4f,
	kPhi0 * 0.24f,
	kPhi0 * 0.17f,
	kPhi0 * 0.12f,
	kPhi0 * 0.08f,
	kPhi0 * 0.05f,
	kPhi0 * 0.03f,
	kPhi0 * 0.01f,
	0.0f,
};

// The horizon strip's rows: heights over the ground's plane.
static const float kCylinderZParams[17] = {
	-0.25f, -0.125f, -0.0375f, 0.0f, 0.01275f, 0.0375f, 0.06f, 0.09f, 0.135f,
	0.1875f, 0.24f, 0.3f, 0.36f, 0.4125f, 0.4875f, 0.6f, 0.75f,
};

enum {
	kColumns = 64,
	kDomeRows = 31,
	kStripRows = 17,
	kDomeVertices = kDomeRows * (kColumns + 1),	// 0x7df
	kDomePrimitives = (kDomeRows - 1) * kColumns * 2,	// 0xf00
	kStripVertices = kStripRows * (kColumns + 1),	// 0x451
	kStripPrimitives = (kStripRows - 1) * kColumns * 2,	// 0x800
};

// The sun's distance and radius, in Earth radii.
static const float kSunDistance = 23480.697f;
static const float kSunRadius = 327.37018f;
static const float kMoonRadius = 0.8174883f;

Atmosphere::Atmosphere()
	: _scene(0), _holder(0), _gradient(0), _mesh(0), _sky(0), _clouds0(0), _clouds1(0), _cloudsHigh(0),
	  _visible(false), _horizontMesh(0), _globeGlow(0), _groundHorizont(0), _sunCircleMain(0), _sunGlowGround(0),
	  _sunCrown(0), _sunGlowSpace(0), _eclipseFactor(0.0f), _moon(0), _moonDarkMask(0), _moonCircleMain(0),
	  _lensFlares(0)
{
}

void Atmosphere::dealloc()
{
	if (_gradient) {
		_gradient->dataSourceWillDie();
		releaseAndClear(_gradient);
	}
	if (_mesh) {
		_mesh->dataSourceWillDie();
		releaseAndClear(_mesh);
	}
	releaseAndClear(_sky);
	releaseAndClear(_clouds0);
	releaseAndClear(_clouds1);
	releaseAndClear(_cloudsHigh);
	releaseAndClear(_globeGlow);
	releaseAndClear(_groundHorizont);
	if (_horizontMesh) {
		_horizontMesh->dataSourceWillDie();
		releaseAndClear(_horizontMesh);
	}
	releaseAndClear(_sunCircleMain);
	releaseAndClear(_sunGlowGround);
	releaseAndClear(_sunGlowSpace);
	releaseAndClear(_sunCrown);
	releaseAndClear(_moonCircleMain);
	releaseAndClear(_moonDarkMask);
	releaseAndClear(_moon);
	releaseAndClear(_lensFlares);
	LObject::dealloc();
}

float Atmosphere::transformFactorFromDistance(float distance)
{
	float f = (1.02f - distance) / 0.00999999f;
	if (f < 0.0f)
		return 0.0f;
	if (f > 1.0f)
		return 1.0f;
	return f * f * (3.0f - 2.0f * f);
}

// A new glow of the sphere model, retained.
static GlowDrawableData *newGlow(SphereGlowModel *model)
{
	return LRetain(lobj_cast<GlowDrawableData>(model->detachModelDataInstance()));
}

Atmosphere *Atmosphere::initWithScene(PerspectiveScene *scene)
{
	if (LObject::init() == 0)
		return 0;
	CHolder *holder = scene->holder();
	_holder = holder;
	_scene = scene;
	if (holder)
		holder->beginAccumulationBlock();

	// The horizon strip, faded by the gradient.
	CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(
	    LString::stringWithCString("RENDERPROFILES//SkyHorizontStrip.yaf"), _holder);
	_horizontMesh = (HorizontMesh *)HorizontMesh::_alloc()->initWithDataSource(this, profile, 0);
	_gradient = C2DTexture::_alloc()->initWithDataSource(this, _holder, 0);
	_groundHorizont = LRetain(lobj_cast<HorizontDrawableData>(_horizontMesh->detachModelDataInstance()));
	if (_groundHorizont) {
		_groundHorizont->_stateOwnerName = kObjNameHorizontDataGround;
		_groundHorizont->setTextureForSource(_gradient, 0);
		_groundHorizont->_factor = 0.8f;
	}

	// The glows.
	CRenderProfile *glowProfile = CRenderProfile::renderProfileByFileDesc(
	    LString::stringWithCString("RENDERPROFILES//GlobeGlow.yaf"), _holder);
	if (glowProfile) {
		CHolder *h = _holder;
		if (h)
			h->beginAccumulationBlock();

		// Around the globe.
		float globeRadii[3] = {0.97f, 1.0f, 1.05f};
		float globeTex[3] = {0.0f, 1.0f, 0.0f};
		SphereGlowModel *model = SphereGlowModel::_alloc()->initWithParams(
		    256, 3, globeRadii, globeTex, SphereGlowModel::kTexGenCylindric, glowProfile);
		model->_typeName = "PerspectiveScene::sunCircleMain";
		_globeGlow = lobj_cast<GlowDrawableData>(model->detachModelDataInstance());
		model->release();
		if (_globeGlow) {
			LRetain(_globeGlow);
			_globeGlow->_stateOwnerName = kObjNameHorizontDataGlobe;
			_globeGlow->setTextureForSource(_gradient, 0);
			_globeGlow->setGlowColors(Color(0.4f, 0.8f, 1.0f, 1.0f), Color(0.2f, 0.35f, 0.55f, 1.0f));
			_globeGlow->setDynamicProlongFactor(0.7f);
		}

		// The sun's and the moon's disks.
		float diskRadii[3] = {0.0f, 1.0f, 2.5f};
		float diskTex[3] = {0.0f, 0.48f, 1.0f};
		model = SphereGlowModel::_alloc()->initWithParams(32, 3, diskRadii, diskTex,
		                                                  SphereGlowModel::kTexGenRadial, glowProfile);
		model->_typeName = "PerspectiveScene::sunCircleMain";
		_sunCircleMain = lobj_cast<GlowDrawableData>(model->detachModelDataInstance());
		_moonCircleMain = lobj_cast<GlowDrawableData>(model->detachModelDataInstance());
		_moonDarkMask = lobj_cast<GlowDrawableData>(model->detachModelDataInstance());
		C2DTexture *texture = C2DTexture::_alloc()->initWithHolderAndPath(
		    _holder, LString::stringWithCString("TEXTURES\\sun_circle_main.dds"));
		_sunCircleMain->setTextureForSource(texture, 0);
		_sunCircleMain->setGlowColors(Color(1.0f, 1.0f, 1.0f, 1.0f), Color(1.0f, 1.0f, 1.0f, 1.0f));
		_sunCircleMain->setDynamicProlongFactor(0.0f);
		_sunCircleMain->setBlendingMode(GlowDrawableData::kBlendMode2);
		_sunCircleMain->_stateOwnerName = kObjNameSunCircleMain;
		LRetain(_sunCircleMain);
		model->release();
		_moonCircleMain->setTextureForSource(texture, 0);
		_moonCircleMain->setGlowColors(Color(0.0f, 0.0f, 0.0f, 1.0f), Color(0.0f, 0.0f, 0.0f, 1.0f));
		_moonCircleMain->setDynamicProlongFactor(0.0f);
		_moonCircleMain->setBlendingMode(GlowDrawableData::kBlendMode3);
		_moonCircleMain->_stateOwnerName = kObjNameMoonCircleMain;
		LRetain(_moonCircleMain);
		_moonDarkMask->setTextureForSource(texture, 0);
		_moonDarkMask->setGlowColors(Color(0.0f, 0.0f, 0.0f, 1.0f), Color(0.0f, 0.0f, 0.0f, 1.0f));
		_moonDarkMask->setDynamicProlongFactor(0.0f);
		_moonDarkMask->setBlendingMode(GlowDrawableData::kBlendAlpha);
		_moonDarkMask->_stateOwnerName = kObjNameMoonCircleDarkMask;
		LRetain(_moonDarkMask);
		LRelease(texture);

		// The sun's beams, seen from the ground and from space.
		float beamRadii[3] = {0.0f, 1.0f, 50.0f};
		float beamTex[3] = {0.0f, 0.125f, 1.0f};
		model = SphereGlowModel::_alloc()->initWithParams(32, 3, beamRadii, beamTex,
		                                                  SphereGlowModel::kTexGenRadial, glowProfile);
		model->_typeName = "PerspectiveScene::sunCircleGlow";
		_sunGlowGround = lobj_cast<GlowDrawableData>(model->detachModelDataInstance());
		_sunGlowSpace = lobj_cast<GlowDrawableData>(model->detachModelDataInstance());
		C2DTexture *beams = C2DTexture::_alloc()->initWithHolderAndPath(
		    _holder, LString::stringWithCString("TEXTURES\\sun_beams.dds"));
		// Loaded and never used.
		C2DTexture *glow = C2DTexture::_alloc()->initWithHolderAndPath(
		    _holder, LString::stringWithCString("TEXTURES\\sun_circle_glow.dds"));
		_sunGlowGround->setTextureForSource(beams, 0);
		_sunGlowGround->setTextureForSource(beams, 1);
		_sunGlowGround->setGlowColors(Color(0.7f, 0.6f, 0.45f, 1.0f), Color(0.7f, 0.6f, 0.45f, 1.0f));
		_sunGlowGround->setDynamicProlongFactor(0.0f);
		_sunGlowGround->setBlendingMode(GlowDrawableData::kBlendAdd);
		_sunGlowGround->_stateOwnerName = kObjNameSunGlowGround;
		LRetain(_sunGlowGround);
		_sunGlowSpace->setTextureForSource(beams, 0);
		_sunGlowSpace->setTextureForSource(beams, 1);
		_sunGlowSpace->setGlowColors(Color(0.7f, 0.7f, 0.7f, 1.0f), Color(0.7f, 0.7f, 0.7f, 1.0f));
		_sunGlowSpace->setDynamicProlongFactor(0.0f);
		_sunGlowSpace->setBlendingMode(GlowDrawableData::kBlendAdd);
		_sunGlowSpace->_stateOwnerName = kObjNameSunGlowSpace;
		LRetain(_sunGlowSpace);
		LRelease(beams);
		LRelease(glow);
		model->release();

		// The corona in a total eclipse.
		float crownRadii[3] = {0.9f, 1.0f, 5.0f};
		float crownTex[3] = {0.33f, 0.348f, 1.0f};
		model = SphereGlowModel::_alloc()->initWithParams(32, 3, crownRadii, crownTex,
		                                                  SphereGlowModel::kTexGenRadial, glowProfile);
		model->_typeName = "PerspectiveScene::sunCircleCrown";
		_sunCrown = lobj_cast<GlowDrawableData>(model->detachModelDataInstance());
		texture = C2DTexture::_alloc()->initWithHolderAndPath(_holder,
		                                                      LString::stringWithCString("TEXTURES\\sun_cr_256.dds"));
		_sunCrown->setTextureForSource(texture, 0);
		_sunCrown->setGlowColors(Color(1.0f, 0.9f, 0.8f, 1.0f), Color(1.0f, 0.9f, 0.8f, 1.0f));
		_sunCrown->setDynamicProlongFactor(0.0f);
		_sunCrown->setBlendingMode(GlowDrawableData::kBlendAlpha);
		LRetain(_sunCrown);
		_sunCrown->_stateOwnerName = kObjNameSunCrown;
		LRelease(texture);
		model->release();
		if (h)
			h->endAccumulationBlock();
	}

	// The moon.
	profile = CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\planet.yaf"),
	                                                  _holder);
	Planet *planet = LAutorelease(Planet::_alloc()->initWithRenderProfile(profile));
	_moon = LRetain(planet->detachDrawableData());
	_moon->setRadius(kMoonRadius);
	_moon->setLightingEnabled(1);
	_moon->_stateOwnerName = kObjNameMoonSphere;
	CHolder *h = _holder;
	if (h)
		h->beginAccumulationBlock();
	C2DTexture *texture =
	    C2DTexture::_alloc()->initWithHolderAndPath(_holder, LString::stringWithCString("TEXTURES\\moon.dds"));
	_moon->setDiffuseTexture(texture);
	LRelease(texture);
	if (h)
		h->endAccumulationBlock();

	// The dome: the sky and two cloud layers.
	profile = CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\Atmosphere.yaf"),
	                                                  _holder);
	if (profile) {
		h = _holder;
		if (h)
			h->beginAccumulationBlock();
		_mesh = AtmosphereMesh::_alloc();
		_mesh->initWithDataSource(this, profile, 0);
		_mesh->_typeName = "Atmosphere::mAtmosphereMesh";
		_sky = LRetain(lobj_cast<AtmosphereDrawableData>(_mesh->detachModelDataInstance()));
		_sky->_stateOwnerName = kObjNameAtmosphereSky;
		_clouds0 = LRetain(lobj_cast<AtmosphereDrawableData>(_mesh->detachModelDataInstance()));
		_clouds0->_stateOwnerName = kObjNameAtmosphereClouds0;
		_clouds0->_mode = AtmosphereDrawableData::kModeClouds;
		_clouds1 = LRetain(lobj_cast<AtmosphereDrawableData>(_mesh->detachModelDataInstance()));
		_clouds1->_stateOwnerName = kObjNameAtmosphereClouds1;
		_clouds1->_mode = AtmosphereDrawableData::kModeClouds;
		if (h)
			h->endAccumulationBlock();
	}

	_lensFlares = LRetain(scene->objectsContainer()->lensFlares());
	if (holder)
		holder->endAccumulationBlock();
	return this;
}

void Atmosphere::registerDrawDatas(CScene *scene)
{
	scene->registerStateOwner(_globeGlow);
	scene->registerStateOwner(_groundHorizont);
	scene->registerStateOwner(_sunCircleMain);
	scene->registerStateOwner(_moonCircleMain);
	scene->registerStateOwner(_moonDarkMask);
	scene->registerStateOwner(_sunGlowGround);
	scene->registerStateOwner(_sunGlowSpace);
	scene->registerStateOwner(_sunCrown);
	scene->registerStateOwner(_moon);
	scene->registerStateOwner(_sky);
	scene->registerStateOwner(_clouds0);
	scene->registerStateOwner(_clouds1);
	if (_cloudsHigh)
		scene->registerStateOwner(_cloudsHigh);
	scene->registerStateOwner(_lensFlares);
}

void Atmosphere::textureWasInstantiated(CBaseTexture *texture)
{
	if (texture != _gradient)
		return;
	C2DTexture::StLocker locker(_gradient, 0, D3DLOCK_DISCARD);
	if (CBaseTexture::sizeOfPixelInFormat(locker.desc()->Format) != 4)
		return;
	uint32_t *p = (uint32_t *)locker.bits();
	int width = locker.desc()->Width, height = locker.desc()->Height;
	for (int y = 0; y < height; y++) {
		float v = powf((float)y / (float)(height - 1), 4.0f) * 1.02f;
		float a = v < 1.0f ? v : 1.0f;
		uint32_t pixel = (uint32_t)((int)(a * 255.1f) << 24) | 0xffffff;
		for (int x = 0; x < width; x++)
			*p++ = pixel;
	}
}

unsigned int Atmosphere::numPrimitivesForMesh(CMesh *mesh)
{
	if (mesh == _mesh)
		return kDomePrimitives;
	if (mesh == _horizontMesh)
		return kStripPrimitives;
	return 0;
}

unsigned int Atmosphere::numVerticesForMesh(CMesh *mesh)
{
	if (mesh == _mesh)
		return kDomeVertices;
	if (mesh == _horizontMesh)
		return kStripVertices;
	return 0;
}

// Two triangles a quad between rows of `kColumns + 1` vertices.
template <typename T> static void fillGridIndices(T *p, int rows)
{
	const int row = kColumns + 1;
	for (int a = 0; a < (rows - 1) * row; a += row) {
		T v = (T)a;
		for (int j = 0; j < kColumns; j++, v++, p += 6) {
			p[0] = v;
			p[1] = v + 1;
			p[2] = v + row + 1;
			p[3] = v;
			p[4] = v + row + 1;
			p[5] = v + row;
		}
	}
}

static void fillIndices(CMesh *mesh, int rows, unsigned int primitives)
{
	StIndexBufferLocker *locker = mesh->detachIBLocker(0);
	bool wide = locker->_desc.Format == D3DFMT_INDEX32;
	if (locker->_desc.Size / (wide ? 4 : 2) / 3 != primitives)
		LF_RAISE(WrongConditionException());
	if (wide)
		fillGridIndices((uint32_t *)locker->data(), rows);
	else
		fillGridIndices((uint16_t *)locker->data(), rows);
	delete locker;
}

void Atmosphere::meshWasInstantiated(CMesh *mesh)
{
	if (mesh == _horizontMesh) {
		StVertexBufferLocker *locker = mesh->detachVBLocker(0, 0);
		if (numVerticesForMesh(mesh) != kStripVertices)
			LF_RAISE(WrongConditionException());
		char *p = (char *)locker->data();
		CRenderProfile *profile = mesh->renderProfile();
		unsigned int stride = profile->vertexStructSize(0);
		int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
		int normal = profile->drawableElementOffset(D3DDECLUSAGE_NORMAL, 0, 0);
		int uv0 = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
		int uv1 = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 1, 0);
		for (int r = 0; r < kStripRows; r++) {
			float z = kCylinderZParams[r];
			for (int j = 0; j <= kColumns; j++, p += stride) {
				float u = (float)j * 0.015625f;
				float c = cosf(u * 6.2831855f), s = sinf(u * 6.2831855f);
				if (pos >= 0) {
					float *v = (float *)(p + pos);
					v[0] = c;
					v[1] = s;
					v[2] = z - 0.005f;
				}
				if (normal >= 0) {
					float *v = (float *)(p + normal);
					v[0] = c;
					v[1] = s;
					v[2] = 0.0f;
				}
				if (uv0 >= 0) {
					float *v = (float *)(p + uv0);
					v[0] = u;
					v[1] = (float)r * 0.0625f;
				}
				if (uv1 >= 0) {
					float *v = (float *)(p + uv1);
					v[0] = u;
					v[1] = z >= 0.0f ? 1.0f - z / 0.75f : z * 4.0f + 1.0f;
				}
			}
		}
		delete locker;
		fillIndices(mesh, kStripRows, kStripPrimitives);
	} else if (mesh == _mesh) {
		StVertexBufferLocker *locker = mesh->detachVBLocker(0, 0);
		CRenderProfile *profile = mesh->renderProfile();
		if (profile->_shader == 0)
			LF_RAISE(WrongConditionException());
		int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
		int normal = profile->drawableElementOffset(D3DDECLUSAGE_NORMAL, 0, 0);
		int uv0 = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
		int binormal = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 1, 0);
		int tangent = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 2, 0);
		int angle = profile->drawableElementOffset(D3DDECLUSAGE_PSIZE, 0, 0);
		char *p = (char *)locker->data();
		unsigned int stride = profile->vertexStructSize(0);
		float cosines[kColumns + 1], sines[kColumns + 1];
		for (int r = 0; r < kDomeRows; r++) {
			float a = kYAngles[r];
			float s = sinf(a), c = cosf(a);
			float k = a / (kPhiMin * 0.5f);
			for (int j = 0; j <= kColumns; j++, p += stride) {
				if (r == 0) {
					float t = ((float)j * 0.015625f + (float)j * 0.015625f) * 3.1415927f;
					cosines[j] = cosf(t);
					sines[j] = sinf(t);
				}
				float ct = cosines[j], st = sines[j];
				float *v = (float *)(p + pos);
				v[0] = s * ct * 1.05f;
				v[1] = s * st * 1.05f;
				v[2] = c * 1.05f;
				Vector3 *n = (Vector3 *)(p + normal);
				n->x = s * ct;
				n->y = s * st;
				n->z = c;
				v = (float *)(p + uv0);
				v[0] = ct * k;
				v[1] = st * k;
				Vector3 x(1.0f, 0.0f, 0.0f), t, b;
				FHVec3Cross(&t, &x, n);
				FHVec3Normalize(&t, &t);
				FHVec3Cross(&b, &t, n);
				if (binormal >= 0)
					*(Vector3 *)(p + binormal) = b;
				if (tangent >= 0)
					*(Vector3 *)(p + tangent) = t;
				*(float *)(p + angle) = a;
			}
		}
		delete locker;
		fillIndices(mesh, kDomeRows, kDomePrimitives);
	}
}

void Atmosphere::storeSceneStateInStream(LData *, unsigned int)
{
}

const void *Atmosphere::restoreSceneStateFromData(const void *data, unsigned int)
{
	return data;
}

void Atmosphere::renderSelfHolderInScene(CScene *scene, unsigned int order)
{
	NatureParams *params = _scene->natureParams();
	if (_clouds0) {
		_clouds0->setTextureForSource(params->_skyTexture, 0);
		_clouds0->setTextureForSource(params->_skyNormals, 1);
	}
	if (_clouds1) {
		_clouds1->setTextureForSource(params->_nextSkyTexture, 0);
		_clouds1->setTextureForSource(params->_nextSkyNormals, 1);
	}
	CSceneStatePropertyOwner::renderSelfHolderInScene(scene, order);
}

void Atmosphere::adjustParameters(const NatureParams *params, const CContextCamera *camera)
{
	_eclipseFactor = params->_eclipseFactor;
	double now = LTimer::currentTimeValue();
	double turns = floor(now * 0.004);
	SphereLOD::SCameraParams p(camera->position());
	TimeController *time = TimeController::instance();
	Vector3 sun = -*time->sunPosition() * kSunDistance;
	Vector3 moon = *time->moonPosition();
	moon /= 6371.1f;
	Vector3 moonDir;
	FHVec3Normalize(&moonDir, &moon);

	// The ground horizon: the circle where the view touches the globe.
	Vector3 z(0.0f, 0.0f, 1.0f);
	Quater q1, q2, q;
	FHQuaternionRotationAxis(&q1, &z, p.longitude + 1.5707964f);
	Vector3 axis;
	FHVec3Cross(&axis, &z, &p.direction);
	FHVec3Normalize(&axis, &axis);
	FHQuaternionRotationAxis(&q2, &axis, acosf(z.dot(p.direction)));
	FHQuaternionMultiply(&q, &q1, &q2);
	Matrix &m = _groundHorizont->_matrix;
	FHMatrixRotationQuaternion(&m, &q);
	Vector3 center = p.position;
	center /= p.position.dot(p.position);
	float r = sqrtf(1.0f - center.dot(center));
	m._11 *= r; m._12 *= r; m._13 *= r;
	m._21 *= r; m._22 *= r; m._23 *= r;
	m._31 *= r; m._32 *= r; m._33 *= r;
	m._41 = center.x;
	m._42 = center.y;
	m._43 = center.z;

	Vector3 origin(0.0f, 0.0f, 0.0f);
	_globeGlow->adjustParameters(&origin, 1.0f, camera);
	float ground = transformFactorFromDistance(p.distance);
	_groundHorizont->_visibility = (float)pow((double)ground, (double)0.4f);
	_globeGlow->setVisibilityFactor((float)pow((double)(1.0f - ground), (double)0.4f));

	_sky->adjustToCamera(camera);
	_clouds0->adjustToCamera(camera);
	_clouds1->adjustToCamera(camera);
	if (_cloudsHigh)
		_cloudsHigh->adjustToCamera(camera);

	// The sun.
	_sunCircleMain->adjustParameters(&sun, kSunRadius, camera);
	_sunGlowGround->adjustParameters(&sun, kSunRadius, camera);
	_sunGlowSpace->adjustParameters(&sun, kSunRadius, camera);
	if (_eclipseFactor > 0.01f)
		_sunCrown->adjustParameters(&sun, kSunRadius, camera);
	float v = params->_sunGlowVisibility;
	Color beams(v * 0.7f, v * 0.6f, v * 0.45f, v);
	_sunGlowGround->setGlowColors(beams, beams);
	float spin = (float)(now * 0.004 - turns);
	_sunGlowGround->setTextureRotation(spin, 0);
	_sunGlowGround->setTextureRotation(-spin, 1);
	v = params->_spaceGlowVisibility;
	Color space(v * 0.8f, v * 0.8f, v * 0.8f, v);
	_sunGlowSpace->setGlowColors(space, space);
	_sunGlowSpace->setTextureRotation(spin, 0);
	_sunGlowSpace->setTextureRotation(-spin, 1);

	// The moon, turned to face the Earth.
	_moonCircleMain->adjustParameters(&moon, kMoonRadius, camera);
	_moonDarkMask->adjustParameters(&moon, kMoonRadius, camera);
	if (_moon) {
		Vector3 up(0.0f, 0.0f, 1.0f);
		Quater rotation(0.0f, 0.0f, 0.0f, 1.0f);
		float k = 1.0f / sqrtf(moon.x * moon.x + moon.y * moon.y);
		float x = moon.x * k;
		float a = (float)atan((double)(moon.y * k) / ((double)x + 1e-05));
		if (x < 0.0f)
			a += 3.1415927f;
		FHQuaternionRotationAxis(&rotation, &up, a);
		_moon->adjustMatrixWithCamera(rotation, moon, *camera->position(), camera->_far * 0.95f);
		float lit = ground * params->_dayFactor;
		float f = 1.0f - lit;
		float gray = (f * 0.9f + lit * 0.8f) * params->_eclipseDistance;
		_moon->setDiffuseColor(Color(gray, gray, gray, (lit * 0.8f + f) * params->_eclipseDistance));
	}

	_groundHorizont->_color = params->_params.fog;
	if (params->_atmosphereFactor <= 0.01f) {
		_visible = false;
	} else {
		_sky->adjustWithNatureParams(params, 0, 0);
		_clouds0->adjustWithNatureParams(params, 0, 0);
		_clouds1->adjustWithNatureParams(params, 1, 0);
		if (_cloudsHigh)
			_cloudsHigh->adjustWithNatureParams(params, 1, 1);
		_visible = true;
	}

	if (_lensFlares) {
		Vector3 position = p.position - *time->sunDirection() * (camera->_far * 0.999f);
		_lensFlares->setPosition(&position);
		_lensFlares->setGlowBrightness(params->_sunVisibility);
	}
}

void Atmosphere::drawWithDrawer(CFrameDrawer *drawer)
{
	RenderOptions *options = _scene->renderOptions();
	bool earth = options->renderEarth();
	bool space = options->spaceEnabled();
	bool sun = space && options->sunEnabled();
	bool moon = space && options->moonEnabled();
	// Printing without the Earth skips the dome.
	bool dome = options->isPrinting() ? earth : true;

	if (moon && _moonDarkMask)
		drawer->renderDrawableData(_moonDarkMask, 1);
	if (_sky && dome && _visible)
		drawer->renderDrawableData(_sky, 1);
	if (_moonCircleMain && moon)
		drawer->renderDrawableData(_moonCircleMain, 1);
	if (_sunCircleMain && sun)
		drawer->renderDrawableData(_sunCircleMain, 1);
	if (_sunCrown && sun && _eclipseFactor > 0.01f && moon) {
		Color crown(1.0f, 0.9f, 0.8f, _eclipseFactor);
		_sunCrown->setGlowColors(crown, crown);
		drawer->renderDrawableData(_sunCrown, 1);
	}
	if (_moon && moon)
		drawer->renderDrawableData(_moon, 1);
	if (_clouds0 && dome && _visible) {
		if (_cloudsHigh)
			drawer->renderDrawableData(_cloudsHigh, 1);
		drawer->renderDrawableData(_clouds0, 1);
		if (_clouds1)
			drawer->renderDrawableData(_clouds1, 1);
	}
	if (_groundHorizont && earth && _visible)
		drawer->renderDrawableData(_groundHorizont, 1);
	if (sun && _sunGlowGround)
		drawer->renderDrawableData(_sunGlowGround, 1);
	if (earth && _globeGlow)
		drawer->renderDrawableData(_globeGlow, 3);
	if (sun && _sunGlowSpace)
		drawer->renderDrawableData(_sunGlowSpace, 4);
	if (sun && _lensFlares)
		drawer->renderDrawableData(_lensFlares, 4);
}
