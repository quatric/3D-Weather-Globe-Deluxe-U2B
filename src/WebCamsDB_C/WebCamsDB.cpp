// WebCamsManagerImp, WebCameraImp, WebCamContainerImp, WebCamsTreeImp, the
// SQL requests (SQL_DB_Strings) and the C API.

#include "WebCamsDB.h"
#include "SQLiteLib.h"
#include "LFileManager.h"
#include "LInvocation.h"
#include "LNotification.h"
#include "LRunLoop.h"
#include "LScanner.h"
#include <CoreServices/CoreServices.h>

class WebCameraImp;
class WebCamContainerImp;
class WebCamsTreeImp;

// The singleton holds the database.
class WebCamsManagerImp : public LObject {
	LF_DECLARE_CLASS(WebCamsManagerImp)
public:
	WebCamsManagerImp();
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual WebCamsManagerImp *getOwnerLObject() { return this; }
	virtual intptr_t _rescanAndApplyUpdates(char *args);
	virtual intptr_t _rescanAndApplyUpdatesCompleted(char *args);

	static WebCamsManagerImp *manager();

	// The rows of `request` as LMaps of strings, or 0.
	LArray *performRequest(LString *request);
	SQLiteResultsContainer *performRequestWithStatement(LString *request);
	WebCamContainerImp *allWebCamsFlat();
	WebCamContainerImp *webCamsForTerms(LString *terms);
	WebCamContainerImp *webCamsForWBRIN(LString *rin);
	WebCamContainerImp *webCamsForCategoryWithUID(LString *uid);
	WebCamsTreeImp *categoriesTree();
	// Looks for update files not applied yet, on another thread if `async`;
	// `notify` posts the WillStart/Changed/DidEnd notifications.
	void rescanForUpdates(unsigned char async, unsigned char notify);

	LString *_dbFilePathCached();
	LString *_dbFilePathBundled() { return _dbPathBundled; }
	LString *_updatesDownloadFolder() { return _updatesFolder; }
	LArray *_allUpdateFiles();
	void _initializeSQLDatabase();
	LArray *_simpleSearchWithRequest(const LString *request);

	static LString *kMainDatabaseWillStartUpdate;
	static LString *kMainDatabaseChangedNotification;
	static LString *kMainDatabaseDidEndUpdate;
	static LString *kAppKeyStringKey;
	static LString *kDBFilePathCachedKey;
	static LString *kDBFilePathBundledKey;
	static LString *kUpdatesFolderPathKey;

	LString *_appKey;
	LString *_dbPathCached;
	LString *_dbPathBundled;
	LString *_updatesFolder;
	SQLiteDatabase *_db;
	WebCamsTreeImp *_categoriesTree;
	bool _updating;
	bool _notifyUpdate;
	CSharableResource _lock;
};

class WebCameraImp : public LObject {
	LF_DECLARE_CLASS(WebCameraImp)
public:
	WebCameraImp();
	virtual void dealloc();
	virtual WebCameraImp *getOwnerLObject() { return this; }

	// Cameras are shared through a cache by UID.
	static WebCameraImp *webCamWithUID(LString *uid);
	static WebCameraImp *webCamWithDBMap(LMap *map);
	static void clearWebCamsCache();
	WebCameraImp *initWithUID(LString *uid);
	LData *previewImageData() const;
	LData *descriptionRTFData() const;
	// Loads the camera's row the first time.
	void _validate();

	static LMap *sCache;

	bool _validated;
	LString *_uid;
	LString *_name;
	LString *_imageURL;
	LString *_pageName;
	LString *_pageURL;
	LString *_copyright;
	int _frameRate;
	LString *_descriptionString;
	LString *_wbRINs;
};

class WebCamContainerImp : public LObject {
	LF_DECLARE_CLASS(WebCamContainerImp)
public:
	WebCamContainerImp() : _rows(0) {}
	virtual void dealloc();
	virtual WebCamContainerImp *getOwnerLObject() { return this; }

	WebCamContainerImp *initWithResults(TLArray<LMap *> *rows);
	int count();
	WebCameraImp *webCamAtIndex(int index);

	LArray *_rows;
};

class WebCamsTreeImp : public LObject {
	LF_DECLARE_CLASS(WebCamsTreeImp)
public:
	WebCamsTreeImp() : _parent(0), _uid(0), _title(0), _children(0), _webCams(0), _validated(false), _webCamsLoaded(false) {}
	virtual void dealloc();
	virtual WebCamsTreeImp *getOwnerLObject() { return this; }

	WebCamsTreeImp *initWithParams(_opaqueWebCamsTree *parent, LString *title, LString *uid);
	WebCamContainerImp *webCams();
	int childrenCount();
	WebCamsTreeImp *childAtIndex(int index);
	// Loads the subcategories the first time.
	void _validate();

	_opaqueWebCamsTree *_parent;
	LString *_uid;
	LString *_title;
	LArray *_children;
	WebCamContainerImp *_webCams;
	bool _validated;
	bool _webCamsLoaded;
};

LF_IMPLEMENT_CLASS(WebCamsManagerImp, 0)
LF_IMPLEMENT_CLASS(WebCameraImp, 0)
LF_IMPLEMENT_CLASS(WebCamContainerImp, 0)
LF_IMPLEMENT_CLASS(WebCamsTreeImp, 0)
LF_SELECTOR(WebCamsManagerImp, _rescanAndApplyUpdates);
LF_SELECTOR(WebCamsManagerImp, _rescanAndApplyUpdatesCompleted);

// ---------------------------------------------------------------------------
// The database's tables and columns.

static LConstantString _sAppsTable("ZAPPLICATION");
static LConstantString _sWebCamsTable("ZCATEGORYITEM");
static LConstantString _sKeywordsTable("ZKEYWORD");
static LConstantString _sOccurenceTable("Z_2KEYWORD");
static LConstantString _sCategoryTreeTable("Z_3SUBITEM");
static LConstantString _sAppKeyID("Z_PK");
static LConstantString _sAppKeyString("ZBUNDLEIDENTIFIER");
static LConstantString _sWebCamsTableClassColumn("Z_ENT");
static LConstantString _sCategoryClassID("3");
static LConstantString _sWebCamsClassID("4");
static LConstantString _sKeywordString("ZKEYSTRING");
static LConstantString _sKeywordID("Z_PK");
static LConstantString _sKeywordOccurence("Z_6KEYWORD");
static LConstantString _sWebCamOccurence("Z_2ITEM");
static LConstantString _sCategoryParent("Z_3SUPERCATEGORY");
static LConstantString _sCategoryChild("Z_2SUBITEM");
static LConstantString _sWebCamID("Z_PK");
static LConstantString _sWebCamName("ZNAME");
static LConstantString _sWebCamImageURL("ZWEBCAMURL");
static LConstantString _sWebCamPageURL("ZSOURCEPAGEURL");
static LConstantString _sWebCamDescription("ZCAMDESCRIPTIONDATA");
static LConstantString _sWebCamDescriptionString("ZCAMDESCRIPTIONSTRING");
static LConstantString _sWebCamIsPublic("ZISPUBLIC");
static LConstantString _sWebCamFrameRate("ZFRAMERATE");
static LConstantString _sWebCamPageName("ZSOURCEPAGENAME");
static LConstantString _sWebCamCopyright("ZCOPYRIGHT");
static LConstantString _sWebCamPreview("ZTHUMBNAILDATA");
static LConstantString _sWebCamWBRINs("ZWBARTICLEIDS");
static LConstantString _sCategoryUID("Z_PK");
static LConstantString _sCategoryTitle("ZNAME");
static LConstantString _sColumnNameUpdateFileName("fileName");
static LConstantString _sUpdatesTableName("updates");
static LConstantString _sORString("or");
static LConstantString _sANDString("and");
static LConstantString _sNOTString("not");

namespace SQL_DB_Strings {
LString *kWebCamID = &_sWebCamID;
LString *kWebCamName = &_sWebCamName;
LString *kWebCamImageURL = &_sWebCamImageURL;
LString *kWebCamPageURL = &_sWebCamPageURL;
LString *kWebCamDescription = &_sWebCamDescription;
LString *kWebCamDescriptionString = &_sWebCamDescriptionString;
LString *kWebCamIsPublic = &_sWebCamIsPublic;
LString *kWebCamFrameRate = &_sWebCamFrameRate;
LString *kWebCamPageName = &_sWebCamPageName;
LString *kWebCamCopyright = &_sWebCamCopyright;
LString *kWebCamPreview = &_sWebCamPreview;
LString *kWebCamWBRINs = &_sWebCamWBRINs;
LString *kCategoryUID = &_sCategoryUID;
LString *kCategoryTitle = &_sCategoryTitle;
} // namespace SQL_DB_Strings
LString *_kColumnNameUpdateFileName = &_sColumnNameUpdateFileName;
LString *_kUpdatesTableName = &_sUpdatesTableName;

using namespace SQL_DB_Strings;

static LConstantString _kSMKSupportFolder("Software MacKiev");
static LConstantString _kSMKWebCamCollectionFolder("WebCam Collection");
static LConstantString _kDBFilePath("WebCamCollection.sql");
static LConstantString _kUpdatePrefix("wc");
static LConstantString _sMainDatabaseWillStartUpdate("WEBCAMS_MainDatabaseWillStartUpdate");
static LConstantString _sMainDatabaseChangedNotification("WEBCAMS_MainDatabaseChangedNotification");
static LConstantString _sMainDatabaseDidEndUpdate("WEBCAMS_MainDatabaseDidEndUpdate");
static LConstantString _sAppKeyStringKey("AppKeyStringKey");
static LConstantString _sDBFilePathCachedKey("DBFilePathCachedKey");
static LConstantString _sDBFilePathBundledKey("DBFilePathBundledKey");
static LConstantString _sUpdatesFolderPathKey("UpdatesFolderPathKey");

LString *WebCamsManagerImp::kMainDatabaseWillStartUpdate = &_sMainDatabaseWillStartUpdate;
LString *WebCamsManagerImp::kMainDatabaseChangedNotification = &_sMainDatabaseChangedNotification;
LString *WebCamsManagerImp::kMainDatabaseDidEndUpdate = &_sMainDatabaseDidEndUpdate;
LString *WebCamsManagerImp::kAppKeyStringKey = &_sAppKeyStringKey;
LString *WebCamsManagerImp::kDBFilePathCachedKey = &_sDBFilePathCachedKey;
LString *WebCamsManagerImp::kDBFilePathBundledKey = &_sDBFilePathBundledKey;
LString *WebCamsManagerImp::kUpdatesFolderPathKey = &_sUpdatesFolderPathKey;

static int _sWCMRetainCount = 0;

// ---------------------------------------------------------------------------
// SQL_DB_Strings

#define WC_TERM_CONDITION \
	"(%@\t like '%%%@%%' OR\t%@ IN\t  (SELECT %@ FROM %@ WHERE %@ IN (SELECT %@ FROM %@ WHERE %@ LIKE '%%%@%%')))"
#define WC_NOT_TERM_CONDITION \
	"(%@ NOT like '%%%@%%' AND\t%@ NOT IN (SELECT %@ FROM %@ WHERE %@ IN (SELECT %@ FROM %@ WHERE %@ LIKE '%%%@%%')))"

namespace SQL_DB_Strings {

LString *selectWebCamWithUID(LString *uid)
{
	return LString::stringWithFormat("SELECT %@, %@, %@, %@, %@, %@, %@, %@ FROM %@ WHERE %@='%@'", kWebCamName,
	                                 kWebCamImageURL, kWebCamPageURL, kWebCamFrameRate, kWebCamPageName,
	                                 kWebCamCopyright, kWebCamDescriptionString, kWebCamWBRINs, &_sWebCamsTable,
	                                 kWebCamID, uid);
}

LString *selectWebCamDescriptionWithUID(LString *uid)
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@='%@'", kWebCamDescription, &_sWebCamsTable,
	                                 kWebCamID, uid);
}

LString *selectWebCamPreviewWithUID(LString *uid)
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@='%@'", kWebCamPreview, &_sWebCamsTable, kWebCamID,
	                                 uid);
}

// A camera whose name or keywords contain the term (or don't, with `negate`).
LString *conditionForTerm(LString *term, unsigned char negate)
{
	return LString::stringWithFormat(negate ? WC_NOT_TERM_CONDITION : WC_TERM_CONDITION, kWebCamName, term,
	                                 &_sWebCamID, &_sWebCamOccurence, &_sOccurenceTable, &_sKeywordOccurence,
	                                 &_sKeywordID, &_sKeywordsTable, &_sKeywordString, term);
}

LString *selectWebCamsForWBRIN(LString *rin)
{
	return LString::stringWithFormat(
	    "SELECT %@, %@ FROM %@ WHERE %@ like '%%;%@;%%' AND %@=%@ AND %@ NOTNULL ORDER BY %@", kWebCamID,
	    kWebCamName, &_sWebCamsTable, kWebCamWBRINs, rin, &_sWebCamsTableClassColumn, &_sWebCamsClassID,
	    kWebCamImageURL, kWebCamName);
}

LString *selectWebCamsForCategory(LString *uid)
{
	return LString::stringWithFormat(
	    "SELECT %@, %@ FROM %@ WHERE %@=%@ AND %@ NOTNULL AND %@ IN (SELECT %@ FROM %@ WHERE %@=%@) ORDER BY %@",
	    kCategoryUID, kWebCamName, &_sWebCamsTable, &_sWebCamsTableClassColumn, &_sWebCamsClassID, kWebCamImageURL,
	    kCategoryUID, &_sCategoryChild, &_sCategoryTreeTable, &_sCategoryParent, uid, kCategoryTitle);
}

LString *selectAllWebCamsFlat()
{
	return LString::stringWithFormat("SELECT %@, %@ FROM %@ WHERE %@=%@ AND %@ NOTNULL ORDER BY %@", kCategoryUID,
	                                 kWebCamName, &_sWebCamsTable, &_sWebCamsTableClassColumn, &_sWebCamsClassID,
	                                 kWebCamImageURL, kCategoryTitle);
}

// The subcategories of `parent`, or the root categories (those no category
// contains) without one.
LString *selectCategoriesUIDNameForParent(LString *parent)
{
	if (parent && parent->length())
		return LString::stringWithFormat(
		    "SELECT %@, %@ FROM %@ WHERE %@=%@ AND %@ IN (SELECT %@ FROM %@ WHERE %@=%@) ORDER BY %@", kCategoryUID,
		    kCategoryTitle, &_sWebCamsTable, &_sWebCamsTableClassColumn, &_sCategoryClassID, kCategoryUID,
		    &_sCategoryChild, &_sCategoryTreeTable, &_sCategoryParent, parent, kCategoryTitle);
	return LString::stringWithFormat("SELECT %@, %@ FROM %@ WHERE %@=%@ AND %@ IN (SELECT DISTINCT %@ FROM %@ "
	                                 "EXCEPT SELECT DISTINCT %@ FROM %@) ORDER BY %@",
	                                 kCategoryUID, kCategoryTitle, &_sWebCamsTable, &_sWebCamsTableClassColumn,
	                                 &_sCategoryClassID, kCategoryUID, &_sCategoryParent, &_sCategoryTreeTable,
	                                 &_sCategoryChild, &_sCategoryTreeTable, kCategoryTitle);
}

LString *selectFilenameFromUpdatesThatIsLike(LString *name)
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@ like '%@'", _kColumnNameUpdateFileName,
	                                 _kUpdatesTableName, _kColumnNameUpdateFileName, name);
}

LString *insertFilenameIntoUpdates(LString *name)
{
	return LString::stringWithFormat("INSERT INTO %@ (%@) VALUES ('%@')", _kUpdatesTableName,
	                                 _kColumnNameUpdateFileName, name);
}

LArray *separatedTermsFromSearchString(LString *search)
{
	LString *str = LString::stringWithString(search);
	str->trimWithCharacters(LCharSet::charSetWithType(LCharSet::kSeparators));
	if (str->length() == 0)
		return 0;
	return SQLiteUtils::separatedSearchTermsForString(str);
}

// The cameras matching the terms, among those the application may show: the
// public ones it doesn't exclude and the private ones it includes.
LString *selectWebCamUIDs(LString *search, LString *appKey)
{
	LArray *terms = separatedTermsFromSearchString(search);
	int count = terms ? terms->count() : 0;
	if (count == 0)
		return 0;
	LString *conditions = LString::string();
	bool negate = false, joined = true;
	for (int i = 0; i < count; i++) {
		LString *term = ((LString *)terms->objectAtIndex(i))->lowercaseString();
		if (term->isEqualTo(&_sNOTString)) {
			negate = true;
		} else if (term->isEqualTo(&_sORString) || term->isEqualTo(&_sANDString)) {
			if (!negate) {
				if (i < count - 1) {
					// The conditions are parenthesized: "(...)or(...)".
					conditions->appendString(term->isEqualTo(&_sORString) ? &_sORString : &_sANDString);
					joined = true;
				} else {
					negate = false;
				}
			}
		} else {
			if (!joined)
				conditions->appendString(&_sANDString);
			conditions->appendString(conditionForTerm(term, negate));
			negate = false;
			joined = false;
		}
	}
	LString *appID =
	    LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@ like '%@'", &_sAppKeyID, &_sAppsTable, &_sAppKeyString, appKey);
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE (%@) AND %@=%@ AND %@ NOTNULL AND ((%@=1 AND %@ NOT "
	                                 "IN (%@)) OR (%@=0 AND %@ IN (%@))) ORDER BY %@",
	                                 kWebCamID, &_sWebCamsTable, conditions, &_sWebCamsTableClassColumn,
	                                 &_sWebCamsClassID, kWebCamImageURL, kWebCamIsPublic, kWebCamID, appID,
	                                 kWebCamIsPublic, kWebCamID, appID, kWebCamName);
}

} // namespace SQL_DB_Strings

// ---------------------------------------------------------------------------
// WebCamsManagerImp

WebCamsManagerImp::WebCamsManagerImp()
    : _appKey(0), _dbPathCached(0), _dbPathBundled(0), _updatesFolder(0), _db(0), _categoriesTree(0),
      _updating(false), _notifyUpdate(false)
{
}

WebCamsManagerImp *WebCamsManagerImp::manager()
{
	if (_sClassImp._singleton == 0)
		return 0;
	return (WebCamsManagerImp *)_sClassImp.singleton();
}

void WebCamsManagerImp::initAsSingleton(void *param)
{
	LMap *params = (LMap *)param;
	_sWCMRetainCount++;
	_appKey = LRetain((LString *)params->objectForKey(kAppKeyStringKey));
	_dbPathCached = LRetain((LString *)params->objectForKey(kDBFilePathCachedKey));
	_dbPathBundled = LRetain((LString *)params->objectForKey(kDBFilePathBundledKey));
	_updatesFolder = LRetain((LString *)params->objectForKey(kUpdatesFolderPathKey));
	_initializeSQLDatabase();
}

void WebCamsManagerImp::dealloc()
{
	LRelease(_appKey);
	_appKey = 0;
	LRelease(_dbPathCached);
	_dbPathCached = 0;
	LRelease(_dbPathBundled);
	_dbPathBundled = 0;
	LRelease(_updatesFolder);
	_updatesFolder = 0;
	LRelease(_db);
	_db = 0;
	LRelease(_categoriesTree);
	_categoriesTree = 0;
	LObject::dealloc();
	_sWCMRetainCount--;
}

void WebCamsManagerImp::rescanForUpdates(unsigned char async, unsigned char notify)
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

intptr_t WebCamsManagerImp::_rescanAndApplyUpdatesCompleted(char *args)
{
	LValue *changed = LF_ARG(args, 0, LValue *);
	if (changed && changed->intValue()) {
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

LArray *WebCamsManagerImp::performRequest(LString *request)
{
	if (_db == 0)
		return 0;
	return _db->processRequest((const LString *)request);
}

SQLiteResultsContainer *WebCamsManagerImp::performRequestWithStatement(LString *request)
{
	if (_db == 0)
		return 0;
	SQLiteStatement *statement = LAutorelease(SQLiteStatement::_alloc()->initWithDB(_db, request));
	if (statement == 0)
		return 0;
	return statement->process();
}

// The update files ("wc..."), sorted by name.
LArray *WebCamsManagerImp::_allUpdateFiles()
{
	LFileManager *fm = LFileManager::manager();
	LString *folder = _updatesFolder;
	LArray *names = folder ? fm->directoryContentsAtPath(folder) : 0;
	LArray *files = LArray::array();
	int count = names ? names->count() : 0;
	for (int i = 0; i < count; i++) {
		LString *name = ((LString *)names->objectAtIndex(i))->lowercaseString();
		if (name->hasPrefix(&_kUpdatePrefix))
			files->addObject(folder->stringByAppendingPathComponent(name));
	}
	files->setComparator(_LComparableObjectCompareCallback);
	files->sort();
	return files;
}

WebCamContainerImp *WebCamsManagerImp::allWebCamsFlat()
{
	LArray *rows = performRequest(selectAllWebCamsFlat());
	return WebCamContainerImp::_alloc()->initWithResults((TLArray<LMap *> *)rows);
}

WebCamContainerImp *WebCamsManagerImp::webCamsForTerms(LString *terms)
{
	LString *request = selectWebCamUIDs(terms, _appKey);
	if (request == 0)
		return 0;
	LArray *rows = performRequest(request);
	return WebCamContainerImp::_alloc()->initWithResults((TLArray<LMap *> *)rows);
}

WebCamContainerImp *WebCamsManagerImp::webCamsForWBRIN(LString *rin)
{
	LArray *rows = performRequest(selectWebCamsForWBRIN(rin));
	return WebCamContainerImp::_alloc()->initWithResults((TLArray<LMap *> *)rows);
}

WebCamContainerImp *WebCamsManagerImp::webCamsForCategoryWithUID(LString *uid)
{
	LArray *rows = performRequest(selectWebCamsForCategory(uid));
	return WebCamContainerImp::_alloc()->initWithResults((TLArray<LMap *> *)rows);
}

// ~/Library/Application Support/Software MacKiev/WebCam Collection/
// WebCamCollection.sql unless given, its folders created as needed.
LString *WebCamsManagerImp::_dbFilePathCached()
{
	LFileManager *fm = LFileManager::manager();
	if (_dbPathCached == 0) {
		_dbPathCached = LRetain(LFileManager::pathToFolder(kApplicationSupportFolderType, kUserDomain, true));
		if (_dbPathCached == 0 || !fm->isReadableFileAtPath(_dbPathCached))
			return 0;
		_dbPathCached->appendPathComponent(&_kSMKSupportFolder);
		_dbPathCached->appendPathComponent(&_kSMKWebCamCollectionFolder);
		_dbPathCached->appendPathComponent(&_kDBFilePath);
	}
	if (!fm->objectExistsAtPath(_dbPathCached, 0)) {
		LArray *missing = LArray::array();
		LString *folder = _dbPathCached->stringByDeletingLastPathComponent();
		if (folder == 0 || folder->length() == 0)
			return 0;
		while (!fm->objectExistsAtPath(folder, 0)) {
			missing->addObject(folder->lastPathComponent());
			folder = folder->stringByDeletingLastPathComponent();
		}
		for (int i = missing->count() - 1; i >= 0; i--) {
			folder = folder->stringByAppendingPathComponent((LString *)missing->objectAtIndex(i));
			if (!fm->createFolderAtPath(folder))
				return 0;
			fm->changeFileAttributesAtPath(
			    folder, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777), LFileManager::fileAttrPosixPermissions, (LObject *)0));
		}
	}
	return _dbPathCached;
}

WebCamsTreeImp *WebCamsManagerImp::categoriesTree()
{
	if (_categoriesTree == 0)
		_categoriesTree = WebCamsTreeImp::_alloc()->initWithParams(0, LString::stringWithCString(""), 0);
	return _categoriesTree;
}

// Opens the cached database, first copying the bundled one over it when that
// is newer, then applies updates.
void WebCamsManagerImp::_initializeSQLDatabase()
{
	LRelease(_db);
	_db = 0;
	LString *cached = _dbFilePathCached();
	if (cached == 0)
		LF_RAISE(NilPtrException());
	LString *bundled = _dbPathBundled;
	if (bundled == 0) {
		_db = SQLiteDatabase::_alloc()->initWithDatabasePath(cached);
		return;
	}
	LFileManager *fm = LFileManager::manager();
	double cachedTime = fm->lastUpdateTimeForFile(cached);
	double bundledTime = fm->lastUpdateTimeForFile(bundled);
	if (fm->objectExistsAtPath(cached, 0) && bundledTime <= cachedTime) {
		_db = SQLiteDatabase::_alloc()->initWithDatabasePath(cached);
		if (_db) {
			rescanForUpdates(false, false);
			return;
		}
	}
	fm->removeFileAtPath(cached);
	LData *data = LData::dataWithContentsOfFile(bundled);
	if (data == 0 || !data->writeToFile(cached))
		return;
	fm->changeFileAttributesAtPath(
	    cached, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0666), LFileManager::fileAttrPosixPermissions, (LObject *)0));
	_db = SQLiteDatabase::_alloc()->initWithDatabasePath(cached);
	rescanForUpdates(false, false);
}

LArray *WebCamsManagerImp::_simpleSearchWithRequest(const LString *request)
{
	StSharableLocker lock(&_lock);
	if (_db == 0)
		_initializeSQLDatabase();
	return _db->processRequest(request);
}

// Every update file not recorded in the updates table is a compressed SQL
// dump: run it, then record it.
intptr_t WebCamsManagerImp::_rescanAndApplyUpdates(char *)
{
	bool changed = false;
	{
		StSharableLocker lock(&_lock);
		LArray *files = _allUpdateFiles();
		int count = files ? files->count() : 0;
		for (int i = 0; i < count; i++) {
			LString *path = (LString *)files->objectAtIndex(i);
			LString *name = path->lastPathComponent()->lowercaseString();
			LArray *found = _simpleSearchWithRequest(selectFilenameFromUpdatesThatIsLike(name));
			if (found == 0)
				LLog("upadtes table not found...");
			else if (found->count() != 0)
				continue;
			LData *dump = LData::dataWithContentsOfFile(path);
			dump = dump ? dump->detachDecomressedData() : 0;
			if (dump == 0) {
				LLog("Unable to load and decomress data at path %@\n", path);
				break;
			}
			if (!_db->processRequestsWithData(dump, true)) {
				if (_simpleSearchWithRequest(LString::stringWithCString("ROLLBACK TRANSACTION")) == 0)
					LLog("Unable to rollback after execute dump at %@\n", path);
				break;
			}
			_simpleSearchWithRequest(insertFilenameIntoUpdates(name));
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

// ---------------------------------------------------------------------------
// WebCameraImp

LMap *WebCameraImp::sCache = 0;

WebCameraImp::WebCameraImp()
    : _validated(false), _uid(0), _name(0), _imageURL(0), _pageName(0), _pageURL(0), _copyright(0), _frameRate(0),
      _descriptionString(0), _wbRINs(0)
{
}

void WebCameraImp::clearWebCamsCache()
{
	if (sCache) {
		sCache->release();
		sCache = 0;
	}
}

void WebCameraImp::dealloc()
{
	LRelease(_uid);
	_uid = 0;
	LRelease(_name);
	_name = 0;
	LRelease(_imageURL);
	_imageURL = 0;
	LRelease(_pageName);
	_pageName = 0;
	LRelease(_pageURL);
	_pageURL = 0;
	LRelease(_copyright);
	_copyright = 0;
	LRelease(_descriptionString);
	_descriptionString = 0;
	LRelease(_wbRINs);
	_wbRINs = 0;
	LObject::dealloc();
}

WebCameraImp *WebCameraImp::initWithUID(LString *uid)
{
	if (LObject::init() == 0)
		return 0;
	_uid = LRetain(uid);
	return this;
}

WebCameraImp *WebCameraImp::webCamWithUID(LString *uid)
{
	if (sCache == 0)
		sCache = LMap::_alloc()->init();
	WebCameraImp *cam = (WebCameraImp *)sCache->objectForKey(uid);
	if (cam == 0) {
		cam = _alloc()->initWithUID(uid);
		if (cam) {
			sCache->setObjectForKey(cam, uid);
			cam->release();
		}
	}
	return cam;
}

WebCameraImp *WebCameraImp::webCamWithDBMap(LMap *map)
{
	LString *uid = lobj_cast<LString>(map->objectForKey(kWebCamID));
	if (uid == 0)
		return 0;
	WebCameraImp *cam = webCamWithUID(uid);
	if (cam && cam->_name == 0)
		cam->_name = LRetain(lobj_cast<LString>(map->objectForKey(kWebCamName)));
	return cam;
}

void WebCameraImp::_validate()
{
	if (_validated)
		return;
	_validated = true;
	LArray *rows = WebCamsManagerImp::manager()->performRequest(SQL_DB_Strings::selectWebCamWithUID(_uid));
	if (rows == 0 || rows->count() != 1)
		return;
	LMap *row = (LMap *)rows->objectAtIndex(0);
	_name = LRetain(lobj_cast<LString>(row->objectForKey(kWebCamName)));
	_imageURL = LRetain(lobj_cast<LString>(row->objectForKey(kWebCamImageURL)));
	_pageName = LRetain(lobj_cast<LString>(row->objectForKey(kWebCamPageName)));
	_pageURL = LRetain(lobj_cast<LString>(row->objectForKey(kWebCamPageURL)));
	_copyright = LRetain(lobj_cast<LString>(row->objectForKey(kWebCamCopyright)));
	_descriptionString = LRetain(lobj_cast<LString>(row->objectForKey(kWebCamDescriptionString)));
	_wbRINs = LRetain(lobj_cast<LString>(row->objectForKey(kWebCamWBRINs)));
	_frameRate = UMapParsingUtils::loadIntFromObject(row->objectForKey(kWebCamFrameRate));
}

LData *WebCameraImp::previewImageData() const
{
	SQLiteResultsContainer *results =
	    WebCamsManagerImp::manager()->performRequestWithStatement(SQL_DB_Strings::selectWebCamPreviewWithUID(_uid));
	if (results == 0 || results->count() != 1)
		return 0;
	return lobj_cast<LData>(results->objectAtIndex(0)->objectForKey(kWebCamPreview));
}

LData *WebCameraImp::descriptionRTFData() const
{
	SQLiteResultsContainer *results =
	    WebCamsManagerImp::manager()->performRequestWithStatement(SQL_DB_Strings::selectWebCamDescriptionWithUID(_uid));
	if (results == 0 || results->count() != 1)
		return 0;
	return lobj_cast<LData>(results->objectAtIndex(0)->objectForKey(kWebCamDescription));
}

// ---------------------------------------------------------------------------
// WebCamContainerImp

void WebCamContainerImp::dealloc()
{
	LRelease(_rows);
	_rows = 0;
	LObject::dealloc();
}

// Keeps the rows that have a camera ID.
WebCamContainerImp *WebCamContainerImp::initWithResults(TLArray<LMap *> *rows)
{
	if (LObject::init() == 0)
		return 0;
	int count = rows ? rows->count() : 0;
	for (int i = 0; i < count; i++) {
		LMap *row = rows->objectAtIndex(i);
		if (lobj_cast<LString>(row->objectForKey(kWebCamID))) {
			if (_rows == 0)
				_rows = LArray::_alloc()->init();
			_rows->addObject(row);
		}
	}
	return this;
}

int WebCamContainerImp::count()
{
	return _rows ? _rows->count() : 0;
}

WebCameraImp *WebCamContainerImp::webCamAtIndex(int index)
{
	if (_rows == 0 || index < 0 || index >= (int)_rows->count())
		return 0;
	return WebCameraImp::webCamWithDBMap((LMap *)_rows->objectAtIndex(index));
}

// ---------------------------------------------------------------------------
// WebCamsTreeImp

void WebCamsTreeImp::dealloc()
{
	LRelease(_title);
	_title = 0;
	LRelease(_children);
	_children = 0;
	LRelease(_uid);
	_uid = 0;
	LRelease(_webCams);
	_webCams = 0;
	LObject::dealloc();
}

WebCamsTreeImp *WebCamsTreeImp::initWithParams(_opaqueWebCamsTree *parent, LString *title, LString *uid)
{
	if (LObject::init() == 0)
		return 0;
	_uid = LRetain(uid);
	_parent = parent;
	_title = LRetain(title);
	return this;
}

WebCamContainerImp *WebCamsTreeImp::webCams()
{
	if (!_webCamsLoaded) {
		_webCamsLoaded = true;
		_webCams = WebCamsManagerImp::manager()->webCamsForCategoryWithUID(_uid);
	}
	return _webCams;
}

void WebCamsTreeImp::_validate()
{
	if (_validated)
		return;
	_validated = true;
	LArray *rows = WebCamsManagerImp::manager()->performRequest(SQL_DB_Strings::selectCategoriesUIDNameForParent(_uid));
	int count = rows ? rows->count() : 0;
	for (int i = 0; i < count; i++) {
		if (_children == 0)
			_children = LArray::_alloc()->init();
		LMap *row = (LMap *)rows->objectAtIndex(i);
		LString *uid = lobj_cast<LString>(row->objectForKey(kCategoryUID));
		LString *title = lobj_cast<LString>(row->objectForKey(kCategoryTitle));
		WebCamsTreeImp *child = _alloc()->initWithParams((_opaqueWebCamsTree *)this, title, uid);
		if (child) {
			_children->addObject(child);
			child->release();
		}
	}
}

int WebCamsTreeImp::childrenCount()
{
	_validate();
	return _children ? _children->count() : 0;
}

WebCamsTreeImp *WebCamsTreeImp::childAtIndex(int index)
{
	_validate();
	if (_children == 0 || index < 0 || index >= (int)_children->count())
		return 0;
	return (WebCamsTreeImp *)_children->objectAtIndex(index);
}

// ---------------------------------------------------------------------------
// The C API

static CFStringRef _cf(LString *str)
{
	return str ? str->cfString() : 0;
}

static CFDataRef _cf(LData *data)
{
	return data ? data->cfData() : 0;
}

#define CAM(ref) ((WebCameraImp *)(ref))
#define CONTAINER(ref) ((WebCamContainerImp *)(ref))
#define TREE(ref) ((WebCamsTreeImp *)(ref))

static bool _initializeManager(LMap *params)
{
	WebCamsManagerImp *manager = (WebCamsManagerImp *)WebCamsManagerImp::classObject()->singleton(params);
	return manager && manager->_db;
}

bool InitializeWebCamsManager(CFStringRef appKey)
{
	return _initializeManager(
	    LMap::mapWithObjectsAndKeys(LString::stringWithCFString(appKey), WebCamsManagerImp::kAppKeyStringKey, (LObject *)0));
}

bool InitializeWebCamsManager2(CFStringRef appKey, CFStringRef dbFilePathBundled, CFStringRef dbFilePathCached,
                               CFStringRef updatesFolderPath)
{
	return _initializeManager(LMap::mapWithObjectsAndKeys(
	    LString::stringWithCFString(appKey), WebCamsManagerImp::kAppKeyStringKey,
	    LString::stringWithCFString(dbFilePathBundled), WebCamsManagerImp::kDBFilePathBundledKey,
	    LString::stringWithCFString(dbFilePathCached), WebCamsManagerImp::kDBFilePathCachedKey,
	    LString::stringWithCFString(updatesFolderPath), WebCamsManagerImp::kUpdatesFolderPathKey, 0));
}

void FinalizeWebCamsManager()
{
	if (WebCamsManagerImp::manager())
		WebCamsManagerImp::classObject()->killSingleton();
	WebCameraImp::clearWebCamsCache();
}

void WCManagerRescanForUpdates()
{
	if (WebCamsManagerImp *manager = WebCamsManagerImp::manager())
		manager->rescanForUpdates(false, false);
}

WebCamsContainerRef WCManagerGetAllWebCamsFlat()
{
	WebCamsManagerImp *manager = WebCamsManagerImp::manager();
	return manager ? (WebCamsContainerRef)manager->allWebCamsFlat() : 0;
}

WebCamsTreeRef WCManagerGetRootCategories()
{
	WebCamsManagerImp *manager = WebCamsManagerImp::manager();
	return manager ? (WebCamsTreeRef)manager->categoriesTree() : 0;
}

WebCamsContainerRef WCManagerGetFilteredWebCams(CFStringRef terms)
{
	LString *str = LString::stringWithCFString(terms);
	WebCamsManagerImp *manager = WebCamsManagerImp::manager();
	return manager ? (WebCamsContainerRef)manager->webCamsForTerms(str) : 0;
}

WebCamsContainerRef WCManagerGetWebCamsForWBRIN(CFStringRef rin)
{
	LString *str = LString::stringWithCFString(rin);
	WebCamsManagerImp *manager = WebCamsManagerImp::manager();
	return manager ? (WebCamsContainerRef)manager->webCamsForWBRIN(str) : 0;
}

void WCContainerRetain(WebCamsContainerRef container)
{
	LRetain(CONTAINER(container));
}

void WCContainerRelease(WebCamsContainerRef container)
{
	LRelease(CONTAINER(container));
}

int WCContainerCamerasCount(WebCamsContainerRef container)
{
	return container ? CONTAINER(container)->count() : 0;
}

WebCamRef WCContainerCameraAtIndex(WebCamsContainerRef container, int index)
{
	return container ? (WebCamRef)CONTAINER(container)->webCamAtIndex(index) : 0;
}

void WCTreeRetain(WebCamsTreeRef tree)
{
	LRetain(TREE(tree));
}

void WCTreeRelease(WebCamsTreeRef tree)
{
	LRelease(TREE(tree));
}

WebCamsContainerRef WCTreeGetWebCams(WebCamsTreeRef tree)
{
	return tree ? (WebCamsContainerRef)TREE(tree)->webCams() : 0;
}

int WCTreeChildrenCount(WebCamsTreeRef tree)
{
	return tree ? TREE(tree)->childrenCount() : 0;
}

WebCamsTreeRef WCTreeChildAtIndex(WebCamsTreeRef tree, int index)
{
	return tree ? (WebCamsTreeRef)TREE(tree)->childAtIndex(index) : 0;
}

WebCamsTreeRef WCTreeSuperObject(WebCamsTreeRef tree)
{
	return tree ? TREE(tree)->_parent : 0;
}

CFStringRef WCTreeTitle(WebCamsTreeRef tree)
{
	return tree ? _cf(TREE(tree)->_title) : 0;
}

void WebCamRetain(WebCamRef cam)
{
	LRetain(CAM(cam));
}

void WebCamRelease(WebCamRef cam)
{
	LRelease(CAM(cam));
}

WebCamRef WebCamWithUID(CFStringRef uid)
{
	return (WebCamRef)WebCameraImp::webCamWithUID(LString::stringWithCFString(uid));
}

// A RIN is the camera's UID plus a multiple of a million.
WebCamRef WebCamWithRin(int rin)
{
	if (rin == 0)
		return 0;
	return WebCamWithUID(LString::stringWithInt(rin % 1000000)->cfString());
}

CFStringRef WebCamUID(WebCamRef cam)
{
	return cam ? _cf(CAM(cam)->_uid) : 0;
}

int WebCamRin(WebCamRef cam)
{
	return cam && CAM(cam)->_uid ? CAM(cam)->_uid->intValue() : 0;
}

CFStringRef WebCamTitle(WebCamRef cam)
{
	if (cam == 0)
		return 0;
	if (CAM(cam)->_name == 0)
		CAM(cam)->_validate();
	return _cf(CAM(cam)->_name);
}

int WebCamUpdateFrequency(WebCamRef cam)
{
	if (cam == 0)
		return 0;
	CAM(cam)->_validate();
	return CAM(cam)->_frameRate;
}

CFDataRef WebCamPreviewImageData(WebCamRef cam)
{
	return cam ? _cf(CAM(cam)->previewImageData()) : 0;
}

CFDataRef WebCamDescription(WebCamRef cam)
{
	return cam ? _cf(CAM(cam)->descriptionRTFData()) : 0;
}

#define WC_VALIDATED_STRING(name, field)        \
	CFStringRef name(WebCamRef cam)          \
	{                                        \
		if (cam == 0)                    \
			return 0;                \
		CAM(cam)->_validate();           \
		return _cf(CAM(cam)->field);     \
	}

WC_VALIDATED_STRING(WebCamDescriptionString, _descriptionString)
WC_VALIDATED_STRING(WebCamGetWBRINs, _wbRINs)
WC_VALIDATED_STRING(WebCamPageTitle, _pageName)
WC_VALIDATED_STRING(WebCamPageURLString, _pageURL)
WC_VALIDATED_STRING(WebCamImageURLString, _imageURL)
WC_VALIDATED_STRING(WebCamCopyrightString, _copyright)
