// Atlas entries: the cities, features, shapes, user locations and routes the
// database holds, the interfaces the application reads them through and the
// proxies that present several entries as one.

#ifndef AMENTRIES_H
#define AMENTRIES_H

#include "AMBase.h"
#include "LContainers.h"

// Which property keys an entry lists, and whether for reading or editing.
enum TEntryKeySet { kEntryKeySetShort = 0, kEntryKeySetFull = 1, kEntryKeySetWithWeather = 2 };
enum TEntryKeyAccessMode { kEntryKeyRead = 0, kEntryKeyWrite = 1 };

// The weather reports an online city keeps; scheduling takes a mask of
// (1 << type).
enum TReportType {
	kReportTypeNone = 0,
	kReportTypeCurrent = 1,
	kReportTypeForecast = 2,
	kReportTypeExpanded = 3,
	kReportTypeAstronomy = 4,
	kReportTypeSevereWarnings = 5,
};
enum { kAllReportsMask = 0x1f };

enum TEntriesSortingMode {
	kEntriesSortByName = 0,
	kEntriesSortByNameDescending,
	kEntriesSortByRegion,
	kEntriesSortByRegionDescending,
	kEntriesSortByKind,
	kEntriesSortByKindDescending,
	kEntriesSortByPopulation,
	kEntriesSortByPopulationDescending
};

// ---------------------------------------------------------------------------
// Interfaces

class IEntryBaseInterface {
public:
	virtual ~IEntryBaseInterface() {}
	virtual LObject *getOwnerLObject() = 0;
	virtual void interfaceRetain() = 0;
	virtual void interfaceRelease() = 0;
};

// Every getter takes an optional flag, set when the value differs between the
// entries a proxy stands for.
class IGenericEntryInterface : public virtual IEntryBaseInterface {
public:
	virtual LString *uid(unsigned char *mixed) const = 0;
	virtual LString *ownName(unsigned char *mixed) const = 0;
	virtual LString *shortCaption(unsigned char *mixed) const = 0;
	virtual LString *ownType(unsigned char *mixed) const = 0;
	virtual LString *altType(unsigned char *mixed) const = 0;
	virtual LString *kind(unsigned char *mixed) const = 0;
	virtual LString *timeZoneName(unsigned char *mixed) const = 0;
	virtual LArray *alternativeNames(unsigned char *mixed) const = 0;
	virtual LMap *userInfo(unsigned char *mixed) const = 0;
	virtual LString *countyName(unsigned char *mixed) const = 0;
	virtual LString *stateName(unsigned char *mixed) const = 0;
	virtual LString *countryName(unsigned char *mixed) const = 0;
	virtual LString *regionName(unsigned char *mixed) const = 0;
	virtual int population(unsigned char *mixed) const = 0;
};

class IPointEntryInterface : public virtual IEntryBaseInterface {
public:
	virtual LPoint geoPoint(unsigned char *mixed) const = 0;
	virtual int customWeatherID(unsigned char *mixed) const = 0;
	virtual LTimeZone *timeZone(unsigned char *mixed) const = 0;
	virtual int currentGMTOffset(unsigned char *mixed) const = 0;
};

class ICityEntryInterface : public virtual IEntryBaseInterface {};
class IFeatureEntryInterface : public virtual IEntryBaseInterface {};
class IShapeEntryInterface : public virtual IEntryBaseInterface {};
class IAppleStoreEntryInterface : public virtual IEntryBaseInterface {};
class IUserCustomEntryInterface : public virtual IEntryBaseInterface {};

class IRouteObjectEntryInterface : public virtual IEntryBaseInterface {
public:
	virtual LString *blobUID(unsigned char *mixed) const = 0;
};

// ---------------------------------------------------------------------------
// Route set items: an entry or a break between route legs

class RouteSetBreak;

class RouteSetItem : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(RouteSetItem)
public:
	virtual GenericEntry *genericEntryInstance() = 0;
	virtual RouteSetBreak *routeBreakInstance() = 0;
};

class RouteSetBreak : public RouteSetItem {
	LF_DECLARE_CLASS(RouteSetBreak)
public:
	static RouteSetBreak *routeSetBreak();
	virtual RouteSetBreak *init();
	virtual GenericEntry *genericEntryInstance() { return 0; }
	virtual RouteSetBreak *routeBreakInstance() { return this; }
	virtual RouteSetBreak *getOwnerLObject() { return this; }
};

// ---------------------------------------------------------------------------
// Entries

class GenericEntry : public RouteSetItem, public virtual IGenericEntryInterface {
	LF_DECLARE_CLASS(GenericEntry)
public:
	static LString *kGenericEntryUID;
	static LString *kEntryDidChangeNotification;
	static LMap *__iconCacheMap;
	static LMap *__smallIconCacheMap;

	GenericEntry();

	// The class an entry row of the database is loaded as.
	static LClass *entryClassFromDBMap(LMap *dbMap);
	static GenericEntry *entryWithDBMap(LMap *dbMap);
	static GenericEntry *entryWithUID(LString *uid);
	static GenericEntry *entryWithDictionaryRep(LMap *rep);

	virtual GenericEntry *init();
	virtual void dealloc();
	virtual GenericEntry *genericEntryInstance() { return this; }
	virtual RouteSetBreak *routeBreakInstance() { return 0; }
	virtual GenericEntry *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual bool isEntryTemporary() const;
	virtual LImage *icon(unsigned char *custom) const;
	virtual LImage *previewIcon(unsigned char *custom) const;
	// What identifies the entry in a saved list.
	virtual LMap *entryDictionary() const;
	virtual LObject *wrapperWithRepresentationOfType(LString *repType, unsigned char load) const;
	virtual EntryInfo *entryInfoObject();
	virtual LString *uid(unsigned char *mixed) const;
	virtual LString *ownName(unsigned char *mixed) const;
	virtual LString *shortCaption(unsigned char *mixed) const;
	virtual LString *ownType(unsigned char *mixed) const;
	virtual LString *altType(unsigned char *mixed) const;
	virtual LString *kind(unsigned char *mixed) const;
	virtual LString *timeZoneName(unsigned char *mixed) const;
	virtual LArray *alternativeNames(unsigned char *mixed) const;
	virtual LMap *userInfo(unsigned char *mixed) const;
	virtual LString *countyName(unsigned char *mixed) const;
	virtual LString *stateName(unsigned char *mixed) const;
	virtual LString *countryName(unsigned char *mixed) const;
	virtual LString *regionName(unsigned char *mixed) const;
	virtual LString *fullRegionName(unsigned char *mixed) const;
	virtual int population(unsigned char *mixed) const;
	virtual LString *countyUID() const;
	virtual LString *stateUID() const;
	virtual LString *countryUID() const;
	virtual float visualWeight() const;
	virtual bool getEntryGeoRect(LRect *rect);
	virtual bool commit();
	virtual GenericEntry *initWithDBMap(LMap *dbMap);
	virtual GenericEntry *initWithNameAndType(LString *name, LString *type);
	virtual GenericEntry *initWithDictionaryRepAndType(LMap *rep, LString *type);
	virtual void loadValuesFromDictionaryRep(LMap *rep);
	virtual bool _commit();
	virtual LMap *dbMap() const;
	virtual LString *iconKey() const;
	virtual bool shouldCommit() const;
	virtual LSet *propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const;
	virtual LObject *propertyForKey(const LString *key);
	virtual void setPropertyForKey(LObject *value, const LString *key);
	virtual void _loadUserInfo() const;

	void releaseNormalIcon() const;
	void releasePreviewIcon() const;

protected:
	LImage *_iconForKey(LString *key, unsigned char small) const;

	mutable LMap *_wrappers;
	LString *_uid;
	LString *_ownName;
	LString *_ownType;
	LString *_altNamesString;
	LString *_timeZoneName;
	LString *_altType;
	LString *_countryUID;
	LString *_stateUID;
	LString *_countyUID;
	int _population;
	mutable LString *_countryName;
	mutable LString *_stateName;
	mutable LString *_countyName;
	mutable LString *_regionName;
	mutable LString *_kind;
	mutable LMap *_userInfo;
	mutable bool _userInfoChanged;
	mutable LString *_shortCaption;
	mutable LArray *_alternativeNames;
	EntryInfo *_entryInfo;
	mutable LImage *_icon;
	mutable LImage *_previewIcon;
};

class PointEntry : public GenericEntry, public virtual IPointEntryInterface {
	LF_DECLARE_CLASS(PointEntry)
public:
	static LString *kPointEntryLat;
	static LString *kPointEntryLong;
	static LString *kPointEntryWasExpiredViaOnlineNotification;
	static LString *kPointEntryWasUpdatedViaOnlineNotification;

	PointEntry();
	virtual void dealloc();
	virtual PointEntry *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual LMap *entryDictionary() const;
	virtual float visualWeight() const;
	virtual bool getEntryGeoRect(LRect *rect);
	virtual PointEntry *initWithDBMap(LMap *dbMap);
	virtual void loadValuesFromDictionaryRep(LMap *rep);
	virtual LMap *dbMap() const;
	virtual LSet *propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const;
	virtual LObject *propertyForKey(const LString *key);
	virtual LPoint geoPoint(unsigned char *mixed) const;
	virtual int customWeatherID(unsigned char *mixed) const;
	virtual LTimeZone *timeZone(unsigned char *mixed) const;
	virtual int currentGMTOffset(unsigned char *mixed) const;
	virtual float distanceToEntry(const PointEntry *entry) const;
	virtual PointEntry *initWithPointTypeName(LPoint geoPoint, LString *type, LString *name);
	virtual void _setGeoPoint(LPoint geoPoint);
	// The online weather data tells the entry when it was refreshed or expired.
	virtual void _wrapperDataWasUpdated(LMap *info);
	virtual void _wrapperDataWasExpired(LMap *info);

	const Vector3 *earthVec() const { return &_earthVec; }
	CWOnlineCityData *onlineWrapper();
	void scheduleReports(unsigned int mask);
	void unscheduleReports(unsigned int mask);
	void unscheduleAllReports();
	void manualUpdateOfScheduledReports();
	void singleUpdateTaskForScheduledReports();
	void singleUpdateTaskForReports(unsigned int mask);
	bool downloadingReport(TReportType type);
	LMap *dictionaryForReport(TReportType type);

protected:
	CWOnlineCityData *_createWrapperIfNeeded();

	LPoint _geoPoint;
	Vector3 _earthVec;
	CWOnlineCityData *_onlineData;
	int _unused;
	int _customWeatherID;
	mutable LTimeZone *_timeZone;
};

class CityPointEntry : public PointEntry, public virtual ICityEntryInterface {
	LF_DECLARE_CLASS(CityPointEntry)
public:
	virtual void dealloc();
	virtual CityPointEntry *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual float visualWeight() const;
	virtual LString *iconKey() const;
};

class FeatureEntry : public PointEntry, public virtual IFeatureEntryInterface {
	LF_DECLARE_CLASS(FeatureEntry)
public:
	FeatureEntry();
	virtual void dealloc();
	virtual FeatureEntry *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual LString *iconKey() const;

protected:
	LRect _featureRect;	// never set; LZeroRect
};

class ShapeEntry : public GenericEntry, public virtual IShapeEntryInterface {
	LF_DECLARE_CLASS(ShapeEntry)
public:
	ShapeEntry();
	virtual void dealloc();
	virtual ShapeEntry *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual float visualWeight() const;
	virtual bool getEntryGeoRect(LRect *rect);

	// Which shape set (countries, states, counties, time zones) holds the
	// entry's outline.
	LString *shapeID() const;
	CShapeContainer *shapeContainer() const;

protected:
	mutable LString *_shapeID;
};

class AppleStoreEntry : public PointEntry, public virtual IAppleStoreEntryInterface {
	LF_DECLARE_CLASS(AppleStoreEntry)
public:
	virtual AppleStoreEntry *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual LString *iconKey() const;
};

// A location the user made: it can be renamed, moved and placed in a region.
class UserCustomEntry : public PointEntry, public virtual IUserCustomEntryInterface {
	LF_DECLARE_CLASS(UserCustomEntry)
public:
	UserCustomEntry();
	static UserCustomEntry *entryWithGeoPointAndName(LPoint geoPoint, LString *name);

	virtual void dealloc();
	virtual UserCustomEntry *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual LString *countyUID() const;
	virtual LString *stateUID() const;
	virtual LString *countryUID() const;
	virtual bool _commit();
	virtual LMap *dbMap() const;
	virtual LString *iconKey() const;
	virtual bool shouldCommit() const;
	virtual LSet *propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const;
	virtual LObject *propertyForKey(const LString *key);
	virtual void setPropertyForKey(LObject *value, const LString *key);
	virtual UserCustomEntry *initWithGeoPointAndName(LPoint geoPoint, LString *name);
	virtual UserCustomEntry *initWithPointEntry(PointEntry *entry);
	virtual void setOwnName(LString *name);
	virtual void setGeoPoint(LPoint geoPoint);
	virtual void setRegionUID(LString *uid);
	virtual void setTimeZoneName(LString *name);
	virtual UserCustomEntry *initWithDictionaryRep(LMap *rep);

protected:
	void _validateRegionUID() const;

	bool _changed;
	mutable bool _regionNeedsValidation;
};

class RouteObjectEntry : public GenericEntry, public virtual IRouteObjectEntryInterface {
	LF_DECLARE_CLASS(RouteObjectEntry)
public:
	RouteObjectEntry();
	virtual void dealloc();
	virtual RouteObjectEntry *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual bool getEntryGeoRect(LRect *rect);
	virtual LString *iconKey() const;
	virtual LSet *propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const;
	virtual LObject *propertyForKey(const LString *key);
	virtual LString *blobUID(unsigned char *mixed) const;
	virtual RouteObject *routeObject() const;

protected:
	LString *_blobUID;
	mutable RouteObject *_routeObject;
};

class UserRouteObjectEntry : public RouteObjectEntry {
	LF_DECLARE_CLASS(UserRouteObjectEntry)
public:
	UserRouteObjectEntry();
	static UserRouteObjectEntry *entryWithRouteObject(RouteObject *route);

	virtual UserRouteObjectEntry *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual bool _commit();
	virtual LMap *dbMap() const;
	virtual bool shouldCommit() const;
	virtual LSet *propertyKeys(TEntryKeySet set, TEntryKeyAccessMode mode) const;
	virtual void setPropertyForKey(LObject *value, const LString *key);
	virtual RouteObject *routeObject() const;
	virtual UserRouteObjectEntry *initWithRouteObject(RouteObject *route);
	virtual void setOwnName(LString *name);

protected:
	bool _routeSaved;
	bool _changed;
};

// A row of the search results: an entry, a "see all" row or a text row.
class SearchPreviewEntry : public LObject {
	LF_DECLARE_CLASS(SearchPreviewEntry)
public:
	SearchPreviewEntry();
	static SearchPreviewEntry *entryWithStrings(LString *type, LString *title, unsigned char alternative);
	static SearchPreviewEntry *entryWithUID(LString *uid, unsigned char showType, unsigned char alternative);
	static SearchPreviewEntry *entryWithCollapsedInfo(int count, int entryType, unsigned char showType,
	                                                  unsigned char alternative);

	SearchPreviewEntry *initWithStrings(LString *type, LString *title, unsigned char alternative);
	SearchPreviewEntry *initWithUID(LString *uid, unsigned char showType, unsigned char alternative);
	SearchPreviewEntry *initWithCollapsedInfo(int count, int entryType, unsigned char showType,
	                                          unsigned char alternative);
	virtual void dealloc();
	virtual SearchPreviewEntry *getOwnerLObject() { return this; }
	virtual LString *publicTitle();
	virtual LString *publicType();
	virtual GenericEntry *geoEntry();
	virtual bool emphasizeTypeString();
	virtual bool isAlternative();
	virtual bool isClickable();
	virtual void setFirstRegionRowFlag(unsigned char flag);
	virtual void setAllNamesRowFlag(unsigned char flag);
	virtual LString *customRequestForSubstring(LString *substring);

protected:
	bool _collapsed;
	int _entryType;
	int _count;
	bool _clickable;
	bool _firstRegionRow;
	bool _allNamesRow;
	bool _alternative;
	bool _emphasizeType;
	bool _showType;
	GenericEntry *_entry;
	LString *_title;
	LString *_type;
};

// ---------------------------------------------------------------------------
// Proxies: several selected entries read as one

class GenericEntryProxy : public LObject, public virtual IGenericEntryInterface {
	LF_DECLARE_CLASS(GenericEntryProxy)
public:
	GenericEntryProxy();
	void initWithEntryArray(TLArray<GenericEntry *> *entries);
	TLArray<GenericEntry *> *entries() const { return _entries; }

	virtual void dealloc();
	virtual GenericEntryProxy *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual LString *uid(unsigned char *mixed) const;
	virtual LString *ownName(unsigned char *mixed) const;
	virtual LString *shortCaption(unsigned char *mixed) const;
	virtual LString *ownType(unsigned char *mixed) const;
	virtual LString *altType(unsigned char *mixed) const;
	virtual LString *kind(unsigned char *mixed) const;
	virtual LString *timeZoneName(unsigned char *mixed) const;
	virtual LArray *alternativeNames(unsigned char *mixed) const;
	virtual LMap *userInfo(unsigned char *mixed) const;
	virtual LString *countyName(unsigned char *mixed) const;
	virtual LString *stateName(unsigned char *mixed) const;
	virtual LString *countryName(unsigned char *mixed) const;
	virtual LString *regionName(unsigned char *mixed) const;
	virtual int population(unsigned char *mixed) const;

	// Reads one property of every entry: the common value, or `def` with
	// *mixed set when they differ.
	template <class T, class E> struct _getter {
		typedef T (E::*Getter)(unsigned char *) const;
		static T getProperty(TLArray<GenericEntry *> *entries, Getter getter, unsigned char *mixed,
		                     bool (*areEqual)(T, T), T def)
		{
			if (mixed)
				*mixed = 1;
			int count = entries ? entries->count() : 0;
			if (count < 1)
				return def;
			T value = (static_cast<E *>(entries->objectAtIndex(0))->*getter)(mixed);
			for (int i = 1; i < count; i++) {
				T other = (static_cast<E *>(entries->objectAtIndex(i))->*getter)(mixed);
				if (!areEqual(value, other)) {
					if (mixed)
						*mixed = 1;
					return def;
				}
			}
			return value;
		}
		static bool areObjectsEqual(T a, T b);
		static bool areEqual(T a, T b) { return a == b; }
	};

protected:
	TLArray<GenericEntry *> *_entries;
};

// Two property values are the same object or compare equal.
bool entryPropertyObjectsEqual(LObject *a, LObject *b);
template <class T, class E> bool GenericEntryProxy::_getter<T, E>::areObjectsEqual(T a, T b)
{
	return entryPropertyObjectsEqual(a, b);
}

class PointEntryProxy : public GenericEntryProxy, public virtual IPointEntryInterface {
	LF_DECLARE_CLASS(PointEntryProxy)
public:
	virtual PointEntryProxy *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual LPoint geoPoint(unsigned char *mixed) const;
	virtual int customWeatherID(unsigned char *mixed) const;
	virtual LTimeZone *timeZone(unsigned char *mixed) const;
	virtual int currentGMTOffset(unsigned char *mixed) const;
};

#define AM_DECLARE_ENTRY_PROXY(Name, Base, Interface)              \
	class Name : public Base, public virtual Interface {       \
		LF_DECLARE_CLASS(Name)                             \
	public:                                                    \
		virtual Name *getOwnerLObject() { return this; }   \
		virtual void interfaceRetain();                    \
		virtual void interfaceRelease();                   \
	};

AM_DECLARE_ENTRY_PROXY(CityEntryProxy, PointEntryProxy, ICityEntryInterface)
AM_DECLARE_ENTRY_PROXY(FeatureEntryProxy, PointEntryProxy, IFeatureEntryInterface)
AM_DECLARE_ENTRY_PROXY(ShapeEntryProxy, GenericEntryProxy, IShapeEntryInterface)
AM_DECLARE_ENTRY_PROXY(AppleStoreEntryProxy, PointEntryProxy, IAppleStoreEntryInterface)
AM_DECLARE_ENTRY_PROXY(UserCustomEntryProxy, PointEntryProxy, IUserCustomEntryInterface)

class RouteObjectEntryProxy : public GenericEntryProxy, public virtual IRouteObjectEntryInterface {
	LF_DECLARE_CLASS(RouteObjectEntryProxy)
public:
	virtual RouteObjectEntryProxy *getOwnerLObject() { return this; }
	virtual void interfaceRetain();
	virtual void interfaceRelease();
	virtual LString *blobUID(unsigned char *mixed) const;
};

// The lookup tables the entry list sorts with.
struct SEntryCompareContext {
	SEntryCompareContext(TEntriesSortingMode mode);
	~SEntryCompareContext();

	TEntriesSortingMode mode;
	TStrMap<const char *, 10> *sortKeys;
	TStrMap<const char *, 10> *regionSortKeys;
	TStrMap<LString *, 28> *names;
};

#endif
