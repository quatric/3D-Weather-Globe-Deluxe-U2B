// The window side of RenderLib: the interface the application's window
// implements, its base implementation that owns the 3D view and the panel
// controllers, and the controllers that turn input into camera parameters.

#ifndef RLPANELS_H
#define RLPANELS_H

#include "RLScenes.h"

class AtlasPanelController;
class Custom3DView;
class _LogoSceneEndSwitcher;
class LogoScene;
class WeatherTracer;

enum TEarthLockMode { kEarthLockNone = 0, kEarthLockNormal = 1, kEarthLockTilted = 2 };
enum TCompassValue { kCompassValueNone = 0 };
enum TMouseCursorType {
	kCursorArrow = 0,
	kCursorOpenHand = 1,
	kCursorClosedHand = 2,
	kCursorZoomIn = 3,
	kCursorZoomOut = 4,
	kCursorPlaceEntry = 5,
	kCursorCrosshair = 6,
};

// ---------------------------------------------------------------------------
// The application's window, as the panels see it.

class IWindowInterface {
public:
	virtual C3DView *acceleratedView() = 0;
	virtual LSize acceleratedViewSize() = 0;
	virtual void refreshAcceleratedView() = 0;
	virtual SharedContextObjectsContainer *contextObjectsContainer() = 0;
	virtual void toogleFullScreen() = 0;
	virtual bool isInFullScreenMode() = 0;
	virtual bool entryIsMarked(GenericEntry *entry) = 0;
	virtual void appendEntryToSet(GenericEntry *entry) = 0;
	// Logs the entry.
	virtual void entryWasClicked(GenericEntry *entry, LPoint, TBillboardKind);
	virtual bool isInCoordinatesPickerMode() = 0;
	virtual void locationWasPicked(LPoint geoPoint) = 0;
	virtual void setAnimatedMarkerPosition(LPoint geoPoint) = 0;
	virtual LPoint animatedMarkerPosition(unsigned char *visible) = 0;
	virtual void stopAnyFlying() = 0;
	virtual void setScaleValue(float scale) = 0;
	virtual void setMinimapWorldRect(LRect rect) = 0;
	virtual void compassValueChangedTo(TCompassValue value) = 0;
	virtual void setMouseCursorType(TMouseCursorType type) = 0;
	virtual void showPopupEntryInfo(GenericEntry *entry) = 0;
	virtual bool isPopupEntryInfoVisible() = 0;
	virtual void hidePopupEntryInfo() = 0;
	virtual ~IWindowInterface() {}
	virtual void startIntro() = 0;
	virtual void startDemo(unsigned char saver) = 0;
	virtual void enterFullScreenMode() = 0;
	virtual void exitFullScreenMode() = 0;
	virtual void setCoordinatesPickerMode(unsigned char on) = 0;
	virtual void hideAnimatedMarker() = 0;

	// Posted by the accelerated view when its frame changes; object the view.
	static LString *frameWasChangedNotification;
};

// The window's 3D view and its panel controllers, one per map mode.
class BaseWindowInterface : public IWindowInterface {
public:
	BaseWindowInterface(void *nativeView, CDeviceFormat *format);
	virtual ~BaseWindowInterface();
	virtual C3DView *acceleratedView();
	virtual LSize acceleratedViewSize();
	virtual void refreshAcceleratedView();
	virtual SharedContextObjectsContainer *contextObjectsContainer();
	virtual bool isInFullScreenMode() { return _fullScreen; }
	virtual bool isInCoordinatesPickerMode() { return _coordinatesPicker; }
	virtual void setAnimatedMarkerPosition(LPoint geoPoint);
	virtual LPoint animatedMarkerPosition(unsigned char *visible);
	virtual void startIntro();
	virtual void startDemo(unsigned char saver);
	virtual void enterFullScreenMode();
	virtual void exitFullScreenMode();
	virtual void setCoordinatesPickerMode(unsigned char on);
	virtual void hideAnimatedMarker() { _markerVisible = false; }

	CContextCaps *contextCaps();
	bool isVideoCardSupported();
	// Installs the panel of `mode` (TMapMode: 0 globe, 1 flat map, 2 and 3
	// their saver versions, 4 the logo).
	void openPanel(int mode);
	AtlasPanelController *currentPanel() const { return _panel; }

	Custom3DView *_view;
	AtlasPanelController *_panel;
	LArray *_panels;	// made so far
	_LogoSceneEndSwitcher *_logoSwitcher;
	bool _fullScreen;
	bool _coordinatesPicker;
	bool _markerVisible;
	LPoint _markerPosition;
};

// The window's 3D view: it owns the context's shared objects and redraws
// when textures arrive.
class Custom3DView : public C3DView {
	LF_DECLARE_CLASS(Custom3DView)
public:
	Custom3DView();
	virtual void dealloc();
	virtual void recreateContext(CDeviceFormat *format);
	virtual void drawAccelerated(bool postNotification, bool endFrame, bool present);
	virtual intptr_t _texturesSetWasUpdated(char *args);

	Custom3DView *initWithParams(IWindowInterface *wnd, void *systemView, CDeviceFormat *format);

	SharedContextObjectsContainer *_objectsContainer;
	LObject *_unused64;	// released, never set
};

// Leaves full screen when the intro or the saver's demo ends, and starts
// the next demo sequence.
class _LogoSceneEndSwitcher : public LObject {
	LF_DECLARE_CLASS(_LogoSceneEndSwitcher)
public:
	virtual void dealloc();
	virtual intptr_t _cinematicWasInterrupted(char *args);
	virtual intptr_t _doToggleFS(char *args);
	virtual intptr_t _demoSequenceWasEnded(char *args);

	_LogoSceneEndSwitcher *initWithBaseInterface(BaseWindowInterface *wnd);

	BaseWindowInterface *_window;
	bool _intro;
	bool _toggleFullScreen;	// at the end: the saver's demo runs full screen
};

// ---------------------------------------------------------------------------
// AtlasPanelController: the camera parameters of a panel and their
// animations (slides, zooms, fades of the on-screen devices), and the mouse
// and keyboard handling.

class AtlasPanelController : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(AtlasPanelController)
public:
	typedef PanelDrawIterator drawIterator;
	typedef PanelDrawIteratorSimple drawIteratorSimple;

	// The camera: map position, compass angle and tilt, scale, and the
	// spin.
	struct TParamsHolder {
		void clampSelf(const TParamsHolder *min, const TParamsHolder *max);

		float x, y;	// map point
		float angle;	// 0..1
		float tilt;	// -1..1
		float scale;	// 0..1
		float spin;
	};
	enum TAttenuationFactorType { kAttenuationMove = 1 };

	// _animFlags
	enum {
		kAnimControlDown = 0x2,	// a navigation control is held
		kAnimScaleTimer = 0x4,
		kAnimSliding = 0x8,
		kAnimInertia = 0x10,
		kAnimZooming = 0x40,
		kAnimMinimapFade = 0x80,
		kAnimNavFade = 0x100,
		kAnimInfoFade = 0x200,
		kAnimTracking = 0x400,
		kAnimKeyboard = 0x800,
		kAnimPreviewFade = 0x1000,
	};
	// Navigation device controls (TNavControlID).
	enum {
		kNavMoveUp = 1,
		kNavMoveDown = 2,
		kNavMoveLeft = 3,
		kNavMoveRight = 4,
		kNavTiltUp = 5,
		kNavTiltDown = 6,
		kNavTurnLeft = 7,
		kNavTurnRight = 8,
		kNavZoomIn = 9,
		kNavZoomOut = 10,
		kNavReset = 11,
		kNavCompass = 12,
		kNavStop = 13,
	};

	static LString *kCinematicStoppedNotification;
	static LString *kCountdownFinishedNotification;
	static LString *kDemoSequenceWasEndedNotification;

	AtlasPanelController();
	virtual void dealloc();
	virtual AtlasPanelController *initWithMainWindowInterface(IWindowInterface *wnd);
	virtual void invalidate();
	virtual void resetPosScaleTimer();
	virtual void refreshView();
	virtual void sceneWillBeInstalled();
	virtual void sceneWasUninstalled();
	virtual void resetView();
	virtual void updateCursor();
	virtual void stopSounds();
	virtual BaseScene *scene() = 0;
	virtual drawIterator *detachDrawIterator(const LSize *size);
	virtual IWindowInterface *windowInterface() { return _window; }
	virtual LObject *detachPrintObjectWithFrame(LRect, unsigned int) { return 0; }
	virtual void setPosition(LPoint mapPoint);
	virtual void setScale(float scale);
	virtual void setScaleFromUI(float scale);
	virtual float compassAngle() { return 0.0f; }
	virtual void setCompassAngle(float) {}
	virtual float compassTrapetionFactor() { return 0.0f; }
	virtual void setLockMode(TEarthLockMode mode) { _lockMode = mode; }
	virtual void dragBringsEntries(LData *) {}
	virtual void dragExitedWithAllEntries() {}
	virtual LString *currentCaption();
	virtual LString *shapesCaption();
	virtual float geoScale() = 0;
	virtual void slideToEntry(GenericEntry *entry);
	virtual void zoomToEntry(GenericEntry *entry, unsigned char tilt);
	virtual void zoomToSelectionRect(LRect rect);
	virtual void zoomToGeoRect(LRect rect, unsigned char tilt);
	virtual void zoomToCurrentShape();
	virtual void installButtonsIfNeeded();
	virtual void uninstallButtonsIfNeeded();
	virtual void refreshCameraSettings();
	virtual void startFlying();
	virtual void stopFlying();
	virtual void startIntro();
	virtual void startDemoSequence();
	virtual void onCountdownFinished();
	virtual void startCapturing();
	virtual void stopCapturing();
	virtual void playSequence();
	virtual void stopSequence();
	virtual LMap *collectSnapshotInfo();
	virtual void installSnapshotInfo(LMap *info);
	virtual bool processMessage(unsigned int, unsigned int, unsigned int) { return false; }
	virtual void mouseDown(unsigned int flags, LPoint point);
	virtual void mouseMove(unsigned int flags, LPoint point);
	virtual void mouseUp(unsigned int flags, LPoint point);
	virtual void onMouseLeave(unsigned int flags, LPoint point);
	virtual void mouseDblClick(unsigned int flags, LPoint point);
	virtual void mouseWheel(short delta, unsigned int flags, LPoint point);
	virtual void mouseDragged(unsigned int flags, LPoint point);
	virtual void dragStarted(unsigned int flags, LPoint point);
	virtual void dragFinished(unsigned int flags, LPoint point);
	virtual bool onKeyDown(unsigned int modifiers, unsigned int key);
	virtual bool onKeyUp(unsigned int modifiers, unsigned int key);
	virtual bool becameInFocus() { return true; }
	virtual bool resignFocus();
	virtual void singleActionOnGeoPoint(LPoint geoPoint);
	virtual void doubleActionOnGeoPoint(LPoint geoPoint);
	virtual void startTrackingAtViewPoint(LPoint point);
	virtual void trackToViewPoint(LPoint point);
	virtual void endTrackingAtViewPoint(LPoint point);
	virtual void mousePositionChanged(LPoint geoPoint);
	virtual void mouseLost();
	virtual bool canConvertNormalizedViewPointToGeoPoint(LPoint, LPoint *) { return false; }
	virtual float attenuationFactorFromScale(TAttenuationFactorType) { return 1.0f; }
	virtual float currentAltitude() = 0;
	virtual float gridStepFromScale(unsigned char latitude);
	virtual int shapesLODFromScale();
	virtual float zoomScaleFromBaseScale(float scale) = 0;
	virtual float zoomScaleToFitGeoRect(LRect geoRect) = 0;
	// The visible part of the map, in map widths and heights.
	virtual LSize visibleParts() = 0;
	virtual LRect minimapTextureRect();
	virtual LRect minimapMapRect();
	virtual LRect navDeviceRect();
	virtual LRect infoDeviceRect(unsigned char full);
	virtual intptr_t _onTimer(char *args);
	virtual intptr_t _delayedCurrentCitySelect(char *args);
	virtual intptr_t _timerOnFirstClickProcess(char *args);
	virtual intptr_t _timerOnShowPopupInfo(char *args);
	virtual intptr_t _timerOnHidePopupInfo(char *args);
	virtual intptr_t _accelViewFrameDidChange(char *args);
	virtual intptr_t _worldTimeWasChanged(char *args);
	virtual intptr_t _playSoundsStateWasChanged(char *args);
	virtual intptr_t _enabledBordersSetWasChanged(char *args);
	virtual intptr_t _imagesSelectionWasChanged(char *args);
	virtual intptr_t _onDBUpdateFinished(char *args);
	virtual intptr_t _onUIButton(char *args);
	virtual intptr_t _selectNextDemoSequence(char *args);
	virtual void _setupTimer();
	virtual void recacheAll() {}
	virtual void recacheNeighborsOnly() {}
	virtual void doInstallButtons();
	virtual void doUninstallButtons();
	virtual void _terminateCinematics();
	virtual TNavControlID controlIDFromScreenPosition(LPoint point);
	virtual void highlightControlWithID(TNavControlID) {}
	virtual void unhighlightControlWithID(TNavControlID) {}
	virtual void controlDown(TNavControlID control);
	virtual void controlAction(LPoint point, TNavControlID control);
	virtual void controlUp(TNavControlID control);
	virtual TMouseCursorType _mouseCursorType();
	virtual float _gridStepScaleFactor(unsigned char) { return 1.0f; }
	virtual float _interpolationDistance(const TParamsHolder *from, const TParamsHolder *to);
	virtual void _interpolateSlidingParams(const TParamsHolder *from, const TParamsHolder *to, TParamsHolder *out,
	                                       float t);
	virtual void _adjustToSlidingProgress(float, const TParamsHolder *) {}
	virtual void _moveWithDelta(LPoint delta);
	virtual void _tiltWithDelta(LPoint) {}
	virtual void _directDragStarted();
	virtual void _directDragWithDiff(LPoint) {}
	virtual void _evalParameterForces(TParamsHolder *forces, TParamsHolder *min, TParamsHolder *max);
	virtual void _addKeyboardForces(TParamsHolder *forces);
	virtual void _developParametersWithDeltaTime(float dt);

	float scale() const { return _params.scale; }
	LPoint position() const { LPoint p = {_params.x, _params.y}; return p; }
	void _updateMinimapWeigths();
	void _resetKBFlags();
	void startCountdown(int seconds);
	void stopCountdown();
	static void _reinstallDemoTimer() {}
	bool isCapturing();
	bool isSequencePlaying();
	bool sequenceWasRecorded();
	void _showPopupInfoForEntry(GenericEntry *entry);
	float minimapVisibility();
	float infoDeviceVisibility();
	float navDevicesVisibility() const { return _navVisibility; }
	float imagesPreviewVisibility() const { return _previewVisibility; }
	void setCurrentEntry(GenericEntry *entry, unsigned char delayed);
	TNavControlID highlightedControlID() const { return _highlightedControl; }
	TNavControlID pressedControlID() const { return _pressedInside ? _pressedControl : (TNavControlID)0; }
	TNavControlID toogledControlID() const { return _toggledControl; }
	void _firstClickProcess(LPoint point, unsigned char billboard);
	void hideImagesPreview();
	LRect _selectionRectForPoint(LPoint point, LPoint start);
	LMap *shapesToHighlight() const { return _highlightedShapes; }
	void hideNavDevice();
	void showNavDevice();
	void toggleNavDevice();
	void _fadeToMinimapMode(TMinimapMode mode);
	void _hideFullMinimap();
	void _showFullMinimap();
	void cycleMinimapMode();
	void showInfo();
	void hideInfo();
	void toggleInfo();
	void showImagesPreview();
	void toggleImagesPreview();
	LString *_cameraParamsKey();
	static LPoint positionFromMinimap(LPoint point, LRect minimap);
	static LPoint minimapPositionFromPosition(LPoint position, LRect minimap);
	static LPoint navPositionFromScreenPosition(LPoint point, LRect nav);
	float _compassAngleFromScreenPos(LPoint point);
	// What a drag with the current buttons and keys does: 0 nothing, 1
	// move, 2 direct drag, 3 tilt, 4 zoom in rect, 5 zoom out. `point`
	// becomes relative to where the button went down.
	int _mouseTrackBehaviour(LPoint *point);
	LString *currentCoordsString();
	void _slideWithParams(const TParamsHolder &to, unsigned char keepTilt);
	TNavControlID navValueFromScreenPosition(LPoint point);
	int _nearestPointIndexForScreenPositionWithRadius(LData *indices, LPoint point, float radius,
	                                                  unsigned char *inRadius, float *distance);
	bool getDefaultCamParams(TParamsHolder *params);
	void _zoomWithParams(float scale, LPoint geoPoint, unsigned char tilt);
	LArray *allStaticHilitedShapes();
	LArray *_cameraParamsVal();
	void _storeCurrentCamParams();
	GenericEntry *_entryNearestToGeoPointWithScreenPosition(LPoint geoPoint, LPoint point, unsigned char *exact);
	LString *currentRegionString();

	IWindowInterface *_window;
	LMap *_highlightedShapes;	// shape ID -> shape under the mouse
	LArray *_staticHilitedShapes;
	LString *_shapesCaption;
	LString *_regionString;
	LString *_coordsString;
	int _unused2C;
	TParamsHolder _params;
	float _requestedScale;	// unclamped
	TEarthLockMode _lockMode;
	float _zoomScale;
	float _zoomStartScale;
	LPoint _zoomGeoPoint;
	LPoint _zoomStartGeoPoint;
	float _zoomStartAngle;
	float _zoomAngle;
	double _slideStart;
	float _slideDuration;
	TParamsHolder _slideFrom;
	TParamsHolder _slideTo;
	LTimer *_timer;
	double _zoomStart;
	LRect _selectionRect;
	unsigned int _animFlags;
	bool _zoomMoves;
	bool _flying;
	double _lastFlyTime;
	bool _flyAfterSlide;
	TNavControlID _highlightedControl;
	TNavControlID _pressedControl;
	TNavControlID _toggledControl;
	bool _pressedInside;
	TMinimapMode _minimapMode;
	Vector3 _minimapModeWeights[3];	// full, small, hidden by mode
	TMinimapMode _minimapTargetMode;
	Vector3 _minimapWeights;
	float _minimapHiddenWeight;
	float _minimapSmallWeight;
	float _minimapFullWeight;
	bool _minimapTracking;
	bool _fullMinimap;
	double _minimapFadeStart;
	double _navFadeStart;
	float _navVisibility;
	float _navDirection;
	float _navStartVisibility;
	double _infoFadeStart;
	float _infoVisibility;
	float _infoDirection;
	float _infoStartVisibility;
	double _previewFadeStart;
	float _previewVisibility;
	float _previewDirection;
	float _previewStartVisibility;
	bool _flyingPaused;
	double _unused17C;
	double _flyDistance;
	GenericEntry *_hoveredEntry;
	bool _hoveredBillboard;
	TBillboardKind _hoveredKind;
	bool _hoveredExact;
	LTimer *_popupTimer;
	LPoint _mouseGeoPoint;
	bool _mouseWasLost;
	bool _mouseInside;
	LTimer *_citySelectTimer;
	CSharableResource _shapesLock;
	unsigned int _buttons;
	LPoint _buttonStart[3];	// of buttons 8, 0x10, 0x20
	TParamsHolder _velocity;
	LPoint _trackPoint;
	bool _directDragging;
	LPoint _dragStart;
	LPoint _dragLast;
	double _dragTime;
	unsigned int _keys;
	bool _zoomKey;
	bool _zoomOutKey;
	bool _tiltKey;
	double _lastDevelopTime;
	float _compassGrab;
	bool _dragging;
	LPoint _firstClickPoint;
	GenericEntry *_firstClickEntry;
	bool _firstClickBillboard;
	LTimer *_firstClickTimer;
	SoundSource *_flyingSound;
	LTimer *_demoTimer;
	bool _demoAllowed;
	bool _introRunning;
	bool _sceneInstalled;
	CSceneCapturer *_capturer;
	bool _captureAfterCountdown;
	// In the i386 build, a byte in LObject's tail padding (0x11): the
	// region captions need laying out again once the camera stops.
	bool _captionsDirty;
};

// ---------------------------------------------------------------------------
// The logo shown at the start: a fixed camera, the scene plays by itself.

class LogoController : public AtlasPanelController {
	LF_DECLARE_CLASS(LogoController)
public:
	LogoController();
	virtual void dealloc();
	virtual LogoController *initWithMainWindowInterface(IWindowInterface *wnd);
	virtual void sceneWillBeInstalled();
	virtual void sceneWasUninstalled();
	virtual BaseScene *scene();
	virtual float geoScale() { return 1.0f; }
	virtual float currentAltitude() { return 1.0f; }
	virtual float zoomScaleFromBaseScale(float) { return 1.0f; }
	virtual float zoomScaleToFitGeoRect(LRect) { return 1.0f; }
	virtual LSize visibleParts() { return LZeroSize; }
	virtual void _terminateCinematics();
	virtual LogoController *getOwnerLObject() { return this; }

	LogoScene *_scene;
};

// ---------------------------------------------------------------------------
// The flat map: the position is the view's center; the scale sets the part
// of the map's height in view. The map repeats across.

class FlatMapController : public AtlasPanelController {
	LF_DECLARE_CLASS(FlatMapController)
public:
	// Steps through the copies of the map in view, left to right.
	class drawIteratorFlat : public PanelDrawIterator {
	public:
		drawIteratorFlat(LSize viewSize, LPoint position, float visiblePart);
		// For the holder's render target, which may draw one segment of
		// a larger frame.
		drawIteratorFlat(CHolder *holder, LPoint position, float visiblePart);
		virtual bool next();
		virtual float viewSpaceTranslationX() { return _mapOffset * _translationScale; }

		// The part of the map (0..1) the current copy shows of the view
		// centered on `position`, in the render target's segment.
		void evalRenderMapRect(LPoint position, LRect *out);

		int _count;
		int _index;	// -1 before the first
		float _start;	// the first copy's offset
		float _width;	// in view, in map widths
		float _visiblePart;	// in view, in map heights
		float _translationScale;
		int _segmentX;
		int _segmentY;
		int _segmentCols;
		int _segmentRows;
	};

	FlatMapController();
	virtual void dealloc();
	virtual FlatMapController *initWithMainWindowInterface(IWindowInterface *wnd);
	virtual void sceneWillBeInstalled();
	virtual void sceneWasUninstalled();
	virtual BaseScene *scene();
	virtual drawIterator *detachDrawIterator(const LSize *size);
	virtual LObject *detachPrintObjectWithFrame(LRect frame, unsigned int flags);
	virtual void setPosition(LPoint mapPoint);
	virtual void setScale(float scale);
	virtual float geoScale();
	virtual bool canConvertNormalizedViewPointToGeoPoint(LPoint point, LPoint *geoPoint);
	virtual float attenuationFactorFromScale(TAttenuationFactorType type);
	virtual float currentAltitude();
	virtual float zoomScaleFromBaseScale(float scale);
	virtual float zoomScaleToFitGeoRect(LRect geoRect);
	virtual LSize visibleParts();
	virtual float _gridStepScaleFactor(unsigned char) { return 5.5f; }
	virtual void _directDragStarted();
	virtual void _directDragWithDiff(LPoint diff);
	virtual void _addKeyboardForces(TParamsHolder *forces);
	virtual FlatMapController *getOwnerLObject() { return this; }
	// In degrees.
	virtual LRect visibleGeoRect();

	// The part of the map's height in view.
	float visiblePartFromCtrlScale();

	FlatMapScene *_scene;
	LPoint _dragStartPosition;
};

// ---------------------------------------------------------------------------
// The globe: the camera looks at the map position from above, turned by the
// compass angle (`_params.angle`), tilted by `_params.tilt` and rolled by
// `_params.spin`.

class PerspPanelController : public AtlasPanelController {
	LF_DECLARE_CLASS(PerspPanelController)
public:
	PerspPanelController();
	virtual void dealloc();
	virtual PerspPanelController *initWithMainWindowInterface(IWindowInterface *wnd);
	virtual void sceneWillBeInstalled();
	virtual void sceneWasUninstalled();
	virtual void stopSounds();
	virtual BaseScene *scene();
	virtual LObject *detachPrintObjectWithFrame(LRect frame, unsigned int flags);
	virtual void setPosition(LPoint mapPoint);
	virtual float compassAngle() { return _params.angle; }
	virtual void setCompassAngle(float angle) { setHorizAngle(angle); }
	virtual float compassTrapetionFactor();
	virtual void setLockMode(TEarthLockMode mode);
	virtual float geoScale() { return 1.0f; }
	virtual void zoomToSelectionRect(LRect rect);
	virtual LMap *collectSnapshotInfo();
	virtual void installSnapshotInfo(LMap *info);
	virtual bool canConvertNormalizedViewPointToGeoPoint(LPoint point, LPoint *geoPoint);
	virtual float attenuationFactorFromScale(TAttenuationFactorType type);
	virtual float currentAltitude();
	virtual float zoomScaleFromBaseScale(float) { return 0.0f; }
	virtual float zoomScaleToFitGeoRect(LRect geoRect);
	virtual LSize visibleParts();
	virtual void doInstallButtons();
	virtual void doUninstallButtons();
	virtual TMouseCursorType _mouseCursorType();
	virtual float _gridStepScaleFactor(unsigned char latitude);
	virtual float _interpolationDistance(const TParamsHolder *from, const TParamsHolder *to);
	virtual void _adjustToSlidingProgress(float t, const TParamsHolder *params);
	virtual void _moveWithDelta(LPoint delta);
	virtual void _tiltWithDelta(LPoint delta);
	virtual void _directDragStarted();
	virtual void _directDragWithDiff(LPoint diff);
	virtual PerspPanelController *getOwnerLObject() { return this; }
	// The compass angle, wrapped into 0..1.
	virtual void setHorizAngle(float angle);
	// The tilt, -0.996..0.996.
	virtual void setVertAngle(float tilt);
	// The roll (`_params.spin`), -1..1.
	virtual void setTiltAngle(float spin);
	// In Earth radii above the ground.
	virtual float altitudeFromScale(float scale);
	virtual intptr_t _demLoadCompleteForGroundElement(char *args);
	virtual intptr_t _internetStateWasChanged(char *args);

	// The direction the camera looks in; `up` gets its up vector. Both are
	// cached until the parameters change (or, while flying, recomputed).
	Vector3 viewDirection(Vector3 *up);
	void _fillFlatViewDir(Vector2 *out);
	// Turns the camera at `earthVec` looking along `viewDir` by `delta` and
	// moves the position there (no more than 0.01 of longitude at once).
	void setupViewParams(const Vector3 *earthVec, const Vector3 *viewDir, LPoint delta);
	WeatherTracer *weatherTracer() const { return _weatherTracer; }

	PerspectiveScene *_scene;
	Vector3 _viewDir;	// cached by viewDirection
	Vector3 _upVector;
	bool _viewDirValid;
	int _unused2A0;
	int _unused2A4;
	Vector3 _dragViewDir;	// when the direct drag started
	Vector3 _dragEarthVec;
	float _dragAngle;
	LPoint _dragDiff;	// the last
	WeatherTracer *_weatherTracer;
};

#endif
