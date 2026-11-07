// Preferences: the user's settings, kept in the user defaults. Most setters
// skip unchanged values and tell the observers through notifications.

#include "AppModel.h"
#include <CoreServices/CoreServices.h>
#include <math.h>

PreferencesOverloader *Preferences::_overolader = 0;
int Preferences::_dayNightIsEnabledFlag = 0;
int Preferences::_oceanTranspIsEnabledFlag = 0;
int Preferences::_animatedWaterIsEnabledFlag = 1;
int Preferences::_cloudsAnimationQualityFlag = 1;
static float _sVertScaleFactor = 1.0f;

// _kShowPointsFlags bits.
enum { kPointsAll = 1, kPointsCapitals = 2, kPointsAppleStores = 4, kPointsOther = 8, kPointsKinds = 0xe };
// _kSpaceViewFlags bits.
enum {
	kSpaceAll = 1,
	kSpaceStars = 2,
	kSpaceSun = 4,
	kSpaceMoon = 8,
	kSpacePlanets = 0x10,
	kSpaceConstellations = 0x20,
	kSpaceKinds = 0x1f
};

static inline LUserDefaults *UD() { return LUserDefaults::standardUserDefaults(); }

static inline void post(LString *name) { LNotificationCenter::center()->postNotification(name, 0, 0); }

static inline LObject *valueForKey(LString *key) { return UD()->objectForKey(key); }

// The low byte of the stored int, as the original's bool getters return it.
static inline unsigned char boolForKey(LString *key) { return (unsigned char)loadIntFromObject(valueForKey(key)); }

static inline int intForKey(LString *key) { return loadIntFromObject(valueForKey(key)); }

static inline float floatForKey(LString *key) { return loadFloatFromObject(valueForKey(key)); }

// Stores a bool unless the stored value already is `value`; true if stored.
static bool setBoolIfChanged(LString *key, unsigned char value)
{
	LObject *old = valueForKey(key);
	if (old && (unsigned char)loadIntFromObject(old) == value)
		return false;
	UD()->setObjectForKey(LValue::valueWithBoolean(value), key);
	return true;
}

// Stores a bool and returns whether it differs from what was stored.
static bool setBoolReportingChange(LString *key, unsigned char value)
{
	unsigned char old = boolForKey(key);
	UD()->setObjectForKey(LValue::valueWithBoolean(value), key);
	return old != value;
}

static inline void setInt(LString *key, int value) { UD()->setObjectForKey(LValue::valueWithInt(value), key); }

static inline void setFloat(LString *key, float value) { UD()->setObjectForKey(LValue::valueWithFloat(value), key); }

static inline LString *shapeKey(LString *shapeID) { return LString::stringWithFormat("shape highlight: %@", shapeID); }

LString *Preferences::_regKeyFilePathCached()
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LString *folder;
	if (callbacks && (folder = callbacks->_applicationDataPath(1)) != 0)
		return folder->stringByAppendingPathComponent(kRegFileName);
	return 0;
}

LString *Preferences::_keyForShapeID(LString *shapeID) { return shapeKey(shapeID); }

int *Preferences::oceanTransparencyIsEnabledFlagPtr() { return &_oceanTranspIsEnabledFlag; }

int *Preferences::animatedWaterIsEnabledFlagPtr() { return &_animatedWaterIsEnabledFlag; }

float Preferences::verticalScaleFactor() { return _sVertScaleFactor; }

void Preferences::setSimulatedWeatherEnabled(unsigned char) {}

int Preferences::cloudsUpdateTime() { return 3; }

void Preferences::_postInternetStateNotification()
{
	static bool entered = false;
	if (!entered) {
		entered = true;
		post(kInternetStateWasChanged);
		entered = false;
	}
}

// The original asked Gestalt for 10.4 or later.
bool Preferences::isSoundsAvailable() { return true; }

float Preferences::separatorColorWhiteComponent() { return 0.1686f; }

void Preferences::setScreenSaverBillboardsEnabled(unsigned char enabled)
{
	setInt(kScreenSaverBillboardsEnabled, enabled);
}

LString *Preferences::publicActivationKey()
{
	LArray *list = LArray::arrayWithContentsOfFile(_regKeyFilePathCached());
	if (list && list->count() > 0)
		return lobj_cast<LString>(list->objectAtIndex(0));
	return 0;
}

void Preferences::setPublicActivationKey(LString *key)
{
	LString *path = _regKeyFilePathCached();
	LArray *list;
	if (key && (list = LArray::arrayWithObjects(key, (LObject *)0)) != 0) {
		list->writeToFile(path);
		LMap *attributes = LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0644),
		                                               LFileManager::fileAttrPosixPermissions, 0);
		LFileManager::manager()->changeFileAttributesAtPath(path, attributes);
		return;
	}
	LFileManager::manager()->removeFileAtPath(path);
}

LArray *Preferences::snapshotInfoKeys()
{
	LString *timezones = shapeKey(CShapeManager::kShapeIDTimezones);
	LString *counties = shapeKey(CShapeManager::kShapeIDCounties);
	LString *states = shapeKey(CShapeManager::kShapeIDStates);
	LString *countries = shapeKey(CShapeManager::kShapeIDCountries);
	return TLArray<LString *>::arrayWithObjects(kSpaceViewFlags, kShowPointsFlags, kDayNightEnabled,
	                                            kShowLocationNamesEnabled, kShowPlanetsNamesEnabled,
	                                            kShowConstellationsNamesEnabled, kShowAllBorders, kShowRouteLines,
	                                            kShowRouteMarkers, kGridEnabled, kMarkersScale,
	                                            kGlobeTransparencyLevel, countries, states, counties, timezones,
	                                            (LString *)0);
}

bool Preferences::showNearestPoint() { return boolForKey(kShowNearestPointName); }

void Preferences::setShowNearestPoint(unsigned char show) { setBoolIfChanged(kShowNearestPointName, show); }

unsigned int Preferences::_flagsValueForKey(LString *key) { return intForKey(key); }

unsigned int Preferences::showOtherPoints() { return _flagsValueForKey(kShowPointsFlags) & kPointsOther; }
unsigned int Preferences::showAppleStorePoints() { return _flagsValueForKey(kShowPointsFlags) & kPointsAppleStores; }
unsigned int Preferences::showCapitalPoints() { return _flagsValueForKey(kShowPointsFlags) & kPointsCapitals; }
unsigned int Preferences::showAllPoints() { return _flagsValueForKey(kShowPointsFlags) & kPointsAll; }
unsigned int Preferences::moonEnabled() { return _flagsValueForKey(kSpaceViewFlags) & kSpaceMoon; }
unsigned int Preferences::sunEnabled() { return _flagsValueForKey(kSpaceViewFlags) & kSpaceSun; }
unsigned int Preferences::constellationsEnabled() { return _flagsValueForKey(kSpaceViewFlags) & kSpaceConstellations; }
unsigned int Preferences::planetsEnabled() { return _flagsValueForKey(kSpaceViewFlags) & kSpacePlanets; }
unsigned int Preferences::starsEnabled() { return _flagsValueForKey(kSpaceViewFlags) & kSpaceStars; }
unsigned int Preferences::spaceEnabled() { return _flagsValueForKey(kSpaceViewFlags) & kSpaceAll; }

void Preferences::_setFlagsValueForKey(unsigned int flags, LString *key)
{
	setInt(key, flags);
	post(kDrawOptionsWereChanged);
}

void Preferences::_setFlagEnabledForKey(unsigned int flag, unsigned char enabled, LString *key, unsigned int allFlag,
                                        unsigned int otherFlags)
{
	unsigned int old = _flagsValueForKey(key);
	unsigned int flags;
	if (!enabled) {
		flags = old & ~flag;
		// The "all" flag stays on while any kind is; turning off the last
		// kind leaves nothing on.
		if (flag != allFlag)
			flags = (flags & ~allFlag) ? (flags | allFlag) : 0;
	}
	else {
		if ((old | flag) & ~allFlag)
			otherFlags = allFlag;
		flags = otherFlags | old | flag;
	}
	if (flags != old)
		_setFlagsValueForKey(flags, key);
}

void Preferences::setShowOtherPoints(unsigned char show)
{
	_setFlagEnabledForKey(kPointsOther, show, kShowPointsFlags, kPointsAll, kPointsKinds);
}
void Preferences::setShowAppleStorePoints(unsigned char show)
{
	_setFlagEnabledForKey(kPointsAppleStores, show, kShowPointsFlags, kPointsAll, kPointsKinds);
}
void Preferences::setShowCapitalPoints(unsigned char show)
{
	_setFlagEnabledForKey(kPointsCapitals, show, kShowPointsFlags, kPointsAll, kPointsKinds);
}
void Preferences::setShowAllPoints(unsigned char show)
{
	_setFlagEnabledForKey(kPointsAll, show, kShowPointsFlags, kPointsAll, kPointsKinds);
}
void Preferences::setMoonEnabled(unsigned char enabled)
{
	_setFlagEnabledForKey(kSpaceMoon, enabled, kSpaceViewFlags, kSpaceAll, kSpaceKinds);
}
void Preferences::setSunEnabled(unsigned char enabled)
{
	_setFlagEnabledForKey(kSpaceSun, enabled, kSpaceViewFlags, kSpaceAll, kSpaceKinds);
}
void Preferences::setConstellationsEnabled(unsigned char enabled)
{
	_setFlagEnabledForKey(kSpaceConstellations, enabled, kSpaceViewFlags, kSpaceAll, kSpaceKinds);
}
void Preferences::setPlanetsEnabled(unsigned char enabled)
{
	_setFlagEnabledForKey(kSpacePlanets, enabled, kSpaceViewFlags, kSpaceAll, kSpaceKinds);
}
void Preferences::setStarsEnabled(unsigned char enabled)
{
	_setFlagEnabledForKey(kSpaceStars, enabled, kSpaceViewFlags, kSpaceAll, kSpaceKinds);
}
void Preferences::setSpaceEnabled(unsigned char enabled)
{
	_setFlagEnabledForKey(kSpaceAll, enabled, kSpaceViewFlags, kSpaceAll, kSpaceKinds);
}

LMap *Preferences::registrationInfo() { return lobj_cast<LMap>(valueForKey(kRegInfo)); }

void Preferences::storeRegistrationInfo(LMap *info)
{
	if (!info)
		UD()->removeObjectForKey(kRegInfo);
	else
		UD()->setObjectForKey(info, kRegInfo);
}

void Preferences::setDayNightEnabled(unsigned char enabled)
{
	UD()->setObjectForKey(LValue::valueWithBoolean(enabled), kDayNightEnabled);
	if (!_overolader)
		_dayNightIsEnabledFlag = enabled;
	post(kDrawOptionsWereChanged);
}

bool Preferences::dayNightIsEnabled() { return _overolader ? 1 : boolForKey(kDayNightEnabled); }

void Preferences::setOverloader(PreferencesOverloader *overloader)
{
	_overolader = overloader;
	_dayNightIsEnabledFlag = dayNightIsEnabled();
}

bool Preferences::shapesDrawEnabled(LString *shapeID) { return boolForKey(shapeKey(shapeID)); }

bool Preferences::showTimeZoneEnabled() { return shapesDrawEnabled(CShapeManager::kShapeIDTimezones); }

LComparableSet *Preferences::shapesIDsToHighlight()
{
	LArray *ids = LArray::array();
	if (shapesDrawEnabled(CShapeManager::kShapeIDCountries))
		ids->addObject(CShapeManager::kShapeIDCountries);
	if (shapesDrawEnabled(CShapeManager::kShapeIDStates))
		ids->addObject(CShapeManager::kShapeIDStates);
	if (shapesDrawEnabled(CShapeManager::kShapeIDTimezones))
		ids->addObject(CShapeManager::kShapeIDTimezones);
	LComparableSet *set = LComparableSet::_alloc();
	set->initWithArray(ids);
	set->l_autorelease();
	return set;
}

void Preferences::setHighlightBordersAnyway(unsigned char highlight)
{
	setBoolIfChanged(kAlwaysHighlightBordersEnabled, highlight);
}

bool Preferences::highlightBordersAnyway() { return _overolader ? 0 : boolForKey(kAlwaysHighlightBordersEnabled); }

bool Preferences::showAllBorders() { return _overolader ? 0 : boolForKey(kShowAllBorders); }

LComparableSet *Preferences::shapesIDsToDraw()
{
	if (showAllBorders())
		return shapesIDsToHighlight();
	return LComparableSet::set();
}

static bool anyBordersEnabled()
{
	return Preferences::shapesDrawEnabled(CShapeManager::kShapeIDCountries) ||
	       Preferences::shapesDrawEnabled(CShapeManager::kShapeIDStates) ||
	       Preferences::shapesDrawEnabled(CShapeManager::kShapeIDTimezones);
}

void Preferences::setShapesDrawEnabled(LString *shapeID, unsigned char enabled)
{
	UD()->setObjectForKey(LValue::valueWithBoolean(enabled), shapeKey(shapeID));
	if (shapeID == CShapeManager::kShapeIDCountries || shapeID == CShapeManager::kShapeIDStates ||
	    shapeID == CShapeManager::kShapeIDTimezones) {
		// The borders are shown while any kind of them is on.
		bool all = showAllBorders();
		if (!enabled && !anyBordersEnabled()) {
			if (all)
				setShowAllBorders(0);
		}
		else if (!all) {
			setShowAllBorders(1);
		}
	}
	post(kEnabledBordersSetWasChanged);
}

void Preferences::setShowAllBorders(unsigned char show)
{
	if (!setBoolIfChanged(kShowAllBorders, show))
		return;
	if (show && !anyBordersEnabled()) {
		setShapesDrawEnabled(CShapeManager::kShapeIDCountries, 1);
		setShapesDrawEnabled(CShapeManager::kShapeIDStates, 1);
		setShapesDrawEnabled(CShapeManager::kShapeIDTimezones, 1);
	}
	post(kEnabledBordersSetWasChanged);
	post(kDrawOptionsWereChanged);
}

void Preferences::setShowTimeZoneEnabled(unsigned char enabled)
{
	setShapesDrawEnabled(CShapeManager::kShapeIDTimezones, enabled);
	post(kDrawOptionsWereChanged);
}

void Preferences::setShowLocationNamesEnabled(unsigned char enabled)
{
	if (setBoolIfChanged(kShowLocationNamesEnabled, enabled))
		post(kDrawOptionsWereChanged);
}

bool Preferences::showLocationNamesEnabled() { return _overolader ? 0 : boolForKey(kShowLocationNamesEnabled); }

void Preferences::setShowLocationNamesNearestOnly(unsigned char nearest)
{
	if (setBoolIfChanged(kShowLocationNamesNearest, nearest))
		post(kDrawOptionsWereChanged);
}

bool Preferences::showLocationNamesNearestOnly() { return _overolader ? 0 : boolForKey(kShowLocationNamesNearest); }

void Preferences::setShowPlanetsNamesEnabled(unsigned char enabled)
{
	if (setBoolIfChanged(kShowPlanetsNamesEnabled, enabled))
		post(kDrawOptionsWereChanged);
}

bool Preferences::showPlanetsNamesEnabled() { return _overolader ? 0 : boolForKey(kShowPlanetsNamesEnabled); }

void Preferences::setShowConstellationsNamesEnabled(unsigned char enabled)
{
	if (setBoolIfChanged(kShowConstellationsNamesEnabled, enabled))
		post(kDrawOptionsWereChanged);
}

bool Preferences::showConstellationsNamesEnabled()
{
	return _overolader ? 0 : boolForKey(kShowConstellationsNamesEnabled);
}

void Preferences::setShowWeatherOnMapEnabled(unsigned char enabled)
{
	if (!setBoolIfChanged(kShowWeatherOnMapEnabled, enabled))
		return;
	if (enabled)
		CWManager::instance()->resetWeatherInternetState();
	post(kDrawOptionsWereChanged);
}

bool Preferences::showWeatherOnMapEnabled() { return _overolader ? 0 : boolForKey(kShowWeatherOnMapEnabled); }

void Preferences::setShowImagesOnMapEnabled(unsigned char enabled)
{
	if (setBoolIfChanged(kShowImagesOnMapEnabled, enabled))
		post(kDrawOptionsWereChanged);
}

bool Preferences::showImagesOnMapEnabled() { return _overolader ? 0 : boolForKey(kShowImagesOnMapEnabled); }

void Preferences::setShowAppleStores(unsigned char show) { setBoolIfChanged(kShowAppleStores, show); }

bool Preferences::showAppleStores() { return _overolader ? 0 : boolForKey(kShowAppleStores); }

void Preferences::setShowRouteLines(unsigned char show)
{
	if (setBoolIfChanged(kShowRouteLines, show))
		post(kDrawOptionsWereChanged);
}

bool Preferences::showRouteLines() { return _overolader ? 0 : boolForKey(kShowRouteLines); }

void Preferences::setShowRouteMarkers(unsigned char show)
{
	if (setBoolIfChanged(kShowRouteMarkers, show))
		post(kDrawOptionsWereChanged);
}

bool Preferences::showRouteMarkers() { return _overolader ? 0 : boolForKey(kShowRouteMarkers); }

TMapMouseMode Preferences::mouseBehaviourOnMap() { return (TMapMouseMode)intForKey(kMouseBehaviourOnMap); }

void Preferences::setMouseBehaviourOnMap(TMapMouseMode mode)
{
	setInt(kMouseBehaviourOnMap, mode);
	post(kDrawOptionsWereChanged);
}

float Preferences::globeTransparencyLevel() { return floatForKey(kGlobeTransparencyLevel); }

void Preferences::setGlobeTransparencyLevel(float level)
{
	setFloat(kGlobeTransparencyLevel, level);
	_oceanTranspIsEnabledFlag = 0.001f < level;
	post(kGlobeTransparencyWasChanged);
}

float Preferences::soundVolume() { return floatForKey(kSoundVolume); }

void Preferences::setSoundVolume(float volume)
{
	setFloat(kSoundVolume, volume);
	post(kSoundVolumeWasChanged);
}

TRenderingQuality Preferences::sceneRenderingQuality() { return (TRenderingQuality)intForKey(kRenderingQuality); }

bool Preferences::animatedWaterEnabled() { return intForKey(kAnimWaterEnabled) != 0; }

void Preferences::setAnimatedWaterEnabled(unsigned char enabled)
{
	setInt(kAnimWaterEnabled, enabled);
	_animatedWaterIsEnabledFlag = enabled != 0;
}

bool Preferences::animatedWaterUsesLargeTexture() { return intForKey(kAnimWaterLargeTexture) != 0; }

void Preferences::setAnimatedWaterUseLargeTexture(unsigned char large)
{
	unsigned char old = animatedWaterUsesLargeTexture();
	setInt(kAnimWaterLargeTexture, large);
	if (old != large)
		post(kWaterTextureSizeWasChanged);
}

bool Preferences::drawWithReliefEnabled() { return intForKey(kReliefEnabled) != 0; }

void Preferences::setDrawWithReliefEnabled(unsigned char enabled)
{
	unsigned char old = drawWithReliefEnabled();
	setInt(kReliefEnabled, enabled);
	if (old != enabled)
		post(kRenderWith3DStateWasChanged);
}

void Preferences::setTerrainDetailsLevel(float level)
{
	setFloat(kTerrainLOD, level);
	post(kTerrainLODWasChanged);
}

void Preferences::setSceneRenderingQuality(TRenderingQuality quality)
{
	setInt(kRenderingQuality, quality);
	float lod;
	if (quality == kRenderingQualityMedium) {
		setAnimatedWaterEnabled(1);
		setAnimatedWaterUseLargeTexture(0);
		setDrawWithReliefEnabled(1);
		lod = 0.6f;
	}
	else if (quality == kRenderingQualityHigh) {
		setAnimatedWaterEnabled(1);
		setAnimatedWaterUseLargeTexture(1);
		setDrawWithReliefEnabled(1);
		lod = 1.0f;
	}
	else if (quality == kRenderingQualityLow) {
		setAnimatedWaterEnabled(0);
		setAnimatedWaterUseLargeTexture(0);
		setDrawWithReliefEnabled(0);
		lod = 0.0f;
	}
	else {
		return;
	}
	setTerrainDetailsLevel(lod);
}

float Preferences::terrainDetailsLevel() { return _overolader ? 1.0f : floatForKey(kTerrainLOD); }

float Preferences::terrainLODBiasValue()
{
	float level = terrainDetailsLevel();
	if (level < 0.0f)
		level = 0.0f;
	else if (level > 1.0f)
		level = 1.0f;
	return (1.0f - level) * 5000.0f + 5000.0f;
}

// The original stores the factor under the terrain LOD key, which
// setupBeforeMainWindowLoad then never reads back from kVertScale.
void Preferences::setVerticalScaleFactor(float factor)
{
	_sVertScaleFactor = factor;
	setFloat(kTerrainLOD, factor);
}

float Preferences::paletteTransparencyLevel() { return floatForKey(kCITransparencyLevel); }

void Preferences::setPaletteTransparencyLevel(float level)
{
	setFloat(kCITransparencyLevel, level);
	post(kCITransparencyLevelWasChanged);
}

bool Preferences::hidesWFPaletteOnDeactivate() { return intForKey(kHidesWFPaletteOnDeactivate) != 0; }

void Preferences::setHidesWFPaletteOnDeactivate(unsigned char hides)
{
	UD()->setObjectForKey(LValue::valueWithBoolean(hides), kHidesWFPaletteOnDeactivate);
	post(kHidesWFPaletteOnDeactivateStateWasChanged);
}

void Preferences::setEarthTiltedMode(unsigned char tilted) { setBoolIfChanged(kEarthTiltedMode, tilted); }

bool Preferences::earthTiltedMode() { return _overolader ? 0 : boolForKey(kEarthTiltedMode); }

void Preferences::setShowScaleStraightedge(unsigned char show) { setBoolIfChanged(kShowScaleStraightedge, show); }

bool Preferences::showScaleStraightedge() { return _overolader ? 0 : boolForKey(kShowScaleStraightedge); }

bool Preferences::cloudsEnabled() { return _overolader ? 0 : boolForKey(kCloudsEnabled); }

bool Preferences::simulatedWeatherEnabled() { return cloudsEnabled(); }

void Preferences::setCloudsEnabled(unsigned char enabled, unsigned char notify)
{
	if (setBoolReportingChange(kCloudsEnabled, enabled) && notify) {
		CWManager *manager = CWManager::instance();
		if (enabled)
			manager->scheduleCloudsUpdateTask();
		else
			manager->unscheduleCloudsUpdateTask();
		post(kCloudsEnableStateWasChanged);
	}
	post(kDrawOptionsWereChanged);
}

TCloudsDetalizationLevel Preferences::cloudsDetalizationLevel()
{
	return (TCloudsDetalizationLevel)intForKey(kCloudsDetalization);
}

int Preferences::cloudsTextureWidth()
{
	switch (cloudsDetalizationLevel()) {
	case kCloudsDetalization2048:
		return 2048;
	case kCloudsDetalization1024:
		return 1024;
	case kCloudsDetalization512:
		return 512;
	default:
		return 256;
	}
}

void Preferences::setCloudsDetalizationLevel(TCloudsDetalizationLevel level)
{
	int old = intForKey(kCloudsDetalization);
	setInt(kCloudsDetalization, level);
	if (old != level) {
		CWManager::instance()->scheduleCloudsReloadTask();
		post(kCloudsTextureSizeWasChanged);
	}
}

TCloudsAnimation Preferences::cloudsAnimationQuality() { return (TCloudsAnimation)intForKey(kCloudsAnimation); }

void Preferences::setCloudsAnimationQuality(TCloudsAnimation quality)
{
	setInt(kCloudsAnimation, quality);
	int old = _cloudsAnimationQualityFlag;
	_cloudsAnimationQualityFlag = quality;
	if (quality != old)
		post(kCloudsAnimationQualityWasChanged);
}

float Preferences::cloudsAnimationSliderValue() { return floatForKey(kCloudsTimeStep); }

float Preferences::cloudsAnimationDeltaTime() { return expf(cloudsAnimationSliderValue() * -2.0f); }

void Preferences::setCloudsAnimationSliderValue(float value) { setFloat(kCloudsTimeStep, value); }

void Preferences::setGridEnabled(unsigned char enabled)
{
	if (setBoolIfChanged(kGridEnabled, enabled))
		post(kDrawOptionsWereChanged);
}

bool Preferences::gridEnabled() { return _overolader ? 0 : boolForKey(kGridEnabled); }

float Preferences::markersScale() { return floatForKey(kMarkersScale); }

void Preferences::setMarkersScale(float scale)
{
	setFloat(kMarkersScale, scale);
	post(kDrawOptionsWereChanged);
}

float Preferences::automoveSpeedSliderValue() { return floatForKey(kAutomoveSpeed); }

float Preferences::automoveSpeedUIValue()
{
	float value = automoveSpeedSliderValue();
	return powf(4.0f, value + value) * 0.25f;
}

void Preferences::setAutomoveSpeedSliderValue(float value) { setFloat(kAutomoveSpeed, value); }

void Preferences::setShowMainWindow(unsigned char show) { setBoolIfChanged(kDoShowMainWindow, show); }

void Preferences::setOpenFullScreenByDefault(unsigned char fullScreen)
{
	setBoolIfChanged(kFullScreenAsDefault, fullScreen);
}

bool Preferences::openFullScreenByDefault() { return boolForKey(kFullScreenAsDefault); }

void Preferences::setShowDemoOnStartup(unsigned char show) { setBoolIfChanged(kShowDemoOnStartup, show); }

bool Preferences::showDemoOnStartup() { return boolForKey(kShowDemoOnStartup); }

void Preferences::setZoomMainWindow(unsigned char zoom) { setBoolIfChanged(kZoomMainWindow, zoom); }

bool Preferences::shouldZoomMainWindow() { return boolForKey(kZoomMainWindow); }

void Preferences::setShowWeatherOnStartup(unsigned char show)
{
	if (setBoolIfChanged(kDoShowWeather, show))
		post(kDrawOptionsWereChanged);
}

bool Preferences::doShowWeatherOnStartup() { return boolForKey(kDoShowWeather); }

// Something has to show up at launch: without the weather window, the main
// window does.
bool Preferences::doShowMainWindowOnStartup()
{
	unsigned char show = boolForKey(kDoShowMainWindow);
	if (!show && !doShowWeatherOnStartup()) {
		show = 1;
		setShowMainWindow(1);
	}
	return show;
}

bool Preferences::doShowWeatherInDock() { return boolForKey(kDoShowWeatherInDock); }

void Preferences::setShowWeatherInDock(unsigned char show)
{
	if (setBoolReportingChange(kDoShowWeatherInDock, show))
		post(kShowWeatherInDockStateWasChanged);
}

void Preferences::setShowBadVideoAlert(unsigned char show) { setBoolIfChanged(kDoShowBadVideoAlert, show); }

bool Preferences::doShowBadVideoAlert() { return boolForKey(kDoShowBadVideoAlert); }

void Preferences::setShowCloudsDownloadFailedAlert(unsigned char show)
{
	setBoolIfChanged(kDoShowCloudsDwnldFailedAlert, show);
}

bool Preferences::doShowCloudsDownloadFailedAlert() { return boolForKey(kDoShowCloudsDwnldFailedAlert); }

void Preferences::setShowUnsupportedOSAlert(unsigned char show) { setBoolIfChanged(kDoShowUnsupportedOSAlert, show); }

bool Preferences::doShowUnsupportedOSAlert() { return boolForKey(kDoShowUnsupportedOSAlert); }

void Preferences::setShowUnsupportedColorAlert(unsigned char show)
{
	setBoolIfChanged(kDoShowUnsupportedColorAlert, show);
}

bool Preferences::doShowUnsupportedColorAlert() { return boolForKey(kDoShowUnsupportedColorAlert); }

bool Preferences::usesMetricSystem() { return boolForKey(kUsesMetricSystem); }

void Preferences::useMetricSystem(unsigned char metric)
{
	if (setBoolReportingChange(kUsesMetricSystem, metric))
		post(kUnitsOfMeasureWereChanged);
}

bool Preferences::uses24HourClock() { return boolForKey(kUses24HourClock); }

void Preferences::use24HourClock(unsigned char use24)
{
	if (setBoolReportingChange(kUses24HourClock, use24))
		post(kClockFormatWasChanged);
}

TInternetState Preferences::internetState() { return (TInternetState)intForKey(kInternetState); }

bool Preferences::usesInternet() { return internetState() == kInternetStateOnline; }

void Preferences::setInternetState(TInternetState state)
{
	int old = intForKey(kInternetState);
	setInt(kInternetState, state);
	if (old != state)
		_postInternetStateNotification();
}

void Preferences::useSoftwareUpdating(unsigned char use) { setBoolIfChanged(kUsesSoftwareUpdating, use); }

bool Preferences::usesSoftwareUpdating() { return boolForKey(kUsesSoftwareUpdating); }

void Preferences::setPlaySounds(unsigned char play)
{
	if (setBoolIfChanged(kPlaySounds, play))
		post(kPlaySoundsStateWasChanged);
}

bool Preferences::playSounds() { return boolForKey(kPlaySounds); }

void Preferences::setRetainLastView(unsigned char retain) { setBoolIfChanged(kRetainLastView, retain); }

bool Preferences::retainLastView() { return boolForKey(kRetainLastView); }

bool Preferences::showIntroOnStartup() { return showDemoOnStartup() || !retainLastView(); }

void Preferences::setupBeforeMainWindowLoad()
{
	LUserDefaults *ud = UD();
	LObject *scale = ud->objectForKey(kVertScale);
	if (scale)
		_sVertScaleFactor = loadFloatFromObject(scale);
	if (retainLastView())
		return;
	LString *viewKeys[] = {kSpaceViewFlags,
	                       kShowPointsFlags,
	                       kShowNearestPointName,
	                       kAlwaysHighlightBordersEnabled,
	                       kDayNightEnabled,
	                       kShowScaleStraightedge,
	                       kShowLocationNamesEnabled,
	                       kShowPlanetsNamesEnabled,
	                       kShowConstellationsNamesEnabled,
	                       kShowWeatherOnMapEnabled,
	                       kShowImagesOnMapEnabled,
	                       kShowAllBorders,
	                       kShowAppleStores,
	                       kShowRouteLines,
	                       kShowRouteMarkers,
	                       kGridEnabled,
	                       kCloudsEnabled,
	                       kMouseBehaviourOnMap,
	                       kMarkersScale,
	                       kAutomoveSpeed,
	                       kMapMode,
	                       kMinimapMode,
	                       kShowInfoDevice,
	                       kShowNavDevice,
	                       kToggledNavigationControl,
	                       kEarthTiltedMode,
	                       kGlobeTransparencyLevel,
	                       kCloudsTimeStep};
	for (LString *key : viewKeys)
		ud->removeObjectForKey(key);
	ud->removeObjectForKey(shapeKey(CShapeManager::kShapeIDCountries));
	ud->removeObjectForKey(shapeKey(CShapeManager::kShapeIDStates));
	ud->removeObjectForKey(shapeKey(CShapeManager::kShapeIDTimezones));
}

void Preferences::registerDefaults(unsigned char screenSaver)
{
	StAutoreleasePool pool;
	CFLocaleRef locale = CFLocaleCopyCurrent();
	bool metric = CFEqual(CFLocaleGetValue(locale, kCFLocaleMeasurementSystem), CFSTR("Metric"));
	CFRelease(locale);
	bool ss = screenSaver != 0;

	LMap *defaults = LMap::mapWithObjectsAndKeys(
	    LValue::valueWithInt(0), kScreenSaverBillboardsEnabled,
	    LValue::valueWithInt(0), kScreenSaverTiltModeEnabled,
	    LValue::valueWithInt(0), kScreenSaverDisplayMiniMapEnabled,
	    LValue::valueWithInt(0), kScreenSaverDisplayRegionNamesEnabled,
	    LValue::valueWithFloat(0.5f), kScreenSaverCustomAltitudeValue,
	    LValue::valueWithInt(0), kScreenSaverAltitudeMode,
	    LValue::valueWithInt(0), kScreenSaverPointOfViewMode,
	    LValue::valueWithFloat(0.5f), kMarkersScale,
	    LValue::valueWithFloat(0.1f), kCloudsTimeStep,
	    LValue::valueWithBoolean(1), kDoShowUnsupportedColorAlert,
	    LValue::valueWithBoolean(1), kDoShowUnsupportedOSAlert,
	    LValue::valueWithInt(7), kExpandedForecastDaysCount,
	    LValue::valueWithBoolean(1), kCurrentConditionsEnabled,
	    LValue::valueWithBoolean(0), kAstronomyForecastEnabled,
	    LValue::valueWithInt(0x26), kMainPropertiesDetailViewFlags,
	    LValue::valueWithFloat(11.0f), kCIPaletteFontSize,
	    LValue::valueWithBoolean(1), kVerticalStateEnabled,
	    LValue::valueWithInt(20), kCitiesCountLimit,
	    LValue::valueWithBoolean(1), kDoShowCloudsDwnldFailedAlert,
	    LValue::valueWithBoolean(1), kDoShowBadVideoAlert,
	    LValue::valueWithBoolean(1), kDoShowWeatherOnMap,
	    LValue::valueWithBoolean(1), kDoShowWeatherInDock,
	    LValue::valueWithBoolean(0), kDoShowWeather,
	    LValue::valueWithBoolean(1), kDoShowMainWindow,
	    LValue::valueWithBoolean(1), kShowDemoOnStartup,
	    LValue::valueWithBoolean(0), kFullScreenAsDefault,
	    LValue::valueWithBoolean(1), kPlaySounds,
	    LValue::valueWithBoolean(0), kCurrentRouteSetVisible,
	    LValue::valueWithBoolean(1), kInfoAreaVisible,
	    LValue::valueWithBoolean(1), kLeftAreaVisible,
	    LValue::valueWithBoolean(1), kRetainLastView,
	    LValue::valueWithBoolean(1), kUsesSoftwareUpdating,
	    LValue::valueWithInt(kInternetStateOnline), kInternetState,
	    LValue::valueWithBoolean(0), kUses24HourClock,
	    LValue::valueWithBoolean(metric), kUsesMetricSystem,
	    LValue::valueWithBoolean(1), kReliefEnabled,
	    LValue::valueWithBoolean(1), kAnimWaterLargeTexture,
	    LValue::valueWithBoolean(1), kAnimWaterEnabled,
	    LValue::valueWithInt(kRenderingQualityCustom), kRenderingQuality,
	    LValue::valueWithBoolean(1), kHiQualityMap,
	    LValue::valueWithBoolean(0), kShowScaleStraightedge,
	    LValue::valueWithBoolean(0), kEarthTiltedMode,
	    LValue::valueWithBoolean(0), kHidesWFPaletteOnDeactivate,
	    LValue::valueWithFloat(0.9f), kCITransparencyLevel,
	    LValue::valueWithFloat(ss ? 0.0f : 0.8f), kSoundVolume,
	    LValue::valueWithFloat(ss ? 0.0f : 0.2f), kGlobeTransparencyLevel,
	    LValue::valueWithInt(ss ? 0 : 3), kToggledNavigationControl,
	    LValue::valueWithBoolean(1), kShowNavDevice,
	    LValue::valueWithBoolean(0), kShowInfoDevice,
	    LValue::valueWithInt(ss), kMinimapMode,
	    LValue::valueWithInt(0), kMapMode,
	    LValue::valueWithBoolean(1), kZoomMainWindow,
	    LValue::valueWithFloat(0.3f), kAutomoveSpeed,
	    LValue::valueWithInt(kMapMouseModeDefault), kMouseBehaviourOnMap,
	    LValue::valueWithInt(ss ? 0 : 2), kCloudsAnimation,
	    LValue::valueWithInt(kCloudsDetalization2048), kCloudsDetalization,
	    LValue::valueWithBoolean(0), kCloudsEnabled,
	    LValue::valueWithBoolean(0), kGridEnabled,
	    LValue::valueWithBoolean(1), kShowRouteMarkers,
	    LValue::valueWithBoolean(1), kShowRouteLines,
	    LValue::valueWithBoolean(0), kShowAppleStores,
	    LValue::valueWithBoolean(0), shapeKey(CShapeManager::kShapeIDTimezones),
	    LValue::valueWithBoolean(!ss), shapeKey(CShapeManager::kShapeIDStates),
	    LValue::valueWithBoolean(!ss), shapeKey(CShapeManager::kShapeIDCountries),
	    LValue::valueWithBoolean(ss), kShowAllBorders,
	    LValue::valueWithBoolean(0), kShowImagesOnMapEnabled,
	    LValue::valueWithBoolean(ss), kShowWeatherOnMapEnabled,
	    LValue::valueWithBoolean(!ss), kShowConstellationsNamesEnabled,
	    LValue::valueWithBoolean(!ss), kShowPlanetsNamesEnabled,
	    LValue::valueWithBoolean(!ss), kShowLocationNamesEnabled,
	    LValue::valueWithFloat(0.6f), kTerrainLOD,
	    LValue::valueWithBoolean(0), kDayNightEnabled,
	    LValue::valueWithBoolean(0), kAlwaysHighlightBordersEnabled,
	    LValue::valueWithBoolean(0), kShowNearestPointName,
	    LValue::valueWithInt(ss ? kPointsAll : kPointsKinds), kShowPointsFlags,
	    LValue::valueWithInt(kSpaceKinds), kSpaceViewFlags,
	    (LObject *)0);
	UD()->registerDefaults(defaults);
	setupBeforeMainWindowLoad();
	_cloudsAnimationQualityFlag = cloudsAnimationQuality();
	_dayNightIsEnabledFlag = dayNightIsEnabled();
	_animatedWaterIsEnabledFlag = animatedWaterEnabled();
}

void Preferences::setCurrentRouteSetVisible(unsigned char visible) { setBoolIfChanged(kCurrentRouteSetVisible, visible); }

bool Preferences::isCurrentRouteSetVisible() { return boolForKey(kCurrentRouteSetVisible); }

void Preferences::setInfoAreaVisible(unsigned char visible) { setBoolIfChanged(kInfoAreaVisible, visible); }

bool Preferences::isInfoAreaVisible() { return boolForKey(kInfoAreaVisible); }

void Preferences::setLeftAreaVisible(unsigned char visible) { setBoolIfChanged(kLeftAreaVisible, visible); }

bool Preferences::isLeftAreaVisible() { return boolForKey(kLeftAreaVisible); }

LMap *Preferences::lastRouteDictionaryRep() { return lobj_cast<LMap>(valueForKey(kLastRoute)); }

void Preferences::setLastRouteDictionaryRep(LMap *rep)
{
	if (!rep)
		UD()->removeObjectForKey(kLastRoute);
	else
		UD()->setObjectForKey(rep, kLastRoute);
}

TMapMode Preferences::mapMode() { return (TMapMode)intForKey(kMapMode); }

void Preferences::setMapMode(TMapMode mode)
{
	setInt(kMapMode, mode);
	post(kDrawOptionsWereChanged);
}

TMinimapMode Preferences::minimapMode() { return (TMinimapMode)intForKey(kMinimapMode); }

void Preferences::setMinimapMode(TMinimapMode mode) { setInt(kMinimapMode, mode); }

void Preferences::setShowInfoDeviceEnabled(unsigned char enabled) { setBoolIfChanged(kShowInfoDevice, enabled); }

bool Preferences::showInfoDeviceEnabled() { return _overolader ? 0 : boolForKey(kShowInfoDevice); }

void Preferences::setShowNavDeviceEnabled(unsigned char enabled) { setBoolIfChanged(kShowNavDevice, enabled); }

bool Preferences::showNavDeviceEnabled() { return _overolader ? 0 : boolForKey(kShowNavDevice); }

TNavControlID Preferences::toggledNavigationControl()
{
	return (TNavControlID)intForKey(kToggledNavigationControl);
}

void Preferences::setToggledNavigationControl(TNavControlID control) { setInt(kToggledNavigationControl, control); }

LString *Preferences::homeCityUID() { return (LString *)valueForKey(kHomeCityUID); }

PointEntry *Preferences::_homeCity()
{
	LString *uid = homeCityUID();
	if (uid)
		return lobj_cast<PointEntry>(GenericEntry::entryWithUID(uid));
	return 0;
}

PointEntry *Preferences::homeCity()
{
	PointEntry *city = _homeCity();
	if (city)
		return city;
	if (IModelCallbacks::_callbacks)
		IModelCallbacks::_callbacks->_showSelectCityDialog();
	return _homeCity();
}

void Preferences::setHomeCityUID(LString *uid)
{
	LUserDefaults *ud = UD();
	LObject *old = ud->objectForKey(kHomeCityUID);
	bool changed = !old || !old->isEqualTo(uid);
	ud->setObjectForKey(uid, kHomeCityUID);
	PointEntry *city = lobj_cast<PointEntry>(GenericEntry::entryWithUID(uid));
	if (!city)
		return;
	LString *name = city->ownName(0);
	city->stateName(0);
	LString *country = city->countryName(0);
	if (country && name)
		ud->setObjectForKey(LString::stringWithFormat("%s, %s", name->cString(), country->cString()), kHomeCityName);
	if (changed)
		post(kHomeCityWasChanged);
	ud->synchronize();
}

int Preferences::citiesCountLimit() { return intForKey(kCitiesCountLimit); }

float Preferences::paletteFontSize() { return floatForKey(kCIPaletteFontSize); }

void Preferences::setPaletteFontSize(float size)
{
	float old = paletteFontSize();
	setFloat(kCIPaletteFontSize, size);
	if (old != size)
		post(kCIPaletteFontSizeWasChanged);
}

LArray *Preferences::entriesList()
{
	LArray *reps = lobj_cast<LArray>(valueForKey(kCIPaletteCities));
	LArray *entries = LArray::array();
	if (reps) {
		int count = reps->count();
		for (int i = 0; i < count; i++) {
			LMap *rep = lobj_cast<LMap>(reps->objectAtIndex(i));
			if (rep) {
				GenericEntry *entry = GenericEntry::entryWithDictionaryRep(rep);
				if (entry)
					entries->addObject(entry);
			}
		}
	}
	if (entries->count() == 0)
		entries->addObject(homeCity());
	return entries;
}

void Preferences::setEntriesList(TLArray<GenericEntry *> *entries)
{
	LArray *reps = LArray::array();
	int count = entries ? entries->count() : 0;
	for (int i = 0; i < count; i++)
		reps->addObject(entries->objectAtIndex(i)->entryDictionary());
	UD()->setObjectForKey(reps, kCIPaletteCities);
}

bool Preferences::verticalStateEnabled() { return boolForKey(kVerticalStateEnabled); }

void Preferences::setVerticalStateEnabled(unsigned char enabled) { setBoolIfChanged(kVerticalStateEnabled, enabled); }

int Preferences::mainPropertiesDetailViewFlags() { return intForKey(kMainPropertiesDetailViewFlags); }

void Preferences::setMainPropertiesDetailViewFlags(int flags)
{
	LObject *old = valueForKey(kMainPropertiesDetailViewFlags);
	if (old && loadIntFromObject(old) == flags)
		return;
	setInt(kMainPropertiesDetailViewFlags, flags);
	post(kMainPropertiesStateWasChanged);
}

bool Preferences::astronomyForecastEnabled() { return boolForKey(kAstronomyForecastEnabled); }

void Preferences::setAstronomyForecastEnabled(unsigned char enabled)
{
	setBoolIfChanged(kAstronomyForecastEnabled, enabled);
}

bool Preferences::currentConditionsEnabled() { return boolForKey(kCurrentConditionsEnabled); }

void Preferences::setCurrentConditionsEnabled(unsigned char enabled)
{
	if (setBoolIfChanged(kCurrentConditionsEnabled, enabled))
		post(kCurrentConditionsStateWasChanged);
}

TForecastDaysCount Preferences::expandedForecastDaysCount()
{
	LObject *value = valueForKey(kExpandedForecastDaysCount);
	return (TForecastDaysCount)(value ? loadIntFromObject(value) : 7);
}

void Preferences::setExpandedForecastDaysCount(TForecastDaysCount count)
{
	LObject *old = valueForKey(kExpandedForecastDaysCount);
	if (old && loadIntFromObject(old) == count)
		return;
	setInt(kExpandedForecastDaysCount, count);
	post(kExpandedForecastStateWasChanged);
}

TScreenSaverPointOfViewMode Preferences::screenSaverPointOfViewMode()
{
	return (TScreenSaverPointOfViewMode)intForKey(kScreenSaverPointOfViewMode);
}

void Preferences::setScreenSaverPointOfViewMode(TScreenSaverPointOfViewMode mode)
{
	setInt(kScreenSaverPointOfViewMode, mode);
}

TScreenSaverAltitudeMode Preferences::screenSaverAltitudeMode()
{
	return (TScreenSaverAltitudeMode)intForKey(kScreenSaverAltitudeMode);
}

void Preferences::setScreenSaverAltitudeMode(TScreenSaverAltitudeMode mode) { setInt(kScreenSaverAltitudeMode, mode); }

float Preferences::screenSaverCustomAltitudeValue() { return floatForKey(kScreenSaverCustomAltitudeValue); }

void Preferences::setScreenSaverCustomAltitudeValue(float value) { setFloat(kScreenSaverCustomAltitudeValue, value); }

bool Preferences::screenSaverDisplayRegionNamesEnabled() { return intForKey(kScreenSaverDisplayRegionNamesEnabled) != 0; }

void Preferences::setScreenSaverDisplayRegionNamesEnabled(unsigned char enabled)
{
	setInt(kScreenSaverDisplayRegionNamesEnabled, enabled);
}

bool Preferences::screenSaverDisplayMiniMapEnabled() { return intForKey(kScreenSaverDisplayMiniMapEnabled) != 0; }

void Preferences::setScreenSaverDisplayMiniMapEnabled(unsigned char enabled)
{
	setInt(kScreenSaverDisplayMiniMapEnabled, enabled);
}

bool Preferences::screenSaverTiltModeEnabled() { return intForKey(kScreenSaverTiltModeEnabled) != 0; }

void Preferences::setScreenSaverTiltModeEnabled(unsigned char enabled) { setInt(kScreenSaverTiltModeEnabled, enabled); }

bool Preferences::screenSaverBillboardsEnabled() { return intForKey(kScreenSaverBillboardsEnabled) != 0; }
