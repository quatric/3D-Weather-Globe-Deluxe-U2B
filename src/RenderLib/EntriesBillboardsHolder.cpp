// EntriesBillboardsHolder: the weather billboards of the top cities (kind 2)
// and the photo billboards of the entries with images (kind 4), laid out by
// a SmartBillboardSystem and drawn as billboard batches and captions, or
// printed through a BaseDrawing.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(EntriesBillboardsHolder, 0)
LF_SELECTOR(EntriesBillboardsHolder, _mainDBWasChanged);
LF_SELECTOR(EntriesBillboardsHolder, _entryWasChanged);
LF_SELECTOR(EntriesBillboardsHolder, _entryInfoWasChanged);

EntriesBillboardsHolder::EntriesBillboardsHolder()
    : _container(0), _system(0), _imagesDirty(true), _drawing(0), _scene(0), _iterator(0), _visibleKinds(0),
      _namesFont(0), _temperaturesFont(0), _weatherBillboards(0), _weatherIcons(0), _markers(0), _photoIcons(0)
{
}

EntriesBillboardsHolder *EntriesBillboardsHolder::initWithContainer(SharedContextObjectsContainer *container)
{
	_container = container;
	_stateOwnerName = kObjNameWeatherBillboards;
	LNotificationCenter::center()->addObserver(this, "_mainDBWasChanged",
	                                           AtlasDatabase::kMainDatabaseChangedNotification, 0);
	LNotificationCenter::center()->addObserver(this, "_entryInfoWasChanged", EntryInfo::kDidChangeItemNotification, 0);
	LNotificationCenter::center()->addObserver(this, "_entryWasChanged", GenericEntry::kEntryDidChangeNotification,
	                                           0);
	return this;
}

static void _releaseDatas(EntriesBillboardsHolder *h)
{
	releaseAndClear(h->_namesFont);
	releaseAndClear(h->_temperaturesFont);
	releaseAndClear(h->_weatherBillboards);
	releaseAndClear(h->_weatherIcons);
	releaseAndClear(h->_photoIcons);
	releaseAndClear(h->_markers);
}

void EntriesBillboardsHolder::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	_releaseDatas(this);
	if (_system) {
		_system->invalidate();
		_system = 0;
	}
	LObject::dealloc();
}

void EntriesBillboardsHolder::invalImagesBillboards()
{
	if (_system)
		_system->removeAllEntriesWithKind(4);
	_imagesDirty = true;
}

void EntriesBillboardsHolder::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeIntInStream(_visibleKinds, stream);
}

const void *EntriesBillboardsHolder::restoreSceneStateFromData(const void *data, unsigned int)
{
	int kinds;
	data = loadIntFromStream(data, &kinds);
	_visibleKinds = kinds;
	return data;
}

bool EntriesBillboardsHolder::geoPointToViewPoint(LPoint *viewPoint, const LPoint *geoPoint, const LSize *offset)
{
	if (_drawing == 0)
		return _scene->viewPointFromGeoPoint(viewPoint, geoPoint, _iterator, offset);
	unsigned char visible = 0;
	*viewPoint = _drawing->viewPointFromGeoPoint(*geoPoint, _iterator, &visible);
	return visible;
}

void EntriesBillboardsHolder::_printEntry(EntryBillboard *billboard, const LRect *rect)
{
	if (_drawing && (billboard->kind() & 2))
		_drawing->_drawEntryBillboard(billboard->_entry, rect, _iterator);
}

intptr_t EntriesBillboardsHolder::_mainDBWasChanged(char *)
{
	// The original dropped the system without invalidating its timer.
	if (_system) {
		_system->invalidate();
		_system = 0;
	}
	return 0;
}

static CBillboardModelData *_newBillboards(SharedContextObjectsContainer *container, C2DTexture *texture)
{
	CBillboardModelData *data = LRetain(container->detachBillboardData());
	data->_storesState = false;
	data->setTextureForSource(texture, 0);
	return data;
}

static inline float _texWidth(C2DTexture *t)
{
	return (float)t->_info.Width;
}

static inline float _texHeight(C2DTexture *t)
{
	return (float)t->_info.Height;
}

// The marker under the city, then (kind 2) the name, the temperature and the
// weather icon on the billboard (a spinner while the report loads or when
// there's none), then (kind 4) the photo icon.
void EntriesBillboardsHolder::_renderEntry(EntryBillboard *billboard, const LRect *rect)
{
	GenericEntry *entry = billboard->_entry;
	bool weather = (_visibleKinds & billboard->kind() & 2) != 0;
	bool photos = (_visibleKinds & billboard->kind() & 4) != 0;
	LString *name = entry->ownName(0);
	C2DTexture *markerTexture = _container->weatherMarkerTexture();
	if (_markers == 0)
		_markers = _newBillboards(_container, markerTexture);
	if (markerTexture) {
		CBillboardSingleData marker;
		marker.setScreenRect(LMakeRect(rect->origin.x - 5, rect->origin.y + 64 - 5, 10, 10));
		Color color(0.9f, 1.0f, 0.4f, 0.9f);
		marker.setDiffuseColor(&color);
		_markers->addBillboard(&marker);
	}
	if (weather) {
		if (_namesFont == 0) {
			_namesFont = LRetain(_container->detachDataForNormalInfoFont());
			_namesFont->setDrawFlags(0);
			_namesFont->_storesState = false;
			_namesFont->setDrawFlags(0x40);
		}
		C2DTexture *bbTexture = _container->weatherBillboardTexture();
		C2DTexture *iconsTexture = _container->weatherIconsTexture();
		if (_weatherBillboards == 0)
			_weatherBillboards = _newBillboards(_container, bbTexture);
		if (_weatherIcons == 0)
			_weatherIcons = _newBillboards(_container, iconsTexture);
		_namesFont->addCaptionInRect(name, LMakeRect(rect->origin.x + 6, rect->origin.y + 46, 110, 28), 0);
		// The billboard texture is 4x4 frames: the first, or the spinner's.
		CBillboardSingleData board;
		board.setScreenRect(LMakeRect(rect->origin.x - 4, rect->origin.y + 6, _texWidth(bbTexture) / 4,
		                              _texHeight(bbTexture) / 4));
		Color boardColor(1, 1, 1, 0.9f);
		board.setDiffuseColor(&boardColor);
		PointEntry *point = lobj_cast<PointEntry>(entry);
		Color textColor(1, 1, 1, 1);
		double now = LTimer::currentTimeValue();
		int code = 35;
		bool haveReport = false;
		bool loading = false;
		if (point) {
			LMap *report = point->dictionaryForReport(kReportTypeCurrent);
			loading = CWManager::instance()->isWeatherInternetStateOK() && point->downloadingReport(kReportTypeCurrent);
			if (report) {
				LObject *expired = report->objectForKey(kReportCommonDataExpired);
				if (expired && UMapParsingUtils::loadIntFromObject(expired) != 0)
					textColor = Color(0.7f, 0.7f, 0.7f, 0.6f);
				LObject *summary = report->objectForKey(kReportSummaryCode);
				if (summary)
					code = UMapParsingUtils::loadIntFromObject(summary);
				LObject *temperature = report->objectForKey(kReportCurrentTemperature);
				haveReport = true;
				if (temperature) {
					if (_temperaturesFont == 0) {
						_temperaturesFont = LRetain(_container->detachDataForLargeFont());
						_temperaturesFont->setDrawFlags(0);
						_temperaturesFont->_storesState = false;
					}
					LString *text = Utils::temperatureDescription(temperature);
					float width = _temperaturesFont->calcTextSize(text).width;
					_temperaturesFont->addCaptionAtPosition(
					    text, LMakePoint(rect->origin.x + rect->size.width - width - 8, rect->origin.y + 14),
					    &textColor);
				}
			}
		}
		int frame = 0;
		if (loading || !haveReport) {
			Color spinnerColor(0.6f, 0.8f, 1.0f, 0.9f);
			board.setDiffuseColor(&spinnerColor);
			double t = now * 0.699999988079071 + (double)(rect->origin.x + rect->origin.y) * 0.004;
			t -= floor(t);
			t = t >= 0 ? (t <= 0.999 ? t : 0.999) : 0;
			frame = (int)(t * 16);
		}
		Vector2 scale(0.25f, 0.25f);
		board.setTextureScale(&scale);
		Vector2 offset((float)(frame % 4) * scale.x, (float)(frame / 4) * scale.y);
		board.setTextureOffset(&offset);
		// The icons texture: 7 columns of 73x47 icons, by summary code.
		CBillboardSingleData icon;
		icon.setScreenRect(LMakeRect(rect->origin.x, rect->origin.y - 1, 73, 47));
		Vector2 iconScale(73.0f / _texWidth(iconsTexture), 47.0f / _texHeight(iconsTexture));
		icon.setTextureScale(&iconScale);
		Vector2 iconOffset((float)((code - 1) % 7) * iconScale.x, (float)((code - 1) / 7) * iconScale.y);
		icon.setTextureOffset(&iconOffset);
		icon.setDiffuseColor(&textColor);
		_weatherBillboards->addBillboard(&board);
		if (haveReport && !loading)
			_weatherIcons->addBillboard(&icon);
	}
	if (photos) {
		C2DTexture *photoTexture = _container->photoIconBBTexture();
		if (_photoIcons == 0)
			_photoIcons = _newBillboards(_container, photoTexture);
		CBillboardSingleData icon;
		icon.setScreenRect(LMakeRect((weather ? 122 : 0) + rect->origin.x, rect->origin.y + 12,
		                             _texWidth(photoTexture), _texHeight(photoTexture)));
		Color color(1, 1, 1, 0.9f);
		icon.setDiffuseColor(&color);
		_photoIcons->addBillboard(&icon);
	}
}

void EntriesBillboardsHolder::enumEntryWithRect(SmartBillboardSystem *, SmartBillboardData *entry, const LRect *rect)
{
	if (entry == 0 || !entry->getClass()->isSubclassOf(LF_CLASSOF(EntryBillboard)))
		return;
	if (_drawing)
		_printEntry((EntryBillboard *)entry, rect);
	else
		_renderEntry((EntryBillboard *)entry, rect);
}

// The billboard under the view point, in any copy of the map.
EntryBillboard *EntriesBillboardsHolder::hitTest(LPoint viewPoint, BaseScene *scene)
{
	if (_system == 0)
		return 0;
	BaseScene *oldScene = _scene;
	_scene = scene;
	PanelDrawIterator *it = scene->controller()->detachDrawIterator(0);
	PanelDrawIterator *oldIterator = _iterator;
	_iterator = it;
	SmartBillboardData *found = 0;
	while (it->next()) {
		_system->invalLayout();
		found = _system->findBillboard(this, viewPoint);
		if (found)
			break;
	}
	_iterator = oldIterator;
	delete it;
	_scene = oldScene;
	return (EntryBillboard *)found;
}

void EntriesBillboardsHolder::entryBecomesVisible(SmartBillboardData *entry)
{
	if (_drawing == 0 && (entry->kind() & 2)) {
		PointEntry *point = lobj_cast<PointEntry>(((EntryBillboard *)entry)->_entry);
		if (point)
			point->scheduleReports(1);
	}
}

void EntriesBillboardsHolder::entryBecomesInvisible(SmartBillboardData *entry)
{
	if (_drawing == 0 && (entry->kind() & 2)) {
		PointEntry *point = lobj_cast<PointEntry>(((EntryBillboard *)entry)->_entry);
		if (point)
			point->unscheduleReports(1);
	}
}

intptr_t EntriesBillboardsHolder::_entryWasChanged(char *)
{
	invalImagesBillboards();
	return 0;
}

// An entry's images changed: the photo billboards are made again.
intptr_t EntriesBillboardsHolder::_entryInfoWasChanged(char *args)
{
	LNotification *notification = *(LNotification **)args;
	LMap *info = notification ? notification->userInfo() : 0;
	if (info == 0)
		return 0;
	LObject *key = info->objectForKey(EntryInfo::kChangedKey);
	if (key && key->getClass()->isSubclassOf(LF_CLASSOF(LString)) && key->isEqualTo(kEntryInfoImagesKey))
		invalImagesBillboards();
	return 0;
}

void EntriesBillboardsHolder::_createEntriesBillboards()
{
	if (_system == 0) {
		_imagesDirty = true;
		_system = SmartBillboardSystem::_alloc()->init();
		SQLiteResultsContainer *results =
		    AtlasDatabase::instance()->searchWithRequest(SQL_DB_Strings::selectTopCitiesForWeatherOnMap(), (LMap *)0);
		int count = results ? results->count() : 0;
		char uid[26];
		float longitude, latitude;
		void *values[] = { uid, &longitude, &latitude };
		int types[] = { 3, 2, 2 };
		for (int i = 0; i < count; i++) {
			if (!results->loadValues(values, types, 3, i))
				continue;
			LString *string = LString::_alloc()->initWithCString(uid);
			GenericEntry *entry = GenericEntry::entryWithUID(string);
			string->release();
			if (entry == 0)
				continue;
			EntryBillboard *billboard = EntryBillboard::_alloc()->initWithEntry(entry, 2);
			_system->addEntry(billboard);
			LRelease(billboard);
		}
	}
	if (_imagesDirty) {
		_imagesDirty = false;
		LArray *uids = AtlasDatabase::instance()->allEntryUIDsWithImages();
		int count = uids ? uids->count() : 0;
		for (int i = 0; i < count; i++) {
			LString *uid = (LString *)uids->objectAtIndex(i);
			GenericEntry *entry = GenericEntry::entryWithUID(uid);
			if (entry == 0)
				continue;
			SmartBillboardData *billboard = _system->entryForKey(uid);
			if (billboard == 0) {
				billboard = EntryBillboard::_alloc()->initWithEntry(entry, 0);
				if (billboard == 0)
					continue;
				_system->addEntry(billboard);
				billboard->release();
			}
			billboard->setKind(billboard->kind() | 4);
		}
	}
}

// The weather billboards of a printout (kind 2, halved with 0x100).
void EntriesBillboardsHolder::print(BaseDrawing *drawing)
{
	LSize size = drawing->frameSize();
	_createEntriesBillboards();
	if (_system == 0)
		return;
	_system->setViewSize(size);
	_system->setVisibleKindsMask(0x102);
	_system->setAutohideKindsMask(2);
	BaseDrawing *oldDrawing = _drawing;
	_drawing = drawing;
	BaseScene *oldScene = _scene;
	_scene = drawing->scene();
	PanelDrawIterator *it = _scene->controller()->detachDrawIterator(&size);
	PanelDrawIterator *oldIterator = _iterator;
	_iterator = it;
	while (it->next()) {
		_system->invalLayout();
		_system->enumEntries(this, 0, 0, 0);
	}
	_iterator = oldIterator;
	delete it;
	_scene = oldScene;
	_drawing = oldDrawing;
}

void EntriesBillboardsHolder::_render(BaseScene *scene, CFrameDrawer *drawer)
{
	_releaseDatas(this);
	_createEntriesBillboards();
	if (_system == 0)
		return;
	if (_container->weatherBillboardTexture()) {
		_system->setViewSize(scene->holder()->virtualFrameSize());
		_system->setVisibleKindsMask(_visibleKinds);
		_system->setAutohideKindsMask(2);
		PanelDrawIterator *it = scene->controller()->detachDrawIterator(0);
		PanelDrawIterator *oldIterator = _iterator;
		_iterator = it;
		while (it->next()) {
			BaseScene *oldScene = _scene;
			_scene = scene;
			_system->invalLayout();
			_system->enumEntries(this, 0, 0, 1);
			_scene = oldScene;
		}
		_iterator = oldIterator;
		delete it;
	}
	CDrawableModelData *datas[] = { _markers, _weatherBillboards, _weatherIcons, _photoIcons, _namesFont,
		                        _temperaturesFont };
	for (CDrawableModelData *data : datas)
		if (data)
			drawer->renderDrawableData(data, 4);
}

void EntriesBillboardsHolder::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	_render((BaseScene *)scene, scene->frameDrawer());
}

void EntriesBillboardsHolder::render(BaseScene *scene, CFrameDrawer *drawer, unsigned int kinds)
{
	_visibleKinds = kinds;
	scene->storePropertyOwnerIfNeeded(this, 0);
	_render(scene, drawer);
}
