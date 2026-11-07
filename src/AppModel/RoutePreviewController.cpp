// RoutePreviewController: the route manager's preview map. It shows a square
// part (`_scale` of the map's width, 0.1 to 1) of a flat world map around
// `_position`, from tiles of 256x128 pixels: 2x2 tiles at level 0, 4x4 at 1
// and 8x8 at 2, chosen by the scale. Map coordinates go from 0 to 1, x from
// 180 degrees west, y from the south pole.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(RoutePreviewController, 0)

static LString *_tileImagePath(intptr_t key)
{
	return LString::stringWithFormat("INTERFACE\\RouteManagerPalette\\Earth preview\\LOD%d\\Earth_map_%d_%d.png",
	                                 (int)(key >> 24), (int)(key >> 12 & 0xfff), (int)(key & 0xfff));
}

// ---------------------------------------------------------------------------

RPDrawingData::RPDrawingData()
{
	_textures = new TArray<RPTextureData>;
	_columns = new TArray<float>;
	_rows = new TArray<float>;
}

RPDrawingData::~RPDrawingData()
{
	delete _textures;
	_textures = 0;
	delete _columns;
	_columns = 0;
	delete _rows;
	_rows = 0;
}

void RPDrawingData::clean()
{
	_textures->removeAllObjects();
	_columns->removeAllObjects();
	_rows->removeAllObjects();
}

// ---------------------------------------------------------------------------

_RPTexturesCache::_RPTexturesCache()
{
	_images = new TLIntPtrMap<LImage *>;
}

_RPTexturesCache::~_RPTexturesCache()
{
	delete _images;
	_images = 0;
}

void _RPTexturesCache::_intersectSets(CHashSet &a, CHashSet &b, CHashSet &result)
{
	CHashSetIterator it(&a);
	while (it.next()) {
		intptr_t key = it.object();
		if (b.containsObject(key))
			result.addObject(key);
	}
}

LImage *_RPTexturesCache::imageForTextureID(const STextureID &id)
{
	return _images->objectForKey(STextureIDKey(id.x, id.y, id.level));
}

// The dropped tiles age by a frame; their images go after 16 frames.
void _RPTexturesCache::_cleanUnusedCache()
{
	TArray<intptr_t> keys, frames;
	CHashTableIterator it(&_unused);
	while (it.next()) {
		keys.addObject(it.key());
		frames.addObject(it.value());
	}
	for (unsigned int i = 0; i < keys.count(); i++) {
		if (frames[i] + 1 < 16) {
			_unused.setValueForKey(frames[i] + 1, keys[i]);
		} else {
			_images->removeObjectForKey(keys[i]);
			_unused.removeValueForKey(keys[i]);
		}
	}
}

void _RPTexturesCache::_cacheTextures(CHashSet &textures)
{
	CHashSetIterator it(&textures);
	while (it.next()) {
		intptr_t key = it.object();
		LImage *image = LImage::_alloc()->initWithContentsOfFile(_tileImagePath(key));
		if (image) {
			_images->setObjectForKey((LImage *)image->l_autorelease(), key);
			addObject(key);
		}
	}
}

// The tiles of the new frame: the ones no longer shown start aging, the new
// ones are loaded unless they are still cached.
void _RPTexturesCache::proceedNewFrame(CHashSet &textures)
{
	if (isEmpty())
		_cacheTextures(textures);
	CHashSet common;
	_intersectSets(*this, textures, common);
	CHashSet dropped;
	CHashSetIterator it(this);
	while (it.next()) {
		intptr_t key = it.object();
		if (common.isEmpty() || !common.containsObject(key)) {
			dropped.addObject(key);
			_unused.setValueForKey(0, key);
		}
	}
	CHashSetIterator droppedIt(&dropped);
	while (droppedIt.next())
		removeObject(droppedIt.object());
	CHashSetIterator newIt(&textures);
	while (newIt.next()) {
		intptr_t key = newIt.object();
		if (common.containsObject(key))
			continue;
		if (_unused.getValueForKey(0, key)) {
			_unused.removeValueForKey(key);
		} else {
			LImage *image = LImage::_alloc()->initWithContentsOfFile(_tileImagePath(key));
			if (image)
				_images->setObjectForKey((LImage *)image->l_autorelease(), key);
		}
		addObject(key);
	}
	_cleanUnusedCache();
}

// ---------------------------------------------------------------------------

RoutePreviewController::RoutePreviewController()
    : _scale(0), _position(LZeroPoint), _lod(0), _route(0), _routeChanged(false), _needsDisplay(false),
      _drawingData(0), _points(0), _viewPoints(0), _texturesCache(0)
{
}

RoutePreviewController *RoutePreviewController::init()
{
	if (LObject::init() == 0)
		return 0;
	setScale(1.0f);
	setPosition(LMakePoint(0.5f, 0.5f));
	_drawingData = new RPDrawingData;
	_texturesCache = new _RPTexturesCache;
	_points = new TArray<RPRoutePointData>;
	_viewPoints = new TArray<RPRoutePointData>;
	return this;
}

void RoutePreviewController::dealloc()
{
	delete _drawingData;
	_drawingData = 0;
	delete _points;
	_points = 0;
	delete _texturesCache;
	_texturesCache = 0;
	delete _viewPoints;
	_viewPoints = 0;
	if (_route) {
		_route->release();
		_route = 0;
	}
	LObject::dealloc();
}

bool RoutePreviewController::isMinScaled()
{
	return _scale == 0.1f;
}

bool RoutePreviewController::isMaxScaled()
{
	return _scale == 1.0f;
}

int RoutePreviewController::_lodForCurrentScale()
{
	if (_scale <= 0.33333334f)
		return 2;
	return _scale <= 0.6666667f ? 1 : 0;
}

int RoutePreviewController::_numImagesForCurrentLOD()
{
	return _lod == 2 ? 64 : _lod == 1 ? 16 : 4;
}

LSize RoutePreviewController::_currentLODSize()
{
	float side = sqrtf((float)_numImagesForCurrentLOD());
	return LMakeSize(side * 256.0f, side * 128.0f);
}

// Snaps a value within 0.00005 of a tenth to it.
float RoutePreviewController::_fixedFloat(float value)
{
	float up = ceilf(value * 10.0f);
	if (fabsf(up - value * 10.0f) <= 5e-05f)
		value = up / 10.0f;
	float down = floorf(value * 10.0f);
	if (fabsf(down - value * 10.0f) <= 5e-05f)
		value = down / 10.0f;
	return value;
}

// Keeps the shown part on the map.
LPoint RoutePreviewController::_fixedPosition(LPoint position)
{
	float left = position.x - _scale * 0.5f;
	float bottom = position.y - _scale * 0.5f;
	float overRight = 1.0f - (left + _scale);
	float overTop = 1.0f - (bottom + _scale);
	if (0 <= overRight)
		overRight = 0;
	if (0 <= overTop)
		overTop = 0;
	float x = overRight + (position.x - (left < 0 ? left : 0));
	float y = overTop + (position.y - (bottom < 0 ? bottom : 0));
	return LMakePoint(_fixedFloat(x), _fixedFloat(y));
}

void RoutePreviewController::setPosition(LPoint position)
{
	LPoint fixed = _fixedPosition(position);
	if (5e-06f < fabsf(fixed.x - _position.x) || 5e-06f < fabsf(fixed.y - _position.y)) {
		_position = fixed;
		_needsDisplay = true;
		LNotificationCenter::center()->postNotification(kRoutesPreviewNeedsDisplay, this, 0);
	}
}

LRect RoutePreviewController::_renderRect()
{
	return LMakeRect(_scale * -0.5f + _position.x, _position.y + _scale * -0.5f, _scale, _scale);
}

LPoint RoutePreviewController::_renderPointFromViewPoint(LPoint point)
{
	LRect r = _renderRect();
	return LMakePoint((point.x - r.origin.x) / r.size.width, (point.y - r.origin.y) / r.size.height);
}

TArray<RPRoutePointData> *RoutePreviewController::_routePointsToRenderViewPoints(TArray<RPRoutePointData> *points)
{
	_viewPoints->removeAllObjects();
	for (unsigned int i = 0; i < points->count(); i++) {
		RPRoutePointData p = (*points)[i];
		p.point = _renderPointFromViewPoint(p.point);
		_viewPoints->addObject(p);
	}
	return _viewPoints;
}

LImage *RoutePreviewController::imageForTextureID(const STextureID &id)
{
	return _texturesCache->imageForTextureID(id);
}

LPoint RoutePreviewController::_renderPointFromGeoPoint(LPoint geoPoint)
{
	return LMakePoint((geoPoint.x + 180.0f) / 360.0f, (geoPoint.y + 90.0f) / 180.0f);
}

// The route's locations in map coordinates. A route crossing the 180th
// meridian is cut there: a line to the map's edge, a move to the other edge
// and a line on. Breaks (items that aren't locations) start a new line.
TArray<RPRoutePointData> *RoutePreviewController::_getRoutePointDataInLocalCoords()
{
	if (!_routeChanged)
		return _points;
	_points->removeAllObjects();
	if (_route == 0)
		return _points;
	LPoint last = LMakePoint(0.5f, 0.5f);
	for (int i = 0; i < _route->itemsCount(); i++) {
		RouteSetItem *item = _route->itemAtIndex(i);
		if (item == 0)
			continue;
		PointEntry *entry = lobj_cast<PointEntry>(item);
		if (entry == 0) {
			RPRoutePointData p = { last, true, true };
			_points->addObject(p);
			last = LMakePoint(0.5f, 0.5f);
			continue;
		}
		LPoint point = _renderPointFromGeoPoint(entry->geoPoint(0));
		if (0.5f < last.x - point.x || 0.5f < point.x - last.x) {
			float left = last.x < point.x ? last.x : point.x;
			float right = last.x < point.x ? point.x : last.x;
			float bottom = last.y < point.y ? last.y : point.y;
			float y = (1.0f - right) / (left + 1.0f - right) * fabsf(last.y - point.y) + bottom;
			float fromEdge = last.x < point.x ? 0.0f : 1.0f;
			float toEdge = last.x < point.x ? 1.0f : 0.0f;
			RPRoutePointData a = { LMakePoint(fromEdge, y), false, true };
			RPRoutePointData b = { LMakePoint(toEdge, y), true, true };
			RPRoutePointData c = { LMakePoint(toEdge, y), false, true };
			_points->addObject(a);
			_points->addObject(b);
			_points->addObject(c);
		}
		if (i == 0) {
			RPRoutePointData start = { point, true, true };
			_points->addObject(start);
		}
		RPRoutePointData p = { point, false, false };
		_points->addObject(p);
		last = point;
	}
	_routeChanged = false;
	return _points;
}

TArray<RPRoutePointData> *RoutePreviewController::getRoutePointData()
{
	return _routePointsToRenderViewPoints(_getRoutePointDataInLocalCoords());
}

void RoutePreviewController::mouseDraggedWithDiff(LPoint diff)
{
	setPosition(LMakePoint(diff.x + _position.x, diff.y + _position.y));
}

// The part of a tile that is shown, in the tile's pixels, and where it goes
// in the view (0..1).
LPoint RoutePreviewController::_calculateSrcRectAndAnchorPointForPosition(int column, int row, LRect *srcRect)
{
	LRect view = _renderRect();
	LSize size = _currentLODSize();
	LRect tile = LMakeRect(((float)column * 256.0f) / size.width, ((float)row * 128.0f) / size.height,
	                       256.0f / size.width, 128.0f / size.height);
	LRect part = LIntersectionRect(tile, view);
	LPoint anchor = _renderPointFromViewPoint(part.origin);
	if (srcRect)
		*srcRect = LMakeRect(size.width * part.origin.x + (float)column * -256.0f,
		                     size.height * part.origin.y + (float)row * -128.0f, part.size.width * size.width,
		                     part.size.height * size.height);
	return anchor;
}

void RoutePreviewController::setScale(float scale)
{
	if (scale < 0.1f)
		scale = 0.1f;
	else if (scale > 1.0f)
		scale = 1.0f;
	if (scale == _scale)
		return;
	_scale = scale;
	_lod = _lodForCurrentScale();
	LPoint position = _position;
	setPosition(position);
	if (position.x == _position.x && position.y == _position.y) {
		_needsDisplay = true;
		LNotificationCenter::center()->postNotification(kRoutesPreviewNeedsDisplay, this, 0);
	}
}

void RoutePreviewController::scrollWheelWithDiff(float diff)
{
	setScale(diff + _scale);
}

// Shows the whole route, with a margin.
void RoutePreviewController::scaleToFit()
{
	TArray<RPRoutePointData> *points = _getRoutePointDataInLocalCoords();
	if (points == 0 || points->count() == 0) {
		setScale(1.0f);
		return;
	}
	float minX = 1, minY = 1, maxX = LZeroPoint.x, maxY = LZeroPoint.y;
	for (unsigned int i = 0; i < points->count(); i++) {
		const RPRoutePointData &p = (*points)[i];
		if (p.move)
			continue;
		minX = minX <= p.point.x ? minX : p.point.x;
		minY = minY <= p.point.y ? minY : p.point.y;
		maxX = p.point.x <= maxX ? maxX : p.point.x;
		maxY = p.point.y <= maxY ? maxY : p.point.y;
	}
	float height = maxY - minY;
	float width = maxX - minX;
	float scale = height <= width ? width : height;
	if (0.1f < scale)
		scale += 0.023f;
	setScale(scale);
	setPosition(LMakePoint(minX + width * 0.5f, minY + height * 0.5f));
	LNotificationCenter::center()->postNotification(kRoutesPreviewNeedsDisplay, this, 0);
}

void RoutePreviewController::setSelectedRoute(GenericRouteNode *node)
{
	RouteObject *route = node && node->entry() ? node->entry()->routeObject() : 0;
	// The original also dropped the route when the same one was selected again.
	if (route != _route) {
		if (_route) {
			_route->release();
			_route = 0;
		}
		_route = LRetain(route);
	}
	_routeChanged = true;
	_needsDisplay = true;
	scaleToFit();
	LNotificationCenter::center()->postNotification(kRoutesPreviewNeedsDisplay, this, 0);
}

// The tiles in view and where their columns and rows start.
RPDrawingData *RoutePreviewController::getDrawingData()
{
	if (!_needsDisplay)
		return _drawingData;
	LRect view = _renderRect();
	float side = (float)(int)sqrtf((float)_numImagesForCurrentLOD());
	int firstColumn = (int)floorf(side * view.origin.x);
	int endColumn = (int)ceilf((view.origin.x + view.size.width) * side);
	int firstRow = (int)floorf(side * view.origin.y);
	int endRow = (int)ceilf((view.origin.y + view.size.height) * side);
	_drawingData->clean();
	CHashSet keys;
	for (int row = firstRow; row < endRow; row++) {
		for (int column = firstColumn; column < endColumn; column++) {
			keys.addObject(STextureIDKey(column, row, _lod));
			RPTextureData tile;
			tile.textureID.x = (short)column;
			tile.textureID.y = (short)row;
			tile.textureID.level = (char)_lod;
			tile.srcRect = LZeroRect;
			LPoint anchor = _calculateSrcRectAndAnchorPointForPosition(column, row, &tile.srcRect);
			_drawingData->_textures->addObject(tile);
			if (row == firstRow)
				_drawingData->_columns->addObject(anchor.x);
			if (column == firstColumn)
				_drawingData->_rows->addObject(anchor.y);
		}
	}
	_drawingData->_columns->addObject(1.0f);
	_drawingData->_rows->addObject(1.0f);
	_texturesCache->proceedNewFrame(keys);
	_needsDisplay = false;
	return _drawingData;
}
