// FlatDrawing: printing the flat map. Geographic points map linearly to
// the view through the visible geographic rect.

#import <Cocoa/Cocoa.h>
#include "RenderLib.h"

LF_IMPLEMENT_CLASS(FlatDrawing, LF_CLASSOF(BaseDrawing))

void FlatDrawing::dealloc()
{
	BaseDrawing::dealloc();
}

FlatDrawing *FlatDrawing::initWithFrameSizeScene(LSize size, BaseScene *scene, unsigned int flags)
{
	BaseDrawing::initWithFrameSizeScene(size, scene, flags);
	_baseGeoRect = LZeroRect;
	_geoRect = LZeroRect;
	if (_controller) {
		LPoint position = _controller->position();
		float height = ((FlatMapController *)_controller)->visiblePartFromCtrlScale();
		float width = (_frameSize.width / _frameSize.height) * height * 0.5f;
		float x = width * -0.5f + position.x;
		LRect geo = geoRectFromMapRect(LMakeRect(x, height * -0.5f + position.y, width, height));
		_baseGeoRect = geo;
		_geoRect = geo;
		if (x < 0)
			_geoRect.origin.x -= 360.0f;
	}
	return this;
}

// The part of the geographic rect in view. The view clip rect stays the
// whole frame (as in the original).
void FlatDrawing::adjustClippingsToViewRect(LRect rect)
{
	LRect r = LIntersectionRect(rect, LMakeRect(0, 0, _frameSize.width, _frameSize.height));
	_clipGeoRect.origin.y = (_baseGeoRect.size.height * r.origin.y) / _frameSize.height + _baseGeoRect.origin.y;
	_clipGeoRect.origin.x = (_baseGeoRect.size.width * r.origin.x) / _frameSize.width + _baseGeoRect.origin.x;
	_clipGeoRect.size.width = (_baseGeoRect.size.width * r.size.width) / _frameSize.width;
	_clipGeoRect.size.height = (_baseGeoRect.size.height * r.size.height) / _frameSize.height;
}

bool FlatDrawing::isGeoRectVisible(LRect geoRect)
{
	if (geoRect.origin.x + geoRect.size.width < _clipGeoRect.origin.x)
		geoRect.origin.x += 360.0f;
	return LIntersectsRect(_clipGeoRect, geoRect);
}

LPoint FlatDrawing::viewPointFromGeoPoint(const LPoint &geoPoint, PanelDrawIterator *iterator, unsigned char *visible)
{
	float offset = iterator ? iterator->_mapOffset * 360.0f : 0.0f;
	float x = ((offset + geoPoint.x) - _geoRect.origin.x) * _frameSize.width / _geoRect.size.width;
	float y = ((geoPoint.y - _geoRect.origin.y) * _frameSize.height) / _geoRect.size.height;
	if (visible)
		*visible = !(x < 0 || y < 0 || _frameSize.width < x || _frameSize.height < y);
	return LMakePoint(x, y);
}

// Straight parallels and meridians; the parallels' captions at the right
// on the first map copy, the meridians' at the top.
void FlatDrawing::drawGridInRect(LRect)
{
	PanelDrawIterator *it = _controller->detachDrawIterator(&_frameSize);
	float lonStep = _controller->gridStepFromScale(1);
	float latStep = _controller->gridStepFromScale(0);
	LPoint position = _controller->position();
	float width = _frameSize.width;
	float height = _frameSize.height;
	CPRFont *font = gridMarkerFont();
	_setPrimaryColor(gridColor());
	_beginDrawingLines(0.5f);
	for (int pass = 0; it->next(); pass++) {
		LRect mapRect;
		((FlatMapController::drawIteratorFlat *)it)->evalRenderMapRect(position, &mapRect);
		for (float lat = latStep <= 11.0f ? -90.0f : -80.0f; lat < 89.99f; lat += latStep) {
			LPoint p = viewPointFromGeoPoint(LMakePoint(position.x * 360.0f - 180.0f, lat), 0, 0);
			if (p.y <= -10.0f || height + 10.0f <= p.y)
				continue;
			if (lat > -89.99f)
				_drawLine(LMakePoint(0, p.y), LMakePoint(width, p.y));
			if (pass == 0) {
				LPoint at = LMakePoint(width - (lonStep < 0.9999f ? 48.0f : 32.0f), p.y);
				_drawString(font,
				            Utils::shortSingleCoordDescription(lat, 1, (Utils::TGeolocFormat)(latStep < 0.9999f)), at);
			}
		}
		LPoint west = viewPointFromGeoPoint(LMakePoint(0, 0), 0, 0);
		LPoint east = viewPointFromGeoPoint(LMakePoint(360.0f, 0), 0, 0);
		for (float lon = -180.0f; lon < 179.99f; lon += lonStep) {
			LPoint p = viewPointFromGeoPoint(LMakePoint(lon, position.y * 180.0f - 90.0f), 0, 0);
			float x = (east.x - west.x) * it->_mapOffset + p.x;
			if (x < -35.0f || width + 35.0f < x)
				continue;
			_drawLine(LMakePoint(x, 0), LMakePoint(x, height));
			LPoint at = LMakePoint(x, (height - 4.0f) - font->size());
			_drawString(font, Utils::shortSingleCoordDescription(lon, 0, (Utils::TGeolocFormat)(lonStep < 0.9999f)),
			            at);
		}
	}
	_endDrawingLines();
	delete it;
}

// Lines that would jump across the view (a polygon crossing the date line)
// become moves; points left of the view are shifted a map width when that
// keeps them in it.
LObjToNSObjWrapper *FlatDrawing::transformGeoPath(LObjToNSObjWrapper *wrapper, PanelDrawIterator *iterator)
{
	float left = LZeroPoint.x;
	float width = _frameSize.width;
	float mapWidth = _baseGeoRect.size.width;
	NSBezierPath *geoPath = (NSBezierPath *)wrapper->nsObject();
	NSBezierPath *path = [NSBezierPath bezierPath];
	NSInteger count = [geoPath elementCount];
	float previousX = 0;
	for (NSInteger i = 0; i < count; i++) {
		NSPoint points[3];
		NSBezierPathElement element = [geoPath elementAtIndex:i associatedPoints:points];
		unsigned char visible = 0;
		LPoint p = viewPointFromGeoPoint(LMakePoint((float)points[0].x, (float)points[0].y), iterator, &visible);
		float x = p.x;
		if (element == NSBezierPathElementMoveTo) {
			if (visible)
				[path moveToPoint:NSMakePoint(x, p.y)];
		} else if (element == NSBezierPathElementLineTo) {
			if (x + 0.001f < left && !(width + left <= (mapWidth + x) - 0.001f))
				x = mapWidth + x;
			if (fabsf(x - previousX) <= width * 0.98f)
				[path lineToPoint:NSMakePoint(x, p.y)];
			else
				[path moveToPoint:NSMakePoint(x, p.y)];
		}
		previousX = x;
	}
	return LObjToNSObjWrapper::wrapperWithNSObject(path);
}
