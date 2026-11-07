// The flat map's panel controller.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(FlatMapController, LF_CLASSOF(AtlasPanelController))

// log(1 / visible part) for the scale 1.
static const float kScaleLogRange = 5.5f;

// ---------------------------------------------------------------------------
// drawIteratorFlat

FlatMapController::drawIteratorFlat::drawIteratorFlat(LSize viewSize, LPoint position, float visiblePart)
    : _index(-1), _start(0.0f), _segmentX(0), _segmentY(0), _segmentCols(1), _segmentRows(1)
{
	_visiblePart = visiblePart;
	_translationScale = (viewSize.height + viewSize.height) / visiblePart;
	_width = (viewSize.width / viewSize.height) * visiblePart * 0.5f;
	int whole = (int)_width;
	_count = whole + 1;
	float half;
	if (_width * -0.5f + position.x > 0.0f)
		half = _width * 0.5f;
	else {
		// A copy starts left of the first.
		half = _width * 0.5f;
		_count = whole + 2;
		_start = -(floorf(half - position.x) + 1.0f);
	}
	if (position.x + half >= 1.0f)
		_count++;
}

FlatMapController::drawIteratorFlat::drawIteratorFlat(CHolder *holder, LPoint position, float visiblePart)
    : _index(-1), _start(0.0f)
{
	CRenderTarget *target = holder->context()->contextRenderTarget();
	target->getOutSegmentsCount(&_segmentCols, &_segmentRows);
	target->getOutSegment(&_segmentX, &_segmentY);
	LSize size = holder->virtualFrameSize();
	float width = (size.width / size.height) * visiblePart * 0.5f;
	_width = width;
	_count = (int)width + 1;
	_visiblePart = visiblePart;
	_translationScale = (size.height + size.height) / visiblePart;
	float half;
	if (width * -0.5f + position.x > 0.0f)
		half = _width * 0.5f;
	else {
		half = _width * 0.5f;
		_count = (int)width + 2;
		_start = -(floorf(half - position.x) + 1.0f);
	}
	if (half + position.x >= 1.0f)
		_count++;
}

bool FlatMapController::drawIteratorFlat::next()
{
	int index = _index;
	if (index < 0) {
		index = 0;
		_index = 0;
	} else if (index < _count) {
		index++;
		_index = index;
	}
	_mapOffset = (float)_index + _start;
	return index < _count;
}

static inline float clampUnit(float f)
{
	if (!(f >= 0.0f))
		return 0.0f;
	return f <= 1.0f ? f : 1.0f;
}

void FlatMapController::drawIteratorFlat::evalRenderMapRect(LPoint position, LRect *out)
{
	float x = position.x - ((float)_index + _start);
	float bottom = _visiblePart * -0.5f + position.y;
	float left = _width * -0.5f + x;
	float cellWidth = ((x + _width * 0.5f) - left) / (float)_segmentCols;
	float cellHeight = ((position.y + _visiblePart * 0.5f) - bottom) / (float)_segmentRows;
	left = (float)_segmentX * cellWidth + left;
	float right = cellWidth + left;
	bottom = (float)((_segmentRows - _segmentY) - 1) * cellHeight + bottom;
	float top = cellHeight + bottom;
	float x0 = clampUnit(left);
	float y0 = clampUnit(bottom);
	float x1 = clampUnit(right);
	float y1 = clampUnit(top);
	out->origin.x = x0;
	out->origin.y = y0;
	out->size.height = y1 - y0;
	out->size.width = x1 - x0;
}

// ---------------------------------------------------------------------------
// FlatMapController

FlatMapController::FlatMapController() : _scene(0)
{
	_dragStartPosition.x = 0.5f;
	_dragStartPosition.y = 0.5f;
}

void FlatMapController::dealloc()
{
	if (_scene)
		_scene->invalidate();
	_scene = 0;
	AtlasPanelController::dealloc();
}

BaseScene *FlatMapController::scene()
{
	return _scene;
}

void FlatMapController::sceneWillBeInstalled()
{
	AtlasPanelController::sceneWillBeInstalled();
	_animFlags |= kAnimScaleTimer;
	_setupTimer();
}

void FlatMapController::sceneWasUninstalled()
{
	AtlasPanelController::sceneWasUninstalled();
	_animFlags &= ~kAnimScaleTimer;
	_setupTimer();
}

float FlatMapController::visiblePartFromCtrlScale()
{
	return (float)(1.0 / exp((double)(scale() * kScaleLogRange)));
}

// In kilometers.
float FlatMapController::currentAltitude()
{
	return visiblePartFromCtrlScale() * 19000.0f;
}

// The keys move and zoom the map; it doesn't turn.
void FlatMapController::_addKeyboardForces(TParamsHolder *forces)
{
	if (_keys & 4)
		forces->x -= 1.0f;
	if (_keys & 8)
		forces->x += 1.0f;
	if (_keys & 1)
		forces->y += 1.0f;
	if (_keys & 2)
		forces->y -= 1.0f;
	if (_keys & 0x20)
		forces->scale -= 1.0f;
	if (_keys & 0x10)
		forces->scale += 1.0f;
}

void FlatMapController::_directDragStarted()
{
	AtlasPanelController::_directDragStarted();
	_dragStartPosition = position();
}

float FlatMapController::attenuationFactorFromScale(TAttenuationFactorType)
{
	return visiblePartFromCtrlScale() * 0.4f;
}

float FlatMapController::zoomScaleFromBaseScale(float scale)
{
	float s = (float)(-log((double)(visiblePartFromCtrlScale() * scale)) / 5.5);
	if (s < 0.0f)
		return 0.0f;
	return s > 1.0f ? 1.0f : s;
}

float FlatMapController::zoomScaleToFitGeoRect(LRect geoRect)
{
	LSize viewSize = _window->acceleratedViewSize();
	float part = geoRect.size.height / 180.0f;
	float viewAspect = viewSize.width / viewSize.height;
	if (viewAspect < geoRect.size.width / geoRect.size.height)
		part = part * ((geoRect.size.width / geoRect.size.height) / viewAspect);
	float s = (float)(-log((double)part) / 5.5);
	if (s < 0.0f)
		return 0.0f;
	return s > 1.0f ? 1.0f : s;
}

// The visible part follows the scale; the position is clamped again.
void FlatMapController::setScale(float scale)
{
	AtlasPanelController::setScale(scale);
	setPosition(position());
	_scene->invalidateDayNightSeparator();
}

// In degrees.
LSize FlatMapController::visibleParts()
{
	float part = visiblePartFromCtrlScale();
	LSize frame = scene()->holder()->virtualFrameSize();
	float width = (frame.width * part * 180.0f) / frame.height;
	LSize size = {width < 360.0f ? width : 360.0f, part * 180.0f};
	return size;
}

// The longitude wraps; the map's top and bottom stay in view.
void FlatMapController::setPosition(LPoint mapPoint)
{
	_params.x = mapPoint.x - floorf(mapPoint.x);
	_params.y = mapPoint.y;
	float part = visiblePartFromCtrlScale();
	float low = (1.0f - part) * -0.5f + 0.5f;
	if (_params.y < low)
		_params.y = low;
	float high = (1.0f - part) * 0.5f + 0.5f;
	if (_params.y > high)
		_params.y = high;
	_scene->invalidateDayNightSeparator();
	AtlasPanelController::setPosition(position());
}

void FlatMapController::_directDragWithDiff(LPoint diff)
{
	AtlasPanelController::_directDragWithDiff(diff);
	float f = attenuationFactorFromScale((TAttenuationFactorType)0);
	LPoint p = {f * diff.x * -1.5f + _dragStartPosition.x, f * diff.y * -3.0f + _dragStartPosition.y};
	setPosition(p);
	refreshView();
}

bool FlatMapController::canConvertNormalizedViewPointToGeoPoint(LPoint point, LPoint *geoPoint)
{
	LSize viewSize = _window->acceleratedViewSize();
	float part = visiblePartFromCtrlScale();
	if (geoPoint) {
		LPoint mapPoint = {(viewSize.width / viewSize.height) * part * 0.5f * (point.x - 0.5f) + _params.x,
		                   part * (point.y - 0.5f) + _params.y};
		*geoPoint = geoPointFromMapPoint(mapPoint);
	}
	return true;
}

LRect FlatMapController::visibleGeoRect()
{
	LSize viewSize = _window->acceleratedViewSize();
	float part = visiblePartFromCtrlScale();
	LPoint corner0, corner1;
	canConvertNormalizedViewPointToGeoPoint(LZeroPoint, &corner0);
	LPoint one = {1.0f, 1.0f};
	canConvertNormalizedViewPointToGeoPoint(one, &corner1);
	LRect rect = LZeroRect;
	if ((viewSize.width / viewSize.height) * part * 0.5f <= 1.0f) {
		rect.origin.x = corner0.x;
		float width = corner1.x - corner0.x;
		if (width < 0.0f)
			width += (float)(int)((0.0f - width) / 360.0f + 1.0f) * 360.0f;
		else if (width > 360.0f)
			width -= (float)(int)(width / 360.0f) * 360.0f;
		rect.size.width = width;
	} else {
		rect.origin.x = -180.0f;
		rect.size.width = 360.0f;
	}
	if (part <= 1.0f) {
		rect.origin.y = corner0.y;
		float height = corner1.y - corner0.y;
		if (height < 0.0f)
			height += (float)(int)((0.0f - height) / 180.0f + 1.0f) * 180.0f;
		else if (height > 180.0f)
			height -= (float)(int)(height / 180.0f) * 180.0f;
		rect.size.height = height;
	} else {
		rect.origin.y = -90.0f;
		rect.size.height = 180.0f;
	}
	return rect;
}

// The ground distance in view compared with 5 degrees of latitude at the
// equator.
float FlatMapController::geoScale()
{
	static float standardDistance = 0.0f;
	static bool standardDistanceSet = false;
	if (!standardDistanceSet) {
		LPoint a = {0.0f, 0.0f};
		LPoint b = {0.0f, 5.0f};
		standardDistance = distanceOnEarthFromGeoPoints(a, b);
		standardDistanceSet = true;
	}
	float latitude = fabsf(_params.y - 0.5f) * 180.0f;
	LPoint a = {latitude, 0.0f};
	LPoint b = {latitude, 5.0f};
	return (distanceOnEarthFromGeoPoints(a, b) * visiblePartFromCtrlScale()) / standardDistance;
}

AtlasPanelController::drawIterator *FlatMapController::detachDrawIterator(const LSize *size)
{
	if (!size)
		return new drawIteratorFlat(scene()->holder(), position(), visiblePartFromCtrlScale());
	return new drawIteratorFlat(*size, position(), visiblePartFromCtrlScale());
}

FlatMapController *FlatMapController::initWithMainWindowInterface(IWindowInterface *wnd)
{
	if (!AtlasPanelController::initWithMainWindowInterface(wnd))
		return 0;
	_scene = FlatMapScene::_alloc()->initWithController(this, wnd->acceleratedView()->holder());
	if (!getDefaultCamParams(&_params)) {
		_params.x = 0.5f;
		_params.y = 0.5f;
		_params.angle = 0.0f;
		_params.tilt = 0.0f;
		_params.scale = 0.0f;
		_params.spin = 0.0f;
	}
	return this;
}

LObject *FlatMapController::detachPrintObjectWithFrame(LRect frame, unsigned int flags)
{
	return LAutorelease(FlatDrawing::_alloc()->initWithFrameSizeScene(frame.size, scene(), flags));
}
