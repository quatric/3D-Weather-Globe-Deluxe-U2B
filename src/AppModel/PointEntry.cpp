// PointEntry: an entry at a point on the globe, with its time zone and the
// online weather data for the place.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(PointEntry, LF_CLASSOF(GenericEntry))

PointEntry::PointEntry()
    : _geoPoint(LZeroPoint), _earthVec(1.0f, 0.0f, 0.0f), _onlineData(0), _unused(0), _customWeatherID(0),
      _timeZone(0)
{
}

void PointEntry::dealloc()
{
	if (_onlineData) {
		_onlineData->invalidate();
		_onlineData = 0;
	}
	if (_timeZone) {
		_timeZone->release();
		_timeZone = 0;
	}
	GenericEntry::dealloc();
}

void PointEntry::interfaceRetain()
{
	retain();
}

void PointEntry::interfaceRelease()
{
	release();
}

// Unnamed points are called by their coordinates.
PointEntry *PointEntry::initWithPointTypeName(LPoint geoPoint, LString *type, LString *name)
{
	if (name == 0 || name->length() == 0)
		name = Utils::shortGeoCoordinatesDescription(geoPoint, Utils::kGeolocFormatFull);
	if (!initWithNameAndType(name, type))
		return 0;
	_setGeoPoint(geoPoint);
	return this;
}

void PointEntry::loadValuesFromDictionaryRep(LMap *rep)
{
	GenericEntry::loadValuesFromDictionaryRep(rep);
	LPoint p;
	p.y = UMapParsingUtils::loadFloatFromObject(rep->objectForKey(kPointEntryLat));
	p.x = UMapParsingUtils::loadFloatFromObject(rep->objectForKey(kPointEntryLong));
	_setGeoPoint(p);
	if (_ownName && _ownName->length() != 0)
		return;
	_ownName = Utils::shortGeoCoordinatesDescription(_geoPoint, Utils::kGeolocFormatFull);
	if (_ownName)
		_ownName->retain();
}

LPoint PointEntry::geoPoint(unsigned char *mixed) const
{
	if (mixed)
		*mixed = 0;
	return _geoPoint;
}

int PointEntry::customWeatherID(unsigned char *mixed) const
{
	if (mixed)
		*mixed = 0;
	return _customWeatherID;
}

int PointEntry::currentGMTOffset(unsigned char *mixed) const
{
	if (mixed)
		*mixed = 0;
	LTimeZone *zone = timeZone(0);
	return zone ? zone->secondsFromGMT() : 0;
}

float PointEntry::visualWeight() const
{
	return 10.0f;
}

void PointEntry::_setGeoPoint(LPoint geoPoint)
{
	_geoPoint = geoPoint;
	_earthVec = earthVecFromGeoPoint(geoPoint);
}

void PointEntry::_wrapperDataWasUpdated(LMap *info)
{
	LNotificationCenter::center()->postNotification(kPointEntryWasUpdatedViaOnlineNotification, this, info);
}

void PointEntry::_wrapperDataWasExpired(LMap *info)
{
	LNotificationCenter::center()->postNotification(kPointEntryWasExpiredViaOnlineNotification, this, info);
}

CWOnlineCityData *PointEntry::onlineWrapper()
{
	return _onlineData;
}

void PointEntry::unscheduleReports(unsigned int mask)
{
	if (_onlineData)
		_onlineData->unscheduleReports(mask);
}

void PointEntry::unscheduleAllReports()
{
	if (_onlineData)
		_onlineData->unscheduleReports(kAllReportsMask);
}

void PointEntry::manualUpdateOfScheduledReports()
{
	if (_onlineData)
		_onlineData->manualUpdateOfScheduledReports();
}

void PointEntry::singleUpdateTaskForScheduledReports()
{
	if (_onlineData)
		_onlineData->singleUpdateTaskForScheduledReports();
}

bool PointEntry::downloadingReport(TReportType type)
{
	return _onlineData ? _onlineData->downloadingReport(type) : false;
}

float PointEntry::distanceToEntry(const PointEntry *entry) const
{
	return distanceOnEarthFromEarthVecs(&_earthVec, &entry->_earthVec);
}

bool PointEntry::getEntryGeoRect(LRect *rect)
{
	if (rect) {
		rect->size = LZeroSize;
		rect->origin = geoPoint(0);
	}
	return true;
}

PointEntry *PointEntry::initWithDBMap(LMap *dbMap)
{
	if (!GenericEntry::initWithDBMap(dbMap))
		return 0;
	_customWeatherID = UMapParsingUtils::loadIntFromObject(dbMap->objectForKey(kColumnNameCWID));
	LPoint p;
	p.y = UMapParsingUtils::loadFloatFromObject(dbMap->objectForKey(kColumnNameLat));
	p.x = UMapParsingUtils::loadFloatFromObject(dbMap->objectForKey(kColumnNameLong));
	_setGeoPoint(p);
	return this;
}

LSet *PointEntry::propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const
{
	LSet *keys = GenericEntry::propertyKeys(set, mode);
	if (mode == kEntryKeyRead) {
		keys->addObject(kEntryCoordinatesPropertyKey);
		if (timeZone(0)) {
			keys->addObject(kEntryGMTOffsetPropertyKey);
			keys->addObject(kEntryDateTimePropertyKey);
		}
		if (set == kEntryKeySetWithWeather)
			keys->addObject(kEntryWeatherForecastPropertyKey);
	}
	return keys;
}

LMap *PointEntry::entryDictionary() const
{
	LMap *map = GenericEntry::entryDictionary();
	if (map == 0)
		map = LMap::map();
	map->setObjectForKey(LValue::valueWithFloat(_geoPoint.x), kPointEntryLong);
	map->setObjectForKey(LValue::valueWithFloat(_geoPoint.y), kPointEntryLat);
	return map;
}

LMap *PointEntry::dbMap() const
{
	LMap *map = GenericEntry::dbMap();
	if (map == 0)
		map = LMap::map();
	map->setObjectForKey(LValue::valueWithFloat(_geoPoint.x), kColumnNameLong);
	map->setObjectForKey(LValue::valueWithFloat(_geoPoint.y), kColumnNameLat);
	return map;
}

// The zone is named by its tz name, or by a whole number of hours from GMT.
LTimeZone *PointEntry::timeZone(unsigned char *mixed) const
{
	if (mixed)
		*mixed = 0;
	if (_timeZone == 0 && timeZoneName(0) != 0) {
		_timeZone = LTimeZone::_alloc()->initWithUNIXName(timeZoneName(0));
		if (_timeZone == 0)
			_timeZone = LTimeZone::_alloc()->initWithSecondsFromGMT(timeZoneName(0)->intValue() * 3600);
	}
	return _timeZone;
}

CWOnlineCityData *PointEntry::_createWrapperIfNeeded()
{
	if (_onlineData == 0)
		_onlineData = CWOnlineCityData::_alloc()->initWithPointEntry(this);
	return _onlineData;
}

LMap *PointEntry::dictionaryForReport(TReportType type)
{
	return _createWrapperIfNeeded()->dictionaryForReport(type);
}

LObject *PointEntry::propertyForKey(const LString *key)
{
	LObject *value = GenericEntry::propertyForKey(key);
	if (key == 0 || value != 0)
		return value;
	if (key->isEqualTo(kEntryCoordinatesPropertyKey)) {
		LPoint p = geoPoint(0);
		return LArray::arrayWithObjects(LValue::valueWithFloat(p.x), LValue::valueWithFloat(p.y), (LObject *)0);
	}
	if (key->isEqualTo(kEntryGMTOffsetPropertyKey) || key->isEqualTo(kEntryDateTimePropertyKey))
		return timeZone(0);
	if (key->isEqualTo(kEntryWeatherForecastPropertyKey))
		return dictionaryForReport(kReportTypeExpanded);
	return 0;
}

void PointEntry::scheduleReports(unsigned int mask)
{
	_createWrapperIfNeeded()->scheduleReports(mask);
}

void PointEntry::singleUpdateTaskForReports(unsigned int mask)
{
	_createWrapperIfNeeded()->singleUpdateTaskForReports(mask);
}
