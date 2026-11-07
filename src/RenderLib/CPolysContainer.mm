// CPolysContainer: a shape's polygons, read from its index BLOB; the finest
// LOD is kept as NSBezierPaths for hit testing.

#include "RenderLib.h"
#import <AppKit/AppKit.h>

LF_IMPLEMENT_CLASS(CPolysContainer, LF_CLASSOF(CShapeContainer))

static inline uint32_t _be32(const unsigned char *p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return __builtin_bswap32(v);
}

static inline uint16_t _be16(uint16_t v)
{
	return __builtin_bswap16(v);
}

// ---------------------------------------------------------------------------
// _polyLineIterator

CPolysContainer::_polyLineIterator::_polyLineIterator(IndexBuffersBLOB *indexes)
{
	_data = indexes->blobData();
	LF_CHECK_PTR(_data);
	LRetain(_data);
	_cursor = 0;
	_lod = 0;
	_poly = 0;
	_polyCount = 0;
	_uid = 0;
	_chunk = 0;
	_count = 0;
	_indices = 0;
	_lodCount = _be32((const unsigned char *)_data->bytes());
}

CPolysContainer::_polyLineIterator::~_polyLineIterator()
{
	releaseAndClear(_data);
}

bool CPolysContainer::_polyLineIterator::nextLOD()
{
	if (_cursor == 0) {
		_cursor = (const unsigned char *)_data->bytes() + 4;
		if (_lodCount == 0)
			return false;
		_lod = -1;
	}
	if (_lod + 1 >= _lodCount)
		return false;
	_lod = _be32(_cursor);
	_polyCount = _be32(_cursor + 4);
	_cursor += 8;
	_poly = -1;
	return true;
}

bool CPolysContainer::_polyLineIterator::nextPoly()
{
	if (_poly + 1 >= _polyCount)
		return false;
	const unsigned char *p = _cursor;
	_poly = _be32(p);
	_uid = (const char *)p + 4;
	_chunk = _be32(p + 0xe);
	_count = _be32(p + 0x12);
	_indices = (const uint16_t *)(p + 0x16);
	_cursor = p + 0x16 + _count * 2;
	return true;
}

// ---------------------------------------------------------------------------
// CPolysContainer

CPolysContainer::CPolysContainer() : _paths(0), _bounds(0), _allBounds(LZeroRect), _largestBounds(LZeroRect)
{
}

void CPolysContainer::dealloc()
{
	releaseAndClear(_paths);
	releaseAndClear(_bounds);
	CShapeContainer::dealloc();
}

CShapeDrawable *CPolysContainer::drawableForProjection(CProjection *projection, int mode, float scale)
{
	CPolysDrawable *drawable = (CPolysDrawable *)CShapeContainer::drawableForProjection(projection, mode, scale);
	if (drawable)
		drawable->setTriangleEdgeLeng(scale);
	return drawable;
}

LClass *CPolysContainer::drawableClass()
{
	return LF_CLASSOF(CPolysDrawable);
}

bool CPolysContainer::hitTest(LPoint geoPoint)
{
	_loadShapesIfNeeded();
	const LRect *bounds = (const LRect *)_bounds->bytes();
	int count = _paths ? _paths->count() : 0;
	for (int i = 0; i < count; i++, bounds++) {
		if (bounds->origin.x <= geoPoint.x && bounds->origin.y <= geoPoint.y &&
		    geoPoint.x <= bounds->origin.x + bounds->size.width &&
		    geoPoint.y <= bounds->size.height + bounds->origin.y &&
		    _isGeoPointVisibleForPath(geoPoint, (LObjToNSObjWrapper *)_paths->objectAtIndex(i)))
			return true;
	}
	return false;
}

LRect CPolysContainer::allPathsBoundsGeoRect()
{
	_loadShapesIfNeeded();
	return _allBounds;
}

LRect CPolysContainer::largestPathBoundGeoRect()
{
	_loadShapesIfNeeded();
	return _largestBounds;
}

LArray *CPolysContainer::allPaths()
{
	_loadShapesIfNeeded();
	return _paths;
}

bool CPolysContainer::_loadPathsForLod(int lod, LArray *paths, LData *bounds, LRect *allBounds, LRect *largestBounds)
{
	_polyLineIterator it(_indexes);
	StPointerBlock points(1000, false);
	bool measure = bounds != 0 || allBounds != 0 || largestBounds != 0;
	if (allBounds)
		*allBounds = LZeroRect;
	if (largestBounds)
		*largestBounds = LZeroRect;
	while (it.nextLOD()) {
		while (it.nextPoly()) {
			if (it._lod != lod)
				continue;
			VertexBuffersBLOB *vertices = VertexBuffersBLOB::verticesWithUID(LString::stringWithCString(it._uid, 10));
			if (vertices == 0)
				continue;
			const LPoint *src = vertices->pointsForBufferAtIndex(it._chunk, lod);
			if (src == 0)
				return false;
			int count = it._count;
			if (points.size() < (unsigned int)count * sizeof(LPoint))
				points.setSize(count * sizeof(LPoint));
			LPoint *dst = (LPoint *)points.bytes();
			float minX = 360, minY = 360, maxX = -360, maxY = -360;
			for (int i = 0; i < count; i++) {
				LPoint p = src[_be16(it._indices[i])];
				if (measure) {
					if (p.x > maxX)
						maxX = p.x;
					if (p.y > maxY)
						maxY = p.y;
					if (p.x < minX)
						minX = p.x;
					if (p.y < minY)
						minY = p.y;
				}
				dst[i] = p;
			}
			paths->addObject(_createPathWithPoints(dst, count));
			if (!measure)
				continue;
			LRect rect = LMakeRect(minX, minY, maxX - minX, maxY - minY);
			if (bounds)
				bounds->appendBytes(&rect, sizeof(rect));
			if (largestBounds &&
			    largestBounds->size.width * largestBounds->size.height < rect.size.height * rect.size.width)
				*largestBounds = rect;
			if (allBounds && allBounds->size.width == 0) {
				*allBounds = rect;
			} else if (allBounds) {
				// The polygon may lie across the date line from the others:
				// keep the narrowest union.
				LRect here = LUnionRect(*allBounds, rect);
				LRect west = LUnionRect(*allBounds, LMakeRect(rect.origin.x - 360, rect.origin.y, rect.size.width,
				                                              rect.size.height));
				LRect east = LUnionRect(*allBounds, LMakeRect(rect.origin.x + 360, rect.origin.y, rect.size.width,
				                                              rect.size.height));
				*allBounds = west.size.width <= here.size.width ? west : here;
				if (east.size.width < allBounds->size.width)
					*allBounds = east;
			}
		}
	}
	return true;
}

bool CPolysContainer::writeToFile(int lod, const char *path)
{
	FILE *f = fopen(path, "wt");
	_polyLineIterator it(_indexes);
	StPointerBlock points(1000, false);
	while (it.nextLOD()) {
		while (it.nextPoly()) {
			if (it._lod != lod)
				continue;
			VertexBuffersBLOB *vertices = VertexBuffersBLOB::verticesWithUID(LString::stringWithCString(it._uid, 10));
			if (vertices == 0)
				continue;
			const LPoint *src = vertices->pointsForBufferAtIndex(it._chunk, lod);
			if (src == 0) {
				if (f)
					fclose(f);
				return false;
			}
			int count = it._count;
			if (points.size() < (unsigned int)count * sizeof(LPoint))
				points.setSize(count * sizeof(LPoint));
			LPoint *dst = (LPoint *)points.bytes();
			fprintf(f, "%d\n", count);
			for (int i = 0; i < count; i++) {
				dst[i] = src[_be16(it._indices[i])];
				fprintf(f, "%f %f\n", (double)dst[i].x, (double)dst[i].y);
			}
		}
	}
	fclose(f);
	return true;
}

bool CPolysContainer::_loadShapes()
{
	LAutoreleasePool::addNewPoolToTop();
	bool loaded;
	{
		_polyLineIterator it(_indexes);
		if (_paths == 0)
			_paths = LArray::_alloc()->init();
		if (_bounds == 0)
			_bounds = LData::_alloc()->init();
		loaded = _loadPathsForLod(it._lodCount - 1, _paths, _bounds, &_allBounds, &_largestBounds);
	}
	LAutoreleasePool::removeTopPool();
	return loaded;
}

LObjToNSObjWrapper *CPolysContainer::_createPathWithPoints(LPoint *points, int count)
{
	NSBezierPath *path = [NSBezierPath bezierPath];
	// LPoint is two floats; NSPoint two CGFloats.
	NSPoint stack[256];
	NSPoint *ns = count <= 256 ? stack : (NSPoint *)malloc(count * sizeof(NSPoint));
	for (int i = 0; i < count; i++)
		ns[i] = NSMakePoint(points[i].x, points[i].y);
	[path appendBezierPathWithPoints:ns count:count];
	if (ns != stack)
		free(ns);
	[path closePath];
	return LObjToNSObjWrapper::wrapperWithNSObject(path);
}

bool CPolysContainer::_isGeoPointVisibleForPath(LPoint geoPoint, LObjToNSObjWrapper *path)
{
	return [(NSBezierPath *)path->nsObject() containsPoint:NSMakePoint(geoPoint.x, geoPoint.y)];
}
