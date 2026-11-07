// BaseDrawing: printing. The scene's earth, clouds and space are rendered
// at twice the size into a bitmap (through LargeBBEmulator), then drawRect
// draws the bitmap and the overlays over it with Cocoa: the stars, the
// grid, the borders and their names, the points, the routes and the
// weather billboards.

#import <Cocoa/Cocoa.h>
#include "RenderLib.h"

LF_IMPLEMENT_CLASS(BaseDrawing, 0)

// An overlay font at the size, in light gray (or black for the shadow).
static CPRFont *_font(float size, bool shadow)
{
	Color color = shadow ? Color(0.0f, 0.0f, 0.0f, 1.0f) : Color(0.8f, 0.8f, 0.8f, 1.0f);
	return CPRFont::fontWithParams(size, CPRFont::kFontDefaultSystemFont, 0, color, 0);
}

BaseDrawing::BaseDrawing()
    : _scene(0), _controller(0), _flags(0), _frameSize(LZeroSize), _clipRect(LZeroRect), _drawRect(LZeroRect),
      _scale(0), _linesPath(0), _f90(0), _pointIcons(0), _primaryColor(0), _secondaryColor(0), _bitmap(0)
{
}

void BaseDrawing::dealloc()
{
	_releaseBitmap();
	_controller = 0;
	_releasePlatformSpecific();
	releaseAndClear(_scene);
	releaseAndClear(_pointIcons);
	LObject::dealloc();
}

unsigned int BaseDrawing::defaultPrintingFlags()
{
	unsigned int flags = Preferences::spaceEnabled() ? kPrintFlagEarth | kPrintFlagSpace : kPrintFlagEarth;
	if (Preferences::cloudsEnabled())
		flags |= kPrintFlagClouds;
	if (Preferences::dayNightIsEnabled())
		flags |= kPrintFlagDayNight;
	flags |= kPrintFlagRoutes;
	if (Preferences::showRouteLines())
		flags |= kPrintFlagRouteLines;
	if (Preferences::showRouteMarkers())
		flags |= kPrintFlagRouteMarkers;
	if (Preferences::showWeatherOnMapEnabled())
		flags |= kPrintFlagWeather;
	if (Preferences::showLocationNamesEnabled())
		flags |= kPrintFlagRouteNames;
	if (Preferences::gridEnabled())
		flags |= kPrintFlagGrid;
	if (Preferences::showAllBorders()) {
		LComparableSet *shapes = Preferences::shapesIDsToDraw();
		if (shapes->containsObject(CShapeManager::kShapeIDCountries))
			flags |= kPrintFlagCountryBorders | kPrintFlagCountryNames;
		if (shapes->containsObject(CShapeManager::kShapeIDStates))
			flags |= kPrintFlagStateBorders | kPrintFlagStateNames;
	}
	if (Preferences::showAllPoints())
		flags |= kPrintFlagPoints | kPrintFlagPointsOutline;
	return flags;
}

BaseDrawing *BaseDrawing::initWithFrameSizeScene(LSize size, BaseScene *scene, unsigned int flags)
{
	LObject::init();
	_f90 = 0;
	_frameSize = size;
	_clipRect = LMakeRect(0, 0, size.width, size.height);
	_drawRect = _clipRect;
	_flags = flags;
	_scene = LRetain(scene);
	_controller = scene ? scene->controller() : 0;
	_scene->viewProjMatrixForViewWithSize(&_viewProj, _frameSize);
	int mode = Preferences::mapMode();
	if (mode == 1)
		_flags &= ~kPrintFlagSpace;	// the flat map has no space
	else if (mode == 0 && (_flags & kPrintFlagEarth) == 0)
		_flags &= ~kPrintFlagClouds;
	if (_flags & (kPrintFlagEarth | kPrintFlagSpace | kPrintFlagClouds)) {
		bool controlsVisible = _scene->_controlsVisible;
		_scene->setControlsVisibility(0);
		LargeBBEmulator *emulator = LAutorelease(LargeBBEmulator::_alloc()->initWithHolder(_scene->holder()));
		emulator->setBufferParametersSimple(LMakeSize(_frameSize.width * 2, _frameSize.height * 2), _frameSize);
		LSize bufferSize = emulator->size();
		void *data = _createBitmapWithParams((int)bufferSize.width, (int)bufferSize.height,
		                                     (int)emulator->bufferRowSize());
		if (data) {
			RenderOptions *options = LRetain(scene->renderOptions());
			scene->setRenderOptions(LAutorelease(RenderOptionsForPrinting::_alloc()->initWithFlags(_flags)));
			bool grabbing = sLargeBBGrabbingIsActive;
			sLargeBBGrabbingIsActive = true;
			emulator->startGrabbing();
			emulator->grabSnapshot(data, _bitmapRowSize());
			scene->setRenderOptions(options);
			LRelease(options);
			emulator->stopGrabbing();
			sLargeBBGrabbingIsActive = grabbing;
		}
		_scene->setControlsVisibility(controlsVisible);
	}
	// Nothing over the photos preview.
	if (_bitmap && _controller->imagesPreviewVisibility() > 0.01f)
		_flags = 0;
	return this;
}

LPoint BaseDrawing::viewPointFromGeoPoint(const LPoint &geoPoint, PanelDrawIterator *, unsigned char *visible)
{
	Vector3 world;
	_scene->scenePointFromGeoPoint(&world, &geoPoint);
	Vector4 p;
	FHVec3Transform(&p, &world, &_viewProj);
	float w = p.w < 0 ? -p.w : p.w;
	float x = p.x / w, y = p.y / w, z = p.z / w;
	if (visible)
		*visible = !(z < -1 || z > 1 || x < -1 || x > 1 || y < -1 || y > 1);
	return LMakePoint((x + 1) * _frameSize.width * 0.5f, (y + 1) * _frameSize.height * 0.5f);
}

void BaseDrawing::drawRect(LRect rect, void *)
{
	if (rect.origin.x < 0) {
		rect.size.width -= rect.origin.x;
		rect.origin.x = 0;
	}
	if (rect.origin.y < 0) {
		rect.size.height -= rect.origin.y;
		rect.origin.y = 0;
	}
	rect = LMakeRect(floorf(rect.origin.x), floorf(rect.origin.y), floorf(rect.size.width), floorf(rect.size.height));
	_drawRect = rect;
	adjustClippingsToViewRect(rect);
	if (_bitmap)
		drawMapInRect(rect);
	if (_flags & kPrintFlagSpace) {
		if (Preferences::constellationsEnabled())
			drawConstellationsInRect(rect);
		if (Preferences::showPlanetsNamesEnabled() && Preferences::planetsEnabled())
			drawPlanetsNamesInRect(rect);
		if (Preferences::showConstellationsNamesEnabled() && Preferences::constellationsEnabled())
			drawConstellationsNamesInRect(rect);
	}
	if (_flags & kPrintFlagGrid)
		drawGridInRect(rect);
	if (_flags & kPrintFlagStateBorders)
		drawStateBordersInRect(rect);
	if (_flags & kPrintFlagCountryBorders)
		drawCountryBordersInRect(rect);
	if (_flags & kPrintFlagPoints)
		drawAllLocationsInRect(rect);
	if (_flags & kPrintFlagRoutes)
		drawRoutes();
	if (_flags & kPrintFlagWeather)
		drawWeatherBillboardsInRect(rect);
}

float BaseDrawing::scale()
{
	return _scale;
}

void BaseDrawing::setScale(float scale)
{
	_scale = scale;
}

float BaseDrawing::cityRadius()
{
	return _controller->scale() * 3.0f + 1.0f;
}

float BaseDrawing::cityRadiusForWeatherBB()
{
	return 3.0f;
}

float BaseDrawing::routeMarkerRadius()
{
	return Preferences::markersScale() * 4.0f + 2.0f;
}

Color BaseDrawing::cityColor()
{
	return Color(1.0f, 1.0f, 0.0f, 1.0f);
}

Color BaseDrawing::constellationsColor()
{
	return Color(0.7f, 0.9f, 1.0f, 0.75f);
}

Color BaseDrawing::lineColor()
{
	return Color(0.0f, 0.0f, 0.0f, 1.0f);
}

Color BaseDrawing::gridColor()
{
	return Color(0.7f, 0.8f, 0.9f, 1.0f);
}

Color BaseDrawing::countryBorderColor()
{
	return Color(0.9f, 0.8f, 0.6f, 1.0f);
}

CPRFont *BaseDrawing::gridMarkerFont()
{
	return CPRFont::fontWithParams(10.0f, CPRFont::kFontDefaultSystemFont, 0, gridColor(), 0);
}

CPRFont *BaseDrawing::routeCaptionFont()
{
	return _font(10.0f, false);
}

CPRFont *BaseDrawing::routeCaptionShadowFont()
{
	return _font(10.0f, true);
}

CPRFont *BaseDrawing::planetNameFont()
{
	return CPRFont::fontWithParams(10.0f, CPRFont::kFontDefaultSystemFont, 0, Color(0.8f, 0.8f, 0.6f, 1.0f), 0);
}

CPRFont *BaseDrawing::planetNameShadowFont()
{
	return _font(10.0f, true);
}

CPRFont *BaseDrawing::constellationNameFont()
{
	return CPRFont::fontWithParams(10.0f, CPRFont::kFontDefaultSystemFont, 0, Color(0.3f, 0.5f, 0.8f, 1.0f), 0);
}

CPRFont *BaseDrawing::constellationNameShadowFont()
{
	return _font(10.0f, true);
}

// The region names grow with the zoom.
static float _regionFontSize(AtlasPanelController *controller)
{
	float size = controller->scale() * 4.0f + 8.0f;
	return size < 1.0f ? 1.0f : size;
}

CPRFont *BaseDrawing::countryNameFont()
{
	return _font(_regionFontSize(_controller), false);
}

CPRFont *BaseDrawing::countryNameShadowFont()
{
	return _font(_regionFontSize(_controller), true);
}

CPRFont *BaseDrawing::stateNameFont()
{
	return _font(_regionFontSize(_controller), false);
}

CPRFont *BaseDrawing::stateNameShadowFont()
{
	return _font(_regionFontSize(_controller), true);
}

CPRFont *BaseDrawing::temperatureFont()
{
	return _font(12.0f, false);
}

CPRFont *BaseDrawing::temperatureShadowFont()
{
	return _font(12.0f, true);
}

void BaseDrawing::adjustClippingsToViewRect(LRect rect)
{
	_clipRect = rect;
}

float BaseDrawing::degreesGridStep(unsigned char longitude)
{
	return _controller->gridStepFromScale(longitude);
}

void BaseDrawing::drawMapInRect(LRect rect)
{
	_renderBitmapInRect(rect);
}

void BaseDrawing::drawWeatherBillboardsInRect(LRect)
{
	EntriesBillboardsHolder *billboards = _scene->objectsContainer()->entriesBillboards();
	if (billboards)
		billboards->print(this);
}

// The current weather of a city: its dot and name, an underline, the
// weather icon and the temperature.
void BaseDrawing::_drawEntryBillboard(GenericEntry *entry, const LRect *rect, PanelDrawIterator *iterator)
{
	if (entry == 0 || !entry->getClass()->isSubclassOf(LF_CLASSOF(PointEntry)))
		return;
	PointEntry *point = (PointEntry *)entry;
	float radius = cityRadiusForWeatherBB();
	LPoint geoPoint = point->geoPoint(0);
	unsigned char visible = 0;
	LPoint p = viewPointFromGeoPoint(geoPoint, iterator, &visible);
	if (!visible || p.x < rect->origin.x || p.y < rect->origin.y || p.x > rect->origin.x + rect->size.width ||
	    p.y > rect->origin.y + rect->size.height)
		return;
	if (!LIntersectsRect(LMakeRect(p.x, p.y, 100.0f, 50.0f), _drawRect))
		return;
	CWManager *manager = (CWManager *)LF_CLASSOF(CWManager)->singleton();
	if (manager->isWeatherInternetStateOK() && point->downloadingReport((TReportType)1))
		return;
	LMap *report = point->dictionaryForReport((TReportType)1);
	if (report == 0)
		return;
	LObject *expired = report->objectForKey(kReportCommonDataExpired);
	if (expired)
		UMapParsingUtils::loadIntFromObject(expired);	// unused
	_setPrimaryColor(lineColor());
	_setSecondaryColor(cityColor());
	_drawFilledEllipseInRect(LMakeRect(p.x - radius, p.y - radius, radius + radius, radius + radius), 1,
	                         radius * 0.4f);
	LString *name = entry->ownName(0);
	CPRFont *font = routeCaptionFont();
	CPRFont *shadow = routeCaptionShadowFont();
	LPoint at = LMakePoint(p.x + radius + 1.0f, p.y);
	_drawString(shadow, name, at);
	at.y += 0.5f;
	at.x -= 1.0f;
	_drawString(font, name, at);
	at.y += font->size() + 1.0f;
	_setPrimaryColor(Color(0.0f, 0.0f, 0.0f, 0.6f));
	_beginDrawingLines(1.0f);
	_drawLine(LMakePoint(at.x - 1.0f, at.y), LMakePoint(at.x + 50.0f, at.y));
	_endDrawingLines();
	_setPrimaryColor(Color(1.0f, 1.0f, 1.0f, 0.8f));
	_beginDrawingLines(1.0f);
	_drawLine(LMakePoint(at.x - 2.0f, at.y + 0.5f), LMakePoint(at.x + 49.0f, at.y + 0.5f));
	_endDrawingLines();
	int icon = 35;
	LObject *code = report->objectForKey(kReportSummaryCode);
	if (code)
		icon = UMapParsingUtils::loadIntFromObject(code);
	LArray *images = manager->weatherImagesForIconAtIndex(icon);
	LImage *image = images && images->count() > 0 ? (LImage *)images->objectAtIndex(0) : 0;
	if (image) {
		LSize size = image->size();
		_drawImageInRectFromRect(image, LMakeRect(at.x, at.y + 1.0f, size.width * 0.5f, size.height * 0.5f),
		                         LMakeRect(LZeroRect.origin.x, LZeroRect.origin.y, size.width, size.height));
	}
	at.x += 24.0f;
	at.y += 1.0f;
	LObject *temperature = report->objectForKey(kReportCurrentTemperature);
	if (temperature) {
		LString *text = Utils::temperatureShortDescription(temperature);
		CPRFont *temperatureText = temperatureFont();
		_drawString(temperatureShadowFont(), text, at);
		at.y += 0.5f;
		at.x -= 1.0f;
		_drawString(temperatureText, text, at);
	}
}

LImage *BaseDrawing::_pointIconForKey(LString *key)
{
	if (_pointIcons == 0) {
		_pointIcons = LMap::_alloc()->init();
		for (int i = 0; i < 12; i++) {
			LString *iconKey = sPointIconKeys[i];
			if (iconKey == 0)
				continue;
			LString *path = LString::stringWithFormat("INTERFACE\\Entries\\Printing\\%@.png", iconKey);
			LImage *image = LImage::_alloc()->initWithContentsOfFile(path);
			if (image) {
				_pointIcons->setObjectForKey(image, iconKey);
				image->release();
			}
		}
	}
	return (LImage *)_pointIcons->objectForKey(key);
}

void BaseDrawing::_drawShapeBordersWithType(LString *shapeID, LRect, unsigned char withNames)
{
	LString *geoType = shapeID == CShapeManager::kShapeIDStates ? kGeoTypeState : kGeoTypeCountry;
	LString *request = SQL_DB_Strings::selectAllObjectUIDsWithGeotypes(
	    (LArray *)LArray::arrayWithObjects(geoType, (LObject *)0), kRepTypeShapeIndices);
	LArray *rows = ((AtlasDatabase *)LF_CLASSOF(AtlasDatabase)->singleton())->simpleSearchWithRequest(request);
	int count = rows ? rows->count() : 0;
	_setPrimaryColor(countryBorderColor());
	CShapeManager *manager = (CShapeManager *)LF_CLASSOF(CShapeManager)->singleton();
	for (int i = 0; i < count; i++) {
		LString *uid = (LString *)((LMap *)rows->objectAtIndex(i))->objectForKey(kColumnNameObjectUID);
		CShapeContainer *shape = manager->shapeForObject(uid, shapeID);
		if (shape)
			drawShapeBorder(shape);
	}
	if (withNames) {
		for (int i = 0; i < count; i++) {
			LString *uid = (LString *)((LMap *)rows->objectAtIndex(i))->objectForKey(kColumnNameObjectUID);
			CShapeContainer *shape = manager->shapeForObject(uid, shapeID);
			if (shape)
				drawShapeName(shape);
		}
	}
}

void BaseDrawing::drawStateBordersInRect(LRect rect)
{
	_drawShapeBordersWithType(CShapeManager::kShapeIDStates, rect, (_flags & kPrintFlagStateNames) != 0);
}

void BaseDrawing::drawCountryBordersInRect(LRect rect)
{
	_drawShapeBordersWithType(CShapeManager::kShapeIDCountries, rect, (_flags & kPrintFlagCountryNames) != 0);
}

void BaseDrawing::_drawImagesInRect(SImageDrawData *images, int count, LRect rect)
{
	PanelDrawIterator *it = _controller->detachDrawIterator(&_frameSize);
	while (it->next()) {
		for (int i = 0; i < count; i++) {
			SImageDrawData &d = images[i];
			if (d.image == 0)
				continue;
			unsigned char visible = 0;
			LPoint p = viewPointFromGeoPoint(d.geoPoint, it, &visible);
			if (visible && rect.origin.x <= p.x && rect.origin.y <= p.y && p.x <= rect.origin.x + rect.size.width &&
			    p.y <= rect.origin.y + rect.size.height) {
				LSize size = d.image->size();
				_drawImageInRectFromRect(
				    d.image,
				    LMakeRect(p.x - d.size.width * 0.5f, p.y - d.size.height * 0.5f, d.size.width, d.size.height),
				    LMakeRect(LZeroRect.origin.x, LZeroRect.origin.y, size.width, size.height));
			}
		}
	}
	delete it;
}

// The other points as dots; the Apple stores and the capitals as icons.
void BaseDrawing::drawAllLocationsInRect(LRect rect)
{
	AtlasDatabase *db = (AtlasDatabase *)LF_CLASSOF(AtlasDatabase)->singleton();
	const SAtlasPoint *points = (const SAtlasPoint *)db->allPointsArray()->bytes();
	float radius = cityRadius();
	bool appleStores = Preferences::showAppleStorePoints();
	bool capitals = Preferences::showCapitalPoints();
	if (Preferences::showOtherPoints()) {
		LData *others = db->allOtherPointsIndices();
		int count = (int)(others->length() >> 2);
		const int *index = (const int *)others->bytes();
		float side = (radius + radius * 0.5f) * 2;
		bool outline = (_flags & kPrintFlagPointsOutline) != 0;
		_setPrimaryColor(lineColor());
		PanelDrawIterator *it = _controller->detachDrawIterator(&_frameSize);
		while (it->next()) {
			for (int i = 0; i < count; i++) {
				const SAtlasPoint &point = points[index[i]];
				unsigned char visible = 0;
				LPoint p = viewPointFromGeoPoint(point.geoPoint, it, &visible);
				if (!visible || p.x < rect.origin.x || p.y < rect.origin.y || p.x > rect.origin.x + rect.size.width ||
				    p.y > rect.origin.y + rect.size.height)
					continue;
				// Drawn as icons below.
				char kind = (char)point.kind;
				if (appleStores && (kind == 8 || kind == 10 || kind == 11))
					continue;
				if (capitals && (unsigned char)(kind - 2) < 2)
					continue;
				_setSecondaryColor(Color(sPointColors[(int)kind]));
				_drawFilledEllipseInRect(LMakeRect(p.x - radius, p.y - radius, side, side), outline, radius * 0.5f);
			}
		}
		delete it;
	}
	if (capitals || appleStores) {
		LData *storeIndices = db->allAppleStoresIndices();
		LData *capitalIndices = db->allCapitalsIndices();
		int stores = appleStores ? (int)(storeIndices->length() >> 2) : 0;
		int caps = capitals ? (int)(capitalIndices->length() >> 2) : 0;
		SImageDrawData *images = new SImageDrawData[stores + caps];
		SImageDrawData *next = images;
		float size = (powf((radius - 1.0f) / 6.0f + 1.0f, 3.0f) + 0.7f) * 9.0f;
		const int *index = (const int *)storeIndices->bytes();
		for (int i = 0; i < stores; i++) {
			const SAtlasPoint &point = points[index[i]];
			LImage *icon = _pointIconForKey(sPointIconKeys[(signed char)point.kind]);
			if (icon) {
				next->image = icon;
				next->geoPoint = point.geoPoint;
				next->size = LMakeSize(size, size);
				next++;
			}
		}
		index = (const int *)capitalIndices->bytes();
		for (int i = 0; i < caps; i++) {
			const SAtlasPoint &point = points[index[i]];
			LImage *icon = _pointIconForKey(sPointIconKeys[(signed char)point.kind]);
			if (icon) {
				next->image = icon;
				next->geoPoint = point.geoPoint;
				next->size = LMakeSize(size, size);
				next++;
			}
		}
		if (next != images)
			_drawImagesInRect(images, (int)(next - images), rect);
		delete[] images;
	}
}

// The current route: its lines, the shapes of its regions, the markers and
// the point names.
void BaseDrawing::drawRoutes()
{
	CRoutePointsContainer *routes = _scene->objectsContainer()->routePointsContainer();
	LData *linePoints = routes->routeLinesPoints();
	LData *lineIndices = routes->routeLinesInds();
	const SRoutePoint *lines = linePoints ? (const SRoutePoint *)linePoints->bytes() : 0;
	const uint32_t *pairs = 0;
	int pairCount = 0;
	if (lineIndices) {
		pairs = (const uint32_t *)lineIndices->bytes();
		pairCount = (int)(lineIndices->length() >> 3);
	}
	RouteObject *route = routes->_route;
	const SRoutePoint *routePoints = 0;
	int pointCount = 0;
	if (route) {
		LData *data = route->pointsToHighlight();
		if (data) {
			routePoints = (const SRoutePoint *)data->bytes();
			pointCount = (int)(data->length() / sizeof(SRoutePoint));
		}
	}
	bool names = (_flags & kPrintFlagRouteNames) != 0;
	bool markers = (_flags & kPrintFlagRouteMarkers) != 0;
	_setPrimaryColor(lineColor());
	if (_flags & kPrintFlagRouteLines) {
		_beginDrawingLines(1.0f);
		PanelDrawIterator *it = _controller->detachDrawIterator(&_frameSize);
		while (it->next()) {
			for (int i = 0; i < pairCount; i++) {
				unsigned char visibleA = 0, visibleB = 0;
				LPoint a = viewPointFromGeoPoint(lines[pairs[i * 2]].geoPoint, it, &visibleA);
				LPoint b = viewPointFromGeoPoint(lines[pairs[i * 2 + 1]].geoPoint, it, &visibleB);
				if (visibleA || visibleB)
					_drawLine(a, b);
			}
		}
		_endDrawingLines();
		delete it;
	}
	LArray *shapes = routes->shapesToRender();
	int shapeCount = shapes ? shapes->count() : 0;
	for (int i = 0; i < shapeCount; i++)
		drawShapeBorder((CShapeContainer *)shapes->objectAtIndex(i));
	if (names)
		for (int i = 0; i < shapeCount; i++)
			drawShapeName((CShapeContainer *)shapes->objectAtIndex(i));
	if (!names && !markers)
		return;
	float radius = routeMarkerRadius();
	CPRFont *font = routeCaptionFont();
	CPRFont *shadow = routeCaptionShadowFont();
	PanelDrawIterator *it = _controller->detachDrawIterator(&_frameSize);
	while (it->next()) {
		for (int i = 0; i < pointCount; i++) {
			const SRoutePoint &point = routePoints[i];
			unsigned char visible = 0;
			LPoint p = viewPointFromGeoPoint(point.geoPoint, it, &visible);
			if (!visible)
				continue;
			if (markers) {
				_setSecondaryColor(point.color);
				_drawFilledEllipseInRect(LMakeRect(p.x - radius, p.y - radius, radius + radius, radius + radius), 1,
				                         1.0f);
			}
			LString *title = route ? route->titleForPointAtIndex(i) : 0;
			LPoint at = LMakePoint(radius + 1.0f + p.x, p.y - shadow->size());
			if (title && names) {
				_drawString(shadow, title, at);
				at.y += 0.5f;
				at.x -= 1.0f;
				_drawString(font, title, at);
			}
		}
	}
	delete it;
}

void BaseDrawing::drawShapeBorder(CShapeContainer *shape)
{
	CPolysContainer *polys = lobj_cast<CPolysContainer>(shape);
	if (polys == 0)
		return;
	LArray *paths = polys->allPaths();
	if (paths == 0)
		return;
	const LRect *bounds = polys->_bounds ? (const LRect *)polys->_bounds->bytes() : 0;
	int count = paths->count();
	PanelDrawIterator *it = _controller->detachDrawIterator(&_frameSize);
	while (it->next()) {
		for (int i = 0; i < count; i++) {
			if (!isGeoRectVisible(bounds[i]))
				continue;
			LObjToNSObjWrapper *path = _transformGeoPath((LObjToNSObjWrapper *)paths->objectAtIndex(i), it);
			_drawShapeBorderInRect(path, _clipRect);
			_releasePath(path);
		}
	}
	delete it;
}

// The region's name at the center of its largest polygon.
void BaseDrawing::drawShapeName(CShapeContainer *shape)
{
	CPolysContainer *polys = lobj_cast<CPolysContainer>(shape);
	if (polys == 0)
		return;
	LRect bounds = polys->largestPathBoundGeoRect();
	LPoint center = LMakePoint(bounds.size.width * 0.5f + bounds.origin.x, bounds.size.height * 0.5f + bounds.origin.y);
	PanelDrawIterator *it = _controller->detachDrawIterator(&_frameSize);
	while (it->next()) {
		unsigned char visible = 0;
		LPoint at = viewPointFromGeoPoint(center, it, &visible);
		if (!visible)
			break;
		CPRFont *font = countryNameFont();
		CPRFont *shadow = countryNameShadowFont();
		GenericEntry *entry = GenericEntry::entryWithUID(shape->_objectUID);
		LString *name = entry ? entry->shortCaption(0) : 0;
		if (name == 0)
			continue;
		LSize size = font->stringSize(name);
		at.x += size.width * -0.5f;
		at.y += size.height * -0.5f;
		_drawString(shadow, name, at);
		at.y += 0.5f;
		at.x -= 1.0f;
		_drawString(font, name, at);
	}
	delete it;
}

// ---------------------------------------------------------------------------
// Cocoa.

void BaseDrawing::_releasePlatformSpecific()
{
	[(NSColor *)_primaryColor release];
	_primaryColor = 0;
	[(NSColor *)_secondaryColor release];
	_secondaryColor = 0;
	[(NSBezierPath *)_linesPath release];
	_linesPath = 0;
	[(NSBitmapImageRep *)_bitmap release];
	_bitmap = 0;
}

void BaseDrawing::_setPrimaryColor(Color color)
{
	[(NSColor *)_primaryColor release];
	_primaryColor = [[NSColor colorWithCalibratedRed:color.r green:color.g blue:color.b alpha:color.a] retain];
}

void BaseDrawing::_setSecondaryColor(Color color)
{
	[(NSColor *)_secondaryColor release];
	_secondaryColor = [[NSColor colorWithCalibratedRed:color.r green:color.g blue:color.b alpha:color.a] retain];
}

// An RGBX bitmap the scene is grabbed into; its pixels, or 0.
void *BaseDrawing::_createBitmapWithParams(int width, int height, int rowBytes)
{
	_bitmap = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:0
	                                                  pixelsWide:width
	                                                  pixelsHigh:height
	                                               bitsPerSample:8
	                                             samplesPerPixel:3
	                                                    hasAlpha:NO
	                                                    isPlanar:NO
	                                              colorSpaceName:NSDeviceRGBColorSpace
	                                                 bytesPerRow:rowBytes
	                                                bitsPerPixel:32];
	return [(NSBitmapImageRep *)_bitmap bitmapData];
}

int BaseDrawing::_bitmapRowSize()
{
	return (int)[(NSBitmapImageRep *)_bitmap bytesPerRow];
}

void BaseDrawing::_releaseBitmap()
{
	[(NSBitmapImageRep *)_bitmap release];
	_bitmap = 0;
}

void BaseDrawing::_beginDrawingLines(float width)
{
	[(NSBezierPath *)_linesPath release];
	_linesPath = [[NSBezierPath bezierPath] retain];
	[(NSBezierPath *)_linesPath setLineWidth:width];
	[(NSColor *)_primaryColor set];
}

void BaseDrawing::_drawLineTo(LPoint to)
{
	[(NSBezierPath *)_linesPath lineToPoint:NSMakePoint(to.x, to.y)];
}

void BaseDrawing::_drawLine(LPoint from, LPoint to)
{
	[(NSBezierPath *)_linesPath moveToPoint:NSMakePoint(from.x, from.y)];
	[(NSBezierPath *)_linesPath lineToPoint:NSMakePoint(to.x, to.y)];
}

void BaseDrawing::_endDrawingLines()
{
	[(NSBezierPath *)_linesPath stroke];
	[(NSBezierPath *)_linesPath release];
	_linesPath = 0;
}

// The part of the bitmap behind `rect` (the bitmap is the frame at its
// own resolution), drawn into it.
void BaseDrawing::_renderBitmapInRect(LRect rect)
{
	NSBitmapImageRep *bitmap = (NSBitmapImageRep *)_bitmap;
	if (bitmap == 0)
		return;
	NSBitmapImageRep *part;
	if (_frameSize.width == rect.size.width && _frameSize.height == rect.size.height) {
		part = [bitmap retain];
	} else {
		NSSize size = [bitmap size];
		float bw = (float)size.width, bh = (float)size.height;
		float w = rect.size.width * bw / _frameSize.width;
		float x = rect.origin.x * bw / _frameSize.width;
		float h = rect.size.height * bh / _frameSize.height;
		float y = (_frameSize.height - rect.origin.y - rect.size.height) * bh / _frameSize.height;
		if (bw < w)
			w = bw;
		if (bh < h)
			h = bh;
		if (bw <= w + x)
			x = floorf(bw - w - 0.9999f);
		if (bh <= h + y)
			y = floorf(bh - h - 0.9999f);
		if (x <= 0)
			x = 0;
		if (y <= 0)
			y = 0;
		size_t rowBytes = (size_t)(int)w * 4;
		part = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:0
		                                               pixelsWide:(int)w
		                                               pixelsHigh:(int)h
		                                            bitsPerSample:8
		                                          samplesPerPixel:3
		                                                 hasAlpha:NO
		                                                 isPlanar:NO
		                                           colorSpaceName:NSDeviceRGBColorSpace
		                                              bytesPerRow:rowBytes
		                                             bitsPerPixel:32];
		unsigned char *from = [bitmap bitmapData];
		unsigned char *to = [part bitmapData];
		NSInteger fromRowBytes = [bitmap bytesPerRow];
		if (from && to) {
			from += (int)y * fromRowBytes + (int)x * 4;
			for (int row = 0; row < (int)h; row++) {
				memcpy(to, from, rowBytes);
				to += rowBytes;
				from += fromRowBytes;
			}
		}
	}
	[part drawInRect:NSMakeRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height)];
	[part release];
}

void BaseDrawing::_drawShapeBorderInRect(LObjToNSObjWrapper *wrapper, LRect rect)
{
	NSBezierPath *path = (NSBezierPath *)wrapper->nsObject();
	if (path == nil || [path elementCount] <= 0)
		return;
	NSRect clip = NSMakeRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
	if (!NSIntersectsRect(clip, [path bounds]))
		return;
	[NSGraphicsContext saveGraphicsState];
	[NSBezierPath clipRect:clip];
	[(NSColor *)_primaryColor set];
	[path setLineJoinStyle:NSLineJoinStyleRound];
	[path stroke];
	[NSGraphicsContext restoreGraphicsState];
}

void BaseDrawing::_drawFilledEllipseInRect(LRect rect, unsigned char stroke, float lineWidth)
{
	NSBezierPath *path =
	    [NSBezierPath bezierPathWithOvalInRect:NSMakeRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height)];
	[(NSColor *)_secondaryColor set];
	[path fill];
	if (stroke) {
		[(NSColor *)_primaryColor set];
		[path setLineWidth:lineWidth];
		[path stroke];
	}
}

// `fromRect` is in the image's top-down pixels.
void BaseDrawing::_drawImageInRectFromRect(LImage *image, LRect rect, LRect fromRect)
{
	LSize size = image->size();
	NSImage *nsImage = (NSImage *)image->NSImage();
	if (nsImage == nil) {
		nsImage = [[NSImage alloc] initWithSize:NSMakeSize((int)size.width, size.height)];
		[nsImage lockFocus];
		CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
		CGContextDrawImage(context, CGRectMake(0, 0, size.width, size.height), image->CGImage());
		[nsImage unlockFocus];
		image->setNSImage(nsImage);
		[nsImage release];
	}
	[nsImage drawInRect:NSMakeRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height)
	           fromRect:NSMakeRect(fromRect.origin.x, size.height - fromRect.origin.y - fromRect.size.height,
	                               fromRect.size.width, fromRect.size.height)
	          operation:NSCompositingOperationSourceOver
	           fraction:1.0];
}

void BaseDrawing::_drawString(CPRFont *font, LString *string, const LPoint &point, LSize *size)
{
	font->drawString(string, point, size);
}

// The path in view coordinates; the parts out of view are skipped.
LObjToNSObjWrapper *BaseDrawing::_transformGeoPath(LObjToNSObjWrapper *wrapper, PanelDrawIterator *iterator)
{
	NSBezierPath *geoPath = (NSBezierPath *)wrapper->nsObject();
	NSBezierPath *path = [NSBezierPath bezierPath];
	NSInteger count = [geoPath elementCount];
	bool wasVisible = false;
	for (NSInteger i = 0; i < count; i++) {
		NSPoint points[3];
		NSBezierPathElement element = [geoPath elementAtIndex:i associatedPoints:points];
		unsigned char visible = 0;
		LPoint p = viewPointFromGeoPoint(LMakePoint((float)points[0].x, (float)points[0].y), iterator, &visible);
		NSPoint at = NSMakePoint(p.x, p.y);
		if (element == NSBezierPathElementMoveTo) {
			if (visible)
				[path moveToPoint:at];
		} else if (element == NSBezierPathElementLineTo && (wasVisible || visible)) {
			if (wasVisible)
				[path lineToPoint:at];
			else
				[path moveToPoint:at];
		}
		wasVisible = visible;
	}
	return LObjToNSObjWrapper::wrapperWithNSObject(path);
}
