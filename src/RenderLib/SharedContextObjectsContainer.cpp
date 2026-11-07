// SharedContextObjectsContainer: the objects a view's scenes share (the
// textures, fonts, billboards, the scene controls, the route containers,
// the photo preview), each made on first use inside the holder's
// accumulation block. It is also the data source of the city marker quad.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(SharedContextObjectsContainer, 0)
LF_SELECTOR(SharedContextObjectsContainer, _entryInfoWasChanged);

// Begins the holder's accumulation block for the scope (no holder: nothing).
namespace {
class StAccumulation {
public:
	StAccumulation(CHolder *holder) : _holder(holder)
	{
		if (_holder)
			_holder->beginAccumulationBlock();
	}
	~StAccumulation()
	{
		if (_holder)
			_holder->endAccumulationBlock();
	}

private:
	CHolder *_holder;
};
}	// namespace

SharedContextObjectsContainer::SharedContextObjectsContainer()
    : _scene(0), _textureLoader(0), _defaultRenderOptions(0), _billboardModel(0), _statusBackgroundTexture(0),
      _overlayScaleBackgroundTexture(0), _minimapTexture(0), _minimapData(0), _mapSelection(0), _infoShortTexture(0),
      _infoFullTexture(0), _infoDeviceData(0), _compassTextures(0), _compassCircleData(0), _compassCenterData(0),
      _compassDeviceData(0), _location0Data(0), _location1Data(0), _navButton(0), _infoButton(0), _minimapButton(0),
      _fullScreenButton(0), _lockNormalButton(0), _lockTiltedButton(0), _lensFlares(0), _smallFont(0),
      _normalFont(0), _largeFont(0), _largeBoldFont(0), _captionFont(0), _normalInfoFont(0), _boldInfoFont(0),
      _eclipseTexture(0), _sunSpecularTexture(0), _whiteRectTexture(0), _whiteRectData(0), _linesTexture(0),
      _linesData(0), _noiseTexture(0), _noiseData(0), _clockData(0), _clockArrowData(0), _clockDigitsData(0),
      _cityMarker(0), _markerMesh(0), _markerTexture(0), _markerTextureApple(0), _markerTextureCapital(0),
      _markerTextureCounty(0), _routePoints(0), _routePointsPreview(0), _holder(0), _windowInterface(0),
      _cloudsController(0), _currentEntry(0), _imagesEntry(0), _previewDelegate(0), _entriesBillboards(0),
      _weatherBillboardTexture(0), _weatherIconsTexture(0), _photoIconBBTexture(0), _explosionTexture(0),
      _aboutMesh(0), _imagesPreview(0), _flag118(true)
{
	_previewDelegate = new _ImagesPreviewControllerDS(this);
}

void SharedContextObjectsContainer::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	releaseAndClear(_navButton);
	releaseAndClear(_infoButton);
	releaseAndClear(_minimapButton);
	releaseAndClear(_fullScreenButton);
	releaseAndClear(_lockNormalButton);
	releaseAndClear(_lockTiltedButton);
	releaseAndClear(_entriesBillboards);
	releaseAndClear(_weatherBillboardTexture);
	releaseAndClear(_photoIconBBTexture);
	releaseAndClear(_weatherIconsTexture);
	releaseAndClear(_explosionTexture);
	// Stops the loading queue and releases the loader.
	_textureLoader->invalidate();
	_textureLoader = 0;
	releaseAndClear(_defaultRenderOptions);
	releaseAndClear(_lensFlares);
	releaseAndClear(_smallFont);
	releaseAndClear(_normalFont);
	releaseAndClear(_largeFont);
	releaseAndClear(_largeBoldFont);
	releaseAndClear(_captionFont);
	releaseAndClear(_normalInfoFont);
	releaseAndClear(_boldInfoFont);
	releaseAndClear(_cityMarker);
	if (_markerMesh) {
		_markerMesh->dataSourceWillDie();
		releaseAndClear(_markerMesh);
	}
	if (_aboutMesh) {
		_aboutMesh->dataSourceWillDie();
		releaseAndClear(_aboutMesh);
	}
	releaseAndClear(_imagesPreview);
	releaseAndClear(_markerTexture);
	releaseAndClear(_markerTextureApple);
	releaseAndClear(_markerTextureCapital);
	releaseAndClear(_markerTextureCounty);
	releaseAndClear(_eclipseTexture);
	releaseAndClear(_sunSpecularTexture);
	releaseAndClear(_whiteRectTexture);
	releaseAndClear(_linesTexture);
	releaseAndClear(_noiseTexture);
	releaseAndClear(_billboardModel);
	releaseAndClear(_location0Data);
	releaseAndClear(_location1Data);
	releaseAndClear(_whiteRectData);
	releaseAndClear(_linesData);
	releaseAndClear(_noiseData);
	releaseAndClear(_clockData);
	releaseAndClear(_clockArrowData);
	releaseAndClear(_clockDigitsData);
	releaseAndClear(_statusBackgroundTexture);
	releaseAndClear(_overlayScaleBackgroundTexture);
	releaseAndClear(_minimapTexture);
	releaseAndClear(_minimapData);
	releaseAndClear(_mapSelection);
	releaseAndClear(_infoShortTexture);
	releaseAndClear(_infoFullTexture);
	releaseAndClear(_infoDeviceData);
	releaseAndClear(_compassTextures);
	releaseAndClear(_compassCircleData);
	releaseAndClear(_compassCenterData);
	releaseAndClear(_compassDeviceData);
	releaseAndClear(_routePoints);
	releaseAndClear(_routePointsPreview);
	releaseAndClear(_cloudsController);
	releaseAndClear(_currentEntry);
	releaseAndClear(_imagesEntry);
	delete _previewDelegate;
	_previewDelegate = 0;
	_holder = 0;
	_windowInterface = 0;
	LObject::dealloc();
}

SharedContextObjectsContainer *SharedContextObjectsContainer::initWithHolderAndWndInterface(CHolder *holder,
                                                                                          IWindowInterface *wnd)
{
	_holder = holder;
	_windowInterface = wnd;
	_textureLoader = CTextureLoader::_alloc()->initWithHolder(holder);
	_cloudsController = CloudsController::_alloc()->initWithHolder(_holder);
	_cloudsController->_stateOwnerName = kObjNameCloudsController;
	return this;
}

void SharedContextObjectsContainer::setLockButtonsTransparency(float transparency)
{
	if (_lockNormalButton)
		_lockNormalButton->setTransparency(transparency);
	if (_lockTiltedButton)
		_lockTiltedButton->setTransparency(transparency);
}

void SharedContextObjectsContainer::setSceneForInternalObjects(BaseScene *scene)
{
	_scene = scene;
	if (scene) {
		if (_routePoints)
			_routePoints->setScene(scene);
		if (_routePointsPreview)
			_routePointsPreview->setScene(scene);
	}
}

void SharedContextObjectsContainer::setImagesEntry(GenericEntry *entry)
{
	if (_imagesEntry == entry)
		return;
	LNotificationCenter *center = LNotificationCenter::center();
	if (_imagesEntry) {
		center->removeObserver(this, EntryInfo::kDidChangeItemNotification, _imagesEntry->entryInfoObject());
		releaseAndClear(_imagesEntry);
	}
	_imagesEntry = LRetain(entry);
	// The original called entryInfoObject() on a null entry too.
	if (_imagesEntry)
		center->addObserver(this, "_entryInfoWasChanged", EntryInfo::kDidChangeItemNotification,
		                    _imagesEntry->entryInfoObject());
	if (_imagesPreview)
		_imagesPreview->dropCache();
}

// The images entry's photos changed: the preview reloads them.
intptr_t SharedContextObjectsContainer::_entryInfoWasChanged(char *args)
{
	LNotification *notification = *(LNotification **)args;
	LMap *info = notification ? notification->userInfo() : 0;
	if (info == 0)
		return 0;
	LObject *key = info->objectForKey(EntryInfo::kChangedKey);
	if (key && key->getClass()->isSubclassOf(LF_CLASSOF(LString)) && key->isEqualTo(kEntryInfoImagesKey) &&
	    _imagesPreview)
		_imagesPreview->dropCache();
	return 0;
}

void SharedContextObjectsContainer::setCurrentEntry(GenericEntry *entry)
{
	LRetain(entry);
	releaseAndClear(_currentEntry);
	_currentEntry = entry;
}

EntryBillboard *SharedContextObjectsContainer::billboardsHitTest(LPoint point, BaseScene *scene)
{
	if (_entriesBillboards == 0)
		return 0;
	EntryBillboard *billboard = _entriesBillboards->hitTest(point, scene);
	return billboard ? lobj_cast<EntryBillboard>(billboard) : 0;
}

// The city marker quad: corners (x, y) = (i & 1, i >> 1), the corner index
// in z; two triangles.
void SharedContextObjectsContainer::meshWasInstantiated(CMesh *mesh)
{
	if (mesh != (CMesh *)_markerMesh)
		return;
	StVertexBufferLocker *vbLocker = mesh->detachVBLocker(0, 0);
	CRenderProfile *profile = mesh->renderProfile();
	char *base = (char *)vbLocker->data();
	unsigned int stride = profile->vertexStructSize(0);
	int position = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	for (unsigned int i = 0; i < 4; i++) {
		float *p = (float *)(base + i * stride + position);
		p[0] = (float)(i & 1);
		p[1] = (float)((int)i >> 1);
		p[2] = (float)i;
	}
	delete vbLocker;
	StIndexBufferLocker *ibLocker = mesh->detachIBLocker(0);
	uint16_t *index = (uint16_t *)ibLocker->data();
	static const uint16_t kIndices[6] = { 0, 3, 1, 0, 2, 3 };
	memcpy(index, kIndices, sizeof(kIndices));
	delete ibLocker;
}

RenderOptions *SharedContextObjectsContainer::defaultRenderOptions()
{
	if (_defaultRenderOptions == 0) {
		_defaultRenderOptions = (RenderOptions *)RenderOptionsFromPrefs::_alloc()->init();
		_defaultRenderOptions->_stateOwnerName = kObjNameRenderOptions;
	}
	return _defaultRenderOptions;
}

// ---------------------------------------------------------------------------
// Billboards: each accessor keeps one data of the shared billboard model.

CBillboardModelData *SharedContextObjectsContainer::detachBillboardData()
{
	if (_billboardModel == 0) {
		StAccumulation block(_holder);
		CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(&BillboardRenderProfile, _holder);
		_billboardModel = CBillboardModel::_alloc()->initWithRenderProfile(profile);
	}
	return (CBillboardModelData *)_billboardModel->detachModelDataInstance();
}

// A retained billboard data that doesn't store scene states.
static CBillboardModelData *_keptBillboardData(SharedContextObjectsContainer *self)
{
	CBillboardModelData *data = LRetain(self->detachBillboardData());
	data->_storesState = false;
	return data;
}

// A retained billboard data showing `texture`.
static CBillboardModelData *_texturedBillboardData(SharedContextObjectsContainer *self, C2DTexture *texture,
                                                   bool wrap)
{
	CBillboardModelData *data = LRetain(self->detachBillboardData());
	data->setTextureForSource(texture, 0);
	if (wrap)
		data->setWrapTexture(1);
	return data;
}

CBillboardModelData *SharedContextObjectsContainer::compassDeviceDrawableData()
{
	if (_compassDeviceData == 0)
		_compassDeviceData = _keptBillboardData(this);
	return _compassDeviceData;
}

CBillboardModelData *SharedContextObjectsContainer::compassCenterDeviceDrawableData()
{
	if (_compassCenterData == 0)
		_compassCenterData = _keptBillboardData(this);
	return _compassCenterData;
}

CBillboardModelData *SharedContextObjectsContainer::compassCircleDeviceDrawableData()
{
	if (_compassCircleData == 0)
		_compassCircleData = _keptBillboardData(this);
	return _compassCircleData;
}

CBillboardModelData *SharedContextObjectsContainer::infoDeviceDrawableData()
{
	if (_infoDeviceData == 0)
		_infoDeviceData = _keptBillboardData(this);
	return _infoDeviceData;
}

// The location markers own their textures through the data.
static CBillboardModelData *_locationData(SharedContextObjectsContainer *self, const char *path)
{
	CBillboardModelData *data = LRetain(self->detachBillboardData());
	StAccumulation block(self->_holder);
	C2DTexture *texture =
	    C2DTexture::_alloc()->initWithHolderAndPath(self->_holder, LString::stringWithCString(path));
	data->setTextureForSource(texture, 0);
	data->_storesState = false;
	if (texture)
		texture->release();
	return data;
}

CBillboardModelData *SharedContextObjectsContainer::location1DData()
{
	if (_location1Data == 0)
		_location1Data = _locationData(this, "TEXTURES\\location1.dds");
	return _location1Data;
}

CBillboardModelData *SharedContextObjectsContainer::location0DData()
{
	if (_location0Data == 0)
		_location0Data = _locationData(this, "TEXTURES\\location0.dds");
	return _location0Data;
}

CBillboardModelData *SharedContextObjectsContainer::minimapDrawableData()
{
	if (_minimapData == 0) {
		_minimapData = _texturedBillboardData(this, minimapTexture(), false);
		_minimapData->_storesState = false;
	}
	return _minimapData;
}

CBillboardModelData *SharedContextObjectsContainer::clockDigitsDrawableData()
{
	if (_clockDigitsData == 0)
		_clockDigitsData = _texturedBillboardData(
		    this, C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\digits.dds"), _holder),
		    true);
	return _clockDigitsData;
}

CBillboardModelData *SharedContextObjectsContainer::clockArrowDrawableData()
{
	if (_clockArrowData == 0)
		_clockArrowData = _texturedBillboardData(
		    this,
		    C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\clockarrow.dds"), _holder),
		    true);
	return _clockArrowData;
}

CBillboardModelData *SharedContextObjectsContainer::clockDrawableData()
{
	if (_clockData == 0)
		_clockData = _texturedBillboardData(
		    this, C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\cross.dds"), _holder),
		    true);
	return _clockData;
}

CBillboardModelData *SharedContextObjectsContainer::noiseDrawableData()
{
	if (_noiseData == 0)
		_noiseData = _texturedBillboardData(this, noiseTexture(), true);
	return _noiseData;
}

CBillboardModelData *SharedContextObjectsContainer::linesDrawableData()
{
	if (_linesData == 0)
		_linesData = _texturedBillboardData(this, linesTexture(), true);
	return _linesData;
}

CBillboardModelData *SharedContextObjectsContainer::whiteRectDrawableData()
{
	if (_whiteRectData == 0)
		_whiteRectData = _texturedBillboardData(this, whiteRectTexture(), false);
	return _whiteRectData;
}

// ---------------------------------------------------------------------------
// Textures.

// Loads `*texture` from `path` once.
static C2DTexture *_texture(SharedContextObjectsContainer *self, C2DTexture **texture, const char *path)
{
	if (*texture == 0) {
		StAccumulation block(self->_holder);
		*texture = C2DTexture::_alloc()->initWithHolderAndPath(self->_holder, LString::stringWithCString(path));
	}
	return *texture;
}

C2DTexture *SharedContextObjectsContainer::explosionTexture()
{
	return _texture(this, &_explosionTexture, "TEXTURES\\Backgrounds\\explode_txt.dds");
}

C2DTexture *SharedContextObjectsContainer::weatherIconsTexture()
{
	return _texture(this, &_weatherIconsTexture, "TEXTURES\\Backgrounds\\Weather_Icons.dds");
}

C2DTexture *SharedContextObjectsContainer::photoIconBBTexture()
{
	return _texture(this, &_photoIconBBTexture, "TEXTURES\\Backgrounds\\photo_icon.dds");
}

C2DTexture *SharedContextObjectsContainer::weatherBillboardTexture()
{
	return _texture(this, &_weatherBillboardTexture, "TEXTURES\\Backgrounds\\Weather_Bg_Anim.dds");
}

C2DTexture *SharedContextObjectsContainer::infoDeviceTexture(unsigned char full)
{
	if (full)
		return _texture(this, &_infoFullTexture, "TEXTURES\\Backgrounds\\info_full.dds");
	return _texture(this, &_infoShortTexture, "TEXTURES\\Backgrounds\\info_short.dds");
}

C2DTexture *SharedContextObjectsContainer::minimapTexture()
{
	return _texture(this, &_minimapTexture, "TEXTURES\\earth_minimap.dds");
}

C2DTexture *SharedContextObjectsContainer::overlayScaleBackgroundTexture()
{
	return _texture(this, &_overlayScaleBackgroundTexture, "TEXTURES\\scale.dds");
}

C2DTexture *SharedContextObjectsContainer::statusBackgroundTexture()
{
	return _texture(this, &_statusBackgroundTexture, "TEXTURES\\status_bgr.dds");
}

C2DTexture *SharedContextObjectsContainer::noiseTexture()
{
	return _texture(this, &_noiseTexture, "TEXTURES\\noise.dds");
}

C2DTexture *SharedContextObjectsContainer::linesTexture()
{
	return _texture(this, &_linesTexture, "TEXTURES\\tv_lines.dds");
}

C2DTexture *SharedContextObjectsContainer::whiteRectTexture()
{
	return _texture(this, &_whiteRectTexture, "TEXTURES\\white_rect.dds");
}

C2DTexture *SharedContextObjectsContainer::sunSpecularTexture()
{
	return _texture(this, &_sunSpecularTexture, "TEXTURES\\sun_specular.dds");
}

C2DTexture *SharedContextObjectsContainer::eclipseTexture()
{
	return _texture(this, &_eclipseTexture, "TEXTURES\\moon_shadow.dds");
}

C2DTexture *SharedContextObjectsContainer::markerTextureCounty()
{
	return _texture(this, &_markerTextureCounty, "TEXTURES\\select_county.dds");
}

C2DTexture *SharedContextObjectsContainer::markerTextureCapital()
{
	return _texture(this, &_markerTextureCapital, "TEXTURES\\select_capital.dds");
}

C2DTexture *SharedContextObjectsContainer::markerTextureApple()
{
	return _texture(this, &_markerTextureApple, "TEXTURES\\select_apple.dds");
}

C2DTexture *SharedContextObjectsContainer::markerTexture()
{
	return _texture(this, &_markerTexture, "TEXTURES\\point.dds");
}

C2DTexture *SharedContextObjectsContainer::weatherMarkerTexture()
{
	return markerTexture();
}

static const char *const kCompassTexturePaths[kCompassTexturesCount] = {
	"TEXTURES\\NavDevice\\1_all_normal.dds",       "TEXTURES\\NavDevice\\1_reset_over.dds",
	"TEXTURES\\NavDevice\\1_reset_press.dds",      "TEXTURES\\NavDevice\\2_all_normal.dds",
	"TEXTURES\\NavDevice\\2_down_over.dds",        "TEXTURES\\NavDevice\\2_down_press.dds",
	"TEXTURES\\NavDevice\\2_left_over.dds",        "TEXTURES\\NavDevice\\2_left_press.dds",
	"TEXTURES\\NavDevice\\2_up_over.dds",          "TEXTURES\\NavDevice\\2_up_press.dds",
	"TEXTURES\\NavDevice\\2_RotZ_left_over.dds",   "TEXTURES\\NavDevice\\2_RotZ_left_press.dds",
	"TEXTURES\\NavDevice\\1_ZoomIn_over.dds",      "TEXTURES\\NavDevice\\1_zoomIn_press.dds",
	"TEXTURES\\NavDevice\\1_ZoomOut_over.dds",     "TEXTURES\\NavDevice\\1_zoomout_press.dds",
	"TEXTURES\\NavDevice\\1_RotX_top_over.dds",    "TEXTURES\\NavDevice\\1_RotX_top_press.dds",
	"TEXTURES\\NavDevice\\1_RotX_bottom_over.dds", "TEXTURES\\NavDevice\\1_RotX_bottom_press.dds",
	"TEXTURES\\NavDevice\\compass.dds",            "TEXTURES\\NavDevice\\compass_over.dds",
	"TEXTURES\\NavDevice\\compass_press.dds",
};

C2DTexture *SharedContextObjectsContainer::compassDeviceTexture(TCompassTextureID id)
{
	if (_compassTextures == 0) {
		_compassTextures = LArray::_alloc()->init();
		for (int i = 0; i < kCompassTexturesCount; i++)
			_compassTextures->addObject(&LNull::sNull);
	}
	// The original clamped to kCompassTexturesCount, one past the array.
	int index = id < 0 ? 0 : id >= kCompassTexturesCount ? kCompassTexturesCount - 1 : id;
	LObject *texture = _compassTextures->objectAtIndex(index);
	if (texture != &LNull::sNull)
		return (C2DTexture *)texture;
	StAccumulation block(_holder);
	C2DTexture *loaded = C2DTexture::_alloc()->initWithHolderAndPath(
	    _holder, LString::stringWithCString(kCompassTexturePaths[index]));
	if (loaded) {
		_compassTextures->replaceObjectAtIndex(loaded, index);
		loaded->release();
	}
	return loaded;
}

// ---------------------------------------------------------------------------
// The map selection: a translucent blue unit square with a red outline.

ModelDataWithMatrix *SharedContextObjectsContainer::detachMapSelectionDrawableData()
{
	if (_mapSelection == 0) {
		StAccumulation block(_holder);
		_mapSelection = C2DContainer::_alloc()->initWithHolder(_holder);
		typedef C2DContainer::StPatch::SPoint SPoint;
		unsigned int fill = Color(0.3f, 0.5f, 0.8f, 0.5f);
		SPoint p00 = { 0, 0, 0, fill }, p10 = { 1, 0, 0, fill }, p01 = { 0, 1, 0, fill }, p11 = { 1, 1, 0, fill };
		C2DContainer::StPatch quad(_mapSelection, D3DPT_TRIANGLESTRIP);
		quad.addTriangle(&p00, &p10, &p01);
		quad.triangleTo(&p11);
		unsigned int outline = Color(1.0f, 0.0f, 0.0f, 1.0f);
		p00.color = p10.color = p01.color = p11.color = outline;
		C2DContainer::StPatch frame(_mapSelection, D3DPT_LINESTRIP);
		frame.addLine(&p00, &p10);
		frame.lineTo(&p11);
		frame.lineTo(&p01);
		frame.lineTo(&p00);
	}
	CDrawableModelData *data = _mapSelection->detachModelDataInstance();
	return data ? lobj_cast<ModelDataWithMatrix>(data) : 0;
}

// ---------------------------------------------------------------------------

CRoutePointsContainer *SharedContextObjectsContainer::routePointsPreviewContainer()
{
	if (_routePointsPreview == 0) {
		_routePointsPreview = CRoutePointsContainer::_alloc()->initWithWindowInterface(_windowInterface);
		_routePointsPreview->_storesState = false;
		_routePointsPreview->setScene(_scene);
	}
	return _routePointsPreview;
}

CRoutePointsContainer *SharedContextObjectsContainer::routePointsContainer()
{
	if (_routePoints == 0) {
		_routePoints = CRoutePointsContainer::_alloc()->initWithWindowInterface(_windowInterface);
		_routePoints->_stateOwnerName = kObjNameRouteCountainer;
		_routePoints->setScene(_scene);
	}
	return _routePoints;
}

EntriesBillboardsHolder *SharedContextObjectsContainer::entriesBillboards()
{
	if (_entriesBillboards == 0)
		_entriesBillboards = EntriesBillboardsHolder::_alloc()->initWithContainer(this);
	return _entriesBillboards;
}

ImagesPreviewController *SharedContextObjectsContainer::imagesPreviewController()
{
	if (_imagesPreview == 0)
		_imagesPreview = ImagesPreviewController::_alloc()->initWithHolder(_holder, _previewDelegate);
	return _imagesPreview;
}

// ---------------------------------------------------------------------------
// Fonts: Arial at a point size, bold or not; a new data each call.

static CFontDrawableData *_fontData(SharedContextObjectsContainer *self, CFontDrawable **font, unsigned int size,
                                    unsigned int bold, unsigned int drawFlags)
{
	if (*font == 0) {
		StAccumulation block(self->_holder);
		*font = CFontDrawable::_alloc()->initWithHolderAndParameters(self->_holder, LString::stringWithCString("Arial"),
		                                                             size, bold);
		if (*font == 0)
			return 0;
	}
	CFontDrawableData *data = (CFontDrawableData *)(*font)->detachModelDataInstance();
	data->setDrawFlags(drawFlags);
	return data;
}

CFontDrawableData *SharedContextObjectsContainer::detachDataForBoldInfoFont()
{
	return _fontData(this, &_boldInfoFont, 13, 1, 0);
}

CFontDrawableData *SharedContextObjectsContainer::detachDataForNormalInfoFont()
{
	return _fontData(this, &_normalInfoFont, 13, 0, 0);
}

CFontDrawableData *SharedContextObjectsContainer::detachDataForCaptionFont()
{
	return _fontData(this, &_captionFont, 14, 0, 0x80);
}

CFontDrawableData *SharedContextObjectsContainer::detachDataForLargeBoldFont()
{
	return _fontData(this, &_largeBoldFont, 16, 1, 0x80);
}

CFontDrawableData *SharedContextObjectsContainer::detachDataForLargeFont()
{
	return _fontData(this, &_largeFont, 16, 0, 0x80);
}

CFontDrawableData *SharedContextObjectsContainer::detachDataForSmallFont()
{
	return _fontData(this, &_smallFont, 12, 0, 0x80);
}

CFontDrawableData *SharedContextObjectsContainer::detachDataForNormalFont()
{
	return _fontData(this, &_normalFont, 15, 0, 0x80);
}

// The sun's lens flares; the model lives as long as its one data.
CGlowModelData *SharedContextObjectsContainer::lensFlares()
{
	if (_lensFlares)
		return _lensFlares;
	StAccumulation block(_holder);
	CRenderProfile *profile =
	    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\GlowSprite.yaf"), _holder);
	CSpriteModel *model = CLensFlareModel::_alloc()->initWithParams(profile);
	if (model == 0)
		return 0;
	C2DTexture *texture =
	    C2DTexture::_alloc()->initWithHolderAndPath(_holder, LString::stringWithCString("TEXTURES\\flares.dds"));
	model->setTextureForSource(texture, 0);
	if (texture)
		texture->release();
	CDrawableModelData *data = model->detachModelDataInstance();
	_lensFlares = data ? lobj_cast<CGlowModelData>(data) : 0;
	model->release();
	if (_lensFlares) {
		LRetain(_lensFlares);
		Vector2 size(0.2f, 0.2f);
		_lensFlares->setSpriteSize(&size);
		_lensFlares->_stateOwnerName = kObjNameSunLensFlares;
	}
	return _lensFlares;
}

// ---------------------------------------------------------------------------
// Scene controls: normal, checked (the "over" image) and pressed.

static CSceneControl *_control(CHolder *holder, const char *normal, const char *checked, const char *pressed)
{
	return CSceneControl::_alloc()->initWithPaths(LString::stringWithCString(normal),
	                                              checked ? LString::stringWithCString(checked) : 0,
	                                              LString::stringWithCString(pressed), holder);
}

CSceneControl *SharedContextObjectsContainer::lockTiltedButton()
{
	if (_lockTiltedButton == 0) {
		_lockTiltedButton = _control(_holder, "TEXTURES\\Butts\\lock_tilt_norm.dds", 0,
		                             "TEXTURES\\Butts\\lock_tilt_press.dds");
		_lockTiltedButton->setIsToggle(true);
	}
	return _lockTiltedButton;
}

CSceneControl *SharedContextObjectsContainer::lockNormalButton()
{
	if (_lockNormalButton == 0) {
		_lockNormalButton = _control(_holder, "TEXTURES\\Butts\\lock_direction_norm.dds", 0,
		                             "TEXTURES\\Butts\\lock_direction_press.dds");
		_lockNormalButton->setIsToggle(true);
	}
	return _lockNormalButton;
}

CSceneControl *SharedContextObjectsContainer::fullScreenButton()
{
	if (_fullScreenButton == 0)
		_fullScreenButton = _control(_holder, "TEXTURES\\Butts\\ExitFS_normal.dds", "TEXTURES\\Butts\\ExitFS_Check.dds",
		                             "TEXTURES\\Butts\\ExitFS_Press.dds");
	return _fullScreenButton;
}

CSceneControl *SharedContextObjectsContainer::minimapButton()
{
	if (_minimapButton == 0)
		_minimapButton = _control(_holder, "TEXTURES\\Butts\\Map_normal.dds", "TEXTURES\\Butts\\Map_checked.dds",
		                          "TEXTURES\\Butts\\Map_Press.dds");
	return _minimapButton;
}

CSceneControl *SharedContextObjectsContainer::infoButton()
{
	if (_infoButton == 0)
		_infoButton = _control(_holder, "TEXTURES\\Butts\\Info_normal.dds", "TEXTURES\\Butts\\Info_checked.dds",
		                       "TEXTURES\\Butts\\Info_Press.dds");
	return _infoButton;
}

CSceneControl *SharedContextObjectsContainer::navButton()
{
	if (_navButton == 0)
		_navButton = _control(_holder, "TEXTURES\\Butts\\Navigation_normal.dds",
		                      "TEXTURES\\Butts\\Navigation_checked.dds", "TEXTURES\\Butts\\Navigation_Press.dds");
	return _navButton;
}

// ---------------------------------------------------------------------------
// Meshes.

MarkerModelData *SharedContextObjectsContainer::cityMarker()
{
	if (_cityMarker)
		return _cityMarker;
	if (_markerMesh == 0) {
		StAccumulation block(_holder);
		CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(
		    LString::stringWithCString("RENDERPROFILES\\BillboardMarker.yaf"), _holder);
		_markerMesh = MarkerMesh::_alloc();
		if (_markerMesh->initWithDataSource(this, profile, 0) == 0) {
			_markerMesh = 0;
			return 0;
		}
	}
	CDrawableModelData *data = _markerMesh->detachModelDataInstance();
	_cityMarker = data ? lobj_cast<MarkerModelData>(data) : 0;
	if (_cityMarker) {
		LRetain(_cityMarker);
		_cityMarker->setParticleTexture(markerTexture());
		_cityMarker->_storesState = false;
	}
	return _cityMarker;
}

AboutMeshData *SharedContextObjectsContainer::detachAboutMeshData()
{
	if (_aboutMesh == 0) {
		StAccumulation block(_holder);
		CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(
		    LString::stringWithCString("RENDERPROFILES\\AboutMesh.yaf"), _holder);
		SStdGeometry geometry;
		memset(&geometry, 0, sizeof(geometry));
		geometry.type = kStdGeometryPlaneXY;
		geometry.cols = 4;
		geometry.rows = 4;
		geometry.radius = 2.0f;
		geometry.height = 2.0f;
		_aboutMesh = (AboutMesh *)AboutMesh::_alloc()->initWithStdGeometry(&geometry, profile);
		if (_aboutMesh == 0)
			return 0;
		_aboutMesh->setTextureForSource(C2DTexture::textureWithContentsOfFile(
		                                    LString::stringWithCString("TEXTURES\\Backgrounds\\about.dds"), _holder),
		                                0);
	}
	CDrawableModelData *data = _aboutMesh->detachModelDataInstance();
	return data ? lobj_cast<AboutMeshData>(data) : 0;
}
