// UserCustomEntry: a location the user placed. Its region is found from the
// map outlines the first time it is asked for.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(UserCustomEntry, LF_CLASSOF(PointEntry))

UserCustomEntry::UserCustomEntry() : _changed(false), _regionNeedsValidation(false)
{
}

void UserCustomEntry::interfaceRetain()
{
	retain();
}

void UserCustomEntry::interfaceRelease()
{
	release();
}

void UserCustomEntry::dealloc()
{
	PointEntry::dealloc();
}

UserCustomEntry *UserCustomEntry::initWithDictionaryRep(LMap *rep)
{
	if (!initWithDictionaryRepAndType(rep, kGeoTypeUserLocation))
		return 0;
	_changed = true;
	return this;
}

UserCustomEntry *UserCustomEntry::initWithGeoPointAndName(LPoint geoPoint, LString *name)
{
	if (!initWithPointTypeName(geoPoint, kGeoTypeUserLocation, name))
		return 0;
	_regionNeedsValidation = true;
	_changed = true;
	return this;
}

UserCustomEntry *UserCustomEntry::entryWithGeoPointAndName(LPoint geoPoint, LString *name)
{
	UserCustomEntry *entry = _alloc()->initWithGeoPointAndName(geoPoint, name);
	return entry ? (UserCustomEntry *)entry->l_autorelease() : 0;
}

// A copy of a database entry the user can edit.
UserCustomEntry *UserCustomEntry::initWithPointEntry(PointEntry *entry)
{
	if (!initWithPointTypeName(entry->geoPoint(0), kGeoTypeUserLocation, entry->ownName(0)))
		return 0;
	if ((_timeZoneName = entry->timeZoneName(0)) != 0)
		_timeZoneName->retain();
	if ((_countryUID = entry->countryUID()) != 0)
		_countryUID->retain();
	if ((_countyUID = entry->countyUID()) != 0)
		_countyUID->retain();
	if ((_stateUID = entry->stateUID()) != 0)
		_stateUID->retain();
	return this;
}

LString *UserCustomEntry::iconKey() const
{
	return kEntryMyLocationsIconKey;
}

bool UserCustomEntry::_commit()
{
	if (_changed || isEntryTemporary()) {
		LString *request = SQL_DB_Strings::insertOrReplaceGeoObjectWithDBMap(dbMap());
		bool ok = AtlasDatabase::instance()->simpleSearchWithRequest(request) != 0;
		_changed = !ok;
		if (!ok)
			return false;
	}
	return GenericEntry::_commit();
}

bool UserCustomEntry::shouldCommit() const
{
	return _changed || GenericEntry::shouldCommit() || isEntryTemporary();
}

LObject *UserCustomEntry::propertyForKey(const LString *key)
{
	LObject *value = PointEntry::propertyForKey(key);
	if (key != 0 && value == 0 && key->isEqualTo(kEntryPreferredNamePropertyKey))
		value = Utils::shortGeoCoordinatesDescription(geoPoint(0), Utils::kGeolocFormatFull);
	return value;
}

// Moving the point drops a name that only gave the old coordinates, and the
// region.
void UserCustomEntry::setGeoPoint(LPoint geoPoint)
{
	if (geoPoint.x == _geoPoint.x && geoPoint.y == _geoPoint.y)
		return;
	LString *oldName = Utils::shortGeoCoordinatesDescription(this->geoPoint(0), Utils::kGeolocFormatFull);
	_setGeoPoint(geoPoint);
	if (_ownName == 0 || _ownName->isEqualTo(oldName))
		setOwnName(0);
	setRegionUID(0);
	_changed = true;
}

void UserCustomEntry::setTimeZoneName(LString *name)
{
	LString *current = timeZoneName(0);
	if (current == name)
		return;
	if (name != 0 && name->isEqualTo(current))
		return;
	if (name != 0 && name->length() != 0) {
		if (_timeZoneName)
			_timeZoneName->release();
		_timeZoneName = name;
		name->retain();
		_changed = true;
	}
}

void UserCustomEntry::setPropertyForKey(LObject *value, const LString *key)
{
	if (key == 0)
		return;
	if (key->isEqualTo(kEntryOwnNamePropertyKey))
		setOwnName(lobj_cast<LString>(value));
	else if (key->isEqualTo(kEntryRegionPropertyKey))
		setRegionUID(lobj_cast<LString>(value));
	if (key->isEqualTo(kEntryCoordinatesPropertyKey)) {
		LArray *coordinates = lobj_cast<LArray>(value);
		if (coordinates && coordinates->count() == 2) {
			LValue *x = lobj_cast<LValue>(coordinates->objectAtIndex(0));
			LValue *y = lobj_cast<LValue>(coordinates->objectAtIndex(1));
			LPoint p;
			p.x = x ? x->floatValue() : 0.0f;
			p.y = y ? y->floatValue() : 0.0f;
			setGeoPoint(p);
		}
	} else if (!key->isEqualTo(kEntryAltNamesPropertyKey)) {
		GenericEntry::setPropertyForKey(value, key);
	}
}

LSet *UserCustomEntry::propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const
{
	LSet *keys = PointEntry::propertyKeys(set, mode);
	if (mode == kEntryKeyWrite) {
		keys->addObject(kEntryCoordinatesPropertyKey);
		if (set == kEntryKeySetFull)
			keys->addObject(kEntryOwnNamePropertyKey);
	}
	return keys;
}

void UserCustomEntry::setOwnName(LString *name)
{
	if (_ownName == name)
		return;
	if (name != 0 && name->isEqualTo(_ownName))
		return;
	if (_ownName) {
		_ownName->release();
		_ownName = 0;
	}
	if (name == 0 || name->length() == 0)
		name = Utils::shortGeoCoordinatesDescription(geoPoint(0), Utils::kGeolocFormatFull);
	_ownName = name;
	if (name)
		name->retain();
	if (_shortCaption) {
		_shortCaption->release();
		_shortCaption = 0;
	}
	_changed = true;
}

// A country, state or county UID makes the region be looked up again from
// the outlines; any other string is taken as the country's name.
void UserCustomEntry::setRegionUID(LString *uid)
{
	GenericEntry *entry = (uid != 0 && uid->length() != 0) ? GenericEntry::entryWithUID(uid) : 0;
	bool given = uid != 0;
	bool isCountry, isState, isCounty;
	if (!given) {
		isCountry = isState = isCounty = true;
	} else if (entry == 0) {
		isCountry = isState = isCounty = false;
	} else {
		isCountry = kGeoTypeCountry->isEqualTo(entry->ownType(0));
		isState = kGeoTypeState->isEqualTo(entry->ownType(0));
		isCounty = kGeoTypeCounty->isEqualTo(entry->ownType(0));
	}
	bool countryByName = given && entry == 0 && !uid->isEqualTo(_countryName);
	bool countryChanged = false, stateChanged = false, countyChanged = false;
	if (isCountry)
		countryChanged = (!given && _countryUID != 0) || (given && !uid->isEqualTo(_countryUID));
	if (isState)
		stateChanged = (!given && _stateUID != 0) || (given && !uid->isEqualTo(_stateUID));
	if (isCounty)
		countyChanged = (!given && _countyUID != 0) || (given && !uid->isEqualTo(_countyUID));
	if (!(countryByName || countryChanged || stateChanged || countyChanged))
		return;

	LObject **fields[] = { (LObject **)&_countryUID, (LObject **)&_countryName, (LObject **)&_stateUID,
		               (LObject **)&_stateName,  (LObject **)&_countyUID,   (LObject **)&_countyName,
		               (LObject **)&_regionName };
	for (unsigned int i = 0; i < sizeof(fields) / sizeof(fields[0]); i++)
		if (*fields[i]) {
			(*fields[i])->release();
			*fields[i] = 0;
		}
	_changed = true;
	if (countryByName) {
		_countryName = uid;
		uid->retain();
		_regionNeedsValidation = false;
	} else {
		_regionNeedsValidation = true;
	}
}

static LString *retainedUID(LString *uid)
{
	if (uid)
		uid->retain();
	return uid;
}

// Finds the county, state and country under the point, from the smallest
// region up.
void UserCustomEntry::_validateRegionUID() const
{
	UserCustomEntry *self = const_cast<UserCustomEntry *>(this);
	if (!_regionNeedsValidation)
		return;
	if (_countryUID == 0) {
		CShapeManager *shapes = CShapeManager::instance();
		if (_stateUID == 0) {
			if (_countyUID == 0)
				self->_countyUID = retainedUID(shapes->objectUIDByHit(geoPoint(0), CShapeManager::kShapeIDCounties));
			if (_countyUID != 0) {
				GenericEntry *county = GenericEntry::entryWithUID(_countyUID);
				if (county) {
					self->_stateUID = retainedUID(county->stateUID());
					self->_countryUID = retainedUID(county->countryUID());
				}
			}
		}
		if (_countryUID == 0) {
			if (_stateUID == 0)
				self->_stateUID = retainedUID(shapes->objectUIDByHit(geoPoint(0), CShapeManager::kShapeIDStates));
			if (_stateUID != 0) {
				GenericEntry *state = GenericEntry::entryWithUID(_stateUID);
				self->_countryUID = retainedUID(state ? state->countryUID() : 0);
			}
			if (_countryUID == 0)
				self->_countryUID =
				    retainedUID(shapes->objectUIDByHit(geoPoint(0), CShapeManager::kShapeIDCountries));
		}
	}
	_regionNeedsValidation = false;
}

LString *UserCustomEntry::countryUID() const
{
	_validateRegionUID();
	return GenericEntry::countryUID();
}

LString *UserCustomEntry::stateUID() const
{
	_validateRegionUID();
	return GenericEntry::stateUID();
}

LString *UserCustomEntry::countyUID() const
{
	_validateRegionUID();
	return GenericEntry::countyUID();
}

// User locations also store the names of their regions.
LMap *UserCustomEntry::dbMap() const
{
	LMap *map = PointEntry::dbMap();
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
