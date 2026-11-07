// EntryBillboard: an atlas entry's weather and photo billboard.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(EntryBillboard, LF_CLASSOF(SmartBillboardData))

PointEntry *EntryBillboard::pointEntry()
{
	return lobj_cast<PointEntry>(_entry);
}

// The weather part is the left 128 points; the photos to its right.
unsigned int EntryBillboard::kindFromViewPoint(LPoint point, unsigned int mask)
{
	mask &= _kind;
	if ((mask & 4) == 0)
		return mask & 2;
	if ((mask & 2) != 0 && point.x - _rect.origin.x < 128.0f)
		return 2;
	return 4;
}

void EntryBillboard::dealloc()
{
	releaseAndClear(_entry);
	LObject::dealloc();
}

LSize EntryBillboard::bbSizeForVisibleMask(unsigned int mask)
{
	float height = 64;
	float width = 0;
	unsigned int kinds = _kind & mask;
	if (kinds & 2)
		width = 128;
	if (kinds & 4)
		width += 64;
	if (_kind & 0x100) {
		width *= 0.5f;
		height = 32;
	}
	return LMakeSize(width, height);
}

LComparable *EntryBillboard::uniqueKey()
{
	return _entry ? _entry->uid(0) : 0;
}

EntryBillboard *EntryBillboard::initWithEntry(GenericEntry *entry, unsigned int kind)
{
	LPoint position;
	PointEntry *point = lobj_cast<PointEntry>(entry);
	if (point) {
		position = point->geoPoint(0);
	} else {
		LRect bounds;
		bool found = false;
		if (entry && entry->getClass()->isSubclassOf(LF_CLASSOF(ShapeEntry))) {
			CShapeContainer *container = ((ShapeEntry *)entry)->shapeContainer();
			if (container->getClass()->isSubclassOf(LF_CLASSOF(CPolysContainer)))
				bounds = ((CPolysContainer *)container)->largestPathBoundGeoRect();
			else
				bounds = container->allPathsBoundsGeoRect();
			found = true;
		} else if (entry && entry->getClass()->isSubclassOf(LF_CLASSOF(RouteObjectEntry))) {
			RouteObject *route = ((RouteObjectEntry *)entry)->routeObject();
			if (route) {
				bounds = route->boundsGeoRect();
				found = true;
			}
		}
		if (!found) {
			dealloc();
			return 0;
		}
		position = LMakePoint(bounds.size.width * 0.5f + bounds.origin.x, bounds.size.height * 0.5f + bounds.origin.y);
	}
	if (!SmartBillboardData::initWithParams(position, 0, 0, kind))
		return 0;
	_entry = LRetain(entry);
	return this;
}
