// AdaptiveLODSphere: the globe's earth. The patches of the top levels are
// split while nearer parts need finer textures; level 5 patches near the
// camera are drawn as ground elements with their terrain. Also the animated
// water texture, a 64-frame loop.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(AdaptiveLODSphere, 0)
LF_SELECTOR(AdaptiveLODSphere, _waterTxtResWasChanged);

// How many patches were evaluated and skipped (for profiling).
static int sphereLOD_numEvals;
static int sphereLOD_numEvals_lev5;
static int sphereLOD_skipped;

// floor(log2(f)) of a positive float, from its exponent bits.
static int floatExponent(float f)
{
	uint32_t bits;
	memcpy(&bits, &f, 4);
	return (signed char)((bits >> 23) - 0x7f);
}

// Children of a patch per side, from `level` down to `to`.
static int childrenPerSide(int level, int to)
{
	int n = 1;
	for (int l = to; l > level; l--) {
		if (l == 1 || l == 2 || l == 4 || l == 5)
			n *= 3;
		else if (l == 3)
			n *= 2;
	}
	return n;
}

float AdaptiveLODSphere::calcFogClip(float farClip)
{
	float f = 0.7f / farClip - 1.0f;
	if (f > 1.0f)
		f = 1.0f;
	if (!(f >= 0.3f))
		f = 0.3f;
	return farClip / f;
}

AdaptiveLODSphere::AdaptiveLODSphere()
    : _waterVisible(false), _waterFrames(0), _waterTexture(0), _waterFrame(-1), _nightGlowsVisibility(1.0f),
      _waterVisibility(0.0f), _waterSpecularsVisibility(0.0f), _waterDayFactor(1.0f), _holder(0), _lodScale(320.0f),
      _unused4C(80.0f), _data(0), _hasPatches(false), _replaying(false), _transparent(false), _visibility(0.0f)
{
	_lowResWater = !Preferences::animatedWaterUsesLargeTexture();
}

void AdaptiveLODSphere::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	releaseAndClear(_data);
	releaseAndClear(_waterTexture);
	releaseAndClear(_waterFrames);
	LObject::dealloc();
}

void AdaptiveLODSphere::setNightGlowsVisibility(float visibility)
{
	GroundElement::setNightGlowsVisibility(visibility);
	_nightGlowsVisibility = visibility;
}

void AdaptiveLODSphere::setAnimatedWaterVisibility(float visibility)
{
	GroundElement::setAnimatedWaterVisibility(visibility);
	_waterVisible = visibility > 0.01f;
	_waterVisibility = visibility;
}

void AdaptiveLODSphere::setAnimatedWaterSpecularsVisibility(float visibility)
{
	GroundElement::setAnimatedWaterSpecularsVisibility(visibility);
	_waterSpecularsVisibility = visibility;
}

void AdaptiveLODSphere::setAnimatedWaterDayFactor(float factor)
{
	GroundElement::setAnimatedWaterDayFactor(factor);
	_waterDayFactor = factor;
}

int AdaptiveLODSphere::_calcTextureLevel(float distance, float cosAngle, float pixelScale)
{
	if (cosAngle < 0.0001f)
		cosAngle = 0.0001f;
	float size = distance * ((_lodScale * 1.3f * pixelScale) / sqrtf(cosAngle));
	int e = size > 0.0f ? floatExponent(size * size) : 0;
	int v = (signed char)(20 - e);
	int level;
	if (v <= 2)
		level = 0;
	else if (v <= 5)
		level = 1;
	else if (v <= 8)
		level = 2;
	else if (v <= 10)
		level = 3;
	else
		level = v > 13 ? 5 : 4;
	return level;
}

intptr_t AdaptiveLODSphere::_waterTxtResWasChanged(char *)
{
	releaseAndClear(_waterFrames);
	releaseAndClear(_waterTexture);
	_lowResWater = !Preferences::animatedWaterUsesLargeTexture();
	return 0;
}

void AdaptiveLODSphere::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeFloatInStream(_nightGlowsVisibility, stream);
	storeFloatInStream(_waterVisibility, stream);
	storeFloatInStream(_waterSpecularsVisibility, stream);
	storeFloatInStream(_waterDayFactor, stream);
	storeFloatInStream(_visibility, stream);
	storeIntInStream(_waterFrame, stream);
}

AdaptiveLODSphere::SPatchDrawInfo::SPatchDrawInfo(const STextureID &patch, unsigned char isLocal)
{
	id = patch;
	local = isLocal;
}

void AdaptiveLODSphere::SPatchDrawInfo::adjustWithLoader(unsigned int maxLevel, CTextureLoader *loader)
{
	STextureID available = id;
	available.x = (short)(id.x / STextureID::columnsMergedInRow(id.level, id.y));
	while ((unsigned int)available.level > maxLevel && available.scaleDownIfPossible())
		;
	texture.adjust(id, available, loader->requestTextureBeforeDraw(available), local);
	glows.adjust(id, available, CTextureLoader::requestGlowsTextureBeforeDraw(available), local);
}

void AdaptiveLODSphere::_drawElementWithTextureLevel(const STextureID &id, int level, const SDrawPartParams *params)
{
	if (id.level < 5) {
		int i = SphereLOD::_indexOf(id);
		int visible;
		if (!_transparent)
			visible = params->context->boxIntersectionTest(&SphereLOD::sMinCorners[i], &SphereLOD::sMaxCorners[i], false);
		else
			visible = params->context->boxIntersectionTest(&SphereLOD::sMinCorners[i], &SphereLOD::sMaxCorners[i],
			                                               _data->worldMatrix(), false);
		if (visible && _data) {
			_hasPatches = true;
			_data->addElement(id, level);
		}
		return;
	}
	// A ground element: its box is the patch's unit square raised to its
	// highest ground, in the patch's frame.
	STextureID element = id;
	float minZ, maxZ;
	if (!GroundElement::getZRange(element, &minZ, &maxZ))
		maxZ = 0.0042929994f;
	Vector3 boxMin(0.0f, 0.0f, 0.0f);
	Vector3 boxMax(1.0f, 1.0f, maxZ);
	const Matrix *m = &SphereLOD::sMaxLevelMatrices[element.x + element.y * SphereLOD::sItemsCountForLOD[5] * 2];
	if (params->context->boxIntersectionTest(&boxMin, &boxMax, m, false))
		GroundElement::postElementToQueue(element, level, params->farClip);
}

void AdaptiveLODSphere::drawPart(const STextureID &id, CFrameDrawer *drawer, const SDrawPartParams *params,
                                 float nearDistance, float farDistance, int farLevel, int nearLevel)
{
	// Evaluate the patch unless its parent's levels hold for it too.
	if (farLevel != nearLevel || farLevel < 0 || (nearDistance < params->farClip && params->farClip < farDistance)) {
		sphereLOD_numEvals++;
		float nearCos, farCos;
		bool facing = SphereLOD::getEntryViewParams(id, 0.0f, params->camera, &nearDistance, &farDistance, &nearCos,
		                                            &farCos);
		bool behind = false;
		if (!facing) {
			if (!_transparent)
				return;
			behind = true;
		}
		if (params->farClip < nearDistance && !_transparent)
			return;
		farLevel = 0;
		nearLevel = 0;
		if (!behind) {
			farLevel = _calcTextureLevel(farDistance, farCos, params->pixelScale);
			nearLevel = _calcTextureLevel(nearDistance, nearCos, params->pixelScale);
		}
	} else {
		sphereLOD_skipped++;
	}
	int level = id.level;
	if (farLevel < nearLevel && level < 4) {
		// The near part needs finer textures: split.
		int n = childrenPerSide(level, level + 1);
		for (int y = id.y * n; y < id.y * n + n; y++)
			for (int x = id.x * n; x < id.x * n + n; x++) {
				STextureID child;
				child.x = (short)x;
				child.y = (short)y;
				child.level = (char)(level + 1);
				drawPart(child, drawer, params, nearDistance, farDistance, farLevel, nearLevel);
			}
		return;
	}
	// Far patches (0.1 radius away or more) need no terrain.
	float d = nearDistance * 5000.0f;
	int e = d > 0.0f ? floatExponent(d * d) : 0;
	bool far = (signed char)(24 - e) < 7;
	bool hiResAllowed = SphereLOD::doAllowHiResDrawForID(id);
	if ((far && nearLevel <= level) || !hiResAllowed) {
		_drawElementWithTextureLevel(id, nearLevel, params);
		return;
	}
	int target = far ? nearLevel : 5;
	if (level + 1 < target) {
		int n = childrenPerSide(level, level + 1);
		for (int y = id.y * n; y < id.y * n + n; y++)
			for (int x = id.x * n; x < id.x * n + n; x++) {
				STextureID child;
				child.x = (short)x;
				child.y = (short)y;
				child.level = (char)(level + 1);
				drawPart(child, drawer, params, nearDistance, farDistance, farLevel, nearLevel);
			}
		return;
	}
	// The children at the target level, each with its own texture level.
	int n = childrenPerSide(level, target);
	for (int y = id.y * n; y < id.y * n + n; y++) {
		for (int x = id.x * n; x < id.x * n + n; x++) {
			STextureID child;
			child.x = (short)x;
			child.y = (short)y;
			child.level = (char)target;
			int childLevel = nearLevel;
			if (target == 5 &&
			    !(farLevel == nearLevel && farLevel >= 0 &&
			      (params->farClip <= nearDistance || farDistance <= params->farClip))) {
				sphereLOD_numEvals++;
				sphereLOD_numEvals_lev5++;
				float childNear = nearDistance, childFar = farDistance, nearCos, farCos;
				if (!SphereLOD::getEntryViewParams(child, 0.0f, params->camera, &childNear, &childFar, &nearCos,
				                                   &farCos))
					continue;
				if (childNear <= params->farClip) {
					childLevel = _calcTextureLevel(childNear, nearCos, params->pixelScale);
				} else {
					childLevel = 0;
					if (!_transparent)
						continue;
				}
			} else if (target == 5) {
				sphereLOD_skipped++;
			}
			_drawElementWithTextureLevel(child, childLevel, params);
		}
	}
}

void AdaptiveLODSphere::_updateWaterTexture(int frame)
{
	if (!Preferences::animatedWaterEnabled()) {
		_waterFrame = -1;
		releaseAndClear(_waterTexture);
		releaseAndClear(_waterFrames);
		return;
	}
	unsigned int size = _lowResWater ? 512 : 1024;
	if (_waterTexture && (_lowResWater ? _waterTexture->_info.Width > 512 : _waterTexture->_info.Width < 1024)) {
		// The resolution was changed.
		_waterFrame = -1;
		releaseAndClear(_waterTexture);
		releaseAndClear(_waterFrames);
	}
	if (_waterTexture == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_waterTexture = C2DTexture::_alloc()->initWithHolderAndParams(_holder, size, size, D3DFMT_DXT1, 0, 1);
		releaseAndClear(_waterFrames);
		_waterFrames = LArray::_alloc()->init();
		for (int i = 0; i < 64; i++)
			_waterFrames->addObject(&LNull::sNull);
		if (holder)
			holder->endAccumulationBlock();
		if (_waterTexture == 0)
			return;
	}
	if (frame < 0 || frame >= (int)_waterFrames->count()) {
		// 64 frames in 3 seconds.
		double t = LTimer::currentTimeValue() / 3.0;
		frame = (int)(unsigned int)((t - floor(t)) * 63.9);
	}
	if (frame == _waterFrame)
		return;
	_waterFrame = frame;
	LObject *data = _waterFrames->objectAtIndex(frame);
	if (data == &LNull::sNull) {
		LString *path = LString::stringWithFormat(
		    _lowResWater ? "water_textures_low\\water_%03d.dds" : "water_textures\\water_%03d.dds", frame);
		data = LData::dataWithContentsOfFile(path);
		if (data == 0)
			return;
		_waterFrames->replaceObjectAtIndex(data, frame);
	} else if (data == 0) {
		return;
	}
	ImageDecompressor *decompressor = ImageDecompressor::decompressorForData((LData *)data);
	if (decompressor)
		decompressor->decompressToTexture(_waterTexture);
}

const void *AdaptiveLODSphere::restoreSceneStateFromData(const void *data, unsigned int)
{
	int frame = -1;
	data = loadFloatFromStream(data, &_nightGlowsVisibility);
	data = loadFloatFromStream(data, &_waterVisibility);
	data = loadFloatFromStream(data, &_waterSpecularsVisibility);
	data = loadFloatFromStream(data, &_waterDayFactor);
	data = loadFloatFromStream(data, &_visibility);
	data = loadIntFromStream(data, &frame);
	setNightGlowsVisibility(_nightGlowsVisibility);
	setAnimatedWaterVisibility(_waterVisibility);
	setAnimatedWaterSpecularsVisibility(_waterSpecularsVisibility);
	setAnimatedWaterDayFactor(_waterDayFactor);
	_updateWaterTexture(frame);
	return data;
}

AdaptiveLODSphere *AdaptiveLODSphere::initWithParams(CHolder *holder)
{
	if (LObject::init() == 0)
		return 0;
	_holder = holder;
	_stateOwnerName = kObjNameAdaptiveSphere;
	LNotificationCenter::center()->addObserver(this, "_waterTxtResWasChanged", Preferences::kWaterTextureSizeWasChanged,
	                                           0);
	_updateWaterTexture(-1);
	return this;
}

AdaptiveSphereDrawableData *AdaptiveLODSphere::wholeGlobeData()
{
	if (_data == 0) {
		CRenderProfile *profile =
		    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\SphereNoDEM.yaf"), _holder);
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		AdaptiveSphereDrawable *mesh = AdaptiveSphereDrawable::_alloc();
		if (mesh->initWithDataSource(mesh, profile, 0) && mesh) {
			_data = LRetain(lobj_cast<AdaptiveSphereDrawableData>(mesh->detachModelDataInstance()));
			_data->_storesState = false;
			mesh->release();
		}
		if (holder)
			holder->endAccumulationBlock();
	}
	return _data;
}

void AdaptiveLODSphere::performDraw(CFrameDrawer *drawer, float visibility)
{
	_hasPatches = false;
	_visibility = visibility;
	CContext *context = drawer->_context;
	if (context == 0)
		return;
	CContextCamera *camera = context->_currentRenderTarget->_camera;
	SphereLOD::SCameraParams cameraParams(camera->position());
	float farPlane = camera->_far;
	SDrawPartParams params;
	params.drawer = drawer;
	params.context = context;
	params.camera = &cameraParams;
	params.viewMatrix = context->_currentRenderTarget->viewMatrix();
	params.pixelScale = 600.0f / _holder->fullFrameSize().height;
	Vector3 position = *camera->position();
	float distance = sqrtf(position.x * position.x + position.y * position.y + position.z * position.z);
	params.cameraDistance2 = distance * distance;
	Vector3 direction = position / distance;
	params.farClip = SphereLOD::farClipDistanceForAltitude(distance - 1.0f);
	params.cameraPosition = &position;

	wholeGlobeData();
	_data->startFrame(params.farClip, _visibility);
	_transparent = _data->_alpha < 0.98f;
	if (_waterVisible && !_replaying)
		_updateWaterTexture(-1);
	GroundElement::setWaterTextureScale(_lowResWater ? 3.0f : 2.0f);
	GroundElement::startFrame(drawer, _waterTexture, mapPointFromEarthVec(&direction));

	Matrix m;
	m.setIdentity();
	if (_transparent && farPlane < distance + 1.0f) {
		// The far side would be clipped: shrink the globe towards the camera.
		float f = 1.0f - farPlane / (distance + 1.0f);
		Vector3 shift = position * f;
		m._11 = m._22 = m._33 = 1.0f - f;
		m._41 = shift.x;
		m._42 = shift.y;
		m._43 = shift.z;
	}
	_data->_matrix = m;
	context->setWorldTransform(_data->worldMatrix());

	// The two hemispheres.
	STextureID east = {0, 0, 0}, west = {1, 0, 0};
	drawPart(east, drawer, &params, 0.0f, 0.0f, -1, -1);
	drawPart(west, drawer, &params, 0.0f, 0.0f, -1, -1);
	if (_hasPatches && _data)
		drawer->renderDrawableData(_data, 1);
	GroundElement::endFrame(drawer);
}

void AdaptiveLODSphere::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	bool replaying = _replaying;
	_replaying = true;
	performDraw(scene->_frameDrawer, _visibility);
	_replaying = replaying;
}
