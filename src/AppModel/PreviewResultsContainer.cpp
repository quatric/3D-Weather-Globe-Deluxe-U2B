// PreviewResultsContainer: the rows of the search preview. The results come
// sorted by entry type; the runs of a type become zones, sorted by the type's
// weight. When there are more results than the preview has rows, the zones
// at the end are cut first: down to 8 rows (4 for typed entries) in a first
// pass, then to 1.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(PreviewResultsContainer, 0)

PreviewResultsContainer *PreviewResultsContainer::initWithSQLResults(SQLiteResultsContainer *results,
                                                                     unsigned char foundInRegions)
{
	if (LObject::init() == 0)
		return 0;
	_results = LRetain(results);
	_foundInRegions = foundInRegions != 0;
	return this;
}

void PreviewResultsContainer::dealloc()
{
	delete _zones;
	_zones = 0;
	if (_limitedList) {
		_limitedList->release();
		_limitedList = 0;
	}
	if (_results) {
		_results->release();
		_results = 0;
	}
	if (_regionsString) {
		_regionsString->release();
		_regionsString = 0;
	}
	LObject::dealloc();
}

int PreviewResultsContainer::_compareGeoZones(const void *a, const void *b, const void *)
{
	const SGeoZone *za = *(SGeoZone *const *)a;
	const SGeoZone *zb = *(SGeoZone *const *)b;
	AtlasDatabase *db = AtlasDatabase::instance();
	return db->entryWeight(zb->typeID) - db->entryWeight(za->typeID);
}

static int _typeOfRow(AtlasDatabase *db, SQLiteResultsContainer *results, unsigned int index)
{
	LMap *row = results->objectAtIndex(index);
	return db->entryTypeIDFromStrings(kEntryTypeUseCaseSearchPreview, (LString *)row->objectForKey(kColumnNameType),
	                                  (LString *)row->objectForKey(kColumnNameAltType));
}

void PreviewResultsContainer::_createZonesIfNeeded()
{
	if (_zones)
		return;
	_zones = new TArray<SGeoZone *, _TPtrArrayAddRemoveFunc<SGeoZone> >;
	int count = _results ? _results->count() : 0;
	AtlasDatabase *db = AtlasDatabase::instance();
	int start = 0;
	while (start < count) {
		LAutoreleasePool::addNewPoolToTop();
		int type = _typeOfRow(db, _results, start);
		// The end of the run of the type.
		int lo = start + 1, hi = count - 1, mid = lo, last = 0;
		while (lo <= hi) {
			mid = (lo + hi) >> 1;
			if (_typeOfRow(db, _results, mid) == type) {
				last = 1;
				lo = mid + 1;
			} else {
				hi = mid - 1;
				last = -1;
			}
		}
		int end = mid + (last > 0 ? 1 : 0);
		if (start < end) {
			SGeoZone *zone = new SGeoZone;
			zone->typeID = type;
			LRange range = LMakeRange(start, end - start);
			zone->ranges.addObject(range);
			zone->count = end - start;
			_zones->addObject(zone);
		}
		LAutoreleasePool::removeTopPool();
		start = end;
	}
	_zones->setComparator(_compareGeoZones);
	_zones->sort();
	// Zones of the same type join.
	int zones = _zones->count();
	int lastType = zones > 0 ? (*_zones)[0]->typeID : 0;
	for (int i = 1; i < zones; i++) {
		SGeoZone *zone = (*_zones)[i];
		if (zone->typeID == lastType) {
			SGeoZone *previous = (*_zones)[i - 1];
			previous->count += (int)zone->ranges[0].length;
			previous->ranges.addObject(zone->ranges[0]);
			_zones->removeObjectAtIndex(i);
			zones--;
			i--;
		} else {
			lastType = zone->typeID;
		}
	}
}

int PreviewResultsContainer::allGeotypesCount()
{
	if (_zones == 0)
		_createZonesIfNeeded();
	return _zones ? _zones->count() : 0;
}

void PreviewResultsContainer::_createLimitedListIfNeeded()
{
	if (_limitedList)
		return;
	_limitedList = LArray::_alloc()->init();
	TArray<SGeoZoneRange> shown;
	int zones = allGeotypesCount();
	int limit = zones <= _limit ? _limit : zones;
	int total = _results ? _results->count() : 0;
	for (int i = 0; i < zones; i++) {
		SGeoZoneRange r = { (*_zones)[i]->typeID, (*_zones)[i]->count };
		shown.addObject(r);
	}
	int excess = total - limit;
	for (int pass = 0; excess > 0 && pass < 2; pass++) {
		for (int i = zones - 1; i >= 0 && excess > 0; i--) {
			SGeoZoneRange &r = shown[i];
			if (r.count <= 1)
				continue;
			int keep = pass != 0 ? 1 : r.typeID == 0 ? 8 : 4;
			int cut = r.count - keep;
			if (cut < 0)
				cut = 0;
			if (excess < cut)
				cut = excess;
			r.count -= cut;
			excess -= cut;
		}
	}
	AtlasDatabase *db = AtlasDatabase::instance();
	int cityType = db->entryTypeIDFromStrings(kEntryTypeUseCaseSearchPreview, kGeoTypeCity, 0);
	int capitalType = db->entryTypeIDFromStrings(kEntryTypeUseCaseSearchPreview, kGeoTypeCity, kAltTypeCapital);
	int stateCapitalType = db->entryTypeIDFromStrings(kEntryTypeUseCaseSearchPreview, kGeoTypeCity, kAltTypeStateCapital);
	int countryStateCapitalType =
	    db->entryTypeIDFromStrings(kEntryTypeUseCaseSearchPreview, kGeoTypeCity, kAltTypeCountryStateCapital);
	int countySeatType = db->entryTypeIDFromStrings(kEntryTypeUseCaseSearchPreview, kGeoTypeCity, kAltTypeCountySeat);
	// The capitals are counted with the cities in the cities' rest row.
	int capitals = 0;
	for (int z = 0; z < zones; z++) {
		SGeoZone *zone = (*_zones)[z];
		const SGeoZoneRange &r = shown[z];
		int range = 0;
		unsigned int taken = 0;	// from the range
		bool fromEnd = true;	// the last of a range first, while they have a population
		int takenFromEnd = 0;
		for (int k = 0; k < r.count; k++) {
			SearchPreviewEntry *entry;
			if (k == r.count - 1 && zone->count != r.count) {
				int rest = zone->count;
				if (cityType == zone->typeID)
					rest += capitals;
				entry = SearchPreviewEntry::entryWithCollapsedInfo(rest, zone->typeID, k == 0, _foundInRegions);
			} else {
				const LRange &rr = zone->ranges[range];
				int location = (int)rr.location;
				unsigned int length = rr.length;
				LString *uid;
				for (;;) {
					unsigned int index = fromEnd ? location + length - 1 - taken : taken + location - takenFromEnd;
					uid = (LString *)_results->objectAtIndex(index)->objectForKey(kColumnNameUID);
					if (!fromEnd)
						break;
					GenericEntry *e = GenericEntry::entryWithUID(uid);
					if (e && e->population(0) >= 0)
						break;
					fromEnd = false;
				}
				entry = SearchPreviewEntry::entryWithUID(uid, k == 0, _foundInRegions);
				taken++;
				if (fromEnd)
					takenFromEnd++;
				if (length <= taken) {
					range++;
					taken = 0;
					fromEnd = true;
					takenFromEnd = 0;
				}
			}
			if (_limitedList->count() == 0) {
				LString *subtitle;
				if (_foundInRegions)
					subtitle = _regionsString;
				else if (_showAll)
					subtitle = LString::stringWithFormat(LString::stringFromTable("Show All Entries", "AppModel",
					                                                              getAppModelModule()),
					                                     _results ? _results->count() : 0);
				else
					subtitle = LString::stringWithCString("");
				const char *title;
				if (_foundInRegions)
					title = _manyRegions ? "Found In Regions" : "Found In Region";
				else
					title = _showAll ? "All entries" : "Found By Name";
				SearchPreviewEntry *header = SearchPreviewEntry::entryWithStrings(
				    LString::stringFromTable(title, "AppModel", getAppModelModule()), subtitle, _foundInRegions);
				_limitedList->addObject(header);
				if (_foundInRegions)
					header->setFirstRegionRowFlag(1);
				else if (_showAll)
					header->setAllNamesRowFlag(1);
			}
			_limitedList->addObject(entry);
		}
		int type = zone->typeID;
		if (type == capitalType || type == stateCapitalType || type == countySeatType || type == countryStateCapitalType)
			capitals += zone->count;
	}
}

int PreviewResultsContainer::limitedListCount()
{
	if (_limitedList == 0)
		_createLimitedListIfNeeded();
	return _limitedList ? _limitedList->count() : 0;
}

SearchPreviewEntry *PreviewResultsContainer::limitedEntryAtIndex(int index)
{
	if (_limitedList == 0)
		_createLimitedListIfNeeded();
	if (_limitedList == 0 || index < 0 || index >= (int)_limitedList->count())
		return 0;
	return (SearchPreviewEntry *)_limitedList->objectAtIndex(index);
}
