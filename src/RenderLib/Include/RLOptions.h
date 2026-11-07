// Render options: what a scene draws. The scenes ask their options object;
// RenderOptionsFromPrefs answers from the preferences, the screen saver's
// RenderOptionsSaver hides the overlays, and RenderOptionsForPrinting draws
// only the layers its flags ask for.

#ifndef RLOPTIONS_H
#define RLOPTIONS_H

#include "AppModel.h"

class RenderOptions : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_ABSTRACT_CLASS(RenderOptions)
public:
	// The holder key of the scene's options.
	static LString *kRenderOptionsContextKey;

	RenderOptions();
	virtual bool showCaptions() { return _showCaptions; }
	virtual bool isPrinting() { return false; }
	virtual bool renderEarth() { return true; }
	virtual bool showOverlays() { return true; }
	virtual bool showAbout() { return false; }
	virtual bool showClouds() = 0;
	virtual bool showGrid() = 0;
	virtual bool showWeatherOnMap() = 0;
	virtual bool showPhotosOnMap() = 0;
	virtual bool showAllPoints() = 0;
	virtual bool showAllBorders() = 0;
	virtual bool highlightBordersAnyway() = 0;
	virtual bool showTimeZoneEnabled() = 0;
	virtual LComparableSet *shapesIDsToDraw() = 0;
	virtual bool renderWithDayNight() = 0;
	virtual bool showLocationNamesInSet() = 0;
	virtual bool showPlanetsNamesInSet() = 0;
	virtual bool showConstellationsNamesInSet() = 0;
	virtual bool showLinesInSet() = 0;
	virtual bool showMarkers() = 0;
	virtual float markersScale() = 0;
	virtual bool showScale() = 0;
	virtual bool skipSpaceAndSky() = 0;
	virtual bool spaceEnabled() = 0;
	virtual bool starsEnabled() = 0;
	virtual bool planetsEnabled() = 0;
	virtual bool constellationsEnabled() = 0;
	virtual bool sunEnabled() = 0;
	virtual bool moonEnabled() = 0;
	// The flag the shaders read for day and night.
	virtual int *_dayNightIsEnabledFlagPtr() = 0;
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *, unsigned int) {}

	void setShowCaptions(unsigned char show) { _showCaptions = show; }

	// The current options' day and night flag.
	static int *dayNightIsEnabledFlagPtr() { return sCurDayNightVarPtr; }
	static void setCurrentOptions(RenderOptions *options);

	static int *sCurDayNightVarPtr;

	// The recorded state: the day and night and clouds settings.
	bool _recordedDayNight;
	bool _recordedClouds;
	bool _showCaptions;
};

class RenderOptionsFromPrefs : public RenderOptions {
	LF_DECLARE_CLASS(RenderOptionsFromPrefs)
public:
	virtual bool showClouds() { return Preferences::cloudsEnabled(); }
	virtual bool showGrid() { return Preferences::gridEnabled(); }
	virtual bool showWeatherOnMap() { return Preferences::showWeatherOnMapEnabled(); }
	virtual bool showPhotosOnMap() { return Preferences::showImagesOnMapEnabled(); }
	virtual bool showAllPoints() { return Preferences::showAllPoints(); }
	virtual bool showAllBorders() { return Preferences::showAllBorders(); }
	virtual bool highlightBordersAnyway() { return Preferences::highlightBordersAnyway(); }
	virtual bool showTimeZoneEnabled() { return Preferences::showTimeZoneEnabled(); }
	virtual LComparableSet *shapesIDsToDraw() { return Preferences::shapesIDsToDraw(); }
	virtual bool renderWithDayNight() { return Preferences::dayNightIsEnabled(); }
	virtual bool showLocationNamesInSet() { return Preferences::showLocationNamesEnabled(); }
	virtual bool showPlanetsNamesInSet() { return Preferences::showPlanetsNamesEnabled(); }
	virtual bool showConstellationsNamesInSet() { return Preferences::showConstellationsNamesEnabled(); }
	virtual bool showLinesInSet() { return Preferences::showRouteLines(); }
	virtual bool showMarkers() { return Preferences::showRouteMarkers(); }
	virtual float markersScale() { return Preferences::markersScale(); }
	virtual bool showScale() { return Preferences::showScaleStraightedge(); }
	virtual bool skipSpaceAndSky() { return false; }
	virtual bool spaceEnabled() { return Preferences::spaceEnabled(); }
	virtual bool starsEnabled() { return Preferences::starsEnabled(); }
	virtual bool planetsEnabled() { return Preferences::planetsEnabled(); }
	virtual bool constellationsEnabled() { return Preferences::constellationsEnabled(); }
	virtual bool sunEnabled() { return Preferences::sunEnabled(); }
	virtual bool moonEnabled() { return Preferences::moonEnabled(); }
	virtual int *_dayNightIsEnabledFlagPtr() { return &Preferences::_dayNightIsEnabledFlag; }
	// Plays back the recorded day and night setting.
	virtual void renderSelfHolderInScene(CScene *, unsigned int) { Preferences::setDayNightEnabled(_recordedDayNight); }
	virtual RenderOptionsFromPrefs *getOwnerLObject() { return this; }
};

class RenderOptionsSaver : public RenderOptionsFromPrefs {
	LF_DECLARE_CLASS(RenderOptionsSaver)
public:
	virtual bool showOverlays() { return false; }
	virtual bool showAbout() { return false; }
	virtual RenderOptionsSaver *getOwnerLObject() { return this; }

	// Autoreleased.
	static RenderOptionsSaver *options();
};

enum {
	kPrintEarth = 1,
	kPrintClouds = 2,
	kPrintSpace = 8,
};

class RenderOptionsForPrinting : public RenderOptions {
	LF_DECLARE_CLASS(RenderOptionsForPrinting)
public:
	RenderOptionsForPrinting() : _flags(0) {}
	virtual bool isPrinting() { return true; }
	virtual bool renderEarth() { return _flags & kPrintEarth; }
	virtual bool showOverlays() { return false; }
	virtual bool showClouds() { return _flags & kPrintClouds; }
	virtual bool showGrid() { return false; }
	virtual bool showWeatherOnMap() { return false; }
	virtual bool showPhotosOnMap() { return false; }
	virtual bool showAllPoints() { return false; }
	virtual bool showAllBorders() { return false; }
	virtual bool highlightBordersAnyway() { return false; }
	virtual bool showTimeZoneEnabled() { return false; }
	virtual LComparableSet *shapesIDsToDraw() { return 0; }
	virtual bool renderWithDayNight() { return Preferences::dayNightIsEnabled(); }
	virtual bool showLocationNamesInSet() { return false; }
	virtual bool showPlanetsNamesInSet() { return false; }
	virtual bool showConstellationsNamesInSet() { return false; }
	virtual bool showLinesInSet() { return false; }
	virtual bool showMarkers() { return false; }
	virtual float markersScale() { return Preferences::markersScale(); }
	virtual bool showScale() { return false; }
	// No sky at all when neither space nor clouds are printed.
	virtual bool skipSpaceAndSky() { return (_flags & (kPrintSpace | kPrintClouds)) == 0; }
	virtual bool spaceEnabled() { return _flags & kPrintSpace; }
	virtual bool starsEnabled() { return Preferences::starsEnabled(); }
	virtual bool planetsEnabled() { return Preferences::planetsEnabled(); }
	virtual bool constellationsEnabled() { return false; }
	virtual bool sunEnabled() { return Preferences::sunEnabled(); }
	virtual bool moonEnabled() { return Preferences::moonEnabled(); }
	virtual int *_dayNightIsEnabledFlagPtr() { return &Preferences::_dayNightIsEnabledFlag; }
	virtual RenderOptionsForPrinting *getOwnerLObject() { return this; }

	RenderOptionsForPrinting *initWithFlags(unsigned int flags)
	{
		init();
		_flags = flags;
		return this;
	}

	unsigned int _flags;	// kPrint...
};

#endif
