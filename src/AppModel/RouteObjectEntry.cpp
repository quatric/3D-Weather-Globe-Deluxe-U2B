// RouteObjectEntry: a route from the atlas; its points are kept as a property
// list BLOB.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(RouteObjectEntry, LF_CLASSOF(GenericEntry))

RouteObjectEntry::RouteObjectEntry() : _blobUID(0), _routeObject(0)
{
}

void RouteObjectEntry::interfaceRetain()
{
	retain();
}

void RouteObjectEntry::interfaceRelease()
{
	release();
}

void RouteObjectEntry::dealloc()
{
	if (_blobUID) {
		_blobUID->release();
		_blobUID = 0;
	}
	if (_routeObject) {
		_routeObject->release();
		_routeObject = 0;
	}
	GenericEntry::dealloc();
}

LString *RouteObjectEntry::iconKey() const
{
	return kEntryRouteIconKey;
}

LString *RouteObjectEntry::blobUID(unsigned char *) const
{
	return _blobUID;
}

bool RouteObjectEntry::getEntryGeoRect(LRect *rect)
{
	if (rect == 0 || routeObject() == 0)
		return false;
	*rect = routeObject()->boundsGeoRect();
	return true;
}

LSet *RouteObjectEntry::propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const
{
	LSet *keys = GenericEntry::propertyKeys(set, mode);
	if (mode == kEntryKeyRead) {
		keys->addObject(kEntryRouteTotalDistancePropertyKey);
		keys->addObject(kEntryRouteEntriesCountPropertyKey);
	}
	return keys;
}

LObject *RouteObjectEntry::propertyForKey(const LString *key)
{
	LObject *value = GenericEntry::propertyForKey(key);
	if (key == 0 || value != 0)
		return value;
	if (key->isEqualTo(kEntryPreferredNamePropertyKey))
		return routeObject() ? routeObject()->preferredName() : 0;
	if (key->isEqualTo(kEntryRouteTotalDistancePropertyKey))
		return routeObject() ? LValue::valueWithFloat(routeObject()->totalDistance()) : 0;
	if (key->isEqualTo(kEntryRouteEntriesCountPropertyKey))
		return routeObject() ? LValue::valueWithInt(routeObject()->locationsCount()) : 0;
	return 0;
}

RouteObject *RouteObjectEntry::routeObject() const
{
	if (_routeObject == 0) {
		LMap *rep = 0;
		PlistBLOB *blob = lobj_cast<PlistBLOB>(wrapperWithRepresentationOfType(kRepTypeRouteObject, 1));
		if (blob)
			rep = lobj_cast<LMap>(blob->object());
		_routeObject = rep ? RouteObject::_alloc()->initWithDictionaryRep(rep, 0) : 0;
	}
	return _routeObject;
}
