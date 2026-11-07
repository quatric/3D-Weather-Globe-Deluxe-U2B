// GenericEntry: an atlas entry as loaded from the database or made by the
// user, plus the route set items and the icon cache cleanup.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(RouteSetItem, 0)
LF_IMPLEMENT_CLASS(RouteSetBreak, LF_CLASSOF(RouteSetItem))
LF_IMPLEMENT_CLASS(GenericEntry, LF_CLASSOF(RouteSetItem))

LMap *GenericEntry::__iconCacheMap = 0;
LMap *GenericEntry::__smallIconCacheMap = 0;

// ---------------------------------------------------------------------------
// RouteSetBreak

RouteSetBreak *RouteSetBreak::routeSetBreak()
{
	return (RouteSetBreak *)_alloc()->init()->l_autorelease();
}

RouteSetBreak *RouteSetBreak::init()
{
	return (RouteSetBreak *)LObject::init();
}

// ---------------------------------------------------------------------------
// GenericEntry

class _entryCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (GenericEntry::__iconCacheMap) {
			GenericEntry::__iconCacheMap->release();
			GenericEntry::__iconCacheMap = 0;
		}
		if (GenericEntry::__smallIconCacheMap) {
			GenericEntry::__smallIconCacheMap->release();
			GenericEntry::__smallIconCacheMap = 0;
		}
	}
};
static _entryCleanup _sEntryCleanup;

static inline void releaseField(LObject *&field)
{
	if (field) {
		field->release();
		field = 0;
	}
}
#define RELEASE_FIELD(f) releaseField((LObject *&)(f))

static inline LString *copyString(LString *s)
{
	LString *copy = LString::_alloc()->init();
	copy->setValue(s);
	return copy;
}

GenericEntry::GenericEntry()
    : _wrappers(0), _uid(0), _ownName(0), _ownType(0), _altNamesString(0), _timeZoneName(0), _altType(0),
      _countryUID(0), _stateUID(0), _countyUID(0), _population(-1), _countryName(0), _stateName(0),
      _countyName(0), _regionName(0), _kind(0), _userInfo(0), _userInfoChanged(false), _shortCaption(0),
      _alternativeNames(0), _entryInfo(0), _icon(0), _previewIcon(0)
{
}

void GenericEntry::dealloc()
{
	RELEASE_FIELD(_wrappers);
	RELEASE_FIELD(_uid);
	RELEASE_FIELD(_ownName);
	RELEASE_FIELD(_ownType);
	RELEASE_FIELD(_altNamesString);
	RELEASE_FIELD(_shortCaption);
	RELEASE_FIELD(_alternativeNames);
	RELEASE_FIELD(_userInfo);
	RELEASE_FIELD(_timeZoneName);
	RELEASE_FIELD(_altType);
	RELEASE_FIELD(_countryUID);
	RELEASE_FIELD(_stateUID);
	RELEASE_FIELD(_countyUID);
	RELEASE_FIELD(_countyName);
	RELEASE_FIELD(_stateName);
	RELEASE_FIELD(_countryName);
	RELEASE_FIELD(_regionName);
	RELEASE_FIELD(_kind);
	RELEASE_FIELD(_entryInfo);
	RELEASE_FIELD(_icon);
	RELEASE_FIELD(_previewIcon);
	LObject::dealloc();
}

// Entries are made with one of the specific initializers.
GenericEntry *GenericEntry::init()
{
	dealloc();
	return 0;
}

void GenericEntry::interfaceRetain()
{
	retain();
}

void GenericEntry::interfaceRelease()
{
	release();
}

void GenericEntry::setPropertyForKey(LObject *, const LString *)
{
}

void GenericEntry::releaseNormalIcon() const
{
	RELEASE_FIELD(_icon);
}

void GenericEntry::releasePreviewIcon() const
{
	RELEASE_FIELD(_previewIcon);
}

LString *GenericEntry::iconKey() const
{
	LString *type = _ownType;
	if (type) {
		if (type->isEqualTo(kGeoTypeCounty) || _ownType->isEqualTo(kGeoTypeState) ||
		    _ownType->isEqualTo(kGeoTypeCountry))
			return kEntryCountriesIconKey;
		if (_ownType->isEqualTo(kGeoTypeTimezone))
			return kEntryTimezoneIconKey;
	}
	return kEntryMyLocationsIconKey;
}

bool GenericEntry::_commit()
{
	if (_entryInfo && _entryInfo->shouldCommit())
		return _entryInfo->commit();
	return true;
}

bool GenericEntry::shouldCommit() const
{
	return _entryInfo ? _entryInfo->shouldCommit() : false;
}

#define CLEAR_FLAG(f) \
	if (f)        \
		*(f) = 0;

LString *GenericEntry::uid(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	return _uid;
}

LString *GenericEntry::ownName(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	return _ownName;
}

LString *GenericEntry::ownType(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	return _ownType;
}

LString *GenericEntry::altType(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	return _altType;
}

LString *GenericEntry::timeZoneName(unsigned char *) const
{
	return _timeZoneName;
}

// The region, with the county in front when there is one.
LString *GenericEntry::fullRegionName(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	LString *region = regionName(0);
	if (countryName(0) == 0 && stateName(0) == 0)
		return region;
	if (countyName(0))
		region = LString::stringWithFormat("%@, %@", countyName(0), region);
	return region;
}

int GenericEntry::population(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	return _population;
}

LString *GenericEntry::countyUID() const
{
	return _countyUID;
}

LString *GenericEntry::stateUID() const
{
	return _stateUID;
}

LString *GenericEntry::countryUID() const
{
	return _countryUID;
}

float GenericEntry::visualWeight() const
{
	return 1.0f;
}

bool GenericEntry::getEntryGeoRect(LRect *rect)
{
	if (rect)
		rect->size = LZeroSize;
	return false;
}

LMap *GenericEntry::userInfo(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	if (_userInfo == 0)
		_loadUserInfo();
	return _userInfo;
}

// The localized kind ("City", "Volcano"...), capitalized.
LString *GenericEntry::kind(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	if (_kind == 0 && _ownType != 0) {
		StAutoreleasePool pool;
		_kind = AtlasDatabase::instance()->localizedEntryType(kEntryTypeUseCaseSortByKind, ownType(0), altType(0), false);
		if (_kind)
			_kind = _kind->capitalizedString();
		if (_kind)
			_kind->retain();
	}
	return _kind;
}

void GenericEntry::loadValuesFromDictionaryRep(LMap *rep)
{
	_ownName = lobj_cast<LString>(rep->objectForKey(kColumnNameName));
	if (_ownName)
		_ownName->retain();
}

static inline LObject *retained(LObject *obj)
{
	if (obj)
		obj->retain();
	return obj;
}
#define RETAINED(T, obj) ((T *)retained(obj))

GenericEntry *GenericEntry::initWithDBMap(LMap *dbMap)
{
	if (!LObject::init())
		return 0;
	_uid = RETAINED(LString, dbMap->objectForKey(kColumnNameUID));
	_ownName = RETAINED(LString, dbMap->objectForKey(kColumnNameName));
	_ownType = RETAINED(LString, dbMap->objectForKey(kColumnNameType));
	if (_uid && _ownName && _ownType) {
		_altNamesString = RETAINED(LString, dbMap->objectForKey(kColumnNameAltNames));
		_timeZoneName = RETAINED(LString, dbMap->objectForKey(kColumnNameHostTimeZone));
		_altType = RETAINED(LString, lobj_cast<LString>(dbMap->objectForKey(kColumnNameAltType)));
		_countryUID = RETAINED(LString, dbMap->objectForKey(kColumnNameHostCountryUID));
		_stateUID = RETAINED(LString, dbMap->objectForKey(kColumnNameHostStateUID));
		_countyUID = RETAINED(LString, dbMap->objectForKey(kColumnNameHostCountyUID));
		LObject *population = dbMap->objectForKey(kColumnNamePopulation);
		_population = population ? UMapParsingUtils::loadIntFromObject(population) : -1;
		return this;
	}
	dealloc();
	return 0;
}

LClass *GenericEntry::entryClassFromDBMap(LMap *dbMap)
{
	static LStrMap *sRemap = 0;
	if (dbMap == 0)
		return 0;
	if (sRemap == 0) {
		sRemap = new LStrMap;
		sRemap->setObjectForKey((intptr_t)LF_CLASSOF(CityPointEntry), kGeoTypeCity);
		sRemap->setObjectForKey((intptr_t)LF_CLASSOF(RouteObjectEntry), kGeoTypeRoute);
		LString *shapeTypes[] = { kGeoTypeCounty, kGeoTypeState, kGeoTypeCountry, kGeoTypeTimezone };
		for (unsigned int i = 0; i < sizeof(shapeTypes) / sizeof(shapeTypes[0]); i++)
			sRemap->setObjectForKey((intptr_t)LF_CLASSOF(ShapeEntry), shapeTypes[i]);
		LString *featureTypes[] = { kGeoTypeVolcano,          kGeoTypePeak,         kGeoTypeHills,
		                            kGeoTypeDesert,           kGeoTypeMountain,     kGeoTypeMountains,
		                            kGeoTypeDestroyedLandmark, kGeoTypeEndangeredSite, kGeoTypeBuildingStructure,
		                            kGeoTypeNationalPark,     kGeoTypeContinent,    kGeoTypeStrait,
		                            kGeoTypeBay,              kGeoTypeLagoon,       kGeoTypeFjord,
		                            kGeoTypeGulf,             kGeoTypeSea,          kGeoTypeLake,
		                            kGeoTypeOcean,            kGeoTypeWaterfall,    kGeoTypeZoo,
		                            kGeoTypeRiver,            kGeoTypeRoad };
		for (unsigned int i = 0; i < sizeof(featureTypes) / sizeof(featureTypes[0]); i++)
			sRemap->setObjectForKey((intptr_t)LF_CLASSOF(FeatureEntry), featureTypes[i]);
		sRemap->setObjectForKey((intptr_t)LF_CLASSOF(AppleStoreEntry), kGeoTypeAppleStore);
		sRemap->setObjectForKey((intptr_t)LF_CLASSOF(UserCustomEntry), kGeoTypeUserLocation);
	}
	LString *type = (LString *)dbMap->objectForKey(kColumnNameType);
	if (type) {
		LClass *cls = (LClass *)sRemap->objectForKey(type);
		if (cls == LF_CLASSOF(ShapeEntry)) {
			// Only the regions with an outline are shapes.
			LString *uid = (LString *)dbMap->objectForKey(kColumnNameUID);
			if (uid && AtlasDatabase::instance()->shapeExistsForEntryWithUID(uid))
				return cls;
			return LF_CLASSOF(PointEntry);
		}
		if (cls == LF_CLASSOF(RouteObjectEntry)) {
			// User routes have the longer UIDs.
			LString *uid = (LString *)dbMap->objectForKey(kColumnNameUID);
			if (uid->length() != 21)
				return cls;
			return LF_CLASSOF(UserRouteObjectEntry);
		}
		if (cls)
			return cls;
	}
	return LF_CLASSOF(GenericEntry);
}

GenericEntry *GenericEntry::entryWithDBMap(LMap *dbMap)
{
	LClass *cls = entryClassFromDBMap(dbMap);
	if (cls && cls->isSubclassOf(LF_CLASSOF(GenericEntry))) {
		GenericEntry *entry = ((GenericEntry *)cls->alloc())->initWithDBMap(dbMap);
		if (entry)
			return (GenericEntry *)entry->l_autorelease();
	}
	return 0;
}

GenericEntry *GenericEntry::entryWithUID(LString *uid)
{
	if (uid == 0)
		return 0;
	AtlasDatabase *db = AtlasDatabase::instance();
	GenericEntry *entry = db->temporaryEntryWithUID(uid);
	if (entry == 0)
		entry = db->dbEntryWithUID(uid);
	return entry;
}

// A new entry not yet in the database; it lives in the temporary cache until
// committed.
GenericEntry *GenericEntry::initWithDictionaryRepAndType(LMap *rep, LString *type)
{
	if (!LObject::init())
		return 0;
	_ownType = RETAINED(LString, type);
	_uid = RETAINED(LString, AtlasDatabase::generateUID());
	loadValuesFromDictionaryRep(rep);
	if (_uid && _ownName && _ownType) {
		AtlasDatabase::instance()->cacheTemporaryEntry(this);
		return this;
	}
	dealloc();
	return 0;
}

GenericEntry *GenericEntry::initWithNameAndType(LString *name, LString *type)
{
	if (!LObject::init())
		return 0;
	_ownName = RETAINED(LString, name);
	_ownType = RETAINED(LString, type);
	_uid = RETAINED(LString, AtlasDatabase::generateUID());
	if (_uid && _ownName && _ownType) {
		AtlasDatabase::instance()->cacheTemporaryEntry(this);
		return this;
	}
	dealloc();
	return 0;
}

bool GenericEntry::isEntryTemporary() const
{
	return AtlasDatabase::instance()->temporaryEntryWithUID(uid(0)) != 0;
}

bool GenericEntry::commit()
{
	bool ok = true;
	if (shouldCommit()) {
		ok = AtlasDatabase::instance()->commitEntry(this);
		LNotificationCenter::center()->postNotification(kEntryDidChangeNotification, this, 0);
	}
	return ok;
}

// A saved list entry: the database entry with the UID, or a user location
// made from the dictionary.
GenericEntry *GenericEntry::entryWithDictionaryRep(LMap *rep)
{
	if (rep == 0)
		return 0;
	LString *uid = lobj_cast<LString>(rep->objectForKey(kGenericEntryUID));
	GenericEntry *entry;
	if (uid && (entry = entryWithUID(uid)) != 0)
		return entry;
	entry = UserCustomEntry::_alloc()->initWithDictionaryRep(rep);
	if (entry)
		entry->l_autorelease();
	return entry;
}

LSet *GenericEntry::propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const
{
	LSet *keys = (LSet *)LComparableSet::_alloc()->init()->l_autorelease();
	if (mode == kEntryKeyRead) {
		if (population(0) > -1)
			keys->addObject(kEntryPopulationPropertyKey);
		if (alternativeNames(0) && alternativeNames(0)->count() > 0)
			keys->addObject(kEntryAltNamesPropertyKey);
		if (set == kEntryKeySetFull) {
			keys->addObject(kEntryOwnNamePropertyKey);
			keys->addObject(kEntryKindPropertyKey);
			keys->addObject(kEntryRegionPropertyKey);
		}
	}
	return keys;
}

LObject *GenericEntry::propertyForKey(const LString *key)
{
	if (key == 0)
		return 0;
	if (key->isEqualTo(kEntryOwnNamePropertyKey))
		return ownName(0);
	if (key->isEqualTo(kEntryKindPropertyKey))
		return kind(0);
	if (key->isEqualTo(kEntryRegionPropertyKey))
		return fullRegionName(0);
	if (key->isEqualTo(kEntryPopulationPropertyKey))
		return LValue::valueWithInt(population(0));
	if (key->isEqualTo(kEntryAltNamesPropertyKey))
		return alternativeNames(0);
	return 0;
}

EntryInfo *GenericEntry::entryInfoObject()
{
	if (_entryInfo == 0)
		_entryInfo = EntryInfo::_alloc()->initWithHostEntry(this);
	return _entryInfo;
}

// "Name (Region)", or the name alone.
LString *GenericEntry::shortCaption(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	if (_shortCaption == 0) {
		LString *name = ownName(0);
		LString *region = regionName(0);
		if (region == 0)
			_shortCaption = RETAINED(LString, name);
		else
			_shortCaption = LString::_alloc()->initWithFormat("%@ (%@)", name, region);
	}
	return _shortCaption;
}

// The database keeps alternative names as ";name;name...".
LArray *GenericEntry::alternativeNames(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	if (_alternativeNames == 0 && _altNamesString != 0) {
		LArray *names = 0;
		int count = 0;
		if (_altNamesString->hasPrefix(LString::stringWithCString(";"))) {
			names = _altNamesString->componentsSeparatedByString(LString::stringWithCString(";"));
			if (names)
				count = names->count();
		}
		while (--count >= 0) {
			LString *name = lobj_cast<LString>(names->objectAtIndex(count));
			if (name == 0 || name->length() == 0)
				names->removeObjectAtIndex(count);
		}
		_alternativeNames = RETAINED(LArray, names);
	}
	return _alternativeNames;
}

// The names of the hosting regions come from their entries.
static LString *nameOfEntryWithUID(LString *uid)
{
	GenericEntry *entry = GenericEntry::entryWithUID(uid);
	return entry ? copyString(entry->ownName(0)) : 0;
}

LString *GenericEntry::countyName(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	if (_countyName == 0 && countyUID() != 0)
		_countyName = nameOfEntryWithUID(countyUID());
	return _countyName;
}

LString *GenericEntry::stateName(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	if (_stateName == 0 && stateUID() != 0)
		_stateName = nameOfEntryWithUID(stateUID());
	return _stateName;
}

LString *GenericEntry::countryName(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	if (_countryName == 0 && countryUID() != 0)
		_countryName = nameOfEntryWithUID(countryUID());
	return _countryName;
}

// "State, Country", the country alone, or "County county".
LString *GenericEntry::regionName(unsigned char *mixed) const
{
	CLEAR_FLAG(mixed);
	if (_regionName == 0) {
		LString *country = countryName(0);
		LString *state = stateName(0);
		LString *county = countyName(0);
		if (country || state || county) {
			if (state && country)
				_regionName = LString::_alloc()->initWithFormat("%@, %@", state, country);
			else if (country)
				_regionName = copyString(country);
			else
				_regionName = LString::_alloc()->initWithFormat("%@ county", county);
		}
	}
	return _regionName;
}

// The user's notes are kept as a property list BLOB.
void GenericEntry::_loadUserInfo() const
{
	LMap *info = 0;
	PlistBLOB *blob = lobj_cast<PlistBLOB>(wrapperWithRepresentationOfType(kRepTypeUserNotes, 1));
	if (blob)
		info = lobj_cast<LMap>(blob->object());
	_userInfo = RETAINED(LMap, info);
	if (_userInfo == 0)
		_userInfo = LMap::_alloc()->init();
	_userInfoChanged = false;
}

LImage *GenericEntry::_iconForKey(LString *key, unsigned char small) const
{
	if (key == 0)
		return 0;
	LMap *cache;
	if (!small) {
		if (__iconCacheMap == 0)
			__iconCacheMap = LMap::_alloc()->init();
		cache = __iconCacheMap;
	} else {
		if (__smallIconCacheMap == 0)
			__smallIconCacheMap = LMap::_alloc()->init();
		cache = __smallIconCacheMap;
	}
	LImage *image = (LImage *)cache->objectForKey(key);
	if (image)
		return image;
	if (small)
		image = LImage::_alloc()->initWithContentsOfFile(
		    LString::stringWithFormat("INTERFACE\\Entries\\Small\\%@.png", key));
	if (image == 0)
		image = LImage::_alloc()->initWithContentsOfFile(LString::stringWithFormat("INTERFACE\\Entries\\%@.png", key));
	if (image == 0)
		return 0;
	cache->setObjectForKey(image, key);
	image->release();
	return image;
}

// A custom icon the user set, or the one for the entry's kind; *custom tells
// which.
LImage *GenericEntry::icon(unsigned char *custom) const
{
	if (_icon == 0) {
		StAutoreleasePool pool;
		if (AtlasDatabase::instance()->customIconExistsForEntryWithUID(uid(0))) {
			ImageBLOB *blob = lobj_cast<ImageBLOB>(wrapperWithRepresentationOfType(kRepTypeEntryIcon, 1));
			_icon = blob ? LImage::_alloc()->initWithData(blob->blobData()) : 0;
		}
		if (_icon == 0) {
			_icon = RETAINED(LImage, _iconForKey(iconKey(), 0));
			if (custom)
				*custom = 0;
		} else if (custom) {
			*custom = 1;
		}
	}
	return _icon;
}

LImage *GenericEntry::previewIcon(unsigned char *custom) const
{
	if (_previewIcon == 0) {
		StAutoreleasePool pool;
		if (AtlasDatabase::instance()->previewIconExistsForEntryWithUID(uid(0))) {
			ImageBLOB *blob =
			    lobj_cast<ImageBLOB>(wrapperWithRepresentationOfType(kRepTypeEntryPreviewIcon, 1));
			_previewIcon = blob ? LImage::_alloc()->initWithData(blob->blobData()) : 0;
		}
		if (_previewIcon == 0) {
			_previewIcon = RETAINED(LImage, _iconForKey(iconKey(), 1));
			if (custom)
				*custom = 0;
		} else if (custom) {
			*custom = 1;
		}
	}
	return _previewIcon;
}

// The BLOB of a representation type, loaded from the database on demand.
LObject *GenericEntry::wrapperWithRepresentationOfType(LString *repType, unsigned char load) const
{
	if (_wrappers == 0)
		_wrappers = LMap::_alloc()->init();
	LObject *wrapper = _wrappers->objectForKey(repType);
	if (load && wrapper == 0) {
		wrapper = BLOBWrapper::wrapperForObjectWithUID(uid(0), repType);
		if (wrapper)
			_wrappers->setObjectForKey(wrapper, repType);
	}
	return wrapper;
}

LMap *GenericEntry::entryDictionary() const
{
	return LMap::mapWithObjectsAndKeys(ownName(0), kColumnNameName, uid(0), kGenericEntryUID, (LObject *)0);
}

LMap *GenericEntry::dbMap() const
{
	LMap *map = LMap::mapWithObjectsAndKeys(_uid, kColumnNameUID, _ownName, kColumnNameName, _ownType,
	                                        kColumnNameType, 0);
	if (_altType)
		map->setObjectForKey(_altType, kColumnNameAltType);
	if (_altNamesString)
		map->setObjectForKey(_altNamesString, kColumnNameAltNames);
	if (_timeZoneName)
		map->setObjectForKey(_timeZoneName, kColumnNameHostTimeZone);
	if (countryUID())
		map->setObjectForKey(countryUID(), kColumnNameHostCountryUID);
	if (stateUID())
		map->setObjectForKey(stateUID(), kColumnNameHostStateUID);
	if (countyUID())
		map->setObjectForKey(countyUID(), kColumnNameHostCountyUID);
	if (_population > -1)
		map->setObjectForKey(LValue::valueWithInt(_population), kColumnNamePopulation);
	return map;
}

// ---------------------------------------------------------------------------
// SEntryCompareContext

SEntryCompareContext::SEntryCompareContext(TEntriesSortingMode m) : mode(m)
{
	sortKeys = new TStrMap<const char *, 10>;
	regionSortKeys = new TStrMap<const char *, 10>;
	names = new TStrMap<LString *, 28>;
	sortKeys->_resizeHashTableTo(0x3b3);
	regionSortKeys->_resizeHashTableTo(0x3cb);
	names->_resizeHashTableTo(100);
}

SEntryCompareContext::~SEntryCompareContext()
{
	delete sortKeys;
	sortKeys = 0;
	delete regionSortKeys;
	regionSortKeys = 0;
	delete names;
	names = 0;
}
