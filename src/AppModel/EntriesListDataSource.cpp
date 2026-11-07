// EntriesListDataSource: the search list and its preview. The search results
// are kept as SQL rows; their entries are created as the list shows them.
// The results of the empty search (all entries) are kept until the database
// changes.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(EntriesListDataSource, 0)
LF_SELECTOR(EntriesListDataSource, _deleteEntries);
LF_SELECTOR(EntriesListDataSource, _restoreEntries);
LF_SELECTOR(EntriesListDataSource, _renewSearchResults);
LF_SELECTOR(EntriesListDataSource, _onRefreshDatabase);
LF_SELECTOR(EntriesListDataSource, _onEntryDidChange);

static const unsigned int kMaxSearchStringLength = 200;
static const unsigned int kSearchDelay = 300;	// ms

template <typename T> static inline void _release(T *&object)
{
	if (object) {
		object->release();
		object = 0;
	}
}

// The string cut to 200 characters, without surrounding white space.
static LString *_normalizedSearchString(const LString *string)
{
	LString *result;
	if (!string)
		result = LString::stringWithCString("");
	else if (string->length() <= kMaxSearchStringLength)
		result = LString::stringWithString(string);
	else
		result = string->substringWithRange(0, kMaxSearchStringLength);
	result->trimWithCharacters(LCharSet::charSetWithType(LCharSet::kWhitespaceAndNewline));
	return result;
}

// Drops the container's rows when its limit changes.
static void _setPreviewLimit(PreviewResultsContainer *container, int limit)
{
	if (limit != container->_limit && container->_limitedList) {
		container->_limitedList->release();
		container->_limitedList = 0;
	}
	container->_limit = limit;
}

static PreviewResultsContainer *_previewResultsWithRequest(LString *request, unsigned char foundInRegions)
{
	SQLiteResultsContainer *results = AtlasDatabase::instance()->searchWithRequest(request, (LMap *)0);
	return PreviewResultsContainer::_alloc()->initWithSQLResults(results, foundInRegions);
}

EntriesListDataSource *EntriesListDataSource::init()
{
	if (LObject::init() == 0)
		return 0;
	LNotificationCenter::center()->addObserver(this, "_onRefreshDatabase",
	                                           AtlasDatabase::kMainDatabaseChangedNotification, 0);
	LNotificationCenter::center()->addObserver(this, "_onEntryDidChange", GenericEntry::kEntryDidChangeNotification,
	                                           0);
	return this;
}

void EntriesListDataSource::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	_release(_searchString);
	_release(_lastAutocompleteString);
	_release(_customRequest);
	_release(_allPreviewResults);
	_release(_previewResults);
	_release(_regionPreviewResults);
	_release(_regionsString);
	_release(_searchResults);
	_release(_allEntriesResults);
	_release(_searchedEntries);
	LObject::dealloc();
}

void EntriesListDataSource::invalidate()
{
	if (_timer)
		_timer->invalidate();
	_timer = 0;
	release();
}

bool EntriesListDataSource::isPreviewPanelShown()
{
	return false;
}

void EntriesListDataSource::showPreviewPanel() {}

void EntriesListDataSource::hidePreviewPanel() {}

void EntriesListDataSource::reloadGenericList() {}

void EntriesListDataSource::_invalCurrentEntries()
{
	_release(_searchResults);
	_release(_searchedEntries);
	_usesCustomRequest = false;
}

void EntriesListDataSource::_invalPreviewResults()
{
	_release(_previewResults);
	_release(_regionPreviewResults);
	_release(_regionsString);
	_previewLimitsValid = false;
}

static void _postEntriesWereChanged(EntriesListDataSource *source)
{
	LNotificationCenter::center()->postNotification(EntriesListDataSource::kEntriesWereChangedNotification, source,
	                                                0);
}

void EntriesListDataSource::setGeotypeFilter(unsigned long long filter)
{
	if (_geotypeFilter == filter)
		return;
	_geotypeFilter = filter;
	_invalCurrentEntries();
	_release(_allEntriesResults);
	_invalPreviewResults();
	_postEntriesWereChanged(this);
}

void EntriesListDataSource::setPreviewEntriesLimit(int limit)
{
	if (_previewEntriesLimit != limit) {
		_previewEntriesLimit = limit;
		_previewLimitsValid = false;
	}
}

void EntriesListDataSource::setSortingMode(TEntriesSortingMode mode)
{
	if (_sortingMode == mode)
		return;
	_sortingMode = mode;
	if (_searchResults) {
		if (_searchResults == _allEntriesResults)
			_allEntriesSortingMode = mode;
		AtlasDatabase::instance()->sortSearchResults(_searchResults, _sortingMode);
		_release(_searchedEntries);
		_postEntriesWereChanged(this);
	}
}

// Lists the entries of the clicked row (a type, a region) instead.
void EntriesListDataSource::previewEntryWasClicked(SearchPreviewEntry *entry)
{
	_invalCurrentEntries();
	_release(_customRequest);
	_customRequest = LRetain(entry->customRequestForSubstring(_searchString));
	_usesCustomRequest = _customRequest != 0;
	if (isPreviewPanelShown()) {
		reloadGenericList();
		hidePreviewPanel();
	}
}

void EntriesListDataSource::deleteEntries(TLArray<GenericEntry *> *entries)
{
	invokeSelector("_deleteEntries", entries);
}

intptr_t EntriesListDataSource::_deleteEntries(char *args)
{
	LArray *entries = LF_ARG(args, 0, LArray *);
	if (!entries || entries->count() == 0)
		return 0;
	LUndoManager *undo = LUndoManager::instance();
	undo->registerUndo(this, "_restoreEntries", entries);
	undo->setActionName(LString::stringFromTable(entries->count() != 1 ? "Delete Locations" : "Delete Location",
	                                             "Undo", getAppModelModule()));
	AtlasDatabase::instance()->deleteEntries(entries);
	return 0;
}

intptr_t EntriesListDataSource::_restoreEntries(char *args)
{
	LArray *entries = LF_ARG(args, 0, LArray *);
	if (!entries || entries->count() == 0)
		return 0;
	LUndoManager::instance()->registerUndo(this, "_deleteEntries", entries);
	AtlasDatabase::instance()->restoreEntries(entries);
	return 0;
}

bool EntriesListDataSource::addNewEntry(GenericEntry *entry)
{
	if (!entry || !entry->commit())
		return false;
	LUndoManager *undo = LUndoManager::instance();
	undo->registerUndo(this, "_deleteEntries", LArray::arrayWithObjects(entry, (LObject *)0));
	undo->setActionName(LString::stringFromTable("Add Location", "Undo", getAppModelModule()));
	return true;
}

bool EntriesListDataSource::canDeleteEntries(TLArray<GenericEntry *> *entries)
{
	int count = entries ? entries->count() : 0;
	for (int i = 0; i < count; i++)
		if (!lobj_cast<UserCustomEntry>(entries->objectAtIndex(i)))
			return false;
	return count != 0;
}

bool EntriesListDataSource::canDeleteEntries(TArray<int> *indexes)
{
	int count = indexes ? indexes->count() : 0;
	for (int i = 0; i < count; i++)
		if (!lobj_cast<UserCustomEntry>(searchedEntryAtIndex((*indexes)[i], 1)))
			return false;
	return count != 0;
}

intptr_t EntriesListDataSource::_renewSearchResults(char *)
{
	_timer = 0;
	_invalPreviewResults();
	if (!isPreviewPanelShown())
		showPreviewPanel();
	_invalCurrentEntries();
	_postEntriesWereChanged(this);
	return 0;
}

intptr_t EntriesListDataSource::_onRefreshDatabase(char *)
{
	_release(_allPreviewResults);
	_invalCurrentEntries();
	_release(_allEntriesResults);
	_invalPreviewResults();
	_usesCustomRequest = _customRequest != 0;
	_postEntriesWereChanged(this);
	return 0;
}

intptr_t EntriesListDataSource::_onEntryDidChange(char *)
{
	_postEntriesWereChanged(this);
	return 0;
}

void EntriesListDataSource::setSearchString(const LString *string)
{
	LString *search = _normalizedSearchString(string);
	bool changed = true;
	if (_searchString) {
		changed = !_searchString->isEqualTo(search);
		_release(_searchString);
	}
	_searchString = LRetain(search);
	if (changed) {
		_invalCurrentEntries();
		_invalPreviewResults();
		_postEntriesWereChanged(this);
	}
}

void EntriesListDataSource::deferredSearchingWithString(const LString *string, SAutoCompleteResult *result)
{
	if (result) {
		result->string = 0;
		result->selection = LZeroRange;
	}
	if (_timer)
		_timer->invalidate();
	_timer = 0;
	LString *search = _normalizedSearchString(string);
	if (!SQL_DB_Strings::fastLookupIsPossibleForSearchWithSubstring(search) &&
	    (search->length() != 0 || _showsAll)) {
		// Searched when entered.
		if (search->length() == 0) {
			_release(_searchString);
			_invalCurrentEntries();
			_invalPreviewResults();
			_postEntriesWereChanged(this);
		}
	} else {
		_release(_searchString);
		_searchString = LRetain(search);
		_timer = LTimer::timerWithParams(kSearchDelay, this, "_renewSearchResults", false, 0);
		// Completed while typing, not while deleting.
		if (result && (!_lastAutocompleteString || _lastAutocompleteString->length() < search->length())) {
			LArray *terms = SQLiteUtils::separatedSearchTermsForString(search);
			LString *request = terms->count() != 0 ? SQL_DB_Strings::autocompleteRequestForTerm(terms, _geotypeFilter) : 0;
			LArray *rows = request ? AtlasDatabase::instance()->simpleSearchWithRequest(request) : 0;
			if (rows && rows->count() != 0) {
				LMap *row = (LMap *)rows->objectAtIndex(0);
				LString *keyword = lobj_cast<LString>(row->objectForKey(LString::stringWithCString("kw")));
				LString *lastTerm = (LString *)terms->objectAtIndex(terms->count() - 1);
				if (keyword && keyword->length() > lastTerm->length()) {
					LString *lastChar = search->substringFromIndex(search->length() - 1);
					if (lastChar->isEqualTo(lastChar->lowercaseString()))
						keyword = keyword->lowercaseString();
					LString *completion = keyword->substringFromIndex(lastTerm->length());
					result->string = LString::stringWithFormat("%@%@", search, completion);
					result->selection = LMakeRange(search->length(), completion->length());
				}
			}
		}
	}
	_release(_lastAutocompleteString);
	_lastAutocompleteString = LRetain(search);
}

LArray *EntriesListDataSource::_currentSearchResults()
{
	if (!_searchResults) {
		LAutoreleasePool::addNewPoolToTop();
		LString *search = _searchString;
		_showsAll = (!search || search->length() == 0) && !_usesCustomRequest;
		if (!search)
			search = LString::stringWithCString("");
		LString *request = _usesCustomRequest ? _customRequest
		                                      : SQL_DB_Strings::searchForGeoObjectWithSubstring(search, _geotypeFilter);
		AtlasDatabase *db = AtlasDatabase::instance();
		if (!_showsAll || !_allEntriesResults) {
			_searchResults = LRetain(db->searchWithRequest(request, _sortingMode));
			if (_showsAll) {
				_allEntriesResults = LRetain(_searchResults);
				_allEntriesSortingMode = _sortingMode;
			}
		} else {
			if (_allEntriesSortingMode != _sortingMode) {
				_allEntriesSortingMode = _sortingMode;
				db->sortSearchResults(_allEntriesResults, _sortingMode);
			}
			_searchResults = LRetain(_allEntriesResults);
		}
		LAutoreleasePool::removeTopPool();
	}
	if (!_searchedEntries) {
		int count = _searchResults ? _searchResults->count() : 0;
		_searchedEntries = LArray::_alloc()->initWithCapacity(count);
		for (int i = 0; i < count; i++)
			_searchedEntries->addObject(LNull::null);
	}
	return _searchedEntries;
}

int EntriesListDataSource::searchedEntriesCount()
{
	return _currentSearchResults()->count();
}

GenericEntry *EntriesListDataSource::searchedEntryAtIndex(int index, unsigned char load)
{
	LObject *entry = 0;
	if (_searchedEntries) {
		if (index < 0 || index >= (int)_searchedEntries->count())
			return 0;
		entry = _searchedEntries->objectAtIndex(index);
	}
	if (entry != LNull::null)
		return (GenericEntry *)entry;
	if (!load)
		return 0;
	LAutoreleasePool::addNewPoolToTop();
	char uid[256];
	void *values[] = { uid };
	int types[] = { 3 };	// text
	entry = 0;
	if (_searchResults->loadValues(values, types, 1, index))
		entry = GenericEntry::entryWithUID(LString::stringWithCString(uid));
	if (entry)
		_searchedEntries->replaceObjectAtIndex(entry, index);
	LAutoreleasePool::removeTopPool();
	return (GenericEntry *)entry;
}

int EntriesListDataSource::indexOfSearchedEntryWithUID(LString *uid)
{
	if (!uid || uid->length() == 0)
		return -1;
	const char *wanted = uid->cString();
	char value[256];
	void *values[] = { value };
	int types[] = { 3 };	// text
	int count = _searchResults ? _searchResults->count() : 0;
	for (int i = 0; i < count; i++) {
		if (_searchResults->loadValues(values, types, 1, i) && strcmp(wanted, value) == 0)
			return i;
	}
	return -1;
}

// The name results (or all entries), and the region results.
void EntriesListDataSource::_evalPreviewResultsIfNeeded()
{
	if (_previewResults)
		return;
	_release(_regionPreviewResults);
	_release(_regionsString);
	_manyRegions = false;
	if (!_searchString || _searchString->length() == 0) {
		if (!_allPreviewResults) {
			_allPreviewResults = _previewResultsWithRequest(SQL_DB_Strings::selectColumnsByName(0, 0, 0, 0), 0);
			_allPreviewResults->_showAll = true;
		}
		_previewResults = LRetain(_allPreviewResults);
		return;
	}
	AtlasDatabase *db = AtlasDatabase::instance();
	unsigned char hostsFound = 0;
	_previewResults =
	    _previewResultsWithRequest(SQL_DB_Strings::selectColumnsByName(_searchString, 0, &hostsFound, 0), 0);
	if (hostsFound)
		return;
	SQLiteResultsContainer *regionResults =
	    db->searchWithRequest(SQL_DB_Strings::selectColumnsByRegion(_searchString, 0, 0), (LMap *)0);
	if (!regionResults)
		return;
	_regionPreviewResults = PreviewResultsContainer::_alloc()->initWithSQLResults(regionResults, 1);
	if (regionResults->count() != 0) {
		SQLiteResultsContainer *regions = db->searchWithRequest(
		    SQL_DB_Strings::selectDistinctNameOfRegionsBySubstring(_searchString), (LMap *)0);
		int count = regions ? regions->count() : 0;
		for (int i = 0; i < count; i++) {
			LString *name = lobj_cast<LString>(regions->objectAtIndex(i)->objectForKey(kColumnNameName));
			if (!name)
				continue;
			if (!_regionsString) {
				// A copy: the original appended to the row's string.
				_regionsString = LRetain(LString::stringWithString(name));
			} else {
				_regionsString->appendFormat(", %@", name);
				_manyRegions = true;
			}
		}
	}
	_regionPreviewResults->_regionsString = LRetain(_regionsString);
	if (_manyRegions)
		_regionPreviewResults->_manyRegions = true;
}

// Shares the preview rows between the name and the region results: the
// region results get a row per type, the name results the rest.
void EntriesListDataSource::_recalcPreviewLimitsIfNeeded()
{
	if (_previewLimitsValid)
		return;
	_previewLimitsValid = true;
	_evalPreviewResultsIfNeeded();
	if (!_previewResults)
		return;
	int nameCount = _previewResults->_results ? _previewResults->_results->count() : 0;
	int nameTypes = _previewResults->allGeotypesCount();
	int regionTypes = _regionPreviewResults ? _regionPreviewResults->allGeotypesCount() : 0;
	// Less the title rows.
	int limit = _previewEntriesLimit + (nameTypes == 0) - 2 + (regionTypes == 0);
	if (limit < 0)
		limit = 0;
	if (limit < nameCount + regionTypes) {
		if (_regionPreviewResults)
			_setPreviewLimit(_regionPreviewResults, regionTypes);
		int nameLimit = limit - regionTypes;
		if (nameLimit < nameTypes)
			nameLimit = nameTypes;
		_setPreviewLimit(_previewResults, nameLimit);
	} else {
		_setPreviewLimit(_previewResults, nameCount);
		if (_regionPreviewResults)
			_setPreviewLimit(_regionPreviewResults, limit - nameCount);
	}
}

SearchPreviewEntry *EntriesListDataSource::previewEntryAtIndex(int index)
{
	_evalPreviewResultsIfNeeded();
	_recalcPreviewLimitsIfNeeded();
	if (!_previewResults)
		return 0;
	int nameRows = _previewResults->limitedListCount();
	if (index < nameRows)
		return _previewResults->limitedEntryAtIndex(index);
	return _regionPreviewResults ? _regionPreviewResults->limitedEntryAtIndex(index - nameRows) : 0;
}

int EntriesListDataSource::previewGeoEntriesCount()
{
	_evalPreviewResultsIfNeeded();
	_recalcPreviewLimitsIfNeeded();
	if (!_previewResults)
		return 0;
	int regionCount = _regionPreviewResults && _regionPreviewResults->_results
	                      ? _regionPreviewResults->_results->count()
	                      : 0;
	return regionCount + (_previewResults->_results ? _previewResults->_results->count() : 0);
}

int EntriesListDataSource::previewEntriesCount()
{
	_evalPreviewResultsIfNeeded();
	_recalcPreviewLimitsIfNeeded();
	if (!_previewResults)
		return 0;
	int regionRows = _regionPreviewResults ? _regionPreviewResults->limitedListCount() : 0;
	return regionRows + _previewResults->limitedListCount();
}

static int _indexOfPreviewEntry(PreviewResultsContainer *container, LString *uid)
{
	int count = container ? container->limitedListCount() : 0;
	for (int i = 0; i < count; i++) {
		SearchPreviewEntry *preview = container->limitedEntryAtIndex(i);
		GenericEntry *entry = preview ? preview->geoEntry() : 0;
		if (entry && entry->uid(0)->isEqualTo(uid))
			return i;
	}
	return -1;
}

int EntriesListDataSource::indexOfPreviewEntryWithUID(LString *uid)
{
	if (!_previewResults || !uid || uid->length() == 0)
		return -1;
	int index = _indexOfPreviewEntry(_previewResults, uid);
	if (index >= 0)
		return index;
	index = _indexOfPreviewEntry(_regionPreviewResults, uid);
	return index >= 0 ? index + _previewResults->limitedListCount() : -1;
}
