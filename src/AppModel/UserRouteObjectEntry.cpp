// UserRouteObjectEntry: a route the user made; it saves its row and its route
// BLOB separately.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(UserRouteObjectEntry, LF_CLASSOF(RouteObjectEntry))

UserRouteObjectEntry::UserRouteObjectEntry() : _routeSaved(true), _changed(false)
{
}

void UserRouteObjectEntry::interfaceRetain()
{
	retain();
}

void UserRouteObjectEntry::interfaceRelease()
{
	release();
}

LSet *UserRouteObjectEntry::propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const
{
	LSet *keys = RouteObjectEntry::propertyKeys(set, mode);
	if (mode == kEntryKeyWrite && set == kEntryKeySetFull)
		keys->addObject(kEntryOwnNamePropertyKey);
	return keys;
}

bool UserRouteObjectEntry::shouldCommit() const
{
	return GenericEntry::shouldCommit() || _changed || (_routeObject != 0 && !_routeSaved);
}

// An empty name falls back to the route's own.
void UserRouteObjectEntry::setOwnName(LString *name)
{
	if (name == 0) {
		if (_ownName == 0)
			return;
	} else if (name->isEqualTo(_ownName)) {
		return;
	}
	if (_ownName) {
		_ownName->release();
		_ownName = 0;
	}
	if (name == 0 || name->length() == 0)
		name = routeObject() ? routeObject()->preferredName() : 0;
	_ownName = name;
	if (name)
		name->retain();
	if (_shortCaption) {
		_shortCaption->release();
		_shortCaption = 0;
	}
	_changed = true;
}

void UserRouteObjectEntry::setPropertyForKey(LObject *value, const LString *key)
{
	if (key == 0)
		return;
	if (key->isEqualTo(kEntryOwnNamePropertyKey))
		setOwnName(lobj_cast<LString>(value));
	else
		GenericEntry::setPropertyForKey(value, key);
}

UserRouteObjectEntry *UserRouteObjectEntry::initWithRouteObject(RouteObject *route)
{
	if (route == 0) {
		dealloc();
		return 0;
	}
	if (!initWithNameAndType(route->preferredName(), kGeoTypeRoute))
		return 0;
	route->retain();
	_routeObject = route;
	_routeSaved = false;
	_changed = true;
	return this;
}

UserRouteObjectEntry *UserRouteObjectEntry::entryWithRouteObject(RouteObject *route)
{
	UserRouteObjectEntry *entry = _alloc()->initWithRouteObject(route);
	return entry ? (UserRouteObjectEntry *)entry->l_autorelease() : 0;
}

LMap *UserRouteObjectEntry::dbMap() const
{
	LMap *map = GenericEntry::dbMap();
	if (map == 0)
		map = LMap::map();
	if (countryName(0))
		map->setObjectForKey(countryName(0), kUserColumnNameHostCountryName);
	if (stateName(0))
		map->setObjectForKey(stateName(0), kUserColumnNameHostStateName);
	if (countyName(0))
		map->setObjectForKey(countyName(0), kUserColumnNameHostCountyName);
	return map;
}

// A new route starts empty.
RouteObject *UserRouteObjectEntry::routeObject() const
{
	RouteObjectEntry::routeObject();
	if (_routeObject == 0)
		_routeObject = RouteObject::_alloc()->initWithDictionaryRep(0, 0);
	return _routeObject;
}

bool UserRouteObjectEntry::_commit()
{
	AtlasDatabase *db = AtlasDatabase::instance();
	if (_changed) {
		bool ok = db->simpleSearchWithRequest(SQL_DB_Strings::insertOrReplaceGeoObjectWithDBMap(dbMap())) != 0;
		_changed = !ok;
		if (!ok)
			return false;
	}
	bool ok = GenericEntry::_commit();
	if (ok && _routeObject != 0 && !_routeSaved) {
		PlistBLOB *blob =
		    (PlistBLOB *)PlistBLOB::_alloc()->initWithParams(0, 0, _routeObject->dictionaryRepresentation())->l_autorelease();
		LString *request = SQL_DB_Strings::commitBLOB(blob, kRepTypeRouteObject, uid(0));
		ok = db->simpleSearchWithRequest(request) != 0;
		if (ok)
			_routeSaved = true;
	}
	return ok;
}
