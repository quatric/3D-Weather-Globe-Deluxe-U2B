// The atlas database: the SQL it runs (SQL_DB_Strings), the binary objects
// stored in it (BLOBWrapper and subclasses), the database itself
// (AtlasDatabase) and the user-editable notes of entries (EntryInfo).

#ifndef AMDATABASE_H
#define AMDATABASE_H

#include "AMBase.h"
#include "SQLiteLib.h"
#include "FHModels.h"
#include "FHRender.h"

class CHolder;

// ---------------------------------------------------------------------------
// SQL_DB_Strings

class SQL_DB_Strings {
public:
	static LString *selectGeoObjectWithUID(LString *uid);
	static LString *selectGeoObjectWithBondUID(unsigned char user);
	static LString *selectAbbreviations();
	static LString *selectTopCitiesForWeatherOnMap();
	static LString *selectAllCoordsUIDsKindsForPoints();
	// A point entry kind from the four-character type and alt type.
	static int pointEntryTypeFromTypeAndAltType(const char *type, const char *altType);
	static LString *selectNameForGeoObjectWithUID(LString *uid);
	static LString *selectCitiesUIDsWithCityName(LString *name);
	static LString *selectFilenameFromUpdatesThatIsLike(LString *pattern);
	static LString *insertFilenameIntoUpdates(LString *name);
	static LString *selectAllGeoObjUIDsWithBLOBOfType(LString *repType);
	static LString *selectBLOBUIDForEntryInfoKeysOrder();
	static LString *selectBLOBUIDForGrid(LString *repType);
	static LString *selectBLOBKindWithUID(LString *uid);
	static LString *selectBLOBDataWithUID(LString *uid);
	static LString *selectBLOBUIDWithRepresentationOfType(LString *repType, LString *objectUID);
	static LString *selectAllObjectUIDsWithGeotypes(LArray *types, LString *repType);
	static LString *selectSolarEclipses();
	static LString *stars_selectConstellationsStars();
	static LString *stars_selectConstellationsInds();
	static LString *stars_selectVSOPBlock(int planet, int param, int power);
	static LString *stars_selectSStarInfo();
	static LString *selectEntryTypeRemap(LString *useCase);
	static LString *selectEntryTypeRemapMaxCount();
	static LString *selectEntryTypeRemapMaxCount_SAFE();
	static LString *selectUserDBRevision();
	static LString *selectRouteNodesWithParentUID(LString *parentUID, unsigned char user);
	static LString *deleteRoutesForRouteNodesWithUIDs(LArray *uids);
	static LString *restoreRoutesForRouteNodesWithUIDs(LArray *uids);
	static LString *deleteKeywordsForObject(LString *uid);
	static LString *deleteEntryInfoForObjectWithUID(LString *uid);
	static LString *selectAllUserGeoObjects();
	static LString *selectAllUserKeywords();
	static LString *selectAllUserRouteNodes();
	static LString *selectAllUserBLOBs();
	static LString *selectAllUserGeoData();
	static LString *userDBDump(LArray *requests);
	static LString *selectUIDKeyValKindForParent(LString *parentUID, LString *table, LString *parentColumn,
	                                             LString *uidColumn, LString *keyColumn, LString *valueColumn,
	                                             LString *kindColumn, LString *orderColumn);
	static LString *selectAllObjectUIDsWithImages();
	static LString *selectObjectUIDsWithUserImages();
	static LString *transactionForReuests(LArray *requests);
	static LString *insertKeywordsForObject(LArray *keywords, LString *uid);
	static LString *restoreGeoObjectsWithUIDs(LArray *uids);
	static LString *deleteGeoObjectsWithUIDs(LArray *uids);
	static LString *setParentForRouteNodesWithUIDs(LString *parentUID, LArray *uids);
	static LString *restoreRouteNodesWithUIDs(LArray *uids);
	static LString *deleteRouteNodesWithUIDs(LArray *uids);
	// `geotypes` holds arrays of a type and optionally an alt type.
	static LString *conditionSubstringForGeotypes(LArray *geotypes);
	static bool fastLookupIsPossibleForTerms(LArray *terms, unsigned char checkHosts);
	static LString *selectDistinctNameOfRegionsBySubstring(LString *substring);
	static LArray *separatedTermsFromSearchString(LString *string);
	static bool fastLookupIsPossibleForSearchWithSubstring(LString *substring);
	static LString *insertKeywords(SQLiteResultsContainer *results);
	static LString *selectUIDStatementForTerms(LArray *terms, unsigned char user);
	static LString *selectColumnsForTerms(LString *columns, LArray *terms, unsigned char user);
	static LString *setNameForRouteNodeWithUID(LString *name, LString *uid);
	static LString *setParentForRouteNodeWithUID(LString *parentUID, LString *uid);
	static LString *deleteGeoDataWithUID(LString *uid);
	static LString *restoreGeoDataWithUID(LString *uid);
	static LString *insertGeoData(SQLiteResultsContainer *results);
	static LString *insertBLOBs(SQLiteResultsContainer *results);
	static LString *insertRouteNodes(SQLiteResultsContainer *results);
	static LString *insertGeoObjects(SQLiteResultsContainer *results);
	static LString *insertOrReplaceEntryInfoElementWithDBMap(LMap *map);
	static LString *insertOrReplaceGeoDataWithDBMap(LMap *map);
	static LString *insertOrReplaceBlobObjectWithDBMap(LMap *map);
	static LString *insertOrReplaceRouteNodeWithDBMap(LMap *map);
	// The filter has a bit per geotype; all ones does not filter.
	static LString *autocompleteRequestForTerm(LArray *terms, unsigned long long filter);
	static LArray *createUserDBTables(unsigned char temporary);
	static LArray *cleanUserDB();
	static LString *selectAllCountriesNames();
	static LString *insertOrReplaceGeoObjectWithDBMap(LMap *map);
	static LString *commitBLOB(BLOBWrapper *blob, const LString *repType, const LString *objectUID);
	static LString *searchForGeoObjectWithSubstring(LString *substring, unsigned long long filter);
	static LString *searchForGeoObjectByName(LString *name, unsigned long long filter);
	static LString *selectColumnsByName(LString *name, LString *condition, unsigned char *hostsFound,
	                                    unsigned char forGenericSearch);
	static LString *requestForGenericSearchWithNameParams(LArray *geotypes, LString *name);
	static LString *selectColumnsByRegion(LString *region, LString *condition, unsigned char forGenericSearch);
	static LString *requestForGenericSearchInRegionsParams(LArray *geotypes, LString *region);
};

// ---------------------------------------------------------------------------
// BLOBs

class BLOBWrapper : public LObject {
	LF_DECLARE_CLASS(BLOBWrapper)
public:
	BLOBWrapper();
	virtual void dealloc();
	virtual BLOBWrapper *getOwnerLObject() { return this; }
	virtual LData *blobData();
	// A new UID is made when `uid` is 0.
	virtual BLOBWrapper *initWithData(LData *data, LString *kind, LString *uid);

	LString *uid() const { return _uid; }
	LString *kind() const { return _kind; }

	static BLOBWrapper *wrapperWithBLOBUid(LString *uid);
	static BLOBWrapper *wrapperForObjectWithUID(LString *objectUID, LString *repType);
	static LClass *_classWrapperForKind(LString *kind);

	static LMap *__kindToClassRemap;

protected:
	void *_mutableBlobBytes();

	LData *_data;
	LString *_uid;
	LString *_kind;
};

class ImageBLOB : public BLOBWrapper {
	LF_DECLARE_CLASS(ImageBLOB)
public:
	virtual ImageBLOB *getOwnerLObject() { return this; }
};

class IndexBuffersBLOB : public BLOBWrapper {
	LF_DECLARE_CLASS(IndexBuffersBLOB)
public:
	virtual void dealloc();
	virtual IndexBuffersBLOB *getOwnerLObject() { return this; }
};

class PlistBLOB : public BLOBWrapper {
	LF_DECLARE_CLASS(PlistBLOB)
public:
	PlistBLOB();
	virtual void dealloc();
	virtual PlistBLOB *getOwnerLObject() { return this; }
	virtual LData *blobData();
	virtual PlistBLOB *initWithParams(LString *uid, LString *name, LObject *object);

	LObject *object();
	LString *name();

private:
	LMap *_dictionary();

	LMap *_dict;
};

class TextBLOB : public BLOBWrapper {
	LF_DECLARE_CLASS(TextBLOB)
public:
	TextBLOB();
	virtual void dealloc();
	virtual TextBLOB *getOwnerLObject() { return this; }
	virtual LData *blobData();
	virtual TextBLOB *initWithParams(LString *uid, LString *text);

	LString *text();

private:
	LString *_text;
};

// Quad trees by 10-character object UID.
class _QuadContainerStrMap : public TStrMap<QuadTreeContainer *, 10> {
};

class LookupGridBLOB : public BLOBWrapper {
	LF_DECLARE_CLASS(LookupGridBLOB)
public:
	LookupGridBLOB();
	virtual void dealloc();
	virtual LookupGridBLOB *getOwnerLObject() { return this; }

	QuadTreeContainer *treeForObjectWithUID(LString *uid, unsigned char east);
	LArray *objectUIDsBelowGeoPoint(LPoint geoPoint);

private:
	CHashTable *_lookupDict();

	CHashTable *_lookup;
	_QuadContainerStrMap *_eastTrees;
	_QuadContainerStrMap *_westTrees;
	LArray *_trees;
	float _cellWidth;
	float _cellHeight;
	short _gridWidth;
	short _gridHeight;
};

class VertexBuffersBLOB : public BLOBWrapper {
	LF_DECLARE_CLASS(VertexBuffersBLOB)
public:
	class VBDataSource : public LObject, public IVertexBufferDataSource {
		LF_DECLARE_CLASS(VBDataSource)
	public:
		VBDataSource();
		virtual unsigned int vertCountForBuffer(CVertexBuffer *vb);
		virtual CShader *renderShaderForBuffer(CVertexBuffer *vb);
		virtual unsigned int streamNumberForBuffer(CVertexBuffer *vb);
		virtual void fillBuffer(CVertexBuffer *vb);

		static VBDataSource *dataSourceWithBLOB(VertexBuffersBLOB *blob, int chunk, int lod,
		                                        CRenderProfile *profile);

		VertexBuffersBLOB *_blob;
		CRenderProfile *_profile;
		int _chunk;
		int _lod;
	};

	VertexBuffersBLOB();
	virtual void dealloc();
	virtual VertexBuffersBLOB *getOwnerLObject() { return this; }

	LPoint *pointsForBufferAtIndex(int chunk, int lod);
	CVertexBuffer *deviceBufferAtIndex(int chunk, int lod, CProjection *projection, CRenderProfile *profile);

	static VertexBuffersBLOB *verticesWithUID(LString *uid);
	static void invalidateCaches();

	static LMap *_loadedVBs;

private:
	void _releaseDeviceBuffers();

	bool _parsed;
	// Each points at a level's point count, which its points follow.
	uint32_t *_points[100][6];
	CHashTable *_deviceBuffers;
	int _lastLod;
	CProjection *_projection;
};

// ---------------------------------------------------------------------------
// AtlasDatabase

enum TEntryTypeUseCase { kEntryTypeUseCaseSearchPreview = 0, kEntryTypeUseCaseSortByKind = 1 };

// A point object of the atlas, as the renderers draw it.
struct SAtlasPoint {
	LPoint geoPoint;
	unsigned char kind;
};

enum { kAtlasPointUIDSize = 22 };

// By SAtlasPoint::kind: the points' colors (Colors as unsigned int) and their
// icons' keys (0 for none).
extern unsigned int sPointColors[16];
extern LString *sPointIconKeys[16];

struct SStarInfo {
	int hip;
	float ra;
	float dec;
	float magnitude;
	float parallax;
	float bv;
};

// The names of an entry type and the type/alt type pairs it stands for.
class _LEntryTypeRemapContainer : public LObject {
	LF_DECLARE_CLASS(_LEntryTypeRemapContainer)
public:
	_LEntryTypeRemapContainer();
	virtual void dealloc();
	virtual _LEntryTypeRemapContainer *getOwnerLObject() { return this; }

	LString *_singular;
	LString *_plural;
	int _weight;
	LArray *_typePairs;
};

class AtlasDatabase : public LObject {
	LF_DECLARE_CLASS(AtlasDatabase)
public:
	// A range of allStarsArray() records.
	struct SStarsPathInfo {
		int start;
		int count;
	};
	enum { kStarsPatchesCount = 170 };
	// A patch of the star sphere: column and row in a 2^level grid.
	struct SStarsPathID {
		// Its starsPatches() index: level 3's 128 patches first, then level 2's
		// 32, level 1's 8 and level 0's 2; x and y bits interleaved.
		int patchInfoOffset() const
		{
			int ux = x, uy = y;
			switch (level) {
			case 0:
				return ux + 0xa8;
			case 1:
				return (ux & 1) + (uy & 1) * 2 + (ux & 2) * 2 + 0xa0;
			case 2:
				return (ux & 1) + (uy & 1) * 2 + (ux & 2) * 2 + (uy & 2) * 4 + (ux & 4) * 4 + (uy & 4) * 8 + 0x80;
			case 3:
				return (ux & 1) + (uy & 1) * 2 + (ux & 2) * 2 + (uy & 2) * 4 + (ux & 4) * 4 + (uy & 4) * 8 +
				       (ux & 8) * 8 + (uy & 8) * 0x10;
			default:
				return 0;
			}
		}

		char x;
		char y;
		char level;
		char pad;	// the original's records are 4 bytes
	};

	AtlasDatabase();
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual AtlasDatabase *getOwnerLObject() { return this; }
	virtual intptr_t _rescanAndApplyUpdates(char *args);
	virtual intptr_t _rescanAndApplyUpdatesCompleted(char *args);

	static bool sDoCleanupAtStartup;
	static LString *kMainDatabaseWillStartUpdate;
	static LString *kMainDatabaseChangedNotification;
	static LString *kMainDatabaseDidEndUpdate;

	// Folders and files
	static LString *_dbFilePathBundled();
	static LString *_starsDBFilePathBundled();
	static LString *_dbFilePathCached();
	static LString *_starsDBFilePathCached();
	static LString *_allStarsCachePath();
	static LString *_allStarPatchInfosCachePath();
	static LString *_constellationsCachePath();
	static void _cleanupStarsCaches();
	LString *_tempFolderPath(unsigned char create);
	void _cleanupTempFolder();
	LString *_libraryRootFolderPath();
	LString *_mediaDataFolderPath(unsigned char create);
	void _cleanupMediaFolder();
	LString *_userDBFilePath();
	// New strings.
	LString *pathForMediaDataFolder(LString *name, unsigned char temporary);
	LString *_mediaCachesFolderPath(unsigned char shared, unsigned char create);
	void _cleanupMediaCaches(unsigned char shared);
	LString *_pathForMediaCache(LString *name, LString *folderName, unsigned char temporary, unsigned char shared);
	LString *pathForMediaThumbnail(LString *name, LString *folderName, unsigned char temporary, unsigned char shared);
	LString *pathForMediaTexture(LString *name, LString *folderName, unsigned char temporary, unsigned char shared);

	// Caches
	void _invalidateAllPointsCache();
	void _invalidateCachedStatements();
	void _invalidateUserDBDependedCaches();
	void _synchronize();
	void _invalidateDBCaches();
	void cacheTemporaryEntry(GenericEntry *entry);
	GenericEntry *temporaryEntryWithUID(LString *uid);
	void _cacheEntry(GenericEntry *entry);
	void _invalidateDBCachesWithEntries(LArray *entries, unsigned char restored);
	void _cacheEntryIfNeeded(GenericEntry *entry);

	// The database
	void _initializeSQLDatabase();
	void _cleanupUserDB(SQLiteDatabase *userDB);
	static LArray *_allUpdateFiles();
	void rescanForUpdates(unsigned char async, unsigned char notify);
	SQLiteDatabase *sqlDB();
	SQLiteResultsContainer *searchWithRequest(const LString *request, LMap *parameters);
	SQLiteResultsContainer *searchWithRequest(const LString *request, TEntriesSortingMode mode);
	LArray *simpleSearchWithRequest(const LString *request);
	void sortSearchResults(SQLiteResultsContainer *results, TEntriesSortingMode mode);
	bool exportUserDBToFile(LString *path);
	bool importUserDBFromFile(LString *path);
	void restoreEntries(LArray *entries);
	void deleteEntries(LArray *entries);
	bool commitRouteNode(DBUserRouteNode *node);
	bool commitEntry(GenericEntry *entry);

	// Entries
	bool _blobExistsForEntryWithUID(LString *uid, LString *repType);
	bool shapeExistsForEntryWithUID(LString *uid);
	bool previewIconExistsForEntryWithUID(LString *uid);
	bool customIconExistsForEntryWithUID(LString *uid);
	LArray *allEntryUIDsWithImages();
	// Retained.
	GenericEntry *_createSQL_DB_EntryWithUID(LString *uid);
	GenericEntry *dbEntryWithUID(LString *uid);
	LArray *allRoutesHierarchy();
	LMap *entryInfoKeysOrder(GenericEntry *entry);
	static LString *generateUID();

	// Points
	LData *allPointsArray();
	LData *allOtherPointsIndices();
	LData *allAppleStoresIndices();
	LData *allCapitalsIndices();
	LData *allPointUIDsArray();
	unsigned int *indicesSortedByY();
	PointEntry *entryNearestToGeoPoint(LPoint geoPoint);
	void _createHostObjectsRemapIfNeeded();
	LArray *hostUIDsFromTerms(LArray *terms);

	// Entry types
	static LString *_stringFromEntryUseCase(TEntryTypeUseCase useCase);
	int entryWeight(int typeID);
	int entryTypeIDFromStrings(TEntryTypeUseCase useCase, LString *type, LString *altType);
	LString *localizedEntryType(int typeID, unsigned char plural);
	LString *localizedEntryType(TEntryTypeUseCase useCase, LString *type, LString *altType, unsigned char plural);
	LArray *typePairsForEntryType(int typeID);
	static int _sortByNameCallback(int count, const int *types1, const int *types2, const char *values1,
	                               const char *values2, const void *context);

	// Solar eclipses
	void _loadSolarEclipses();
	const SSolarEclipse *solarEclipseAtIndex(unsigned int index);
	unsigned int solarEclipsesCount();
	// ~0 when there are none.
	unsigned int indexOfNearestSolarEclipseForDate(LDate *date);

	// Stars
	LData *allStarsArray();
	TArray<SStarsPathInfo> *starsPatches();
	LData *constellationsStarsArray();
	LData *constellationsIndsArray();
	LMap *constellations();

	CSharableResource _lock;
	SQLiteDatabase *_db;
	LArray *_routesHierarchy;
	LMap *_blobOwnersByRepType;
	LMap *_entryTypeIDs[2];
	LArray *_entryTypeRemaps;
	int _entryTypesPerUseCase;
	LMap *_entryInfoKeysOrders;
	LData *_solarEclipses;
	LString *_tempFolder;
	LObject *_unused74;
	LObject *_unused78;
	LString *_sharedMediaCachesFolder;
	LString *_mediaCachesFolder;
	LString *_libraryRootFolder;
	LString *_mediaDataFolder;
	LData *_allPoints;
	LData *_allPointUIDs;
	LData *_indicesSortedByY;
	LData *_capitalsIndices;
	LData *_appleStoresIndices;
	LData *_otherPointsIndices;
	LData *_allStars;
	TArray<SStarsPathInfo> *_starsPatches;
	LData *_constellationsData;
	LData *_constellationsStars;
	LData *_constellationsInds;
	LMap *_constellations;
	LMap *_cachedEntries;
	LMap *_temporaryEntries;
	LMap *_hostObjectsRemap;
	SQLiteStatement *_atlasEntryStatement;
	SQLiteStatement *_userEntryStatement;
	CSharableResource _statementsLock;
	bool _updating;
	bool _notifyUpdate;
	bool _userDBUnavailable;
};

// ---------------------------------------------------------------------------
// DBTreeElement: keyed values stored as rows of a table, loaded on first use.

class DBTreeRootElement;

class DBTreeElement : public LObject {
	LF_DECLARE_CLASS(DBTreeElement)
public:
	DBTreeElement();
	virtual void dealloc();
	virtual DBTreeElement *getOwnerLObject() { return this; }

	DBTreeElement *initWithUIDParent(LString *uid, DBTreeElement *parent);
	LString *uid() const { return _uid; }
	LSet *allKeys();
	LArray *valuesForKey(LString *key);
	LObject *firstValueForKey(LString *key);
	void removeValuesForKey(LString *key);
	void removeValueForKeyAtIndex(LString *key, int index);
	void replaceValueForKeyAtIndex(LObject *value, LString *key, int index);
	void insertValueForKeyAtIndex(LObject *value, LString *key, int index);
	void addValueForKey(LObject *value, LString *key);
	DBTreeElement *createSubtreeForKeyAtIndex(LString *key, int index);
	// 0 on failure.
	LString *commitRequest();

protected:
	DBTreeRootElement *_rootElement();
	void _loadKeysIfNeeded();
	LArray *_valuesArrayForKey(LString *key, unsigned char create);

	LString *_uid;
	DBTreeElement *_parent;
	DBTreeRootElement *_root;
	LMap *_values;
	LSet *_keys;
	bool _loaded;
};

class DBTreeRootElement : public DBTreeElement {
	LF_DECLARE_ABSTRACT_CLASS(DBTreeRootElement)
public:
	DBTreeRootElement();
	virtual void dealloc();
	virtual bool commitChanges();
	virtual bool isDirty();
	virtual LString *tableName() = 0;
	virtual LString *parentUIDColumnName() = 0;
	virtual LString *uidColumnName() = 0;
	virtual LString *keyColumnName() = 0;
	virtual LString *valColumnName() = 0;
	virtual LString *valTypeColumnName() = 0;
	virtual LString *sortIdxColumnName() = 0;

	bool _dirty;
};

class InfoTreeRootElement : public DBTreeRootElement {
	LF_DECLARE_CLASS(InfoTreeRootElement)
public:
	InfoTreeRootElement();
	virtual InfoTreeRootElement *getOwnerLObject() { return this; }
	virtual bool commitChanges();
	virtual LString *tableName();
	virtual LString *parentUIDColumnName();
	virtual LString *uidColumnName();
	virtual LString *keyColumnName();
	virtual LString *valColumnName();
	virtual LString *valTypeColumnName();
	virtual LString *sortIdxColumnName();

	InfoTreeRootElement *initWithGeoObjectUID(LString *uid, unsigned char user);

	bool _user;
};

// ---------------------------------------------------------------------------
// EntryInfo

class EntryInfoImageWrapper : public LObject {
	LF_DECLARE_CLASS(EntryInfoImageWrapper)
public:
	static LString *kImageUIDKey;
	static LString *kAspectRatioKey;
	static LString *kDescriptionKey;
	static LString *kImageDefaultName;
	static LSize kTextureSize;
	static LSize kThumbnailSize;
	static LStrMap *__imageWrappersCache;

	EntryInfoImageWrapper();
	virtual void dealloc();
	virtual EntryInfoImageWrapper *getOwnerLObject() { return this; }

	static LString *imageKey(LString *hostUID, LString *imageUID);
	static void invalidateAllCachedPaths();
	static EntryInfoImageWrapper *wrapperWithParams(LString *hostUID, LString *imageUID, LString *description,
	                                                float aspectRatio, unsigned char atlas, unsigned char create);
	static EntryInfoImageWrapper *wrapperWithImageData(LData *data, LString *hostUID, LString *name);

	EntryInfoImageWrapper *initWithParams(LString *hostUID, LString *imageUID, LString *description,
	                                      float aspectRatio, unsigned char atlas);
	EntryInfoImageWrapper *initWithImageData(LData *data, LString *hostUID, LString *name);
	void invalidateCachedPaths();
	LString *imageUID() const { return _imageUID; }
	LString *hostUID() const { return _hostUID; }
	LString *textDescription() const { return _description; }
	void setTextDescription(LString *description);
	bool isAtlasImage() const { return _atlas; }
	LString *originalDataPath();
	LData *originalData();
	LBitmapImage *originalImage();
	float aspectRatio();
	LMap *mapRepresentation();
	LString *thumbnailPath();
	LString *texturePath();
	// Moves the files into the temporary folder, or out of it.
	bool commit(unsigned char temporary);

private:
	friend class EntryInfo;
	LString *_imageUIDFromImageName(LString *name);

	LString *_imageUID;
	LString *_hostUID;
	float _aspectRatio;
	LString *_description;
	LData *_originalData;
	LBitmapImage *_originalImage;
	LString *_originalDataPath;
	LString *_texturePath;
	LString *_thumbnailPath;
	bool _changed;
	bool _atlas;
	bool _temporary;
	unsigned int _nameCounter;
};

class EntryInfo : public LObject {
	LF_DECLARE_CLASS(EntryInfo)
public:
	// Which tree's items.
	enum TKeySet { kKeySetAtlas = 0, kKeySetAll = 1, kKeySetUser = 2 };

	static LString *kChangedKey;
	static LString *kChangedIndex;
	static LString *kChangedKeySet;
	static LString *kOperationKind;
	static LString *kOperationKindInsert;
	static LString *kOperationKindRemove;
	static LString *kOperationKindReplace;
	static LString *kDidChangeItemNotification;

	EntryInfo();
	virtual void dealloc();
	virtual EntryInfo *getOwnerLObject() { return this; }

	EntryInfo *initWithHostEntry(GenericEntry *host);
	bool shouldCommit() const;
	bool commit();
	LSet *allKeys(TEntryKeySet set, TEntryKeyAccessMode mode);
	int itemsCountForKey(LString *key, TEntryKeySet set, TEntryKeyAccessMode mode);
	LObject *itemForKeyAtIndex(LString *key, int index, TEntryKeySet set, TEntryKeyAccessMode mode);
	bool canAddItemsForKey(LString *key, TEntryKeySet set);
	bool canRemoveItemForKeyAtIndex(LString *key, int index, TEntryKeySet set);
	bool removeItemForKeyAtIndex(LString *key, int index, TEntryKeySet set);
	bool replaceItemForKeyAtIndex(LObject *value, LString *key, int index, TEntryKeySet set);
	bool insertItemForKeyAtIndex(LObject *value, LString *key, int index, TEntryKeySet set);
	bool addItemForKey(LObject *value, LString *key, TEntryKeySet set);
	LValue *density();

private:
	EntryInfoImageWrapper *imageWrapperForDBTree(DBTreeElement *tree, unsigned char atlas, unsigned char create);
	int propertyKeysCountForKey(LString *key, TEntryKeySet set, TEntryKeyAccessMode mode) const;
	void initMaps();
	LSet *imageWrappers();
	int mapItemsCountForKey(LString *key, TKeySet set);
	LObject *mapItemForKeyAtIndex(LString *key, int index, TKeySet set);
	bool commitImageWrappers();
	void _postChange(LString *key, int index, TEntryKeySet set, LString *operation);
	void _setUserItemAtIndex(LObject *value, LString *key, int index, bool replace);

	GenericEntry *_host;
	bool _imagesChanged;
	InfoTreeRootElement *_atlasTree;
	InfoTreeRootElement *_userTree;
	bool _mapsInited;
	LValue *_density;
	bool _densityComputed;
	LSet *_imageWrappers;
};

#endif
