// FlatMapRenderer: the flat map's earth: tiles of the earth textures at the
// level the zoom needs, the day and night separator, and the eclipse shadow.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(FlatMapRenderer, 0)

FlatMapRenderer::FlatMapRenderer()
    : _scene(0), _separatorMesh(0), _separatorData(0), _entries(0), _entryMesh(0), _eclipseMesh(0),
      _eclipseDatas(0), _separatorValid(false), _nightAlpha(1.0f), _showDayNight(false)
{
	_sunDirection = Vector3(1.0f, 0.0f, 0.0f);
	_moonPosition = Vector3(1.0f, 0.0f, 0.0f);
}

void FlatMapRenderer::dealloc()
{
	releaseAndClear(_entries);
	releaseAndClear(_entryMesh);
	releaseAndClear(_separatorMesh);
	releaseAndClear(_separatorData);
	if (_eclipseMesh) {
		_eclipseMesh->dataSourceWillDie();
		releaseAndClear(_eclipseMesh);
	}
	releaseAndClear(_eclipseDatas);
	LObject::dealloc();
}

void FlatMapRenderer::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeVectorInStream(&_sunDirection, stream);
	storeVectorInStream(&_moonPosition, stream);
	storeFloatInStream(_nightAlpha, stream);
	storeIntInStream(_showDayNight, stream);
}

const void *FlatMapRenderer::restoreSceneStateFromData(const void *data, unsigned int)
{
	data = loadVectorFromStream(data, &_sunDirection);
	data = loadVectorFromStream(data, &_moonPosition);
	data = loadFloatFromStream(data, &_nightAlpha);
	int show = 0;
	data = loadIntFromStream(data, &show);
	_separatorValid = false;
	_showDayNight = show != 0;
	return data;
}

void FlatMapRenderer::_recalcSeparatorNormalsIfNeeded(float nightAlpha)
{
	CRenderTarget *target = _scene->holder()->context()->currentRenderTarget();
	int cols, rows, segmentX, segmentY;
	target->getOutSegmentsCount(&cols, &rows);
	target->getOutSegment(&segmentX, &segmentY);
	// Shade again when the view may have moved or the buffers were lost.
	if (cols == 1 && rows == 1 && _separatorValid) {
		bool restored = _separatorMesh->_restored;
		_separatorMesh->_restored = false;
		if (!restored)
			return;
	}
	_separatorValid = true;
	FlatMapController *controller = (FlatMapController *)_scene->_controller;
	LSize frame = _scene->holder()->virtualFrameSize();
	float halfAspect = (frame.width * 0.5f) / frame.height;
	float visiblePart = controller->visiblePartFromCtrlScale();
	LPoint position = controller->position();
	float columnWidth = (halfAspect * visiblePart) / (float)cols;
	float rowHeight = visiblePart / (float)rows;
	CRenderProfile *profile = _separatorMesh->renderProfile();
	if (profile->_context == 0)
		LF_RAISE(WrongConditionException());
	StVertexBufferLocker *locker = _separatorMesh->detachVBLocker(0, 0);
	int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int color = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	char *p = (char *)locker->data();
	unsigned int stride = profile->vertexStructSize(0);
	int count = (int)(locker->_desc.Size / stride);
	float sharpness = 0.5f / visiblePart + 3.0f;
	float lastLat = -10.0f, cosLat = 0.0f, sinLat = 0.0f;
	for (int i = 0; i < count; i++, p += stride) {
		const float *v = (const float *)(p + pos);
		float lon = (columnWidth * (float)segmentX + (position.x - halfAspect * visiblePart * 0.5f) + columnWidth * v[0]) *
		            6.2831855f;
		float lat = ((rowHeight * v[1] + rowHeight * (float)(rows - segmentY - 1) + (position.y - visiblePart * 0.5f)) -
		             0.5f) *
		            3.1415927f;
		if (lat != lastLat) {
			cosLat = cosf(lat);
			sinLat = sinf(lat);
			lastLat = lat;
		}
		float cosLon = cosf(lon), sinLon = sinf(lon);
		// How far the sun is above the horizon, sharpened by the zoom.
		float light = 0.35f - (sinLat * _sunDirection.z + sinLon * cosLat * -_sunDirection.y +
		                       cosLon * cosLat * -_sunDirection.x) *
		                          sharpness;
		unsigned int alpha;
		if (light > 1.0f) {
			light = 1.0f;
			alpha = 255;
		} else if (light < 0.3f) {
			// The night: dark gray, as opaque as the twilight is deep.
			if (light < 0.0f)
				light = 0.0f;
			alpha = (unsigned int)((light / 0.3f) * 255.0f);
			light = 0.3f;
		} else {
			alpha = 255;
		}
		unsigned int gray = (unsigned int)(light * 255.0f) & 0xff;
		unsigned int a = (unsigned int)((float)alpha * nightAlpha + (1.0f - nightAlpha) * 255.0f);
		*(uint32_t *)(p + color) = gray | gray << 8 | gray << 16 | a << 24;
	}
	delete locker;
}

void FlatMapRenderer::_render(CFrameDrawer *drawer)
{
	if (_separatorData && _showDayNight) {
		_recalcSeparatorNormalsIfNeeded(_nightAlpha);
		drawer->renderDrawableData(_separatorData, 1);
	}
	FlatMapController *controller = (FlatMapController *)_scene->_controller;
	PanelDrawIterator *it = controller->detachDrawIterator(0);
	LSize fullFrame = _scene->holder()->fullFrameSize();
	int used = 0;
	while (it->next()) {
		LRect mapRect;
		static_cast<FlatMapController::drawIteratorFlat *>(it)->evalRenderMapRect(controller->position(), &mapRect);
		// The level whose patches are about 1400 pixels high in view.
		float pixels = (fullFrame.height / controller->visiblePartFromCtrlScale()) / 1400.0f;
		int rows = 1;
		char level = 0;
		if (pixels > 1.0f) {
			rows = 3;
			level = 1;
			if (pixels > 3.0f) {
				rows = 9;
				level = 2;
				if (pixels > 9.0f) {
					rows = 18;
					level = 3;
					if (pixels > 18.0f) {
						rows = 54;
						level = 4;
						if (pixels > 54.0f) {
							rows = 162;
							level = 5;
						}
					}
				}
			}
		}
		float n = (float)rows;
		float left = mapRect.origin.x * n;
		float right = (mapRect.origin.x + mapRect.size.width) * n;
		float bottom = n * mapRect.origin.y;
		float top = mapRect.origin.y + mapRect.size.height;
		CTextureLoader *loader = _scene->objectsContainer()->textureLoader();
		for (int y = (int)(n * top * 0.99999f); (int)bottom <= y; y--) {
			for (int x = (int)(left + left); x <= (int)((right + right) * 0.99999f); x++) {
				if ((int)_entries->count() <= used) {
					CDrawableModelData *data = _entryMesh->detachModelDataInstance();
					if (lobj_cast<FlatEntryObjectData>(data)) {
						_entries->addObject(data);
						data->_storesState = false;
					}
				}
				FlatEntryObjectData *entry = (FlatEntryObjectData *)_entries->objectAtIndex(used);
				STextureID id;
				id.x = (short)x;
				id.y = (short)y;
				id.level = level;
				entry->setDrawParameters(id, loader, it->_mapOffset, _nightAlpha);
				drawer->renderDrawableData(entry, 1);
				used++;
			}
		}
	}
	if (_eclipseMesh) {
		PanelDrawIterator *copies = controller->detachDrawIterator(0);
		int index = 0;
		while (copies->next()) {
			if (_eclipseDatas == 0)
				_eclipseDatas = LArray::_alloc()->init();
			CEclipseMeshData *data;
			if (index < (int)_eclipseDatas->count()) {
				data = (CEclipseMeshData *)_eclipseDatas->objectAtIndex(index);
			} else {
				data = (CEclipseMeshData *)_eclipseMesh->detachModelDataInstance();
				C2DTexture *texture = _scene->objectsContainer()->eclipseTexture();
				data->setTextureForSource(texture, 0);
				data->_storesState = false;
				_eclipseDatas->addObject(data);
			}
			index++;
			data->adjustToSunMoonParams(&_sunDirection, &_moonPosition);
			// Moved to the copy of the map.
			Matrix m;
			m.setIdentity();
			m._41 = copies->_mapOffset;
			m._42 = copies->_f8;
			m._43 = copies->_fc;
			data->_matrix = m;
			drawer->renderDrawableData(data, 3);
		}
		delete copies;
	}
	delete it;
}

void FlatMapRenderer::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	_render(scene->_frameDrawer);
}

void FlatMapRenderer::render(CFrameDrawer *drawer)
{
	TimeController *time = TimeController::instance();
	_sunDirection = *time->sunDirection();
	_moonPosition = *time->moonPosition();
	// The night darkens as the map zooms out.
	float f = ((1.0f - _scene->_controller->scale()) - 0.25f) * 3.0f;
	if (f < 0.0f)
		_nightAlpha = 0.3f;
	else if (f <= 0.7f)
		_nightAlpha = f + 0.3f;
	else
		_nightAlpha = 1.0f;
	_showDayNight = _scene->_renderOptions->renderWithDayNight();
	_render(drawer);
	_scene->storePropertyOwnerIfNeeded(this, 0);
}

FlatMapRenderer *FlatMapRenderer::initWithScene(FlatMapScene *scene)
{
	_scene = scene;
	_entries = LArray::_alloc()->init();
	CHolder *holder = _scene->holder();
	CRenderProfile *profile =
	    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\FlatMap.yaf"), holder);
	CHolder *accumulating = _scene->holder();
	if (accumulating)
		accumulating->beginAccumulationBlock();

	// A tile: a plane a patch wide.
	SStdGeometry geometry;
	geometry.type = 3;
	geometry.cols = 3;
	geometry.rows = 3;
	geometry.radius = 1.0f;
	geometry.height = 1.0f;
	_entryMesh = (FlatEntryMesh *)FlatEntryMesh::_alloc()->initWithStdGeometry(&geometry, profile);
	_entryMesh->setPerformBoxVisibilityTest(0);

	// The night side over the whole view.
	profile = CRenderProfile::renderProfileByFileDesc(
	    LString::stringWithCString("RENDERPROFILES\\DayNightSeparator.yaf"), _scene->holder());
	geometry.cols = 64;
	geometry.rows = 32;
	_separatorMesh = CMesh::_alloc()->initWithStdGeometry(&geometry, profile);
	_separatorMesh->setPerformBoxVisibilityTest(0);
	_separatorData = LRetain(_separatorMesh->detachModelDataInstance());
	_separatorData->_storesState = false;

	profile = CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\EclipseFlat.yaf"),
	                                                  _scene->holder());
	if (profile) {
		CMeshDataSource *source = _scene->_eclipseSource;
		_eclipseMesh = (CEclipseMesh *)CEclipseMesh::_alloc()->initWithDataSource(source, profile, 1);
		_eclipseMesh->_typeName = "FlatMapScene::mEclipseMesh";
	}
	if (accumulating)
		accumulating->endAccumulationBlock();
	return this;
}
