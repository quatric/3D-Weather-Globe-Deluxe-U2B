// AppModel basics: the module bundle, the callbacks the application
// implements, unit and geographic math, value formatting (Utils) and the user
// preferences.

#ifndef AMBASE_H
#define AMBASE_H

#include "LObject.h"
#include "LContainers.h"
#include "LString.h"
#include "LArray.h"
#include "LMap.h"
#include "LSet.h"
#include "LValue.h"
#include "LDate.h"
#include "LData.h"
#include "LImage.h"
#include "LNotification.h"
#include "LUserDefaults.h"
#include "LFileManager.h"
#include "LScanner.h"
#include "LInvocation.h"
#include "LRunLoop.h"
#include "LProfiler.h"
#include "LUndoManager.h"
#include "LCleanupTask.h"
#include "LNull.h"
#include "LTask.h"
#include "LCoder.h"
#include "FHSound.h"
#include "IAlertCallback.h"
#include "FHMath.h"
#include "FHD3DTypes.h"
#include "AppModelStrings.h"
#include <CoreFoundation/CoreFoundation.h>

using namespace LF;
using namespace LF::UMapParsingUtils;

class AtlasDatabase;
class BLOBWrapper;
class CloudsManager;
class CProjection;
class CShapeContainer;
class CShapeManager;
class CWManager;
class CWOnlineCityData;
class DBMutableRouteNode;
class DBRouteNode;
class DBTreeElement;
class DownloadManager;
class DynamicEnvironment;
class EntryInfo;
class EntryInfoImageWrapper;
class DBUserRouteNode;
class PointEntry;
class GenericEntry;
class GenericRouteNode;
class ImageBLOB;
class ImagesSequence;
class IndexBuffersBLOB;
class LookupGridBLOB;
class MutableRouteObject;
class PlistBLOB;
class PointEntry;
class RouteObject;
class RouteObjectEntry;
class RouteSetItem;
class TextBLOB;
class TimeController;
class UserCustomEntry;
class UserRouteObjectEntry;
class VertexBuffersBLOB;
class C2DTexture;

// ---------------------------------------------------------------------------
// The module

// The AppModel bundle, for its localized string tables.
void setAppModelModule(CFBundleRef bundle);
CFBundleRef getAppModelModule();
// The application calls these first and last.
void initializeAppModel(LString *appKey);
void finalizeAppModel();
// The high resolution maps (TEXTURES.zip) are unpacked once the public
// activation key arrives.
void activatePublicKey();
bool isPublicKeyActivationNeeded();
Color generateRandomColor();

// What the application provides to the model.
class IModelCallbacks {
public:
	virtual int _applicationBuildNumber() = 0;
	virtual void _applicationTerminate(int code) = 0;
	virtual void _applicationRestart() = 0;
	virtual void _openURL(LString *url) = 0;
	// The application support folder, created if `create`.
	virtual LString *_applicationDataPath(unsigned char create) = 0;
	virtual LString *_applicationCachesPath(unsigned char create) = 0;
	virtual bool _showSubscriptionDialog() = 0;
	// fname, lname, email and country of the registered user.
	virtual LMap *_registrationInfo() = 0;
	virtual void _showSelectCityDialog() = 0;
	virtual bool _showSerialNumberDialog() = 0;
	virtual LString *_serialNumber() = 0;
	virtual LString *_hiResMapsRootFolderSourcePath() = 0;

	static void setCallbacks(IModelCallbacks *callbacks);

	static IModelCallbacks *_callbacks;
	static LString *kCountryKey;
	static LString *kLastNameKey;
	static LString *kFirstNameKey;
	static LString *kEmailKey;
};

// ---------------------------------------------------------------------------
// Units and geography. Geographic points are (longitude, latitude) in
// degrees; map points are 0..1 across the equirectangular map; earth vectors
// are unit vectors with z towards the north pole.

struct TDegrees {
	float degrees;
	float minutes;
	float seconds;
	bool isNegative;
};

float celsiusFromFahrenheit(float f);
float fahrenheitFromCelsius(float c);
float feetsFromKilometers(float km);
float milesFromKilometers(float km);
float inchesFromMiliBars(float mb);
float millimetersFromMiliBars(float mb);
TDegrees degreesFromFloat(float value);
float floatFromDegrees(TDegrees d);
// Carries whole minutes and seconds up and wraps the degrees into `period`.
TDegrees clampDegreesWithPeriod(TDegrees d, float period);
float deg2radf(float deg);
float rad2degf(float rad);
// The direction a wind blows from, in degrees clockwise from north.
float windAngleFromWindDir(Vector2 dir);
Vector2 windDirFromWindAngle(float angle);

LPoint geoPointFromMapPoint(LPoint p);
LPoint mapPointFromGeoPoint(LPoint p);
LPoint mapPointFromEarthVec(const Vector3 *v);
LPoint geoPointFromEarthVec(const Vector3 *v);
Vector3 earthVecFromMapPoint(LPoint p);
Vector3 earthVecFromGeoPoint(LPoint p);
// A cheaper earthVecFromGeoPoint with series for sin and cos.
Vector3 earthVecFromGeoPoint_e(LPoint p);
LRect mapRectFromGeoRect(LRect r);
LRect geoRectFromMapRect(LRect r);
bool pointInRect(LPoint p, LRect r);
void planeFromPointAndNormal(Plane *plane, const Vector3 *point, const Vector3 *normal);
// Radians from degrees and back, for points and sizes.
LSize radiansFromDegreesSize(LSize s);
LPoint radiansFromDegreesPoint(LPoint p);
LPoint degreesFromRadiansPoint(LPoint p);
// Smaller is nearer.
float distEstimationFromEarthVecs(const Vector3 *a, const Vector3 *b);
// Great circle distance in kilometers.
float distanceOnEarthFromEarthVecs(const Vector3 *a, const Vector3 *b);
float distanceOnEarthFromGeoPoints(LPoint a, LPoint b);
// A star color for a B-V color index, with the alpha for `brightness`.
unsigned int colorFromBV(float bv, float brightness);
// The apparent magnitude of a planet at `planet` seen from `observer` (sun
// at the origin), from its absolute magnitude.
float planetMagnitude(Vector3 &planet, Vector3 &observer, float absMagnitude);

// The rotations and positions of the sky for a date, in the earth system.
Quater equatorToSphereRotation(LDate *date);
Quater eclipticToEquatorRotation(LDate *date);
Quater planetsRotation_EarthSystem(LDate *date);
Quater starsRotation_EarthSystem(LDate *date);
Vector3 earthVecFromEclipticVec(const Vector3 *v, LDate *date);
Vector3 moonPosition_EarthSystem(LDate *date);
Vector3 sunPosition_EarthSystem(LDate *date);

// A solar eclipse of the eclipses table, in UT.
struct SSolarEclipse {
	short year;
	char month;
	char day;
	char hour;
	char minute;
	char type;	// 'T'otal, 'A'nnular, 'H'ybrid or 'P'artial
	float magnitude;
	float latitude;	// of the greatest eclipse
	float longitude;
};

// ---------------------------------------------------------------------------
// Utils: values formatted for display, in the units the preferences ask for.

// A map from strings, compared by value, to ints; it retains its keys.
class LStrMap : public TMap<LString *, intptr_t, LComparableKeyCallbacks, LObjectKeyCallbacks> {};


// ---------------------------------------------------------------------------
// Typed coordinates

enum CoordinateStringFormat { kCoordinateFormatDMS = 0, kCoordinateFormatDecimal = 1, kCoordinateFormatUnknown = 2 };

enum {
	kCoordinateCharNone = 0,
	kCoordinateCharDigit,
	kCoordinateCharDegree,
	kCoordinateCharMinute,
	kCoordinateCharSecond,
	kCoordinateCharWorldPart,	// N S E W
	kCoordinateCharSeparator,	// . ,
	kCoordinateCharMinus,
	kCoordinateCharTypeCount
};

class CoordinateParsingResult : public LObject {
	LF_DECLARE_CLASS(CoordinateParsingResult)
public:
	CoordinateParsingResult();
	virtual void dealloc();
	virtual CoordinateParsingResult *getOwnerLObject() { return this; }

	CoordinateParsingResult *initWithArraysAndFormat(TLArray<LString *> *tokens, TLArray<LValue *> *types,
	                                                 CoordinateStringFormat format);
	void reset();
	CoordinateStringFormat getCoordinateFormat();
	bool hasNextCoordinate();
	TDegrees getNextCoordinate();
	TDegrees getNextDecimalCoordinate();
	TDegrees getNextDMSCoordinate();

private:
	int _typeAt(int index);

	CoordinateStringFormat _format;
	int _index;
	TLArray<LString *> *_tokens;
	TLArray<LValue *> *_types;
};

class CoordinateParser : public LObject {
	LF_DECLARE_CLASS(CoordinateParser)
public:
	virtual void dealloc();
	virtual CoordinateParser *getOwnerLObject() { return this; }

	static int determineCharType(UniChar c);
	// Autoreleased.
	CoordinateParsingResult *Parse(LString *string);
};

// ---------------------------------------------------------------------------
// Regexp: Henry Spencer's regular expressions.

struct regexp;

class Regexp {
public:
	Regexp(const char *exp);
	Regexp(const char *exp, int length);
	~Regexp();
	int Match(const char *string, int length, int start);
	int Search(const char *string, int length, int start, int range);
	int BeginningOfMatch(int i);
	int EndOfMatch(int i);

private:
	bool _compile();
	void _rebase(const char *copy, const char *string);

	char *_exp;
	regexp *_regexp;
};

// ---------------------------------------------------------------------------
// Geometry helpers

// Turns geographic points into scene points for one kind of map.
class CProjection : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(CProjection)
public:
	virtual void dealloc();
	virtual void scenePointFromGeoPoint(Vector3 *out, const LPoint *geoPoint) = 0;
};

// A quad tree of flags over the unit square: level L has 2^L x 2^L cells,
// one byte each, all levels in one buffer.
class QuadTreeContainer : public LObject {
	LF_DECLARE_CLASS(QuadTreeContainer)
public:
	// A cell: its level in the low byte, then 12 bits of x and 12 of y.
	struct EntryID {
		unsigned int packed;
		EntryID() : packed(0) {}
		EntryID(int level, int x, int y)
		    : packed((unsigned int)(level & 0xff) | ((unsigned int)(x & 0xfff) << 8) | ((unsigned int)y << 20))
		{
		}
		int level() const { return (signed char)(packed & 0xff); }
		int x() const { return (int)(packed << 12) >> 20; }
		int y() const { return (int)packed >> 20; }
	};
	class IEntryVisibilityTester {
	public:
		virtual bool isEntryVisible(const EntryID &entry, const void *context) = 0;
		virtual ~IEntryVisibilityTester() {}
	};

	QuadTreeContainer();
	QuadTreeContainer *initWithLogSize(int logSize);
	virtual void dealloc();
	virtual QuadTreeContainer *getOwnerLObject() { return this; }

	static int bufferOffset(const EntryID &entry);
	unsigned char *elementPtr(const EntryID &entry);
	unsigned char *elementPtr(const LPoint &point);
	LRect findLargestVisibleRect(const EntryID &entry, IEntryVisibilityTester *tester, const void *context);
	int findNearestVisibleElemID(const EntryID &target, EntryID &area, IEntryVisibilityTester *tester,
	                             const void *context);
	void setElementsInRect(LRect rect, unsigned char value);
	void downsampleIfNeeded();
	void setAllChildrenTo(const EntryID &entry, unsigned char value);
	int logSize() const { return _logSize; }

protected:
	int _nearestDistInElemArea(const EntryID &target, const EntryID &area, EntryID &found,
	                           IEntryVisibilityTester *tester, const void *context);

	int _logSize;
	StPointerBlock *_buffer;
	bool _downsampled;
};

class Utils {
public:
	enum TGeolocFormat { kGeolocFormatSimple = 0, kGeolocFormatMedium = 1, kGeolocFormatFull = 2 };

	static LString *temperatureDescription(LObject *celsius);
	static LString *temperatureValueShortDescription(float value);
	static LString *temperatureShortDescription(LObject *celsius);
	static LString *speedDescription(LObject *kmh);
	static LString *percentsDescription(LObject *value);
	static LString *pressureDescription(LObject *millibars);
	static int windImageIndex(int angle);
	static LString *shortSingleCoordDescription(float value, unsigned char isLatitude, TGeolocFormat format);
	static LString *shortGeoCoordinatesDescription(LPoint geoPoint, TGeolocFormat format);
	static LString *shortHeightDesription(float km);
	static LData *ddsDataWithFormatFromBitmap(_D3DFORMAT format, const void *bits, LSize size, unsigned int bytesPerRow);
	static LData *ddsDataWithFormatFromBitmapImage(_D3DFORMAT format, LBitmapImage *image);
	static LString *geoCoordinatesDegreeDelimiter();
	static LString *geoCoordinatesMinuteDelimiter();
	static LString *geoCoordinatesSecondDelimiter();
	static LString *solarEclipseTypeDescription(const SSolarEclipse *eclipse);
	static int daysIntervalBetweenDates(LDate *from, LDate *to);
	static LDate *dateByAddingMinutesInterval(LDate *date, int minutes);
	static int minutesIntervalBetweenDates(LDate *from, LDate *to);
	static LDate *dateFromSolarEclipse(const SSolarEclipse *eclipse, LTimeZone *timeZone);
	static LString *solarEclipseTooltipDescription(const SSolarEclipse *eclipse, LTimeZone *timeZone);
	// The name of the weekday (0 is Sunday), abbreviated if `abbr`.
	static LString *weekdayName(int day, unsigned char abbr);
	static LString *geoCoordinatesWorldPartCharacters(unsigned char negative);
	// "1,234.56" with `fraction` digits and an optional unit suffix.
	static LString *commaSeparatedNumericValue(double value, int fraction, const LString *suffix);
	static LString *commaSeparatedNumericValue(LObject *value, int fraction, const LString *suffix);
	static LString *densityDescription(LObject *perSqKm);
	static LString *areaDescription(LObject *sqKm);
	static LString *smallDistanceDescription(LObject *meters);
	static LString *distanceShortDescription(float km);
	static LString *distanceDescription(float km);
	static LString *distanceDescription(LObject *km);
	static LString *weatherDescription(LMap *report);
	static LString *windShortDescription(int angle);
	static LString *windDescription(int angle);
};

// Orders cloud map file names ("west_YY_MM_DD_HH.dds") newest first, east
// before west.
class CFileNameComparator {
public:
	static int compare(const void *a, const void *b, const void *context);
};

// ---------------------------------------------------------------------------
// Preferences, kept in the user defaults.

// Set while a cinematic plays (the screen saver): the preferences only
// check that there is one.
class PreferencesOverloader {};

// The enums with one named value stand for ints the preferences store; the
// fixed underlying type keeps every value in range.
enum TMapMouseMode : int { kMapMouseModeDefault = 1 };
enum TRenderingQuality { kRenderingQualityLow = 0, kRenderingQualityMedium = 1, kRenderingQualityHigh = 2,
                         kRenderingQualityCustom = 3 };
enum TCloudsDetalizationLevel { kCloudsDetalization256 = 0, kCloudsDetalization512 = 1,
                                kCloudsDetalization1024 = 2, kCloudsDetalization2048 = 3 };
enum TCloudsAnimation { kCloudsAnimationNone = 0, kCloudsAnimationLow = 1, kCloudsAnimationHigh = 2 };
enum TInternetState { kInternetStateOffline = 0, kInternetStateOnline = 1 };
enum TMapMode : int { kMapModeDefault = 0 };
enum TMinimapMode : int { kMinimapModeDefault = 0 };
enum TNavControlID : int { kNavControlNone = 0 };
enum TForecastDaysCount : int { kForecastDaysCountDefault = 7 };
enum TScreenSaverPointOfViewMode : int { kScreenSaverPointOfViewDefault = 0 };
enum TScreenSaverAltitudeMode : int { kScreenSaverAltitudeDefault = 0 };

class Preferences {
public:
	// Registers the defaults; `screenSaver` gets the screen saver's.
	static void registerDefaults(unsigned char screenSaver);
	// Forgets the view settings when the last view isn't kept.
	static void setupBeforeMainWindowLoad();
	// While an overloader is set (the screen saver), the view settings are
	// fixed.
	static void setOverloader(PreferencesOverloader *overloader);

	static int *oceanTransparencyIsEnabledFlagPtr();
	static int *animatedWaterIsEnabledFlagPtr();
	static float verticalScaleFactor();
	static void setVerticalScaleFactor(float factor);
	static void setSimulatedWeatherEnabled(unsigned char enabled);
	static bool simulatedWeatherEnabled();
	// Hours between cloud maps.
	static int cloudsUpdateTime();
	static bool isSoundsAvailable();
	static float separatorColorWhiteComponent();
	static LArray *snapshotInfoKeys();

	static LString *publicActivationKey();
	static void setPublicActivationKey(LString *key);
	static LMap *registrationInfo();
	static void storeRegistrationInfo(LMap *info);

	static bool showNearestPoint();
	static void setShowNearestPoint(unsigned char show);
	static unsigned int showOtherPoints();
	static unsigned int showAppleStorePoints();
	static unsigned int showCapitalPoints();
	static unsigned int showAllPoints();
	static void setShowOtherPoints(unsigned char show);
	static void setShowAppleStorePoints(unsigned char show);
	static void setShowCapitalPoints(unsigned char show);
	static void setShowAllPoints(unsigned char show);
	static unsigned int moonEnabled();
	static unsigned int sunEnabled();
	static unsigned int constellationsEnabled();
	static unsigned int planetsEnabled();
	static unsigned int starsEnabled();
	static unsigned int spaceEnabled();
	static void setMoonEnabled(unsigned char enabled);
	static void setSunEnabled(unsigned char enabled);
	static void setConstellationsEnabled(unsigned char enabled);
	static void setPlanetsEnabled(unsigned char enabled);
	static void setStarsEnabled(unsigned char enabled);
	static void setSpaceEnabled(unsigned char enabled);

	static void setDayNightEnabled(unsigned char enabled);
	static bool dayNightIsEnabled();
	static bool shapesDrawEnabled(LString *shapeID);
	static void setShapesDrawEnabled(LString *shapeID, unsigned char enabled);
	static bool showTimeZoneEnabled();
	static void setShowTimeZoneEnabled(unsigned char enabled);
	static LComparableSet *shapesIDsToHighlight();
	static LComparableSet *shapesIDsToDraw();
	static void setHighlightBordersAnyway(unsigned char highlight);
	static bool highlightBordersAnyway();
	static bool showAllBorders();
	static void setShowAllBorders(unsigned char show);
	static void setShowLocationNamesEnabled(unsigned char enabled);
	static bool showLocationNamesEnabled();
	static void setShowLocationNamesNearestOnly(unsigned char nearest);
	static bool showLocationNamesNearestOnly();
	static void setShowPlanetsNamesEnabled(unsigned char enabled);
	static bool showPlanetsNamesEnabled();
	static void setShowConstellationsNamesEnabled(unsigned char enabled);
	static bool showConstellationsNamesEnabled();
	static void setShowWeatherOnMapEnabled(unsigned char enabled);
	static bool showWeatherOnMapEnabled();
	static void setShowImagesOnMapEnabled(unsigned char enabled);
	static bool showImagesOnMapEnabled();
	static void setShowAppleStores(unsigned char show);
	static bool showAppleStores();
	static void setShowRouteLines(unsigned char show);
	static bool showRouteLines();
	static void setShowRouteMarkers(unsigned char show);
	static bool showRouteMarkers();
	static TMapMouseMode mouseBehaviourOnMap();
	static void setMouseBehaviourOnMap(TMapMouseMode mode);
	static float globeTransparencyLevel();
	static void setGlobeTransparencyLevel(float level);
	static float soundVolume();
	static void setSoundVolume(float volume);

	static TRenderingQuality sceneRenderingQuality();
	static void setSceneRenderingQuality(TRenderingQuality quality);
	static bool animatedWaterEnabled();
	static void setAnimatedWaterEnabled(unsigned char enabled);
	static bool animatedWaterUsesLargeTexture();
	static void setAnimatedWaterUseLargeTexture(unsigned char large);
	static bool drawWithReliefEnabled();
	static void setDrawWithReliefEnabled(unsigned char enabled);
	static float terrainDetailsLevel();
	static void setTerrainDetailsLevel(float level);
	static float terrainLODBiasValue();

	static float paletteTransparencyLevel();
	static void setPaletteTransparencyLevel(float level);
	static bool hidesWFPaletteOnDeactivate();
	static void setHidesWFPaletteOnDeactivate(unsigned char hides);
	static void setEarthTiltedMode(unsigned char tilted);
	static bool earthTiltedMode();
	static void setShowScaleStraightedge(unsigned char show);
	static bool showScaleStraightedge();

	static bool cloudsEnabled();
	// `notify` reschedules the cloud downloads and tells the observers.
	static void setCloudsEnabled(unsigned char enabled, unsigned char notify);
	static TCloudsDetalizationLevel cloudsDetalizationLevel();
	static void setCloudsDetalizationLevel(TCloudsDetalizationLevel level);
	static int cloudsTextureWidth();
	static TCloudsAnimation cloudsAnimationQuality();
	static void setCloudsAnimationQuality(TCloudsAnimation quality);
	static float cloudsAnimationSliderValue();
	static void setCloudsAnimationSliderValue(float value);
	static float cloudsAnimationDeltaTime();

	static void setGridEnabled(unsigned char enabled);
	static bool gridEnabled();
	static float markersScale();
	static void setMarkersScale(float scale);
	static float automoveSpeedSliderValue();
	static void setAutomoveSpeedSliderValue(float value);
	static float automoveSpeedUIValue();

	static void setShowMainWindow(unsigned char show);
	static bool doShowMainWindowOnStartup();
	static void setOpenFullScreenByDefault(unsigned char fullScreen);
	static bool openFullScreenByDefault();
	static void setShowDemoOnStartup(unsigned char show);
	static bool showDemoOnStartup();
	static void setZoomMainWindow(unsigned char zoom);
	static bool shouldZoomMainWindow();
	static void setShowWeatherOnStartup(unsigned char show);
	static bool doShowWeatherOnStartup();
	static bool doShowWeatherInDock();
	static void setShowWeatherInDock(unsigned char show);
	static void setShowBadVideoAlert(unsigned char show);
	static bool doShowBadVideoAlert();
	static void setShowCloudsDownloadFailedAlert(unsigned char show);
	static bool doShowCloudsDownloadFailedAlert();
	static void setShowUnsupportedOSAlert(unsigned char show);
	static bool doShowUnsupportedOSAlert();
	static void setShowUnsupportedColorAlert(unsigned char show);
	static bool doShowUnsupportedColorAlert();
	static bool showIntroOnStartup();

	static bool usesMetricSystem();
	static void useMetricSystem(unsigned char metric);
	static bool uses24HourClock();
	static void use24HourClock(unsigned char use24);
	static TInternetState internetState();
	static bool usesInternet();
	static void setInternetState(TInternetState state);
	static void useSoftwareUpdating(unsigned char use);
	static bool usesSoftwareUpdating();
	static void setPlaySounds(unsigned char play);
	static bool playSounds();
	static void setRetainLastView(unsigned char retain);
	static bool retainLastView();

	static void setCurrentRouteSetVisible(unsigned char visible);
	static bool isCurrentRouteSetVisible();
	static void setInfoAreaVisible(unsigned char visible);
	static bool isInfoAreaVisible();
	static void setLeftAreaVisible(unsigned char visible);
	static bool isLeftAreaVisible();
	static LMap *lastRouteDictionaryRep();
	static void setLastRouteDictionaryRep(LMap *rep);
	static TMapMode mapMode();
	static void setMapMode(TMapMode mode);
	static TMinimapMode minimapMode();
	static void setMinimapMode(TMinimapMode mode);
	static void setShowInfoDeviceEnabled(unsigned char enabled);
	static bool showInfoDeviceEnabled();
	static void setShowNavDeviceEnabled(unsigned char enabled);
	static bool showNavDeviceEnabled();
	static TNavControlID toggledNavigationControl();
	static void setToggledNavigationControl(TNavControlID control);

	static LString *homeCityUID();
	// The home city; asks the user to pick one when there is none.
	static PointEntry *homeCity();
	static void setHomeCityUID(LString *uid);
	static int citiesCountLimit();
	static float paletteFontSize();
	static void setPaletteFontSize(float size);
	// The entries shown in the current conditions palette.
	static LArray *entriesList();
	static void setEntriesList(TLArray<GenericEntry *> *entries);
	static bool verticalStateEnabled();
	static void setVerticalStateEnabled(unsigned char enabled);
	static int mainPropertiesDetailViewFlags();
	static void setMainPropertiesDetailViewFlags(int flags);
	static bool astronomyForecastEnabled();
	static void setAstronomyForecastEnabled(unsigned char enabled);
	static bool currentConditionsEnabled();
	static void setCurrentConditionsEnabled(unsigned char enabled);
	static TForecastDaysCount expandedForecastDaysCount();
	static void setExpandedForecastDaysCount(TForecastDaysCount count);

	static TScreenSaverPointOfViewMode screenSaverPointOfViewMode();
	static void setScreenSaverPointOfViewMode(TScreenSaverPointOfViewMode mode);
	static TScreenSaverAltitudeMode screenSaverAltitudeMode();
	static void setScreenSaverAltitudeMode(TScreenSaverAltitudeMode mode);
	static float screenSaverCustomAltitudeValue();
	static void setScreenSaverCustomAltitudeValue(float value);
	static bool screenSaverDisplayRegionNamesEnabled();
	static void setScreenSaverDisplayRegionNamesEnabled(unsigned char enabled);
	static bool screenSaverDisplayMiniMapEnabled();
	static void setScreenSaverDisplayMiniMapEnabled(unsigned char enabled);
	static bool screenSaverTiltModeEnabled();
	static void setScreenSaverTiltModeEnabled(unsigned char enabled);
	static bool screenSaverBillboardsEnabled();
	static void setScreenSaverBillboardsEnabled(unsigned char enabled);

	static LString *_regKeyFilePathCached();
	static LString *_keyForShapeID(LString *shapeID);
	static void _postInternetStateNotification();
	static unsigned int _flagsValueForKey(LString *key);
	static void _setFlagsValueForKey(unsigned int flags, LString *key);
	// Sets or clears `flag`; `allFlag` stands for all of `otherFlags`.
	static void _setFlagEnabledForKey(unsigned int flag, unsigned char enabled, LString *key, unsigned int allFlag,
	                                  unsigned int otherFlags);
	static PointEntry *_homeCity();

	static PreferencesOverloader *_overolader;
	static int _dayNightIsEnabledFlag;
	static int _oceanTranspIsEnabledFlag;
	static int _animatedWaterIsEnabledFlag;
	static int _cloudsAnimationQualityFlag;

	static LString *kHomeCityWasChanged;
	static LString *kTerrainLODWasChanged;
	static LString *kClockFormatWasChanged;
	static LString *kSoundVolumeWasChanged;
	static LString *kDrawOptionsWereChanged;
	static LString *kInternetStateWasChanged;
	static LString *kPlaySoundsStateWasChanged;
	static LString *kUnitsOfMeasureWereChanged;
	static LString *kWaterTextureSizeWasChanged;
	static LString *kCIPaletteFontSizeWasChanged;
	static LString *kCloudsEnableStateWasChanged;
	static LString *kCloudsTextureSizeWasChanged;
	static LString *kEnabledBordersSetWasChanged;
	static LString *kGlobeTransparencyWasChanged;
	static LString *kRenderWith3DStateWasChanged;
	static LString *kPaleteTransparencyWasChanged;
	static LString *kCITransparencyLevelWasChanged;
	static LString *kMainPropertiesStateWasChanged;
	static LString *kExpandedForecastStateWasChanged;
	static LString *kShowWeatherOnMapStateWasChanged;
	static LString *kCloudsAnimationQualityWasChanged;
	static LString *kCurrentConditionsStateWasChanged;
	static LString *kShowWeatherInDockStateWasChanged;
	static LString *kHidesWFPaletteOnDeactivateStateWasChanged;
};

#endif
