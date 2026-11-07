// CRoutePointsContainer: the scene's current route. Its lines follow great
// circles between the route's points, split where they cross the date line;
// its point markers are a particle system; its shapes and their captions are
// drawn by the scene. The flight along the route moves over Hermite segments
// between the points, at 1200 km a second.

#include "RenderLib.h"

#include <algorithm>
#include <vector>

LF_IMPLEMENT_CLASS(CRoutePointsContainer, 0)
LF_SELECTOR(CRoutePointsContainer, _routeObjectWasChanged);
LF_SELECTOR(CRoutePointsContainer, _updateCaptionLayouts);

CRoutePointsContainer::CRoutePointsContainer()
    : _window(0), _scene(0), _linesProfile(0), _linesMesh(0), _linesDatas(0), _markers(0), _route(0), _linePoints(0),
      _lineIndices(0), _captionTitles(0), _captionPoints(0), _flySegments(0), _savedRoute(0)
{
}

CRoutePointsContainer *CRoutePointsContainer::initWithWindowInterface(IWindowInterface *window)
{
	if (LObject::init()) {
		_window = window;
		LNotificationCenter::center()->addObserver(this, "_updateCaptionLayouts", kUpdateLayoutCaptionsNotification,
		                                           0);
	}
	return this;
}

void CRoutePointsContainer::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	releaseAndClear(_linePoints);
	releaseAndClear(_flySegments);
	releaseAndClear(_lineIndices);
	if (_linesMesh) {
		_linesMesh->dataSourceWillDie();
		releaseAndClear(_linesMesh);
	}
	releaseAndClear(_linesDatas);
	releaseAndClear(_linesProfile);
	releaseAndClear(_markers);
	releaseAndClear(_route);
	releaseAndClear(_savedRoute);
	releaseAndClear(_captionTitles);
	releaseAndClear(_captionPoints);
	_scene = 0;
	_window = 0;
	LObject::dealloc();
}

void CRoutePointsContainer::_releaseVisibleCaptionsData()
{
	releaseAndClear(_captionTitles);
	releaseAndClear(_captionPoints);
}

void CRoutePointsContainer::_releaseRouteDependedDatas()
{
	releaseAndClear(_linePoints);
	releaseAndClear(_lineIndices);
	releaseAndClear(_flySegments);
}

void CRoutePointsContainer::_releaseRouteDependedDrawables()
{
	if (_linesMesh) {
		_linesMesh->dataSourceWillDie();
		releaseAndClear(_linesMesh);
	}
	releaseAndClear(_linesDatas);
	releaseAndClear(_markers);
}

void CRoutePointsContainer::setScene(BaseScene *scene)
{
	if (_scene != scene)
		_releaseRouteDependedDrawables();
	_scene = scene;
}

LArray *CRoutePointsContainer::shapesToRender()
{
	return _route ? _route->shapesToHighlight() : 0;
}

// The scene state keeps only whether there was a route; the scene draws the
// route on restore only then.
void CRoutePointsContainer::storeSceneStateInStream(LData *stream, unsigned int)
{
	_hasRoute = _route && _route->itemsCount() > 0;
	storeIntInStream(_hasRoute, stream);
}

const void *CRoutePointsContainer::restoreSceneStateFromData(const void *data, unsigned int)
{
	int hasRoute = 0;
	data = loadIntFromStream(data, &hasRoute);
	_hasRoute = hasRoute != 0;
	return data;
}

void CRoutePointsContainer::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	if (_hasRoute)
		renderRoutes(scene->frameDrawer());
}

unsigned int CRoutePointsContainer::numPrimitivesForMesh(CMesh *)
{
	return _lineIndices ? _lineIndices->length() / 8 : 0;
}

unsigned int CRoutePointsContainer::numVerticesForMesh(CMesh *)
{
	return _linePoints ? _linePoints->length() / sizeof(SRoutePoint) : 0;
}

intptr_t CRoutePointsContainer::_routeObjectWasChanged(char *)
{
	_releaseRouteDependedDatas();
	_releaseRouteDependedDrawables();
	_releaseVisibleCaptionsData();
	LNotificationCenter::center()->postNotification(kRecordingMustBeStoppedNotification, this, 0);
	return 0;
}

// A marker at each route point, in its color.
void CRoutePointsContainer::_createPointMarkersSystem()
{
	releaseAndClear(_markers);
	if (_route == 0)
		return;
	LData *points = _route->pointsToHighlight();
	if (points == 0)
		return;
	const SRoutePoint *point = (const SRoutePoint *)points->bytes();
	int count = points->length() / sizeof(SRoutePoint);
	CParticleSystem *system = _scene->routePointsParticleSystem(count);
	CParticleSystemData *data = 0;
	if (system)
		data = lobj_cast<CParticleSystemData>(system->detachModelDataInstance());
	_markers = LRetain(data);
	if (_markers == 0)
		return;
	CParticleSystemData::SStoredParticle *particle = (CParticleSystemData::SStoredParticle *)_markers->_particles;
	for (int i = 0; i < count; i++, point++, particle++) {
		_scene->scenePointFromGeoPoint(&particle->position, &point->geoPoint);
		particle->color = point->color;
	}
	_markers->invalidateParticles();
}

void CRoutePointsContainer::setCurrentRoute(RouteObject *route)
{
	if (_route == route)
		return;
	LNotificationCenter *center = LNotificationCenter::center();
	center->removeObserver(this, RouteObject::kRouteObjectItemCountDidChangeNotification, kLock);
	center->removeObserver(this, RouteObject::kRouteObjectAttributesDidChangeNotification, kLock);
	LRetain(route);
	releaseAndClear(_route);
	_route = route;
	_releaseRouteDependedDatas();
	_releaseRouteDependedDrawables();
	_releaseVisibleCaptionsData();
	center->postNotification(kRecordingMustBeStoppedNotification, this, 0);
	if (_route) {
		center->addObserver(this, "_routeObjectWasChanged", RouteObject::kRouteObjectItemCountDidChangeNotification,
		                    _route);
		center->addObserver(this, "_routeObjectWasChanged", RouteObject::kRouteObjectAttributesDidChangeNotification,
		                    _route);
	}
}

void CRoutePointsContainer::setRouteReplicant(RouteObject *route)
{
	if (route == 0) {
		if (_savedRoute) {
			setCurrentRoute(_savedRoute);
			releaseAndClear(_savedRoute);
		}
	} else if (_savedRoute == 0) {
		_savedRoute = LRetain(_route);
		setCurrentRoute(route);
	}
}

static inline unsigned int _colorByte(float f)
{
	f *= 255.0f;
	return f > 0.0f ? (unsigned int)f & 0xff : 0;
}

// The lines' vertices: their scene points and colors (RGBA bytes).
void CRoutePointsContainer::meshWasInstantiated(CMesh *mesh)
{
	StVertexBufferLocker *vertices = mesh->detachVBLocker(0, 0);
	CRenderProfile *profile = mesh->renderProfile();
	if (profile->_shader == 0)
		LF_RAISE(WrongConditionException());
	int position = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int color = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	char *vertex = (char *)vertices->data();
	unsigned int stride = profile->vertexStructSize(0);
	const SRoutePoint *point = (const SRoutePoint *)_linePoints->bytes();
	int count = numVerticesForMesh(mesh);
	for (int i = 0; i < count; i++, point++, vertex += stride) {
		_scene->scenePointFromGeoPoint((Vector3 *)(vertex + position), &point->geoPoint);
		*(unsigned int *)(vertex + color) = _colorByte(point->color.r) | _colorByte(point->color.g) << 8 |
		                                    _colorByte(point->color.b) << 16 | _colorByte(point->color.a) << 24;
	}
	delete vertices;

	StIndexBufferLocker *indices = mesh->detachIBLocker(0);
	int lines = numPrimitivesForMesh(mesh);
	const uint32_t *index = (const uint32_t *)_lineIndices->bytes();
	if (indices->_desc.Format == D3DFMT_INDEX32) {
		memcpy(indices->data(), index, lines * 8);
	} else {
		uint16_t *out = (uint16_t *)indices->data();
		for (int i = 0; i < lines * 2; i++)
			out[i] = (uint16_t)index[i];
	}
	delete indices;
}

// The great circle turns `a` into `b` about their cross product. Where a step
// crosses the date line the line ends at ±180 and starts again on the other
// side, at the latitude interpolated between the step's ends.
bool CRoutePointsContainer::_appendRouteBetweenPoints(const SRouteEarthPoint *a, const SRouteEarthPoint *b, int steps)
{
	_SDVector3 from = {a->earthVec.x, a->earthVec.y, a->earthVec.z};
	_SDVector3 to = {b->earthVec.x, b->earthVec.y, b->earthVec.z};
	FHDVec3Normalize(&from, &from);
	FHDVec3Normalize(&to, &to);
	_SDVector3 axis;
	FHDVec3Cross(&axis, &from, &to);
	if (axis.x == axis.y && axis.y == axis.z && axis.x == 0.0)
		axis.z = 1.0;
	FHDVec3Normalize(&axis, &axis);
	Vector3 rotationAxis((float)axis.x, (float)axis.y, (float)axis.z);
	float angle = (float)acos(from.x * to.x + from.y * to.y + from.z * to.z);
	Matrix rotation;
	FHMatrixIdentity(&rotation);
	float lastLon = geoPointFromEarthVec(&a->earthVec).x;
	unsigned int last = 0;
	unsigned int index = 0;
	for (int i = 0; i <= steps;) {
		float t = (float)i / (float)steps;
		index = last;
		Vector4 rotated;
		FHVec3Transform(&rotated, &a->earthVec, &rotation);
		Vector3 v(rotated.x, rotated.y, rotated.z);
		FHVec3Normalize(&v, &v);
		LPoint geo = geoPointFromEarthVec(&v);
		if (i > 0 && fabsf(geo.x - lastLon) > 90.0f) {
			const SRoutePoint *previous =
			    (const SRoutePoint *)((const char *)_linePoints->bytes() + _linePoints->length()) - 1;
			float toLine = geo.x >= 0.0f ? 180.0f - geo.x : geo.x + 180.0f;
			float previousToLine =
			    previous->geoPoint.x >= 0.0f ? 180.0f - previous->geoPoint.x : previous->geoPoint.x + 180.0f;
			float lat = (toLine * previous->geoPoint.y + geo.y * previousToLine) / (previousToLine + toLine + 1e-10f);
			SRoutePoint end = {{previous->geoPoint.x >= 0.0f ? 180.0f : -180.0f, lat}, previous->color};
			SRoutePoint start = {{geo.x >= 0.0f ? 180.0f : -180.0f, lat}, previous->color};
			index = _linePoints->length() / sizeof(SRoutePoint);
			_linePoints->appendBytes(&end, sizeof(end));
			_lineIndices->appendBytes(&last, 4);
			_lineIndices->appendBytes(&index, 4);
			_linePoints->appendBytes(&start, sizeof(start));
			last = index + 1;
		}
		SRoutePoint point;
		point.geoPoint = geo;
		point.color = Color((1.0f - t) * a->color.r + t * b->color.r, (1.0f - t) * a->color.g + t * b->color.g,
		                    (1.0f - t) * a->color.b + t * b->color.b, 1.0f);
		i++;
		FHMatrixRotationAxis(&rotation, &rotationAxis, (float)i * (angle / (float)steps));
		index = _linePoints->length() / sizeof(SRoutePoint);
		_linePoints->appendBytes(&point, sizeof(point));
		if (i != 1) {
			_lineIndices->appendBytes(&last, 4);
			_lineIndices->appendBytes(&index, 4);
		}
		last = index;
		lastLon = geo.x;
		if (index > 0xff00)
			return false;
	}
	return true;
}

// The position along the current segment: the Hermite curve, eased into the
// straight line as the tangents turn apart, and the direction the straight
// line takes on the map.
bool CRoutePointsContainer::flyStepWithDeltaTime(double dt)
{
	if (_lineIndices == 0 || _flySegments == 0)
		return false;
	int count = _flySegments->length() / sizeof(SFlySegment);
	if (count == 0)
		return false;
	if (!_onLastSegment) {
		if (_flySegment >= count - 1) {
			_flySegment = count - 1;
			_onLastSegment = true;
		}
	} else if (_flySegment < 1) {
		_flySegment = 0;
		_onLastSegment = false;
	}
	if (_flySegment >= count)
		return false;
	const SFlySegment *segment = (const SFlySegment *)_flySegments->bytes() + _flySegment;
	_flyDistance = (float)(_flyDistance + dt * 1200.0);
	while (_flyDistance > segment->length) {
		_flyDistance -= segment->length;
		if (++_flySegment >= count)
			return false;
		segment++;
	}
	float t = _flyDistance / segment->length;
	t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	float u = 1.0f - t;
	Vector3 linear = segment->to * t + segment->from * u;
	Vector3 ahead = linear + (segment->toTangent * t + segment->fromTangent * u) * 0.001f;
	LPoint here = mapPointFromEarthVec(&linear);
	LPoint there = mapPointFromEarthVec(&ahead);
	if (fabsf(here.x - there.x) > 0.5f)
		there.x += here.x > there.x ? 1.0f : -1.0f;
	_flyDirection.x = there.x - here.x;
	_flyDirection.y = there.y - here.y;
	FHVec2Normalize(&_flyDirection, &_flyDirection);

	float cosine = segment->fromTangent.x * segment->toTangent.x + segment->fromTangent.y * segment->toTangent.y +
	               segment->fromTangent.z * segment->toTangent.z;
	cosine = cosine < 0.0f ? 0.0f : (cosine > 1.0f ? 1.0f : cosine);
	float curved = 1.0f - (float)pow(cosine, 4.0);
	float t2 = t * t, t3 = t2 * t;
	Vector3 hermite = segment->from * (2.0f * t3 - 3.0f * t2 + 1.0f) +
	                  segment->fromTangent * 0.005f * (t3 - 2.0f * t2 + t) +
	                  segment->to * (-2.0f * t3 + 3.0f * t2) + segment->toTangent * 0.005f * (t3 - t2);
	Vector3 position = hermite * curved + linear * (1.0f - curved);
	_flyMapPoint = mapPointFromEarthVec(&position);
	return true;
}

void CRoutePointsContainer::_addFlySegment(const Vector3 &from, const Vector3 &fromTangent, const Vector3 &to,
                                           const Vector3 &toTangent)
{
	if (_flySegments == 0)
		_flySegments = (LData *)LData::_alloc()->init();
	SFlySegment segment;
	segment.from = from;
	segment.to = to;
	FHVec3Normalize(&segment.fromTangent, &fromTangent);
	FHVec3Normalize(&segment.toTangent, &toTangent);
	segment.length = distanceOnEarthFromEarthVecs(&from, &to);
	_flySegments->appendBytes(&segment, sizeof(segment));
}

// A segment from midpoint to midpoint of the lines; a line longer than 100 km
// gets 50 km ends of its own so the turns stay at the points. Points closer
// than 0.2 km to the last one are skipped.
void CRoutePointsContainer::_createFlyingPointsArray()
{
	releaseAndClear(_flySegments);
	if (_linePoints == 0 || _lineIndices == 0)
		return;
	const SRoutePoint *points = (const SRoutePoint *)_linePoints->bytes();
	int count = _lineIndices->length() / 4;
	const uint32_t *index = (const uint32_t *)_lineIndices->bytes();
	Vector3 start(0, 0, 0), last(0, 0, 0), tangent(0, 0, 0);
	for (int i = 0; i < count; i++) {
		Vector3 point = earthVecFromGeoPoint(points[index[i]].geoPoint);
		if (i < 1) {
			start = point;
			last = point;
			continue;
		}
		Vector3 delta = point - last;
		if (i == 1)
			tangent = delta;
		float distance = distanceOnEarthFromEarthVecs(&point, &last);
		if (distance < 0.2f)
			continue;
		Vector3 end;
		if (distance <= 100.0f) {
			end = (point + last) * 0.5f;
			_addFlySegment(start, tangent, end, delta);
		} else {
			end = last + delta * (50.0f / distance);
			_addFlySegment(start, tangent, end, delta);
			start = end;
			end = last + delta * (1.0f - 50.0f / distance);
			_addFlySegment(start, delta, end, delta);
		}
		start = end;
		tangent = delta;
		last = point;
	}
}

void CRoutePointsContainer::startFlying()
{
	_flySegment = 0;
	_flyDistance = 0.0f;
	_onLastSegment = false;
	if (_flySegments == 0)
		_createFlyingPointsArray();
	if (_flySegments)
		_flyMapPoint = mapPointFromEarthVec((const Vector3 *)_flySegments->bytes());
	else
		_flyMapPoint.x = _flyMapPoint.y = 0.5f;
	_flyDirection.x = 0.0f;
	_flyDirection.y = 1.0f;
}

// Each line of the route as great circle steps of about 2 degrees.
void CRoutePointsContainer::_fillRouteLines()
{
	releaseAndClear(_linePoints);
	releaseAndClear(_lineIndices);
	if (_route == 0)
		return;
	LData *pointsData = _route->pointsToHighlight();
	const SRoutePoint *points = pointsData ? (const SRoutePoint *)pointsData->bytes() : 0;
	LData *linesData = _route->linesIndices();
	int lines = linesData ? linesData->length() / 8 : 0;
	if (lines == 0)
		return;
	_linePoints = (LData *)LData::_alloc()->init();
	_lineIndices = (LData *)LData::_alloc()->init();
	const uint32_t *line = (const uint32_t *)linesData->bytes();
	for (int i = 0; i < lines; i++, line += 2) {
		const SRoutePoint &from = points[line[0]];
		const SRoutePoint &to = points[line[1]];
		SRouteEarthPoint a = {earthVecFromGeoPoint(from.geoPoint), from.color};
		SRouteEarthPoint b = {earthVecFromGeoPoint(to.geoPoint), to.color};
		Vector3 delta = a.earthVec - b.earthVec;
		if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z > 1e-9f) {
			float angle = acosf(a.earthVec.x * b.earthVec.x + a.earthVec.y * b.earthVec.y + a.earthVec.z * b.earthVec.z);
			if (!_appendRouteBetweenPoints(&a, &b, (int)(angle / 0.034904014f) + 1))
				return;
		}
	}
}

void CRoutePointsContainer::_createRouteLinesMesh()
{
	if (_linePoints == 0 || _lineIndices == 0)
		_fillRouteLines();
	releaseAndClear(_linesMesh);
	if (_linePoints == 0 || _linePoints->length() / sizeof(SRoutePoint) < 2 || _route == 0)
		return;
	CHolder *holder = _scene->holder();
	if (holder)
		holder->beginAccumulationBlock();
	if (_linesProfile == 0)
		_linesProfile = LRetain(CRenderProfile::renderProfileByFileDesc(
		    LString::stringWithCString("RENDERPROFILES\\Route.yaf"), _scene->holder()));
	_linesMesh = ((CMesh *)CMesh::_alloc())->initWithDataSource(this, _linesProfile, 0);
	if (_linesMesh)
		_linesMesh->_typeName = "CRoutePointsContainer::mRouteLinesMesh";
	if (holder)
		holder->endAccumulationBlock();
}

LData *CRoutePointsContainer::routeLinesInds()
{
	if (_lineIndices == 0)
		_fillRouteLines();
	return _lineIndices;
}

LData *CRoutePointsContainer::routeLinesPoints()
{
	if (_linePoints == 0)
		_fillRouteLines();
	return _linePoints;
}

namespace {

// A titled route point in view.
struct SCaptionInfo {
	LPoint geoPoint;
	LPoint viewPoint;
	LString *title;
	int index;
	float weight;	// by the title's length and the point's weight
	float distance2;	// from the camera's map point
};

bool _nearerCaption(const SCaptionInfo &a, const SCaptionInfo &b)
{
	return a.distance2 < b.distance2;
}

} // namespace

// The captions to show: of the titled points in view, the heaviest in each
// 60 by 20 cell of the view; with the nearest-only preference, from the two
// nearest the camera.
void CRoutePointsContainer::_createVisibleCaptionsDatas()
{
	if (_route == 0 || _scene == 0)
		return;
	std::vector<SCaptionInfo> captions;
	LSize frame = _scene->holder()->virtualFrameSize();
	LData *pointsData = _route->pointsToHighlight();
	bool nearestOnly = Preferences::showLocationNamesNearestOnly();
	if (pointsData) {
		Vector3 camera = *_scene->camera()->position();
		FHVec3Normalize(&camera, &camera);
		LPoint cameraMapPoint = mapPointFromEarthVec(&camera);
		PanelDrawIterator *it = _scene->controller()->detachDrawIterator(0);
		while (it->next()) {
			const SRoutePoint *point = (const SRoutePoint *)pointsData->bytes();
			int count = pointsData->length() / sizeof(SRoutePoint);
			for (int i = 0; i < count; i++, point++) {
				SCaptionInfo info;
				if (!_scene->viewPointFromGeoPoint(&info.viewPoint, &point->geoPoint, it, 0))
					continue;
				LString *title = _route->titleForPointAtIndex(i);
				if (title == 0)
					continue;
				float length = (float)title->length();
				float scale;
				if (length < 4.0f)
					scale = length * 0.2f + 0.2f;
				else if (length < 10.0f)
					scale = 1.0f;
				else
					scale = powf(10.0f / length, 0.6f);
				LPoint mapPoint = mapPointFromGeoPoint(point->geoPoint);
				float dx = mapPoint.x - cameraMapPoint.x;
				float dy = mapPoint.y - cameraMapPoint.y;
				info.geoPoint = point->geoPoint;
				info.title = title;
				info.index = i;
				info.weight = scale * _route->visualWeightForPointAtIndex(i);
				info.distance2 = dx * dx + dy * dy;
				captions.push_back(info);
			}
		}
		if (nearestOnly)
			std::stable_sort(captions.begin(), captions.end(), _nearerCaption);
		delete it;
	}
	_releaseVisibleCaptionsData();

	int cols = (int)(frame.width / 60.0f);
	int rows = (int)(frame.height / 20.0f);
	if (cols <= 0 || rows <= 0)
		return;
	// The cells keep the weight truncated, as the original did: lighter
	// captions than 1 never show.
	struct SCell {
		int index;
		int weight;
	};
	StPointerBlock block(cols * rows * sizeof(SCell), true);
	SCell *cells = (SCell *)block.bytes();
	for (int i = 0; i < (int)captions.size() && (!nearestOnly || i < 2); i++) {
		const SCaptionInfo &info = captions[i];
		int col = (int)(info.viewPoint.x / (frame.width / (float)cols));
		int row = (int)(info.viewPoint.y / (frame.height / (float)rows));
		if (col < 0 || col >= cols || row < 0 || row >= rows)
			continue;
		SCell &cell = cells[row * cols + col];
		if (info.weight > (float)cell.weight) {
			cell.weight = (int)info.weight;
			cell.index = i;
		}
	}
	for (int i = 0; i < cols * rows; i++) {
		if ((float)cells[i].weight <= 0.0f)
			continue;
		const SCaptionInfo &info = captions[cells[i].index];
		if (_captionPoints == 0)
			_captionPoints = (LData *)LData::_alloc()->init();
		_captionPoints->appendBytes(&info.geoPoint, sizeof(LPoint));
		if (_captionTitles == 0)
			_captionTitles = (LArray *)LArray::_alloc()->init();
		_captionTitles->addObject(info.title);
	}
}

// The lines, the shapes and their captions, the markers, and the points'
// captions, once for each copy of the map.
void CRoutePointsContainer::renderRoutes(CFrameDrawer *drawer)
{
	if (_route == 0)
		return;
	RenderOptions *options = _scene->renderOptions();
	if (_linesMesh == 0)
		_createRouteLinesMesh();
	if (_linesMesh && options->showLinesInSet()) {
		PanelDrawIterator *it = _scene->controller()->detachDrawIterator(0);
		if (_linesDatas == 0)
			_linesDatas = (LArray *)LArray::_alloc()->init();
		for (int copy = 0; it->next(); copy++) {
			ModelDataWithMatrix *data;
			if (copy < (int)_linesDatas->count()) {
				data = (ModelDataWithMatrix *)_linesDatas->objectAtIndex(copy);
			} else {
				data = lobj_cast<ModelDataWithMatrix>(_linesMesh->detachModelDataInstance());
				if (data) {
					data->_storesState = false;
					_linesDatas->addObject(data);
				}
			}
			if (data == 0)
				continue;
			_scene->_adjustRouteData(data, it);
			drawer->renderDrawableData(data, 3);
		}
		delete it;
	}

	bool captions = options->showLocationNamesInSet() && options->showCaptions();
	LArray *shapes = _route->shapesToHighlight();
	int shapesCount = shapes ? shapes->count() : 0;
	for (int i = 0; i < shapesCount; i++) {
		CShapeContainer *shape = (CShapeContainer *)shapes->objectAtIndex(i);
		GenericEntry *entry = GenericEntry::entryWithUID(shape->objectUID());
		LString *caption = entry ? entry->shortCaption(0) : 0;
		_scene->_renderShapeContainer(drawer, shape, ShapeModelData::kUsageAllShapesHighlight, 0);
		if (captions)
			_scene->_renderShapesCaption(drawer, caption, shape, 0);
	}

	if (_markers == 0)
		_createPointMarkersSystem();
	if (_markers && options->showMarkers()) {
		PanelDrawIterator *it = _scene->controller()->detachDrawIterator(0);
		while (it->next()) {
			CParticleSystemDrawProxy *proxy = (CParticleSystemDrawProxy *)_markers->detachDrawProxy();
			proxy->_storesState = false;
			_scene->_adjustRoutePointsProxy(proxy, it);
			drawer->renderDrawableData(proxy, 3);
		}
		delete it;
	}

	if (captions) {
		if (_captionPoints == 0)
			_createVisibleCaptionsDatas();
		int count = _captionTitles ? _captionTitles->count() : 0;
		const LPoint *points = _captionPoints ? (const LPoint *)_captionPoints->bytes() : 0;
		SharedContextObjectsContainer *objects = _scene->objectsContainer();
		PanelDrawIterator *it = _scene->controller()->detachDrawIterator(0);
		CFontDrawableData *font = 0;
		while (it->next()) {
			for (int i = 0; i < count; i++) {
				LPoint view;
				_scene->viewPointFromGeoPoint(&view, &points[i], it, 0);
				if (font == 0) {
					font = objects->detachDataForCaptionFont();
					font->_storesState = false;
				}
				font->addCaptionAtPosition((LString *)_captionTitles->objectAtIndex(i), view, 0);
			}
		}
		if (font)
			drawer->renderDrawableData(font, 3);
		delete it;
	}
}
