// The star sphere: the catalog's stars, the planets and the constellation
// lines, with the names of the constellations and planets in view.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(Stars, 0)
LF_SELECTOR(Stars, _planetsParamsWereChanged);
LF_SELECTOR(Stars, _worldTimeWasChanged);

const char *kPlanetNames[UPlanets::kPlanetsCount] = {"Mercury", "Venus",  "Earth",   "Mars", "Jupiter",
                                                     "Saturn",  "Uranus", "Neptune", "Pluto"};

// An allStarsArray() record.
struct SStarRecord {
	int id;
	float ra;	// degrees
	float dec;
	float magnitude;
	float unused;
	float bv;
};

Stars::Stars()
    : _scene(0), _stars(0), _planets(0), _constellations(0), _planetsComputed(false), _planetsUploaded(false),
      _namesFlags(0), _stored(false)
{
	_starsRotation.setIdentity();
	_planetsRotation.setIdentity();
	_starsWorld.setIdentity();
	_planetsWorld.setIdentity();
}

void Stars::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	releaseAndClear(_stars);
	releaseAndClear(_planets);
	releaseAndClear(_constellations);
	LObject::dealloc();
}

void Stars::_registerDrawDatas(BaseScene *scene)
{
	if (_stars)
		scene->registerStateOwner(_stars);
	if (_planets)
		scene->registerStateOwner(_planets);
	if (_constellations)
		scene->registerStateOwner(_constellations);
	scene->registerStateOwner(this);
}

// The Earth is not drawn: its slot is skipped.
void Stars::_updatePlanets()
{
	if (_planets) {
		CParticleSystemData::SStoredParticle *p = (CParticleSystemData::SStoredParticle *)_planets->particles();
		int count = _planets->particlesCount();
		for (int i = 0; i < count + 1; i++, p++) {
			if (i == UPlanets::kEarth)
				i++;
			p->size = _planetMagnitudes[i];
			p->position = _planetDirections[i];
			p->color = UPlanets::planetColor((UPlanets::TPlanet)i);
		}
		_planets->invalidateParticles();
	}
	_planetsUploaded = true;
}

intptr_t Stars::_planetsParamsWereChanged(char *)
{
	_planetsUploaded = false;
	_planetsComputed = false;
	return 0;
}

const char *Stars::planetName(unsigned int planet)
{
	return kPlanetNames[planet];
}

void Stars::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeIntInStream(_stored, stream);
	storeMatrixInStream(&_starsWorld, stream);
	storeMatrixInStream(&_planetsWorld, stream);
	if (_stored) {
		storeIntInStream(_namesFlags, stream);
		return;
	}
	for (int i = 0; i < kPlanetsCount; i++) {
		storeFloatInStream(_planetMagnitudes[i], stream);
		storeVectorInStream(&_planetDirections[i], stream);
	}
}

const void *Stars::restoreSceneStateFromData(const void *data, unsigned int)
{
	int stored;
	data = loadIntFromStream(data, &stored);
	_stored = stored != 0;
	data = loadMatrixFromStream(data, &_starsWorld);
	data = loadMatrixFromStream(data, &_planetsWorld);
	if (_stored)
		return loadIntFromStream(data, &_namesFlags);
	for (int i = 0; i < kPlanetsCount; i++) {
		data = loadFloatFromStream(data, &_planetMagnitudes[i]);
		data = loadVectorFromStream(data, &_planetDirections[i]);
	}
	_planetsUploaded = false;
	return data;
}

void Stars::_adjustToCurrentDate()
{
	LDate *date = TimeController::instance()->date();
	Quater q = starsRotation_EarthSystem(date);
	FHMatrixRotationQuaternion(&_starsRotation, &q);
	q = planetsRotation_EarthSystem(date);
	FHMatrixRotationQuaternion(&_planetsRotation, &q);
}

intptr_t Stars::_worldTimeWasChanged(char *)
{
	_adjustToCurrentDate();
	return 0;
}

// The names of the constellations and planets whose points are in view.
void Stars::_renderNames(CFrameDrawer *drawer)
{
	CHolder *holder = _scene->holder();
	CContext *context = holder->context();
	LSize size = holder->virtualFrameSize();
	if (_namesFlags & kNamesConstellations) {
		LMapIterator it(AtlasDatabase::instance()->constellations());
		CFontDrawableData *font = 0;
		while (it.next()) {
			LString *name = (LString *)it.key();
			Vector3 v, p;
			((LValue *)it.object())->getValue(&v);
			context->projectVector(&p, &v, &_starsWorld, 0);
			if (p.x < -1.0f || p.x > 1.0f || p.y < -1.0f || p.y > 1.0f || p.z < -1.0f || p.z > 1.0f)
				continue;
			LRect rect = LZeroRect;
			rect.origin.x = size.width * (p.x + 1.0f) * 0.5f;
			rect.origin.y = size.height * (1.0f - p.y) * 0.5f;
			if (font == 0) {
				font = _scene->objectsContainer()->detachDataForNormalFont();
				font->_storesState = false;
				font->setDrawFlags(0xc);
				font->setColor(Color(0.3f, 0.5f, 0.8f, 1.0f));
			}
			font->addCaptionInRect(name, rect, 0);
		}
		if (font)
			drawer->renderDrawableData(font, 1);
	}
	if ((_namesFlags & kNamesPlanets) == 0)
		return;
	CFontDrawableData *font = 0;
	for (int i = 0; i < kPlanetsCount; i++) {
		if (i == UPlanets::kEarth)
			continue;
		Vector3 p;
		context->projectVector(&p, &_planetDirections[i], &_planetsWorld, 0);
		if (p.x < -1.0f || p.x > 1.0f || p.y < -1.0f || p.y > 1.0f || p.z < -1.0f || p.z > 1.0f)
			continue;
		LRect rect = LZeroRect;
		rect.origin.x = size.width * (p.x + 1.0f) * 0.5f;
		rect.origin.y = size.height * (1.0f - p.y) * 0.5f - 7.0f;
		if (font == 0) {
			font = _scene->objectsContainer()->detachDataForSmallFont();
			font->_storesState = false;
			font->setDrawFlags(0xc);
			font->setColor(Color(0.8f, 0.8f, 0.6f, 1.0f));
		}
		font->addCaptionInRect(LString::stringFromTable(kPlanetNames[i], "RenderLib", getRenderLibModule()), rect, 0);
	}
	if (font)
		drawer->renderDrawableData(font, 1);
}

// Played back: the names, with the planets uploaded once.
void Stars::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	if (!_stored) {
		if (!_planetsUploaded)
			_updatePlanets();
		if (!_stored)
			return;
	}
	if (_namesFlags != 0)
		_renderNames(scene->frameDrawer());
}

void Stars::_render(CFrameDrawer *drawer)
{
	RenderOptions *options = _scene->renderOptions();
	if (_stars && options->starsEnabled())
		drawer->renderDrawableData(_stars, 1);
	if (_planets && options->planetsEnabled()) {
		if (!_planetsUploaded)
			_updatePlanets();
		drawer->renderDrawableData(_planets, 1);
	}
	if (_constellations && options->constellationsEnabled())
		drawer->renderDrawableData(_constellations, 1);
	if (_namesFlags != 0)
		_renderNames(drawer);
}

void Stars::drawWithDrawer(float fov, float scale, const Vector3 *position, CFrameDrawer *drawer)
{
	float magnitude = (float)(log((double)fov) * -1.2000000476837158 + 9.5);
	if (magnitude < 4.0f)
		magnitude = 4.0f;
	else if (magnitude > 12.0f)
		magnitude = 12.0f;
	_starsWorld = _starsRotation;
	_planetsWorld = _planetsRotation;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++) {
			_starsWorld.m[i][j] *= scale;
			_planetsWorld.m[i][j] *= scale;
		}
	_starsWorld._41 = _planetsWorld._41 = position->x;
	_starsWorld._42 = _planetsWorld._42 = position->y;
	_starsWorld._43 = _planetsWorld._43 = position->z;
	RenderOptions *options = _scene->renderOptions();
	if (_stars && options->starsEnabled()) {
		_stars->_magnitude = Vector4(magnitude, magnitude * 0.9f, 0.0f, 0.0f);
		_stars->setWorldMatrix(&_starsWorld);
	}
	if (_constellations && options->constellationsEnabled())
		_constellations->setWorldMatrix(&_starsWorld);
	if (_planets && options->planetsEnabled()) {
		_planets->_magnitude = Vector4(magnitude, magnitude * 0.9f, 0.0f, 0.0f);
		_planets->setWorldMatrix(&_planetsWorld);
	}
	if (!_planetsComputed) {
		_planetsComputed = true;
		_planetsUploaded = false;
		TimeController *time = TimeController::instance();
		Vector3 earth = *time->planetPosition(UPlanets::kEarth);
		for (int i = 0; i < kPlanetsCount; i++) {
			if (i == UPlanets::kEarth)
				continue;
			Vector3 planet = *time->planetPosition((UPlanets::TPlanet)i);
			_planetMagnitudes[i] = UPlanets::planetMagnitude(earth, planet, (UPlanets::TPlanet)i);
			Vector3 d = planet - earth;
			FHVec3Normalize(&_planetDirections[i], &d);
		}
	}
	_namesFlags = 0;
	if (options->showConstellationsNamesInSet() && options->constellationsEnabled())
		_namesFlags |= kNamesConstellations;
	if (options->showPlanetsNamesInSet() && options->planetsEnabled())
		_namesFlags |= kNamesPlanets;
	_stored = false;
	_scene->storePropertyOwnerIfNeeded(this, 0);
	_render(drawer);
	_stored = true;
	_scene->storePropertyOwnerIfNeeded(this, 0);
}

Stars *Stars::initWithHolder(CHolder *holder, BaseScene *scene)
{
	if (LObject::init() == 0)
		return 0;
	_scene = scene;
	CRenderProfile *profile =
	    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\stars_real.yaf"), holder);
	LData *records = AtlasDatabase::instance()->allStarsArray();
	unsigned int count = records->length() / sizeof(SStarRecord);
	if (holder)
		holder->beginAccumulationBlock();
	C2DTexture *texture = LAutorelease(
	    C2DTexture::_alloc()->initWithHolderAndPath(holder, LString::stringWithCString("TEXTURES\\star_max.dds")));

	StarsParticleSystem *system = StarsParticleSystem::_alloc();
	system->initWithParams(0, count, kPartSysQuads, profile, 0, LF_CLASSOF(StarsParticleSubSystem));
	system->_usePatches = true;
	system->setTextureForSource(texture, 0);
	_stars = LRetain(lobj_cast<StarsModelsDrawableData>(system->detachModelDataInstance()));
	system->release();
	if (_stars) {
		_stars->_stateOwnerName = kObjNameAllStars;
		const SStarRecord *r = (const SStarRecord *)records->bytes();
		CParticleSystemData::SStoredParticle *p = (CParticleSystemData::SStoredParticle *)_stars->particles();
		int n = _stars->particlesCount();
		for (int i = 0; i < n; i++, r++, p++) {
			float z = sinf((r->dec * 3.1415927f) / 180.0f);
			float c = sqrtf(1.0f - z * z);
			float a = (r->ra * 3.1415927f) / 180.0f;
			p->position = Vector3(c * cosf(a), c * sinf(a), z);
			p->size = r->magnitude;
			p->color = colorFromBV(r->bv, 1.0f);
		}
		_stars->invalidateParticles();
	}

	system = StarsParticleSystem::_alloc();
	system->initWithParams(0, 7, kPartSysQuads, profile, 0, 0);
	system->setTextureForSource(texture, 0);
	_planets = LRetain(lobj_cast<StarsModelsDrawableData>(system->detachModelDataInstance()));
	system->release();
	if (_planets)
		_planets->_stateOwnerName = kObjNameAllPlanets;

	profile =
	    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\const_lines.yaf"), holder);
	ConstellLinesDataSource *lines = LAutorelease(ConstellLinesDataSource::_alloc()->init());
	CMesh *mesh = ConstellMesh::_alloc()->initWithDataSource(lines, profile, 1);
	_constellations = mesh ? LRetain(lobj_cast<ModelDataWithMatrix>(mesh->detachModelDataInstance())) : 0;
	LRelease(mesh);
	if (_constellations)
		_constellations->_stateOwnerName = kObjNameConstellLines;

	_stateOwnerName = kObjNameStarsManager;
	_adjustToCurrentDate();
	_registerDrawDatas(_scene);
	LNotificationCenter::center()->addObserver(this, "_planetsParamsWereChanged",
	                                           TimeController::kPlanetsParamsWereChanged, 0);
	LNotificationCenter::center()->addObserver(this, "_worldTimeWasChanged", TimeController::kWorldTimeWasChanged, 0);
	if (holder)
		holder->endAccumulationBlock();
	return this;
}
