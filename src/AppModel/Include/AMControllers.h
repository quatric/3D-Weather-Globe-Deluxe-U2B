// The controllers behind the application's panels: the search results and
// lists of entries, the entry info, the current set, the route manager and
// the installation of the high resolution maps.

#ifndef AMCONTROLLERS_H
#define AMCONTROLLERS_H

#include "AMView.h"

// Copies the high resolution maps (the zip files listed in
// "HiResMapsZipFiles.plist") from the installation disc to the application,
// in a background thread, with progress notifications.
class InstallationController : public LObject {
	LF_DECLARE_CLASS(InstallationController)
public:
	static LString *kHiResMapsFileNameKey;
	static LString *kHiResMapsFileSizeKey;
	static LString *kHiResMapsFileIndexKey;
	static LString *kHiResMapsFileCopiedSizeKey;
	static LString *kHiResMapsInstallationDidEndNotification;
	static LString *kHiResMapsInstallationProgressNotification;

	InstallationController()
	    : _mapsList(0), _filesToInstall(0), _totalSize(0), _busy(false), _installing(false), _notify(false)
	{
	}
	virtual void dealloc();
	virtual InstallationController *getOwnerLObject() { return this; }
	virtual intptr_t _installHiResMaps(char *args);
	virtual intptr_t _installHiResMapsCompleted(char *args);
	virtual intptr_t _postNotification(char *args);

	// False when there is nothing to install or the user cancelled.
	bool installHiResMapsInBackground(unsigned char notify);
	bool isHiResMapsValid();
	bool isInstalling() const { return _installing; }
	void _validateHiResMapsList();

	LArray *_mapsList;	// of LString, the files
	LArray *_filesToInstall;	// their paths on the disc
	unsigned long long _totalSize;
	bool _busy;
	bool _installing;
	bool _notify;
};

// The icons of an entry's properties (none in this version).
class EntryPropertyController : public LObject {
	LF_DECLARE_CLASS(EntryPropertyController)
public:
	EntryPropertyController() : _icons(0) {}
	virtual void dealloc();
	virtual EntryPropertyController *getOwnerLObject() { return this; }

	static LImage *iconForEntry(GenericEntry *entry);
	static LImage *smallIconForEntry(GenericEntry *entry);

	LObject *_icons;
};

// Search results grouped by entry type into zones, most important types
// first, and a list of at most `_limit` preview rows of them: a title row,
// the entries, and per type cut short a row standing for the rest.
class PreviewResultsContainer : public LObject {
	LF_DECLARE_CLASS(PreviewResultsContainer)
public:
	struct SGeoZone {
		int typeID;
		int count;
		TArray<LRange> ranges;	// in the results
	};
	struct SGeoZoneRange {
		int typeID;
		int count;	// shown
	};

	PreviewResultsContainer()
	    : _results(0), _limit(0), _foundInRegions(false), _manyRegions(false), _showAll(false), _regionsString(0),
	      _zones(0), _limitedList(0)
	{
	}
	virtual void dealloc();
	virtual PreviewResultsContainer *getOwnerLObject() { return this; }

	PreviewResultsContainer *initWithSQLResults(SQLiteResultsContainer *results, unsigned char foundInRegions);
	int allGeotypesCount();
	int limitedListCount();
	SearchPreviewEntry *limitedEntryAtIndex(int index);
	void _createZonesIfNeeded();
	void _createLimitedListIfNeeded();
	static int _compareGeoZones(const void *a, const void *b, const void *context);

	SQLiteResultsContainer *_results;
	int _limit;
	bool _foundInRegions;
	bool _manyRegions;
	bool _showAll;
	LString *_regionsString;
	TArray<SGeoZone *, _TPtrArrayAddRemoveFunc<SGeoZone> > *_zones;
	LArray *_limitedList;	// of SearchPreviewEntry
};

// The current route set (the one the globe shows), and its editing, with
// undo. Each edit runs through a selector, so the undo manager can replay its
// opposite.
class CurrentSetController : public LObject {
	LF_DECLARE_CLASS(CurrentSetController)
public:
	CurrentSetController() : _route(0) {}
	virtual CurrentSetController *init();
	virtual void dealloc();
	virtual CurrentSetController *getOwnerLObject() { return this; }
	virtual intptr_t _replaceCurrentSet(char *args);
	virtual intptr_t _addToCurrentSet(char *args);
	virtual intptr_t _deleteItemAtIndex(char *args);
	virtual intptr_t _deleteItemsWithOrderedIndicies(char *args);
	virtual intptr_t _insertItemIntoRoute(char *args);
	virtual intptr_t _insertItemsWithOrderedIndexesIntoRoute(char *args);
	virtual intptr_t _moveItemsFromIndexesToIndex(char *args);
	virtual intptr_t _moveItemsStartingAtIndexToIndexes(char *args);
	virtual intptr_t _setNameForEntry(char *args);
	virtual intptr_t _setColorForEntry(char *args);
	virtual intptr_t _setColorForEntries(char *args);
	virtual intptr_t _setColorsForEntries(char *args);

	static LUndoManager *undoManager();
	MutableRouteObject *routeObject() { return _route; }
	// A null route empties the set.
	void replaceCurrentSet(RouteObject *route);
	void addToCurrentSet(RouteObject *route, int index);
	void deleteItemAtIndex(int index);
	void deleteItemsWithOrderedIndicies(TLArray<LValue *> *indexes);
	void deleteAllItems();
	void insertItemIntoRoute(RouteSetItem *item, int index, LString *name, LArray *color);
	// Route entries are inserted as their items, between breaks. Returns
	// the number of items inserted.
	int insertItemsIntoRoute(TLArray<RouteSetItem *> *items, int index);
	// Both return what the WG controller selects afterwards: the number of
	// items inserted, and the index the moved items start at.
	int insertEntriesFromDescrArray(TLArray<LMap *> *descriptions, int index);
	int moveItemsWithOrderedIndexesToIndex(TLArray<LValue *> *indexes, int index);
	void setNameForEntry(LString *name, GenericEntry *entry);
	void setColorForEntry(LArray *components, GenericEntry *entry);
	void setColorForEntries(LArray *components, TLArray<GenericEntry *> *entries);

	MutableRouteObject *_route;
};

// The data source of the route manager's outline: the tree of route sets
// (the user's library, the sample sets, ...), with the editing of the user's
// nodes, undoable, saved to the database as it is made.
class RouteManagerController : public LObject {
	LF_DECLARE_CLASS(RouteManagerController)
public:
	enum TRouteImageID {
		kUserSetImage,
		kSampleSetImage,
		kUserFolderOpenImage,
		kUserFolderClosedImage,
		kSampleFolderOpenImage,
		kSampleFolderClosedImage,
		kRouteImagesCount
	};

	static LString *kRootNodeKey;
	static LString *kRoutesWereChangedNotification;

	RouteManagerController() : _hierarchy(0)
	{
		for (int i = 0; i < kRouteImagesCount; i++)
			_images[i] = 0;
	}
	virtual RouteManagerController *init();
	virtual void dealloc();
	virtual RouteManagerController *getOwnerLObject() { return this; }
	virtual intptr_t _onRefreshDatabase(char *args);
	virtual intptr_t _setObjectValueForItem(char *args);
	virtual intptr_t _deleteRouteNodes(char *args);
	virtual intptr_t _restoreRouteNodes(char *args);
	virtual intptr_t _moveRouteNodes(char *args);

	static LUndoManager *undoManager();
	// A null item stands for the root, whose children are the hierarchy.
	int childrenCountForItem(GenericRouteNode *item);
	GenericRouteNode *childAtIndexForItem(int index, GenericRouteNode *item);
	LString *objectValueForItem(GenericRouteNode *item);
	void setObjectValueForItem(LString *name, GenericRouteNode *item);
	LImage *iconForItem(GenericRouteNode *item, unsigned char expanded);
	bool isItemEjectable(GenericRouteNode *item);
	bool isItemCollapsable(GenericRouteNode *item);
	bool isItemExpandable(GenericRouteNode *item);
	bool isItemEditable(GenericRouteNode *item);
	bool isItemObjectValueEditable(GenericRouteNode *item);
	bool isItemSelectable(GenericRouteNode *item);
	bool validateDrag(TLArray<GenericRouteNode *> *items);
	bool validateDrop(TLArray<GenericRouteNode *> *items, GenericRouteNode *item, int index);
	bool acceptDrop(TLArray<GenericRouteNode *> *items, GenericRouteNode *item, int index);
	void removeItems(TLArray<GenericRouteNode *> *items);
	void addChildrenForItem(TLArray<GenericRouteNode *> *items, GenericRouteNode *item);
	// A folder when `entry` is null. Returns the new node, or 0 when it
	// couldn't be saved.
	DBUserRouteNode *createNewChildForItem(GenericRouteNode *item, UserRouteObjectEntry *entry, LString *name);
	void addRouteObjectNodesContentToRouteObject(TLArray<GenericRouteNode *> *nodes, MutableRouteObject *route);
	void _addUIDsOfNodeDescendantsToUIDsArray(DBRouteNode *node, TLArray<LString *> *uids);
	LString *_imagePathForTRouteImageID(TRouteImageID id);

	LImage *_images[kRouteImagesCount];
	LArray *_hierarchy;	// of GenericRouteNode
};

// The search list: the entries found by the search string (or all of them
// when it is empty), and the preview of the results while typing: by name
// and by region, grouped by entry type.
class EntriesListDataSource : public LObject {
	LF_DECLARE_CLASS(EntriesListDataSource)
public:
	struct SAutoCompleteResult {
		LString *string;	// the search string completed
		LRange selection;	// the completed part
	};

	static LString *kEntriesWereChangedNotification;

	EntriesListDataSource()
	    : _timer(0), _sortingMode(kEntriesSortByName), _allEntriesSortingMode(kEntriesSortByName),
	      _geotypeFilter(~0ULL), _showsAll(false), _usesCustomRequest(false), _customRequest(0), _searchString(0),
	      _lastAutocompleteString(0), _previewEntriesLimit(100), _previewLimitsValid(false), _allPreviewResults(0),
	      _previewResults(0), _regionPreviewResults(0), _regionsString(0), _manyRegions(false), _searchResults(0),
	      _allEntriesResults(0), _searchedEntries(0)
	{
	}
	virtual EntriesListDataSource *init();
	virtual void dealloc();
	virtual EntriesListDataSource *getOwnerLObject() { return this; }
	// The preview panel, in the interface's subclass.
	virtual bool isPreviewPanelShown();
	virtual void showPreviewPanel();
	virtual void hidePreviewPanel();
	virtual void reloadGenericList();
	virtual intptr_t _deleteEntries(char *args);
	virtual intptr_t _restoreEntries(char *args);
	virtual intptr_t _renewSearchResults(char *args);
	virtual intptr_t _onRefreshDatabase(char *args);
	virtual intptr_t _onEntryDidChange(char *args);

	// Stops the deferred search, and releases this object.
	void invalidate();
	void setSearchString(const LString *string);
	// The search starts 300 ms later, when typing stops; `result`, when
	// given, gets the completion of the string.
	void deferredSearchingWithString(const LString *string, SAutoCompleteResult *result);
	void setGeotypeFilter(unsigned long long filter);
	void setSortingMode(TEntriesSortingMode mode);
	void setPreviewEntriesLimit(int limit);
	void previewEntryWasClicked(SearchPreviewEntry *entry);

	int searchedEntriesCount();
	// Loaded on demand when `load`; 0 until then.
	GenericEntry *searchedEntryAtIndex(int index, unsigned char load);
	int indexOfSearchedEntryWithUID(LString *uid);
	int previewEntriesCount();
	int previewGeoEntriesCount();
	SearchPreviewEntry *previewEntryAtIndex(int index);
	int indexOfPreviewEntryWithUID(LString *uid);

	// Only the user's own entries can be deleted.
	bool canDeleteEntries(TLArray<GenericEntry *> *entries);
	bool canDeleteEntries(TArray<int> *indexes);
	void deleteEntries(TLArray<GenericEntry *> *entries);
	// Commits the entry; false when it couldn't be.
	bool addNewEntry(GenericEntry *entry);

	void _invalCurrentEntries();
	void _invalPreviewResults();
	LArray *_currentSearchResults();
	void _evalPreviewResultsIfNeeded();
	void _recalcPreviewLimitsIfNeeded();

	LTimer *_timer;
	TEntriesSortingMode _sortingMode;
	TEntriesSortingMode _allEntriesSortingMode;
	unsigned long long _geotypeFilter;
	bool _showsAll;	// the search string is empty
	bool _usesCustomRequest;	// a clicked preview entry's request
	LString *_customRequest;
	LString *_searchString;
	LString *_lastAutocompleteString;
	int _previewEntriesLimit;
	bool _previewLimitsValid;
	PreviewResultsContainer *_allPreviewResults;
	PreviewResultsContainer *_previewResults;
	PreviewResultsContainer *_regionPreviewResults;
	LString *_regionsString;
	bool _manyRegions;
	SQLiteResultsContainer *_searchResults;
	SQLiteResultsContainer *_allEntriesResults;
	LArray *_searchedEntries;	// GenericEntry, or LNull until loaded
};

// The info panel of an entry: its info items (descriptions, links, images,
// ...) by key, their display strings, and their editing, undoable. Edited
// entries are committed on the next run loop pass while undoing or redoing;
// otherwise the panel commits when editing ends.
class EntryInfoController : public LObject {
	LF_DECLARE_CLASS(EntryInfoController)
public:
	// How a key's values are shown.
	enum TEntryPropertyKind {
		kPropertyKindDefault = 0,
		kPropertyKindAltNames,
		kPropertyKindPopulation,
		kPropertyKindDateTime,
		kPropertyKindGMTOffset,
		kPropertyKindCoordinates,
		kPropertyKindURL,
		kPropertyKindHostLocation,
		kPropertyKindDescription,
		kPropertyKindDensity,
		kPropertyKindDistance,
		kPropertyKindSmallDistance,
		kPropertyKindArea,
		kPropertyKindTemperature
	};

	EntryInfoController() : _entry(0), _editing(false), _scheduledUID(0), _commitTimer(0) {}
	virtual EntryInfoController *init();
	virtual void dealloc();
	virtual EntryInfoController *getOwnerLObject() { return this; }
	virtual intptr_t _insertItemForKeyAtIndex(char *args);
	virtual intptr_t _addItemForKeyAtIndex(char *args);
	virtual intptr_t _replaceItemForKeyAtIndex(char *args);
	virtual intptr_t _moveItemForKeyFromIndexToIndex(char *args);
	virtual intptr_t _removeItemForKeyAtIndex(char *args);
	virtual intptr_t _commitScheduledEntry(char *args);

	static LUndoManager *undoManager();
	static LString *parameterTitleFromKey(LString *key);
	static TEntryPropertyKind rowKindFromKey(LString *key);
	static LString *stringFromInfoObjectForKey(LObject *object, TEntryPropertyKind kind);
	static LString *formattedCoordinates(LObject *object);
	static LString *formattedHostLocationName(LObject *object);
	static LString *formattedCurrentDateWithTimeZone(LObject *object);
	static LString *formattedAltNames(LObject *object);
	static LString *formattedGMTOffsetFromTimeZone(LObject *object);
	// From a link item (a map or a database tree); false when it isn't one.
	static bool extractURLParamsFromObject(LObject *object, LString **title, LString **url);
	static bool extractDescriptionParamsFromObject(LObject *object, LString **title, LString **text);

	GenericEntry *entry() const { return _entry; }
	void setEntry(GenericEntry *entry);
	bool isEditing() const { return _editing; }
	void setIsEditing(unsigned char editing) { _editing = editing != 0; }
	LArray *orderedKeys(TEntryKeySet set, TEntryKeyAccessMode mode);
	int itemsCountForKey(LString *key, TEntryKeySet set, TEntryKeyAccessMode mode);
	LObject *itemForKeyAtIndex(LString *key, int index, TEntryKeySet set, TEntryKeyAccessMode mode);
	bool canRemoveItemForKeyAtIndex(LString *key, int index, TEntryKeySet set);
	bool canAddItemsForKey(LString *key, TEntryKeySet set);
	bool removeItemForKeyAtIndex(LString *key, int index, TEntryKeySet set);
	bool moveItemForKeyFromIndexToIndex(LString *key, int from, int to, TEntryKeySet set);
	bool replaceItemForKeyAtIndex(LObject *value, LString *key, int index, TEntryKeySet set);
	bool addItemForKey(LObject *value, LString *key, TEntryKeySet set);
	bool insertItemForKeyAtIndex(LObject *value, LString *key, int index, TEntryKeySet set);
	EntryInfoImageWrapper *insertImageWrapperWithData(LData *data, LString *name, int index);
	void commitIfNeeded(LString *uid);

	GenericEntry *_entry;
	bool _editing;
	LString *_scheduledUID;	// of the entry to commit
	LTimer *_commitTimer;
};

#endif
