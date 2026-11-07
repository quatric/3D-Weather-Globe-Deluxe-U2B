// AtlasDatabase: the atlas SQLite database, with the stars database and the
// user's database attached to it. It caches the entries made from it, the
// point and star arrays the renderers draw, and keeps the media folders of
// the user's library.

#include "AppModel.h"
#include <CoreFoundation/CFUUID.h>

LF_IMPLEMENT_CLASS(AtlasDatabase, 0)
LF_IMPLEMENT_CLASS(_LEntryTypeRemapContainer, 0)
LF_SELECTOR(AtlasDatabase, _rescanAndApplyUpdates);
LF_SELECTOR(AtlasDatabase, _rescanAndApplyUpdatesCompleted);

bool AtlasDatabase::sDoCleanupAtStartup = true;

static LConstantString ___uid_val_str("UID_VAL");
static LConstantString ___sNoAltTypeStr("no_alt_type");

template <typename T> static inline void releaseField(T *&field)
{
	if (field) {
		field->release();
		field = 0;
	}
}

// ---------------------------------------------------------------------------
// _LEntryTypeRemapContainer: the names of an entry type and the type/alt
// type pairs it stands for.

_LEntryTypeRemapContainer::_LEntryTypeRemapContainer() : _singular(0), _plural(0), _weight(0), _typePairs(0)
{
}

void _LEntryTypeRemapContainer::dealloc()
{
	releaseField(_singular);
	releaseField(_plural);
	releaseField(_typePairs);
	LObject::dealloc();
}

// ---------------------------------------------------------------------------
// AtlasDatabase

AtlasDatabase::AtlasDatabase()
    : _db(0), _routesHierarchy(0), _blobOwnersByRepType(0), _entryTypeRemaps(0), _entryTypesPerUseCase(0),
      _entryInfoKeysOrders(0), _solarEclipses(0), _tempFolder(0), _unused74(0), _unused78(0),
      _sharedMediaCachesFolder(0), _mediaCachesFolder(0), _libraryRootFolder(0), _mediaDataFolder(0),
      _allPoints(0), _allPointUIDs(0), _indicesSortedByY(0), _capitalsIndices(0), _appleStoresIndices(0),
      _otherPointsIndices(0), _allStars(0), _starsPatches(0), _constellationsData(0), _constellationsStars(0),
      _constellationsInds(0), _constellations(0), _cachedEntries(0), _temporaryEntries(0), _hostObjectsRemap(0),
      _atlasEntryStatement(0), _userEntryStatement(0), _updating(false), _notifyUpdate(false),
      _userDBUnavailable(false)
{
	_entryTypeIDs[0] = _entryTypeIDs[1] = 0;
}

void AtlasDatabase::initAsSingleton(void *)
{
}

void AtlasDatabase::dealloc()
{
	releaseField(_allStars);
	releaseField(_constellationsData);
	releaseField(_constellationsInds);
	releaseField(_constellations);
	releaseField(_constellationsStars);
	releaseField(_atlasEntryStatement);
	releaseField(_userEntryStatement);
	releaseField(_allPoints);
	releaseField(_allPointUIDs);
	releaseField(_capitalsIndices);
	releaseField(_appleStoresIndices);
	releaseField(_otherPointsIndices);
	_synchronize();
	releaseField(_cachedEntries);
	releaseField(_temporaryEntries);
	releaseField(_routesHierarchy);
	releaseField(_blobOwnersByRepType);
	releaseField(_indicesSortedByY);
	releaseField(_solarEclipses);
	releaseField(_hostObjectsRemap);
	for (int i = 0; i < 2; i++)
		releaseField(_entryTypeIDs[i]);
	releaseField(_entryTypeRemaps);
	releaseField(_entryInfoKeysOrders);
	releaseField(_db);
	releaseField(_tempFolder);
	releaseField(_unused74);
	releaseField(_unused78);
	releaseField(_sharedMediaCachesFolder);
	releaseField(_mediaCachesFolder);
	releaseField(_libraryRootFolder);
	releaseField(_mediaDataFolder);
	if (_starsPatches) {
		delete _starsPatches;
		_starsPatches = 0;
	}
	LObject::dealloc();
}

// ---------------------------------------------------------------------------
// Files and folders

LString *AtlasDatabase::_dbFilePathBundled()
{
	return &kDatabaseFile;
}

LString *AtlasDatabase::_starsDBFilePathBundled()
{
	return &kStarsDatabaseFile;
}

// The databases are copied out of the bundle into the shared Application
// Support folder.
LString *AtlasDatabase::_dbFilePathCached()
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LString *folder = callbacks ? callbacks->_applicationDataPath(1) : 0;
	return folder ? folder->stringByAppendingPathComponent(&kDatabaseFile) : 0;
}

LString *AtlasDatabase::_starsDBFilePathCached()
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LString *folder = callbacks ? callbacks->_applicationDataPath(1) : 0;
	return folder ? folder->stringByAppendingPathComponent(&kStarsDatabaseFile) : 0;
}

static LString *_cachesFile(LString *name)
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LString *folder = callbacks ? callbacks->_applicationCachesPath(0) : 0;
	return folder ? folder->stringByAppendingPathComponent(name) : 0;
}

LString *AtlasDatabase::_allStarsCachePath()
{
	return _cachesFile(&kStarsCacheFile);
}

LString *AtlasDatabase::_allStarPatchInfosCachePath()
{
	return _cachesFile(&kStarsPatchInfosCacheFile);
}

LString *AtlasDatabase::_constellationsCachePath()
{
	return _cachesFile(&kConstelationsCacheFile);
}

void AtlasDatabase::_cleanupStarsCaches()
{
	LFileManager *fm = LFileManager::manager();
	fm->removeFileAtPath(_allStarsCachePath());
	fm->removeFileAtPath(_allStarPatchInfosCachePath());
	fm->removeFileAtPath(_constellationsCachePath());
}

// The folder in the user's Application Support for temporary media.
LString *AtlasDatabase::_tempFolderPath(unsigned char create)
{
	if (_tempFolder)
		return _tempFolder;
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LString *folder = callbacks ? callbacks->_applicationDataPath(0) : 0;
	if (folder == 0)
		return 0;
	LString *path = folder->stringByAppendingPathComponent(&kTempFolder);
	LFileManager *fm = LFileManager::manager();
	if (!fm->objectExistsAtPath(path, 0)) {
		if (!create)
			return 0;
		fm->createFolderAtPath(path);
	}
	_tempFolder = LRetain(path);
	return _tempFolder;
}

void AtlasDatabase::_cleanupTempFolder()
{
	if (sDoCleanupAtStartup && _tempFolderPath(0)) {
		LFileManager::manager()->removeFileAtPath(_tempFolder);
		releaseField(_tempFolder);
	}
}

static void _createFolderIfNeeded(LFileManager *fm, LString *path)
{
	if (!fm->objectExistsAtPath(path, 0))
		fm->createFolderAtPath(path);
}

// Library.bundle/Contents/Resources in the user's Application Support.
LString *AtlasDatabase::_libraryRootFolderPath()
{
	if (_libraryRootFolder)
		return _libraryRootFolder;
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LString *folder = callbacks ? callbacks->_applicationDataPath(0) : 0;
	if (folder == 0)
		return 0;
	LFileManager *fm = LFileManager::manager();
	LString *path = LString::_alloc()->initWithCString("");
	path->setValue(folder);
	path->appendPathComponent(&kLibraryFolder);
	_createFolderIfNeeded(fm, path);
	path->appendPathComponent(&kContentsFolder);
	_createFolderIfNeeded(fm, path);
	path->appendPathComponent(&kResourcesFolder);
	_createFolderIfNeeded(fm, path);
	_libraryRootFolder = path;
	return _libraryRootFolder;
}

LString *AtlasDatabase::_mediaDataFolderPath(unsigned char create)
{
	if (_mediaDataFolder == 0) {
		LString *root = _libraryRootFolderPath();
		if (root == 0)
			return 0;
		LString *path = root->stringByAppendingPathComponent(&kMediaDataFolder);
		LFileManager *fm = LFileManager::manager();
		if (!fm->objectExistsAtPath(path, 0)) {
			if (!create)
				return 0;
			fm->createFolderAtPath(path);
		}
		_mediaDataFolder = LRetain(path);
	}
	return _mediaDataFolder;
}

void AtlasDatabase::_cleanupMediaFolder()
{
	if (sDoCleanupAtStartup && _mediaDataFolderPath(0)) {
		LFileManager::manager()->removeFileAtPath(_mediaDataFolder);
		releaseField(_mediaDataFolder);
	}
}

LString *AtlasDatabase::_userDBFilePath()
{
	LString *root = _libraryRootFolderPath();
	return root ? root->stringByAppendingPathComponent(&kUserDatabaseFile) : 0;
}

// A folder for an entry's media, in the library or with the temporary
// files.
LString *AtlasDatabase::pathForMediaDataFolder(LString *name, unsigned char temporary)
{
	LString *folder = temporary ? _tempFolderPath(1) : _mediaDataFolderPath(1);
	if (folder == 0)
		return 0;
	LString *path = LString::stringWithString(folder);
	if (name) {
		path->appendPathComponent(name);
		_createFolderIfNeeded(LFileManager::manager(), path);
	}
	return path;
}

// The media caches, per user or shared by all users.
LString *AtlasDatabase::_mediaCachesFolderPath(unsigned char shared, unsigned char create)
{
	LString *cached = shared ? _sharedMediaCachesFolder : _mediaCachesFolder;
	if (cached)
		return cached;
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LString *folder = callbacks ? callbacks->_applicationCachesPath(shared) : 0;
	if (folder == 0)
		return 0;
	LString *path = folder->stringByAppendingPathComponent(&kMediaDataFolder);
	LFileManager *fm = LFileManager::manager();
	if (!fm->objectExistsAtPath(path, 0)) {
		if (!create)
			return 0;
		fm->createFolderAtPath(path);
	}
	if (shared) {
		fm->changeFileAttributesAtPath(path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777),
		                                                                 LFileManager::fileAttrPosixPermissions, 0));
		_sharedMediaCachesFolder = path;
	} else {
		_mediaCachesFolder = path;
	}
	return LRetain(path);
}

void AtlasDatabase::_cleanupMediaCaches(unsigned char shared)
{
	if (!sDoCleanupAtStartup)
		return;
	LString *folder = _mediaCachesFolderPath(shared, 0);
	if (folder == 0)
		return;
	EntryInfoImageWrapper::invalidateAllCachedPaths();
	LFileManager::manager()->removeFileAtPath(folder);
	releaseField(_mediaCachesFolder);
	releaseField(_sharedMediaCachesFolder);
}

LString *AtlasDatabase::_pathForMediaCache(LString *name, LString *folderName, unsigned char temporary,
                                           unsigned char shared)
{
	if (name == 0)
		return 0;
	LString *folder = temporary ? _tempFolderPath(1) : _mediaCachesFolderPath(shared, 1);
	if (folder == 0)
		return 0;
	LString *path = LString::stringWithString(folder);
	if (folderName) {
		path->appendPathComponent(folderName);
		LFileManager *fm = LFileManager::manager();
		_createFolderIfNeeded(fm, path);
		if (shared)
			fm->changeFileAttributesAtPath(path, LMap::mapWithObjectsAndKeys(
			                                         LValue::valueWithIntPtr(0777),
			                                         LFileManager::fileAttrPosixPermissions, 0));
	}
	path->appendPathComponent(name);
	return path;
}

LString *AtlasDatabase::pathForMediaThumbnail(LString *name, LString *folderName, unsigned char temporary,
                                              unsigned char shared)
{
	LString *path = _pathForMediaCache(name, folderName, temporary, shared);
	if (path)
		path->appendPathExtension(&kThumbnailExtension);
	return path;
}

LString *AtlasDatabase::pathForMediaTexture(LString *name, LString *folderName, unsigned char temporary,
                                            unsigned char shared)
{
	LString *path = _pathForMediaCache(name, folderName, temporary, shared);
	if (path)
		path->appendPathExtension(&kTextureExtension);
	return path;
}

// ---------------------------------------------------------------------------
// Caches

void AtlasDatabase::_invalidateAllPointsCache()
{
	releaseField(_allPoints);
	releaseField(_indicesSortedByY);
}

void AtlasDatabase::_invalidateCachedStatements()
{
	releaseField(_atlasEntryStatement);
	releaseField(_userEntryStatement);
}

void AtlasDatabase::_invalidateUserDBDependedCaches()
{
	_invalidateCachedStatements();
	_invalidateAllPointsCache();
}

// Saves the changes of the cached entries.
void AtlasDatabase::_synchronize()
{
	LMapIterator it(_cachedEntries, false);
	while (it.next()) {
		GenericEntry *entry = lobj_cast<GenericEntry>(it.object());
		if (entry)
			entry->commit();
	}
}

void AtlasDatabase::_invalidateDBCaches()
{
	StSharableLocker lock(&_lock);
	_invalidateUserDBDependedCaches();
	_synchronize();
	releaseField(_cachedEntries);
	releaseField(_routesHierarchy);
	releaseField(_blobOwnersByRepType);
	for (int i = 0; i < 2; i++)
		releaseField(_entryTypeIDs[i]);
	releaseField(_entryTypeRemaps);
	_entryTypesPerUseCase = 0;
	releaseField(_entryInfoKeysOrders);
	releaseField(_solarEclipses);
	releaseField(_hostObjectsRemap);
	releaseField(_allStars);
	releaseField(_constellationsData);
	releaseField(_constellationsStars);
	releaseField(_constellationsInds);
	releaseField(_constellations);
	_cleanupStarsCaches();
	_cleanupMediaCaches(1);
	releaseField(_hostObjectsRemap);
	VertexBuffersBLOB::invalidateCaches();
}

void AtlasDatabase::cacheTemporaryEntry(GenericEntry *entry)
{
	if (_temporaryEntries == 0)
		_temporaryEntries = LMap::_alloc()->init();
	_temporaryEntries->setObjectForKey(entry, entry->uid(0));
}

GenericEntry *AtlasDatabase::temporaryEntryWithUID(LString *uid)
{
	if (uid == 0 || _temporaryEntries == 0)
		return 0;
	return (GenericEntry *)_temporaryEntries->objectForKey(uid);
}

void AtlasDatabase::_cacheEntry(GenericEntry *entry)
{
	if (_cachedEntries == 0)
		_cachedEntries = LMap::_alloc()->init();
	_cachedEntries->setObjectForKey(entry, entry->uid(0));
}

// Deleted entries move to the temporary cache, restored ones back.
void AtlasDatabase::_invalidateDBCachesWithEntries(LArray *entries, unsigned char restored)
{
	int count = entries ? entries->count() : 0;
	for (int i = 0; i < count; i++) {
		GenericEntry *entry = (GenericEntry *)entries->objectAtIndex(i);
		if (!restored) {
			cacheTemporaryEntry(entry);
			if (_cachedEntries)
				_cachedEntries->removeObjectForKey(entry->uid(0));
		} else {
			_cacheEntry(entry);
			if (_temporaryEntries)
				_temporaryEntries->removeObjectForKey(entry->uid(0));
		}
	}
	_invalidateUserDBDependedCaches();
}

// A newly committed entry leaves the temporary cache.
void AtlasDatabase::_cacheEntryIfNeeded(GenericEntry *entry)
{
	_invalidateCachedStatements();
	if (_cachedEntries && _cachedEntries->objectForKey(entry->uid(0)))
		return;
	_cacheEntry(entry);
	_invalidateAllPointsCache();
	if (_temporaryEntries)
		_temporaryEntries->removeObjectForKey(entry->uid(0));
	LNotificationCenter::center()->postNotification(kMainDatabaseChangedNotification, this, 0);
}

// ---------------------------------------------------------------------------
// Opening and updating

// Copies the bundled databases out when missing or older than the bundle,
// attaches the stars and user databases, and makes the user tables, in a
// temporary database if the user's can't be used.
void AtlasDatabase::_initializeSQLDatabase()
{
	releaseField(_db);
	LString *dbPath = _dbFilePathCached();
	LF_CHECK_PTR(dbPath);
	LString *starsPath = _starsDBFilePathCached();
	LF_CHECK_PTR(starsPath);
	LString *userPath = _userDBFilePath();
	LF_CHECK_PTR(userPath);
	LFileManager *fm = LFileManager::manager();
	LMap *attributes =
	    LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0666), LFileManager::fileAttrPosixPermissions, (LObject *)0);

	double cachedTime = fm->lastUpdateTimeForFile(dbPath);
	double bundledTime = fm->lastUpdateTimeForFile(&kDatabaseFile);
	if (fm->objectExistsAtPath(dbPath, 0) && cachedTime >= bundledTime)
		_db = SQLiteDatabase::_alloc()->initWithDatabasePath(dbPath);
	if (_db == 0) {
		fm->removeFileAtPath(dbPath);
		_cleanupMediaCaches(1);
		LData *data = LData::dataWithContentsOfFile(&kDatabaseFile);
		if (data == 0 || !data->writeToFile(dbPath))
			return;
		fm->changeFileAttributesAtPath(dbPath, attributes);
		_db = SQLiteDatabase::_alloc()->initWithDatabasePath(dbPath);
	}

	cachedTime = fm->lastUpdateTimeForFile(starsPath);
	bundledTime = fm->lastUpdateTimeForFile(&kStarsDatabaseFile);
	if (!fm->objectExistsAtPath(starsPath, 0) || cachedTime < bundledTime ||
	    !_db->attachDatabase(starsPath, kStarsDBName)) {
		fm->removeFileAtPath(starsPath);
		_cleanupStarsCaches();
		LData *data = LData::dataWithContentsOfFile(&kStarsDatabaseFile);
		LF_CHECK_PTR(data);
		if (!data->writeToFile(starsPath))
			return;
		fm->changeFileAttributesAtPath(starsPath, attributes);
		_db->attachDatabase(starsPath, kStarsDBName);
	}

	_userDBUnavailable = false;
	bool userDBExists = fm->objectExistsAtPath(userPath, 0);
	SQLiteDatabase *userDB = SQLiteDatabase::_alloc()->initWithDatabasePath(userPath);
	if (userDB == 0) {
		_userDBUnavailable = true;
	} else {
		if (!userDBExists || userDB->processRequest((const LString *)SQL_DB_Strings::selectUserDBRevision()) == 0) {
			userDB->processRequestsInArray(SQL_DB_Strings::createUserDBTables(0));
			_cleanupMediaCaches(0);
			_cleanupMediaFolder();
		} else {
			_cleanupUserDB(userDB);
		}
		userDB->release();
		if (!_db->attachDatabase(userPath, kUserDBName))
			_userDBUnavailable = true;
	}
	if (_userDBUnavailable)
		_db->processRequestsInArray(SQL_DB_Strings::createUserDBTables(1));
	_cleanupTempFolder();
	rescanForUpdates(0, 0);
}

// Drops deleted objects, and the media no object uses any more.
void AtlasDatabase::_cleanupUserDB(SQLiteDatabase *userDB)
{
	if (!sDoCleanupAtStartup || userDB == 0)
		return;
	LSingleProfilerPoint profile("cleanup user DB");
	userDB->processRequestsInArray(SQL_DB_Strings::cleanUserDB());
	if (_mediaDataFolderPath(0) == 0)
		return;
	_mediaCachesFolderPath(0, 0);
	LArray *rows = userDB->processRequest((const LString *)SQL_DB_Strings::selectObjectUIDsWithUserImages());
	unsigned int count = rows ? rows->count() : 0;
	LSet *used = 0;
	for (unsigned int i = 0; i < count; i++) {
		LString *uid = lobj_cast<LString>(((LMap *)rows->objectAtIndex(i))->objectForKey(kColumnNameObjectUID));
		if (uid == 0)
			continue;
		if (used == 0) {
			used = (LSet *)LComparableSet::_alloc()->init()->l_autorelease();
			used->preserveCapacity(count);
		}
		used->addObject(uid);
	}
	LFileManager *fm = LFileManager::manager();
	LArray *folders = fm->directoryContentsAtPath(_mediaDataFolder);
	int foldersCount = folders ? folders->count() : 0;
	int removed = 0;
	for (int i = 0; i < foldersCount; i++) {
		LString *name = (LString *)folders->objectAtIndex(i);
		if (used && used->containsObject(name))
			continue;
		fm->removeFileAtPath(_mediaDataFolder->stringByAppendingPathComponent(name));
		if (_mediaCachesFolder)
			fm->removeFileAtPath(_mediaCachesFolder->stringByAppendingPathComponent(name));
		removed++;
	}
	if (foldersCount == removed) {
		fm->removeFileAtPath(_mediaDataFolder);
		releaseField(_mediaDataFolder);
	}
}

static int _lstrAscendingCallback(const void *a, const void *b, const void *)
{
	return (*(LString *const *)a)->compareWith(*(LString *const *)b);
}

// The downloaded update files, in name order.
LArray *AtlasDatabase::_allUpdateFiles()
{
	LFileManager *fm = LFileManager::manager();
	LString *folder = DownloadManager::updatesDownloadFolder();
	LArray *names = fm->directoryContentsAtPath(folder);
	LArray *files = LArray::array();
	int count = names ? names->count() : 0;
	for (int i = 0; i < count; i++) {
		LString *name = (LString *)names->objectAtIndex(i);
		if (name->hasPrefix(&kUpdatePrefix))
			files->addObject(folder->stringByAppendingPathComponent(name));
	}
	files->setComparator(_lstrAscendingCallback);
	files->sort();
	return files;
}

// Looks for update files not applied yet, on another thread if `async`;
// `notify` posts the WillStart/Changed/DidEnd notifications.
void AtlasDatabase::rescanForUpdates(unsigned char async, unsigned char notify)
{
	if (_updating)
		return;
	_updating = true;
	_notifyUpdate = notify;
	if (notify)
		LNotificationCenter::center()->postNotification(kMainDatabaseWillStartUpdate, this, 0);
	if (async)
		LThread::performSingleAction(this, "_rescanAndApplyUpdates", 0);
	else
		_rescanAndApplyUpdates(0);
}

// Applies each update dump not applied yet, recording its name.
intptr_t AtlasDatabase::_rescanAndApplyUpdates(char *)
{
	bool changed = false;
	{
		StSharableLocker lock(&_lock);
		LArray *files = _allUpdateFiles();
		int count = files ? files->count() : 0;
		for (int i = 0; i < count; i++) {
			LString *path = (LString *)files->objectAtIndex(i);
			LString *name = path->lastPathComponent()->lowercaseString();
			LString *request = SQL_DB_Strings::selectFilenameFromUpdatesThatIsLike(name);
			LArray *found = simpleSearchWithRequest(request);
			if (found == 0) {
				LLog("Unable to process the request.\n", request);
				break;
			}
			if (found->count() != 0)
				continue;
			LData *dump = LData::dataWithContentsOfFile(path);
			dump = dump ? dump->detachDecomressedData() : 0;
			if (dump == 0) {
				LLog("Unable to load and decomress data at path %@\n", path);
				break;
			}
			if (!_db->processRequestsWithData(dump, false)) {
				if (simpleSearchWithRequest(LString::stringWithCString("ROLLBACK TRANSACTION")) == 0)
					LLog("Unable to rollback after execute dump at %@\n", path);
				break;
			}
			simpleSearchWithRequest(SQL_DB_Strings::insertFilenameIntoUpdates(name));
			changed = true;
		}
	}
	if (LThread::currentThread() == LThread::sMainThread)
		invokeSelector("_rescanAndApplyUpdatesCompleted", LValue::valueWithBoolean(changed));
	else
		invokeSelectorInMainThreadWithObjects("_rescanAndApplyUpdatesCompleted", false, false,
		                                      LValue::valueWithBoolean(changed));
	return 0;
}

intptr_t AtlasDatabase::_rescanAndApplyUpdatesCompleted(char *args)
{
	LValue *changed = LF_ARG(args, 0, LValue *);
	if (changed && changed->intValue()) {
		_invalidateDBCaches();
		if (!_notifyUpdate) {
			_updating = false;
			return 0;
		}
		LNotificationCenter::center()->postNotification(kMainDatabaseChangedNotification, this, 0);
	}
	if (_notifyUpdate)
		LNotificationCenter::center()->postNotification(kMainDatabaseDidEndUpdate, this, 0);
	_updating = false;
	return 0;
}

SQLiteDatabase *AtlasDatabase::sqlDB()
{
	StSharableLocker lock(&_lock);
	if (_db == 0)
		_initializeSQLDatabase();
	return _db;
}

// ---------------------------------------------------------------------------
// Requests

SQLiteResultsContainer *AtlasDatabase::searchWithRequest(const LString *request, LMap *parameters)
{
	StSharableLocker lock(&_lock);
	if (_db == 0)
		_initializeSQLDatabase();
	LSingleProfilerPoint profile("DB search");
	SQLiteStatement *statement =
	    SQLiteStatement::_alloc()->initWithDB(_db, LString::stringWithString(request));
	SQLiteResultsContainer *results =
	    parameters ? statement->processWithParameters(parameters) : statement->process();
	if (statement)
		statement->release();
	return results;
}

SQLiteResultsContainer *AtlasDatabase::searchWithRequest(const LString *request, TEntriesSortingMode mode)
{
	SQLiteResultsContainer *results = searchWithRequest(request, (LMap *)0);
	sortSearchResults(results, mode);
	return results;
}

LArray *AtlasDatabase::simpleSearchWithRequest(const LString *request)
{
	StSharableLocker lock(&_lock);
	if (_db == 0)
		_initializeSQLDatabase();
	return _db->processRequest(request);
}

void AtlasDatabase::sortSearchResults(SQLiteResultsContainer *results, TEntriesSortingMode mode)
{
	if (results == 0)
		return;
	LSingleProfilerPoint profile("sort search result");
	StAutoreleasePool pool;
	SEntryCompareContext context(mode);
	results->sortWithCallback(_sortByNameCallback, &context);
}

// The user's objects as a compressed SQL dump.
bool AtlasDatabase::exportUserDBToFile(LString *path)
{
	if (path == 0)
		return false;
	StAutoreleasePool pool;
	LArray *dumps = LArray::array();
	dumps->addObject(SQL_DB_Strings::insertGeoObjects(searchWithRequest(SQL_DB_Strings::selectAllUserGeoObjects(), (LMap *)0)));
	dumps->addObject(SQL_DB_Strings::insertKeywords(searchWithRequest(SQL_DB_Strings::selectAllUserKeywords(), (LMap *)0)));
	dumps->addObject(
	    SQL_DB_Strings::insertRouteNodes(searchWithRequest(SQL_DB_Strings::selectAllUserRouteNodes(), (LMap *)0)));
	dumps->addObject(SQL_DB_Strings::insertBLOBs(searchWithRequest(SQL_DB_Strings::selectAllUserBLOBs(), (LMap *)0)));
	dumps->addObject(SQL_DB_Strings::insertGeoData(searchWithRequest(SQL_DB_Strings::selectAllUserGeoData(), (LMap *)0)));
	const char *sql = SQL_DB_Strings::userDBDump(dumps)->UTF8String();
	LData *data = LData::dataWithBytesNoCopy((void *)sql, (unsigned int)strlen(sql), false);
	data->compress();
	return data->writeToFile(path);
}

bool AtlasDatabase::importUserDBFromFile(LString *path)
{
	if (path == 0)
		return false;
	bool imported = false;
	{
		StSharableLocker lock(&_lock);
		LData *data = LData::dataWithContentsOfFile(path);
		LData *dump = data ? data->detachDecomressedData() : 0;
		if (dump == 0) {
			LLog("Unable to load and decomress data at path %@\n", path);
		} else if (_db->processRequestsWithData(dump, true)) {
			imported = true;
		} else {
			simpleSearchWithRequest(LString::stringWithCString("ROLLBACK TRANSACTION"));
		}
	}
	if (imported) {
		_invalidateDBCaches();
		LNotificationCenter::center()->postNotification(kMainDatabaseChangedNotification, this, 0);
	}
	return imported;
}

// ---------------------------------------------------------------------------
// Entries

static LArray *_uidsOfEntries(LArray *entries)
{
	LArray *uids = LArray::array();
	int count = entries ? entries->count() : 0;
	for (int i = 0; i < count; i++)
		uids->addObject(((GenericEntry *)entries->objectAtIndex(i))->uid(0));
	return uids;
}

void AtlasDatabase::restoreEntries(LArray *entries)
{
	LString *request = SQL_DB_Strings::restoreGeoObjectsWithUIDs(_uidsOfEntries(entries));
	if (AtlasDatabase::instance()->simpleSearchWithRequest(request)) {
		_invalidateDBCachesWithEntries(entries, 1);
		LNotificationCenter::center()->postNotification(kMainDatabaseChangedNotification, this, 0);
	}
}

void AtlasDatabase::deleteEntries(LArray *entries)
{
	LString *request = SQL_DB_Strings::deleteGeoObjectsWithUIDs(_uidsOfEntries(entries));
	if (AtlasDatabase::instance()->simpleSearchWithRequest(request)) {
		_invalidateDBCachesWithEntries(entries, 0);
		LNotificationCenter::center()->postNotification(kMainDatabaseChangedNotification, this, 0);
	}
}

// Saves a route node and its entry in one transaction.
bool AtlasDatabase::commitRouteNode(DBUserRouteNode *node)
{
	if (node == 0 || _userDBUnavailable)
		return false;
	if (simpleSearchWithRequest(LString::stringWithCString("BEGIN TRANSACTION")) == 0)
		return false;
	bool ok = simpleSearchWithRequest(SQL_DB_Strings::insertOrReplaceRouteNodeWithDBMap(node->dbMap())) != 0;
	GenericEntry *entry = node->entry();
	if (entry && ok)
		ok = entry->_commit();
	if (!ok) {
		simpleSearchWithRequest(LString::stringWithCString("ROLLBACK TRANSACTION"));
		return false;
	}
	simpleSearchWithRequest(LString::stringWithCString("END TRANSACTION"));
	if (entry)
		_cacheEntryIfNeeded(entry);
	return true;
}

bool AtlasDatabase::commitEntry(GenericEntry *entry)
{
	if (entry == 0 || _userDBUnavailable)
		return false;
	if (simpleSearchWithRequest(LString::stringWithCString("BEGIN TRANSACTION")) == 0)
		return false;
	if (!entry->_commit()) {
		simpleSearchWithRequest(LString::stringWithCString("ROLLBACK TRANSACTION"));
		return false;
	}
	simpleSearchWithRequest(LString::stringWithCString("END TRANSACTION"));
	_cacheEntryIfNeeded(entry);
	return true;
}

// Whether the object has a BLOB of the representation type; the owners of
// each type are loaded once.
bool AtlasDatabase::_blobExistsForEntryWithUID(LString *uid, LString *repType)
{
	if (_blobOwnersByRepType == 0)
		_blobOwnersByRepType = LMap::_alloc()->init();
	LSet *owners = (LSet *)_blobOwnersByRepType->objectForKey(repType);
	if (owners == 0) {
		owners = LComparableSet::_alloc()->init();
		_blobOwnersByRepType->setObjectForKey(owners, repType);
		owners->release();
		LArray *rows = simpleSearchWithRequest(SQL_DB_Strings::selectAllGeoObjUIDsWithBLOBOfType(repType));
		unsigned int count = rows ? rows->count() : 0;
		owners->preserveCapacity(count);
		for (unsigned int i = 0; i < count; i++) {
			LString *owner =
			    lobj_cast<LString>(((LMap *)rows->objectAtIndex(i))->objectForKey(kColumnNameObjectUID));
			if (owner)
				owners->addObject(owner);
		}
	}
	return owners->containsObject(uid);
}

bool AtlasDatabase::shapeExistsForEntryWithUID(LString *uid)
{
	return _blobExistsForEntryWithUID(uid, kRepTypeShapeIndices);
}

bool AtlasDatabase::previewIconExistsForEntryWithUID(LString *uid)
{
	return _blobExistsForEntryWithUID(uid, kRepTypeEntryPreviewIcon);
}

bool AtlasDatabase::customIconExistsForEntryWithUID(LString *uid)
{
	return _blobExistsForEntryWithUID(uid, kRepTypeEntryIcon);
}

LArray *AtlasDatabase::allEntryUIDsWithImages()
{
	LArray *rows = simpleSearchWithRequest(SQL_DB_Strings::selectAllObjectUIDsWithImages());
	int count = rows ? rows->count() : 0;
	LArray *uids = 0;
	for (int i = 0; i < count; i++) {
		LString *uid = lobj_cast<LString>(((LMap *)rows->objectAtIndex(i))->objectForKey(kColumnNameObjectUID));
		if (uid == 0)
			continue;
		if (uids == 0)
			uids = LArray::array();
		uids->addObject(uid);
	}
	return uids;
}

// A new entry from the database row of the UID (retained), using a prepared
// statement per database.
GenericEntry *AtlasDatabase::_createSQL_DB_EntryWithUID(LString *uid)
{
	StAutoreleasePool pool;
	SQLiteResultsContainer *results;
	{
		StSharableLocker lock(&_statementsLock);
		bool user = uid->length() == 21;
		SQLiteStatement *&statement = user ? _userEntryStatement : _atlasEntryStatement;
		if (statement == 0)
			statement =
			    SQLiteStatement::_alloc()->initWithDB(sqlDB(), SQL_DB_Strings::selectGeoObjectWithBondUID(user));
		else
			statement->reset();
		results = statement->processWithParameters(LMap::mapWithObjectsAndKeys(uid, &___uid_val_str, (LObject *)0));
	}
	if (results == 0 || results->count() != 1)
		return 0;
	LMap *map = lobj_cast<LMap>(results->objectAtIndex(0));
	if (map == 0)
		return 0;
	return LRetain(GenericEntry::entryWithDBMap(map));
}

GenericEntry *AtlasDatabase::dbEntryWithUID(LString *uid)
{
	if (uid == 0)
		return 0;
	GenericEntry *entry = _cachedEntries ? (GenericEntry *)_cachedEntries->objectForKey(uid) : 0;
	if (entry)
		return entry;
	entry = _createSQL_DB_EntryWithUID(uid);
	if (entry) {
		_cacheEntry(entry);
		entry->release();
	}
	return entry;
}

// The two route libraries: the user's and the sample sets.
LArray *AtlasDatabase::allRoutesHierarchy()
{
	if (_routesHierarchy == 0) {
		UserLibRouteNode *user = (UserLibRouteNode *)UserLibRouteNode::_alloc()
		                             ->initWithName(LString::stringFromTable("USER LIBRARY", "AppModel",
		                                                                     getAppModelModule()))
		                             ->l_autorelease();
		StaticLibRouteNode *samples = (StaticLibRouteNode *)StaticLibRouteNode::_alloc()
		                                  ->initWithName(LString::stringFromTable("SAMPLE SETS", "AppModel",
		                                                                          getAppModelModule()))
		                                  ->l_autorelease();
		_routesHierarchy = LArray::_alloc()->initWithObjects(samples, user, (LObject *)0);
	}
	return _routesHierarchy;
}

// The names of the entry info items in display order, for the entry's type
// or for generic and point entries.
LMap *AtlasDatabase::entryInfoKeysOrder(GenericEntry *entry)
{
	if (_entryInfoKeysOrders == 0) {
		_entryInfoKeysOrders = LMap::_alloc()->init();
		LArray *rows = simpleSearchWithRequest(SQL_DB_Strings::selectBLOBUIDForEntryInfoKeysOrder());
		if (rows == 0 || rows->count() != 1)
			return 0;
		LString *blobUID = lobj_cast<LString>(((LMap *)rows->objectAtIndex(0))->objectForKey(kColumnNameBLOBUid));
		PlistBLOB *blob = blobUID ? lobj_cast<PlistBLOB>(BLOBWrapper::wrapperWithBLOBUid(blobUID)) : 0;
		if (blob) {
			LMapIterator it(lobj_cast<LMap>(blob->object()), false);
			_entryInfoKeysOrders->preserveCapacity(it.count());
			while (it.next()) {
				LMap *order = LMap::_alloc()->init();
				_entryInfoKeysOrders->setObjectForKey(order, it.key());
				order->release();
				LArray *keys = lobj_cast<LArray>(it.object());
				unsigned int count = keys ? keys->count() : 0;
				order->preserveCapacity(count);
				for (unsigned int i = 0; i < count; i++)
					order->setObjectForKey(LValue::valueWithInt(i), lobj_cast<LString>(keys->objectAtIndex(i)));
			}
		}
	}
	LString *type = entry->ownType(0);
	if (type) {
		LMap *order = (LMap *)_entryInfoKeysOrders->objectForKey(type);
		if (order)
			return order;
	}
	const char *kind = lobj_cast<PointEntry>(entry) ? "point" : "generic";
	return (LMap *)_entryInfoKeysOrders->objectForKey(LString::stringWithCString(kind));
}

// ---------------------------------------------------------------------------
// UIDs

// 21 characters from 124 bits of a new CFUUID.
LString *AtlasDatabase::generateUID()
{
	static const char remap[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_#";
	CFUUIDRef uuid = CFUUIDCreate(0);
	CFUUIDBytes b = CFUUIDGetUUIDBytes(uuid);
	CFRelease(uuid);
	const uint8_t *u = &b.byte0;
	uint64_t high = (uint64_t)u[14] << 56 | (uint64_t)u[15] << 48 | (uint64_t)u[8] << 40 | (uint64_t)u[9] << 32 |
	                (uint64_t)u[10] << 24 | (uint64_t)u[11] << 16 | (uint64_t)u[12] << 8 | u[13];
	uint64_t low = (uint64_t)u[0] << 56 | (uint64_t)u[1] << 48 | (uint64_t)u[2] << 40 | (uint64_t)u[3] << 32 |
	               (uint64_t)u[6] << 24 | (uint64_t)u[7] << 16 | (uint64_t)u[4] << 8 | u[5];
	char uid[22];
	int n = 0;
	for (int shift = 58; shift >= 4; shift -= 6)
		uid[n++] = remap[(high >> shift) & 0x3f];
	uid[n++] = remap[(high & 0xf) << 2 | low >> 62];
	for (int shift = 56; shift >= 2; shift -= 6)
		uid[n++] = remap[(low >> shift) & 0x3f];
	uid[n] = 0;
	return LString::stringWithCString(uid, 21, kLMacRomanEncoding);
}

// ---------------------------------------------------------------------------
// Points

// Every point object: SAtlasPoint records, their UIDs (22 bytes each), and
// the indices of the capitals, Apple stores and all points.
LData *AtlasDatabase::allPointsArray()
{
	if (_allPoints)
		return _allPoints;
	releaseField(_allPointUIDs);
	releaseField(_capitalsIndices);
	releaseField(_appleStoresIndices);
	releaseField(_otherPointsIndices);
	SQLiteResultsContainer *results = searchWithRequest(SQL_DB_Strings::selectAllCoordsUIDsKindsForPoints(), (LMap *)0);
	int count = results ? results->count() : 0;
	_allPoints = LData::_alloc()->init();
	_allPoints->setLength(count * sizeof(SAtlasPoint));
	_allPointUIDs = LData::_alloc()->init();
	_allPointUIDs->setLength(count * kAtlasPointUIDSize);
	_capitalsIndices = LData::_alloc()->init();
	_capitalsIndices->setLength(count * 4);
	_appleStoresIndices = LData::_alloc()->init();
	_appleStoresIndices->setLength(count * 4);
	_otherPointsIndices = LData::_alloc()->init();
	_otherPointsIndices->setLength(count * 4);
	SAtlasPoint *point = (SAtlasPoint *)_allPoints->bytes();
	char *uid = (char *)_allPointUIDs->bytes();
	unsigned int *capitals = (unsigned int *)_capitalsIndices->bytes();
	unsigned int *stores = (unsigned int *)_appleStoresIndices->bytes();
	unsigned int *all = (unsigned int *)_otherPointsIndices->bytes();
	int types[5] = { 2, 2, 3, 3, 3 };
	int capitalsCount = 0, storesCount = 0;
	int i;
	for (i = 0; i < count; i++, point++, uid += kAtlasPointUIDSize) {
		char type[20], altType[20];
		type[0] = 0;
		altType[0] = 0;
		void *values[5] = { &point->geoPoint.x, &point->geoPoint.y, uid, type, altType };
		results->loadValues(values, types, 5, i);
		unsigned char kind = (unsigned char)SQL_DB_Strings::pointEntryTypeFromTypeAndAltType(type, altType);
		point->kind = kind;
		if (kind < 12) {
			unsigned int bit = 1u << kind;
			if (bit & 0xd00)
				stores[storesCount++] = i;
			else if (bit & 0xc)
				capitals[capitalsCount++] = i;
		}
		all[i] = i;
	}
	_capitalsIndices->setLength(capitalsCount * 4);
	_appleStoresIndices->setLength(storesCount * 4);
	_otherPointsIndices->setLength(i * 4);
	return _allPoints;
}

LData *AtlasDatabase::allOtherPointsIndices()
{
	allPointsArray();
	return _otherPointsIndices;
}

LData *AtlasDatabase::allAppleStoresIndices()
{
	allPointsArray();
	return _appleStoresIndices;
}

LData *AtlasDatabase::allCapitalsIndices()
{
	allPointsArray();
	return _capitalsIndices;
}

LData *AtlasDatabase::allPointUIDsArray()
{
	allPointsArray();
	return _allPointUIDs;
}

static int _entriesYComparator(const void *a, const void *b, const void *context)
{
	const SAtlasPoint *points = (const SAtlasPoint *)context;
	float y1 = points[*(const unsigned int *)a].geoPoint.y;
	float y2 = points[*(const unsigned int *)b].geoPoint.y;
	if (y1 < y2)
		return -1;
	return y2 < y1;
}

// The point indices by latitude.
unsigned int *AtlasDatabase::indicesSortedByY()
{
	if (_indicesSortedByY == 0) {
		LData *points = allPointsArray();
		unsigned int count = points->length() / sizeof(SAtlasPoint);
		_indicesSortedByY = LData::_alloc()->init();
		_indicesSortedByY->setLength(count * 4);
		unsigned int *indices = (unsigned int *)_indicesSortedByY->bytes();
		for (unsigned int i = 0; i < count; i++)
			indices[i] = i;
		QSORT::sortInContext(indices, count, 4, points->bytes(), _entriesYComparator);
	}
	return (unsigned int *)_indicesSortedByY->bytes();
}

// The least distance estimate a point at the latitude can have.
static float GetAxisYDistanceByLocationArrayIndex(float latitude, float targetLatitude)
{
	return -cosf(deg2radf(targetLatitude - latitude));
}

// The point entry nearest to the point: from the latitude found in the
// sorted indices, walks outwards while a nearer latitude band is possible.
PointEntry *AtlasDatabase::entryNearestToGeoPoint(LPoint geoPoint)
{
	const SAtlasPoint *points = (const SAtlasPoint *)allPointsArray()->bytes();
	unsigned int last = allPointsArray()->length() / sizeof(SAtlasPoint) - 1;
	const unsigned int *sorted = indicesSortedByY();
	Vector3 target = earthVecFromGeoPoint(geoPoint);

	int low = 0, high = (int)last, direction = 0;
	unsigned int middle = 0;
	bool exact = false;
	while (low <= high) {
		middle = (unsigned int)((low + high) / 2);
		float delta = geoPoint.y - points[sorted[middle]].geoPoint.y;
		if (delta > 0) {
			low = middle + 1;
			direction = 1;
		} else if (delta < 0) {
			high = middle - 1;
			direction = -1;
		} else {
			exact = true;
			break;
		}
	}
	if (!exact)
		middle += direction > 0;

	unsigned int up = 0, down = 0;
	if (middle < last) {
		up = middle + 1;
		down = middle;
	}
	float downBound = GetAxisYDistanceByLocationArrayIndex(points[sorted[down]].geoPoint.y, geoPoint.y);
	float upBound = GetAxisYDistanceByLocationArrayIndex(points[sorted[up]].geoPoint.y, geoPoint.y);
	Vector3 v = earthVecFromGeoPoint(points[sorted[down]].geoPoint);
	float downDist = distEstimationFromEarthVecs(&target, &v);
	v = earthVecFromGeoPoint(points[sorted[up]].geoPoint);
	float upDist = distEstimationFromEarthVecs(&target, &v);
	unsigned int nearest = sorted[down];
	float nearestDist = downDist;
	if (upDist < downDist) {
		nearest = sorted[up];
		nearestDist = upDist;
	}
	float bound;
	do {
		unsigned int index;
		if (upBound < downBound) {
			bound = upBound;
			up = up < last ? up + 1 : 0;
			index = sorted[up];
			upBound = GetAxisYDistanceByLocationArrayIndex(points[index].geoPoint.y, geoPoint.y);
		} else {
			bound = downBound;
			down = down == 0 ? last : down - 1;
			index = sorted[down];
			downBound = GetAxisYDistanceByLocationArrayIndex(points[index].geoPoint.y, geoPoint.y);
		}
		v = earthVecFromGeoPoint(points[index].geoPoint);
		float dist = distEstimationFromEarthVecs(&target, &v);
		if (dist < nearestDist) {
			nearest = index;
			nearestDist = dist;
		}
	} while (bound < nearestDist);

	allPointsArray();
	const char *uid = (const char *)_allPointUIDs->bytes() + nearest * kAtlasPointUIDSize;
	return lobj_cast<PointEntry>(dbEntryWithUID(LString::stringWithCString(uid)));
}

// ---------------------------------------------------------------------------
// Regions named by abbreviations

void AtlasDatabase::_createHostObjectsRemapIfNeeded()
{
	if (_hostObjectsRemap)
		return;
	SQLiteResultsContainer *results = searchWithRequest(SQL_DB_Strings::selectAbbreviations(), (LMap *)0);
	int count = results ? results->count() : 0;
	for (int i = 0; i < count; i++) {
		if (_hostObjectsRemap == 0)
			_hostObjectsRemap = LMap::_alloc()->init();
		LMap *row = results->objectAtIndex(i);
		LString *uid = lobj_cast<LString>(row->objectForKey(kColumnNameUID));
		LString *abbr = lobj_cast<LString>(row->objectForKey(kColumnNameAbbr));
		if (uid == 0 || abbr == 0)
			continue;
		LArray *uids = (LArray *)_hostObjectsRemap->objectForKey(abbr);
		if (uids == 0) {
			uids = LArray::_alloc()->init();
			_hostObjectsRemap->setObjectForKey(uids, abbr);
			uids->release();
		}
		uids->addObject(uid);
	}
}

// The regions the last of several search terms abbreviates ("ca", "uk").
LArray *AtlasDatabase::hostUIDsFromTerms(LArray *terms)
{
	if (terms == 0 || terms->count() < 2)
		return 0;
	_createHostObjectsRemapIfNeeded();
	LString *abbr = ((LString *)terms->lastObject())->lowercaseString();
	return _hostObjectsRemap ? (LArray *)_hostObjectsRemap->objectForKey(abbr) : 0;
}

// ---------------------------------------------------------------------------
// Entry types

LString *AtlasDatabase::_stringFromEntryUseCase(TEntryTypeUseCase useCase)
{
	static LString *const sAllUseCases[] = { kUseCaseSearchPreviewMode, kUseCaseSortByKind };
	return (unsigned int)useCase < 2 ? sAllUseCases[useCase] : 0;
}

int AtlasDatabase::entryWeight(int type)
{
	return type;
}

// The entry type IDs of a use case, by type and alt type, from the remap
// table, or from the geotypes when the table is missing. Each ID has an
// _LEntryTypeRemapContainer in _entryTypeRemaps.
int AtlasDatabase::entryTypeIDFromStrings(TEntryTypeUseCase useCase, LString *type, LString *altType)
{
	if (type == 0 || (unsigned int)useCase > 1)
		return 0;
	LMap *ids = _entryTypeIDs[useCase];
	if (ids == 0) {
		StAutoreleasePool pool;
		SQLiteResultsContainer *fallback = 0;
		if (_entryTypesPerUseCase == 0) {
			SQLiteResultsContainer *counts =
			    searchWithRequest(SQL_DB_Strings::selectEntryTypeRemapMaxCount(), (LMap *)0);
			if (counts == 0 || counts->count() == 0) {
				counts = searchWithRequest(SQL_DB_Strings::selectEntryTypeRemapMaxCount_SAFE(), (LMap *)0);
				fallback = counts;
				if (counts == 0)
					return 0;
			}
			int n = counts->count();
			_entryTypesPerUseCase = n + 5;
			_entryTypeRemaps = LArray::_alloc()->initWithCapacity(n + 6 + n + 5);
			for (int i = 0; i < n + 6 + n + 5; i++)
				_entryTypeRemaps->addObject(LNull::null);
		}
		LString *useCaseName = _stringFromEntryUseCase(useCase);
		if (useCaseName == 0)
			return 0;
		SQLiteResultsContainer *rows = searchWithRequest(SQL_DB_Strings::selectEntryTypeRemap(useCaseName), (LMap *)0);
		if (rows == 0 || rows->count() == 0) {
			rows = fallback;
			if (rows == 0)
				rows = fallback =
				    searchWithRequest(SQL_DB_Strings::selectEntryTypeRemapMaxCount_SAFE(), (LMap *)0);
		}
		int count = rows ? rows->count() : 0;
		int perUseCase = _entryTypesPerUseCase;
		LMap *idsByNames = LMap::map();
		for (int i = 0; i < count; i++) {
			LMap *row = rows->objectAtIndex(i);
			LString *rowType = (LString *)row->objectForKey(kColumnNameType);
			LString *rowAltType = (LString *)row->objectForKey(kColumnNameAltType);
			LString *altKey = rowAltType ? rowAltType : &___sNoAltTypeStr;
			LString *singular = rowType, *plural = rowType;
			if (fallback == 0) {
				singular = (LString *)row->objectForKey(kColumnNameSingular);
				plural = (LString *)row->objectForKey(kColumnNamePlural);
			}
			// Rows with the same names share an ID.
			LString *names = LString::stringWithString(singular);
			if (fallback == 0)
				names->appendString(plural);
			if (ids == 0) {
				ids = LMap::_alloc()->init();
				_entryTypeIDs[useCase] = ids;
			}
			LMap *idsByAltType = (LMap *)ids->objectForKey(rowType);
			if (idsByAltType == 0) {
				idsByAltType = LMap::_alloc()->init();
				ids->setObjectForKey(idsByAltType, rowType);
				idsByAltType->release();
			}
			LValue *id = (LValue *)idsByNames->objectForKey(names);
			if (id == 0) {
				int typeID = useCase * perUseCase + 1 + i;
				id = LValue::valueWithInt(typeID);
				idsByAltType->setObjectForKey(id, altKey);
				idsByNames->setObjectForKey(id, names);
				_LEntryTypeRemapContainer *remap = (_LEntryTypeRemapContainer *)_LEntryTypeRemapContainer::_alloc()->init();
				remap->_singular = LRetain(singular);
				remap->_plural = LRetain(plural);
				remap->_typePairs = LArray::_alloc()->init();
				remap->_typePairs->addObject(LArray::arrayWithObjects(rowType, rowAltType, (LObject *)0));
				remap->_weight = UMapParsingUtils::loadIntFromObject(row->objectForKey(kColumnNameWeight));
				_entryTypeRemaps->replaceObjectAtIndex(remap, typeID);
				remap->release();
			} else {
				idsByAltType->setObjectForKey(id, altKey);
				_LEntryTypeRemapContainer *remap =
				    lobj_cast<_LEntryTypeRemapContainer>(_entryTypeRemaps->objectAtIndex(id->intValue()));
				if (remap)
					remap->_typePairs->addObject(LArray::arrayWithObjects(rowType, rowAltType, (LObject *)0));
			}
		}
		if (ids == 0)
			return 0;
	}
	LMap *idsByAltType = (LMap *)ids->objectForKey(type);
	if (idsByAltType == 0)
		return 0;
	LValue *id = (LValue *)idsByAltType->objectForKey(altType ? altType : &___sNoAltTypeStr);
	return id ? id->intValue() : 0;
}

static _LEntryTypeRemapContainer *_remapForID(LArray *remaps, int typeID)
{
	if (remaps == 0 || typeID < 0 || typeID >= (int)remaps->count())
		return 0;
	return lobj_cast<_LEntryTypeRemapContainer>(remaps->objectAtIndex(typeID));
}

LString *AtlasDatabase::localizedEntryType(int typeID, unsigned char plural)
{
	_LEntryTypeRemapContainer *remap = _remapForID(_entryTypeRemaps, typeID);
	if (remap == 0)
		return 0;
	return plural ? remap->_plural : remap->_singular;
}

LString *AtlasDatabase::localizedEntryType(TEntryTypeUseCase useCase, LString *type, LString *altType,
                                           unsigned char plural)
{
	return localizedEntryType(entryTypeIDFromStrings(useCase, type, altType), plural);
}

LArray *AtlasDatabase::typePairsForEntryType(int typeID)
{
	_LEntryTypeRemapContainer *remap = _remapForID(_entryTypeRemaps, typeID);
	return remap ? remap->_typePairs : 0;
}

// ---------------------------------------------------------------------------
// Sorting search results
//
// The rows have the generic search columns: uid, lower(name), population,
// geotype, alt type, county uid, state_name_pointer, state uid,
// country_name_pointer, country uid.

enum {
	kSortColumnName = 1,
	kSortColumnPopulation = 2,
	kSortColumnType = 3,
	kSortColumnAltType = 4,
	kSortColumnStateUID = 7,
	kSortColumnCountryUID = 9,
};

// The start of a column's value in a row.
static const char *_columnAt(const int *types, const char *values, int column)
{
	const char *p = values;
	for (int i = 0; i < column; i++) {
		switch (types[i]) {
		case kSQLiteFloat:
		case kSQLiteInteger:
			p += 4;
			break;
		case kSQLiteText:
		case kSQLiteBlob: {
			uint32_t length;
			memcpy(&length, p, 4);
			p += 4 + length;
			break;
		}
		default:
			break;
		}
	}
	return p;
}

static bool _textAt(const int *types, const char *values, int column, const char **text, uint32_t *length)
{
	if (types[column] != kSQLiteText)
		return false;
	const char *p = _columnAt(types, values, column);
	memcpy(length, p, 4);
	*text = p + 4;
	return true;
}

static float _numberAt(const int *types, const char *values, int column)
{
	const char *p = _columnAt(types, values, column);
	if (types[column] == kSQLiteFloat) {
		float value;
		memcpy(&value, p, 4);
		return value;
	}
	if (types[column] == kSQLiteInteger) {
		int value;
		memcpy(&value, p, 4);
		return (float)value;
	}
	return -1.0f;
}

// The localized kind of the row, cached by "type_alt" in the context.
static const char *_kindName(const int *types, const char *values, TStrMap<LString *, 28> *names)
{
	char key[29];
	const char *text;
	uint32_t length;
	if (!_textAt(types, values, kSortColumnType, &text, &length))
		return 0;
	unsigned int typeLength = length < 12 ? length : 12;
	memcpy(key, text, typeLength);
	key[typeLength] = 0;
	LString *type = LString::stringWithCString(key, typeLength, kLMacRomanEncoding);
	LString *altType = 0;
	unsigned int keyLength = typeLength;
	if (_textAt(types, values, kSortColumnAltType, &text, &length)) {
		unsigned int altLength = length < 15 ? length : 15;
		altType = LString::stringWithCString(text, altLength, kLMacRomanEncoding);
		key[typeLength] = '_';
		memcpy(key + typeLength + 1, text, altLength);
		keyLength = typeLength + 1 + altLength;
		key[keyLength] = 0;
	}
	LString *name = names->objectForKey(key);
	if (name == 0) {
		name = AtlasDatabase::instance()->localizedEntryType(kEntryTypeUseCaseSortByKind, type, altType, 0);
		// The key must live as long as the map: the sort runs in its own pool.
		const char *storedKey = LString::stringWithCString(key, keyLength, kLMacRomanEncoding)->cString();
		names->setObjectForKey(name ? name : (LString *)LNull::null, storedKey);
	}
	return (name && name != (LString *)LNull::null) ? name->cString() : 0;
}

// The name of the region with the UID in the column, cached by UID.
static const char *_regionName(const int *types, const char *values, int column, TStrMap<const char *, 10> *cache)
{
	const char *uid;
	uint32_t length;
	if (!_textAt(types, values, column, &uid, &length))
		return 0;
	const char *name = cache->objectForKey(uid);
	if (name)
		return name;
	GenericEntry *region =
	    AtlasDatabase::instance()->dbEntryWithUID(LString::stringWithCString(uid, length, kLMacRomanEncoding));
	LString *ownName = region ? region->ownName(0) : 0;
	name = ownName ? ownName->cString() : 0;
	if (name)
		cache->setObjectForKey(name, uid);
	return name;
}

static int _compareNames(const char *a, const char *b)
{
	if (a && b)
		return strcmp(a, b);
	if (a)
		return 1;
	return b ? -1 : 0;
}

// Orders rows by name, by region, by kind or by population; the odd modes
// are descending. Ties are ordered by name.
int AtlasDatabase::_sortByNameCallback(int, const int *types1, const int *types2, const char *values1,
                                       const char *values2, const void *context)
{
	const SEntryCompareContext *ctx = (const SEntryCompareContext *)context;
	int mode = ctx->mode;
	bool byRegion = mode == kEntriesSortByRegion || mode == kEntriesSortByRegionDescending;
	bool byKind = mode == kEntriesSortByKind || mode == kEntriesSortByKindDescending;
	bool byPopulation = mode == kEntriesSortByPopulation || mode == kEntriesSortByPopulationDescending;
	if (types1[0] != kSQLiteText || types2[0] != kSQLiteText)
		return 0;

	const char *name1, *name2;
	uint32_t length1, length2;
	_textAt(types1, values1, kSortColumnName, &name1, &length1);
	_textAt(types2, values2, kSortColumnName, &name2, &length2);
	int byName = strncmp(name1, name2, length1 < length2 ? length1 : length2);
	if (byName == 0)
		byName = (int)length1 - (int)length2;

	int result = byName;
	if (byPopulation) {
		float p1 = _numberAt(types1, values1, kSortColumnPopulation);
		float p2 = _numberAt(types2, values2, kSortColumnPopulation);
		result = p1 < p2 ? -1 : (p2 < p1 ? 1 : 0);
	} else if (byKind) {
		const char *kind1 = _kindName(types1, values1, ctx->names);
		const char *kind2 = _kindName(types2, values2, ctx->names);
		result = _compareNames(kind1, kind2);
	} else if (byRegion) {
		const char *state1 = _regionName(types1, values1, kSortColumnStateUID, ctx->sortKeys);
		const char *state2 = _regionName(types2, values2, kSortColumnStateUID, ctx->sortKeys);
		result = _compareNames(state1, state2);
		if (result == 0) {
			const char *country1 = _regionName(types1, values1, kSortColumnCountryUID, ctx->regionSortKeys);
			const char *country2 = _regionName(types2, values2, kSortColumnCountryUID, ctx->regionSortKeys);
			result = _compareNames(country1, country2);
		}
	}
	if ((byRegion || byKind || byPopulation) && result == 0)
		result = byName;
	if (mode & 1)
		result = -result;
	return result;
}

// ---------------------------------------------------------------------------
// Solar eclipses

void AtlasDatabase::_loadSolarEclipses()
{
	SQLiteResultsContainer *results = searchWithRequest(SQL_DB_Strings::selectSolarEclipses(), (LMap *)0);
	int count = results ? results->count() : 0;
	releaseField(_solarEclipses);
	_solarEclipses = LData::_alloc()->init();
	_solarEclipses->setLength(count * sizeof(SSolarEclipse));
	SSolarEclipse *eclipse = (SSolarEclipse *)_solarEclipses->bytes();
	int types[6] = { 1, 1, 4, 2, 2, 2 };
	for (int i = 0; i < count; i++, eclipse++) {
		int date = 0, time = 0;
		char type = 0;
		void *values[6] = { &date, &time, &type, &eclipse->magnitude, &eclipse->latitude, &eclipse->longitude };
		results->loadValues(values, types, 6, i);
		eclipse->type = type;
		eclipse->year = (short)(date >> 9);
		eclipse->month = (date >> 5) & 0xf;
		eclipse->day = date & 0x1f;
		eclipse->hour = (char)(time >> 6);
		eclipse->minute = time & 0x3f;
	}
}

const SSolarEclipse *AtlasDatabase::solarEclipseAtIndex(unsigned int index)
{
	if (_solarEclipses == 0) {
		_loadSolarEclipses();
		if (_solarEclipses == 0)
			return 0;
	}
	return (const SSolarEclipse *)_solarEclipses->bytes() + index;
}

unsigned int AtlasDatabase::solarEclipsesCount()
{
	if (_solarEclipses == 0) {
		_loadSolarEclipses();
		if (_solarEclipses == 0)
			return 0;
	}
	return _solarEclipses->length() / sizeof(SSolarEclipse);
}

static double _eclipseTime(const SSolarEclipse *eclipse, LTimeZone *utc)
{
	return LDate::dateWithTime(eclipse->year, eclipse->month, eclipse->day, eclipse->hour, eclipse->minute, 0, utc)
	    ->timeIntervalSinceReferenceDate();
}

// The eclipse nearest to the date, by binary search; ~0 when there are none.
unsigned int AtlasDatabase::indexOfNearestSolarEclipseForDate(LDate *date)
{
	unsigned int found = ~0u;
	int count = solarEclipsesCount();
	if (count == 0)
		return found;
	double time = date->timeIntervalSinceReferenceDate();
	LTimeZone *utc = LTimeZone::timeZoneForSecondsFromGMT(0);
	unsigned int low = 0, high = count - 1;
	while (low < high) {
		if (high - low == 1) {
			double lowTime = _eclipseTime(solarEclipseAtIndex(low), utc);
			double highTime = _eclipseTime(solarEclipseAtIndex(high), utc);
			return fabs(time - highTime) < fabs(time - lowTime) ? high : low;
		}
		found = (low + high) >> 1;
		double middle = _eclipseTime(solarEclipseAtIndex(found), utc);
		if (time > middle)
			low = found;
		else if (time < middle)
			high = found;
		else
			return found;
	}
	return found;
}

// ---------------------------------------------------------------------------
// Stars

// The context of __starsCallback: the star count and the stars of each of
// the 16 x 8 sky patches.
struct SStarsLoadContext {
	int count;
	LData *patches[128];
};

// Stars brighter than magnitude 7, by patch of right ascension and
// declination.
static int __starsCallback(int, int *types, sqlite3_stmt *stmt, void *context)
{
	SStarsLoadContext *ctx = (SStarsLoadContext *)context;
	SStarInfo star;
	star.magnitude = (float)SQLiteDatabase::checkAndGetDouble(3, types[3], stmt);
	if (star.magnitude > 7.0f)
		return 0;
	star.hip = SQLiteDatabase::checkAndGetInt(0, types[0], stmt);
	star.ra = (float)SQLiteDatabase::checkAndGetDouble(1, types[1], stmt);
	star.dec = (float)SQLiteDatabase::checkAndGetDouble(2, types[2], stmt);
	star.parallax = (float)SQLiteDatabase::checkAndGetDouble(4, types[4], stmt);
	star.bv = (float)SQLiteDatabase::checkAndGetDouble(5, types[5], stmt);
	int patch = (int)((star.ra + star.ra) * 8.0f / 360.0f) + (int)((star.dec + 90.0f) * 8.0f / 180.0f) * 16;
	LData *&data = ctx->patches[patch];
	if (data == 0)
		data = LData::dataWithLength(0);
	data->appendBytes(&star, sizeof(star));
	ctx->count++;
	return 0;
}

// The stars as SStarInfo records grouped by sky patch, in Morton order in
// each half of the sky, and the patch hierarchy: 128 leaf patches, then
// levels of 32, 8 and 2 patches, each the union of four of the level
// below. Both are cached on disk, each ending with an 'L' marker.
LData *AtlasDatabase::allStarsArray()
{
	if (_allStars)
		return _allStars;
	const char marker = 'L';
	LString *starsPath = _allStarsCachePath();
	LString *patchesPath = _allStarPatchInfosCachePath();
	LFileManager *fm = LFileManager::manager();
	_allStars = LData::_alloc()->initWithContentsOfFile(starsPath);
	if (_allStars && ((const char *)_allStars->bytes())[_allStars->length() - 1] != marker) {
		releaseField(_allStars);
		fm->removeFileAtPath(starsPath);
		fm->removeFileAtPath(patchesPath);
	}
	LData *patchesData = 0;
	if (_allStars == 0) {
		fm->removeFileAtPath(starsPath);
	} else {
		patchesData = LData::dataWithContentsOfFile(patchesPath);
		if (patchesData == 0 || patchesData->length() != kStarsPatchesCount * sizeof(SStarsPathInfo) + 1) {
			releaseField(_allStars);
			fm->removeFileAtPath(starsPath);
		} else if (((const char *)patchesData->bytes())[patchesData->length() - 1] != marker) {
			releaseField(_allStars);
			patchesData = 0;
			fm->removeFileAtPath(starsPath);
			fm->removeFileAtPath(patchesPath);
		}
	}

	if (_allStars) {
		if (_starsPatches)
			delete _starsPatches;
		_starsPatches = new TArray<SStarsPathInfo>;
		unsigned int count = patchesData->length() / sizeof(SStarsPathInfo);
		_starsPatches->preserveCapacity(count);
		const SStarsPathInfo *infos = (const SStarsPathInfo *)patchesData->bytes();
		for (unsigned int i = 0; i < count; i++)
			_starsPatches->addObject(infos[i]);
		return _allStars;
	}

	sqlDB();
	if (_db == 0)
		return 0;
	StSharableLocker lock(&_lock);
	SStarsLoadContext context;
	memset(&context, 0, sizeof(context));
	_db->processRequest(SQL_DB_Strings::stars_selectSStarInfo(), __starsCallback, &context);
	_allStars = LData::_alloc()->init();
	if (_starsPatches)
		delete _starsPatches;
	_starsPatches = new TArray<SStarsPathInfo>;

	int start = 0;
	for (int half = 0; half != 16; half += 8)
		for (int morton = 0; morton != 64; morton++) {
			int x = half, y = 0;
			int bits = morton;
			for (int b = 0; b < 4; b++, bits >>= 2) {
				x += (bits & 1) << b;
				y += ((bits >> 1) & 1) << b;
			}
			LData *patch = context.patches[y * 16 + x];
			SStarsPathInfo info;
			info.start = start;
			info.count = 0;
			if (patch) {
				info.count = patch->length() / sizeof(SStarInfo);
				_allStars->appendBytes(patch->bytes(), patch->length());
			}
			start += info.count;
			_starsPatches->addObject(info);
		}

	unsigned int first = 0;
	for (int level = 0; level < 3; level++) {
		unsigned int end = _starsPatches->count();
		int levelStart = 0;
		while (first < end) {
			SStarsPathInfo info;
			info.count = (*_starsPatches)[first].count + (*_starsPatches)[first + 1].count;
			unsigned int next = first + 2;
			if (next < end) {
				info.count += (*_starsPatches)[next].count + (*_starsPatches)[first + 3].count;
				next = first + 4;
			}
			info.start = levelStart;
			levelStart += info.count;
			_starsPatches->addObject(info);
			first = next;
		}
		first = end;
	}

	_allStars->appendBytes(&marker, 1);
	LData *patches = LData::dataWithBytes(_starsPatches->bytes(), _starsPatches->count() * sizeof(SStarsPathInfo));
	patches->appendBytes(&marker, 1);
	_allStars->writeToFile(starsPath);
	patches->writeToFile(patchesPath);
	LMap *attributes =
	    LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0664), LFileManager::fileAttrPosixPermissions, (LObject *)0);
	fm->changeFileAttributesAtPath(starsPath, attributes);
	fm->changeFileAttributesAtPath(patchesPath, attributes);
	return _allStars;
}

TArray<AtlasDatabase::SStarsPathInfo> *AtlasDatabase::starsPatches()
{
	allStarsArray();
	return _starsPatches;
}

// The constellation stars as unit vectors with a color (16 bytes each) and
// the line indices (pairs of shorts), cached on disk after an 'L' and the
// two counts.
LData *AtlasDatabase::constellationsStarsArray()
{
	if (_constellationsData)
		return _constellationsStars;
	releaseField(_constellationsStars);
	releaseField(_constellationsInds);
	releaseField(_constellations);
	LString *path = _constellationsCachePath();
	_constellationsData = LData::_alloc()->initWithContentsOfFile(path);
	if (_constellationsData) {
		const char *bytes = (const char *)_constellationsData->bytes();
		int32_t stars, lines;
		memcpy(&stars, bytes + 1, 4);
		memcpy(&lines, bytes + 5, 4);
		if (bytes[0] != 'L' || (lines + stars * 4) * 4 + 9 != (int)_constellationsData->length()) {
			releaseField(_constellationsData);
			LFileManager::manager()->removeFileAtPath(path);
		}
	}
	if (_constellationsData == 0) {
		SQLiteResultsContainer *starRows =
		    searchWithRequest(SQL_DB_Strings::stars_selectConstellationsStars(), (LMap *)0);
		SQLiteResultsContainer *lineRows =
		    searchWithRequest(SQL_DB_Strings::stars_selectConstellationsInds(), (LMap *)0);
		int stars = starRows ? starRows->count() : 0;
		int lines = lineRows ? lineRows->count() : 0;
		_constellationsData = LData::_alloc()->init();
		_constellationsData->setLength((lines + stars * 4) * 4 + 9);
		char *bytes = (char *)_constellationsData->bytes();
		bytes[0] = 'L';
		memcpy(bytes + 1, &stars, 4);
		memcpy(bytes + 5, &lines, 4);
		float *star = (float *)(bytes + 9);
		short *line = (short *)(star + stars * 4);
		float ra, dec, bv, magnitude;
		void *starValues[4] = { &ra, &dec, &bv, &magnitude };
		int starTypes[4] = { 2, 2, 2, 2 };
		for (int i = 0; i < stars; i++, star += 4) {
			starRows->loadValues(starValues, starTypes, 4, i);
			float z = sinf(dec * 3.1415927f / 180.0f);
			float r = sqrtf(1.0f - z * z);
			star[0] = cosf(ra * 3.1415927f / 180.0f) * r;
			star[1] = r * sinf(ra * 3.1415927f / 180.0f);
			star[2] = z;
			float brightness = (8.0f - magnitude) * 0.25f;
			brightness = brightness < 0.0f ? 0.0f : (brightness > 1.0f ? 1.0f : brightness);
			uint32_t color = colorFromBV(bv, brightness * brightness * 0.5f);
			memcpy(&star[3], &color, 4);
		}
		int from, to;
		void *lineValues[2] = { &from, &to };
		int lineTypes[2] = { 1, 1 };
		for (int i = 0; i < lines; i++, line += 2) {
			lineRows->loadValues(lineValues, lineTypes, 2, i);
			line[0] = (short)(from - 1);
			line[1] = (short)(to - 1);
		}
		_constellationsData->writeToFile(path);
		LFileManager::manager()->changeFileAttributesAtPath(
		    path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0664), LFileManager::fileAttrPosixPermissions, (LObject *)0));
	}
	const char *bytes = (const char *)_constellationsData->bytes();
	int32_t stars, lines;
	memcpy(&stars, bytes + 1, 4);
	memcpy(&lines, bytes + 5, 4);
	_constellationsStars = LData::_alloc()->initWithBytesNoCopy((void *)(bytes + 9), stars * 16, false);
	_constellationsInds =
	    LData::_alloc()->initWithBytesNoCopy((void *)(bytes + 9 + _constellationsStars->length()), lines * 4, false);
	return _constellationsStars;
}

LData *AtlasDatabase::constellationsIndsArray()
{
	constellationsStarsArray();
	return _constellationsInds;
}

// The direction of each constellation's middle, by its capitalized name,
// from the sum of its lines' end points.
LMap *AtlasDatabase::constellations()
{
	if (_constellations)
		return _constellations;
	const float *stars = (const float *)constellationsStarsArray()->bytes();
	_constellations = LMap::_alloc()->init();
	SQLiteResultsContainer *rows = searchWithRequest(SQL_DB_Strings::stars_selectConstellationsInds(), (LMap *)0);
	int count = rows ? rows->count() : 0;
	int from = 0, to = 0;
	char nameBuffer[300];
	void *values[3] = { &from, &to, nameBuffer };
	int types[3] = { 1, 1, 3 };
	Vector3 sum(0.0f, 0.0f, 0.0f);
	LConstantString underscore("_");
	LConstantString space(" ");
	LString *current = 0;
	for (int i = 0; i < count; i++) {
		if (!rows->loadValues(values, types, 3, i))
			continue;
		LString *name = LString::_alloc()->initWithCString(nameBuffer);
		if (current == 0)
			current = LRetain(name);
		const float *a = stars + (from - 1) * 4;
		const float *b = stars + (to - 1) * 4;
		if (!current->isEqualTo(name)) {
			// The previous constellation is complete. (The last one never
			// is, as in the original.)
			FHVec3Normalize((_SVector3 *)&sum, (_SVector3 *)&sum);
			LValue *value = LValue::valueWithBytes(&sum, sizeof(sum));
			LString *key = current->capitalizedString();
			key->replaceOccurrencesOfString(&underscore, &space);
			_constellations->setObjectForKey(value, key);
			current->release();
			current = LRetain(name);
			sum.x = a[0] + b[0];
			sum.y = a[1] + b[1];
			sum.z = a[2] + b[2];
		} else {
			sum.x += a[0] + b[0];
			sum.y += a[1] + b[1];
			sum.z += a[2] + b[2];
		}
		name->release();
	}
	if (current)
		current->release();
	return _constellations;
}

// ---------------------------------------------------------------------------
// The point kinds' colors and icons.

unsigned int sPointColors[16] = {
	Color(0.5f, 0.5f, 0.5f, 1), Color(1, 1, 0.2f, 1),    Color(1, 0.2f, 0, 1),      Color(1, 0.8f, 0, 1),
	Color(0.9f, 1, 0.5f, 1),    Color(0, 0.3f, 0.8f, 1), Color(0.4f, 0.4f, 0, 1),   Color(0.2f, 1, 0.1f, 1),
	Color(0.5f, 0.8f, 1, 1),    Color(0.6f, 0.8f, 0.1f, 1), Color(0.5f, 0.8f, 1, 1), Color(0.5f, 0.8f, 1, 1),
};

LString *sPointIconKeys[16] = {
	0, 0, kEntryCountryCapitalIconKey, kEntryStateCapitalIconKey, 0, 0, 0, 0, kEntryAppleStoreIconKey, 0,
	kEntryAppleMiniStoreIconKey, kEntryAppleComingSoonStoreIconKey,
};
