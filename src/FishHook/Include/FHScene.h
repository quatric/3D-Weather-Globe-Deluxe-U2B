// Scenes: what a 3D view draws, its lights, clip planes and on-screen
// controls, and the recording and playback of its state.

#ifndef FHSCENE_H
#define FHSCENE_H

#include "FHModels.h"
#include "FHTextures.h"
#include "LRunLoop.h"

class CSceneControl;
class CSceneCapturer;
class CCaptureSession;
class CFontDrawable;
class CFontDrawableData;
class CBillboardModelData;

extern LConstantString SceneStatesWereCaptured;	// object the scene, info {SceneStatesSetKey: set}
extern LConstantString SceneStatesSetKey;

// ---------------------------------------------------------------------------
// CScenePropertiesSet: the states of a frame's objects, each a record
// {data offset, record size, flags, name, state}.

class CScenePropertiesSet : public LObject {
	LF_DECLARE_CLASS(CScenePropertiesSet)
public:
	CScenePropertiesSet();
	virtual CScenePropertiesSet *init();
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual void dealloc();

	void storePropertyOwner(CSceneStatePropertyOwner *owner, unsigned int flags);
	void storeDrawableData(CDrawableModelData *data, TOrder order);
	void captureStartedWithScene(CScene *scene);
	void captureFinished() {}
	void drawStoredObjects(CScene *scene);

	LData *_data;
};

// ---------------------------------------------------------------------------
// CCaptureSession: recorded frames and their times.

class CCaptureSession : public LObject {
	LF_DECLARE_CLASS(CCaptureSession)
public:
	CCaptureSession();
	virtual CCaptureSession *init();
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual void dealloc();

	void startCapturing();
	void stopCapturing() { _capturing = false; }
	bool isCapturing() const { return _capturing; }
	bool isEmpty() const { return _frames == 0 || _frames->count() == 0; }
	double duration() const;
	void addFrameForTime(CScenePropertiesSet *frame, double time);
	// The frame shown at `time`, with its start time and duration.
	CScenePropertiesSet *frameSetForTime(double time, double *frameTime, double *frameDuration);
	LData *dataForObjectWithID(LString *id);
	void setDataForObjectWithID(LData *data, LString *id);

	bool _capturing;
	double _startTime;
	LArray *_frames;
	TArray<double> *_times;
	LMap *_objectsData;
};

// ---------------------------------------------------------------------------
// CSceneCapturer: records a scene's frames and plays them back or grabs
// them as pictures.

class CSceneCapturer : public LObject {
	LF_DECLARE_CLASS(CSceneCapturer)
public:
	CSceneCapturer();
	virtual void dealloc();
	virtual intptr_t _onPlayerTimer(char *args);	// the timer

	CSceneCapturer *initWithScene(CScene *scene);
	void sceneWillDie() { _scene = 0; }
	void startCapturing();
	void stopCapturing();
	void clearCapturedData();
	bool isCapturing() const { return _session && _session->isCapturing(); }
	bool isEmpty() const { return _session == 0 || _session->isEmpty(); }
	CCaptureSession *session() const { return _session; }
	void setSession(CCaptureSession *session);
	void addFrameForTime(CScenePropertiesSet *frame, double time)
	{
		if (_session)
			_session->addFrameForTime(frame, time);
	}

	void playInScene(unsigned char loop);
	bool isPlaying() const { return _timer != 0; }
	void stopPlayback();
	void pausePlayback() { _paused = true; }
	void resumePlayback() { _paused = false; }
	float playbackPosition() const { return (float)_position; }
	void setPlaybackPosition(float position) { _position = position; }
	float playbackDuration() const;

	bool enterGrabMode(LSize size, LSize virtualFrameSize);
	void exitGrabMode();
	bool isInGrabMode() const { return _grabber != 0; }
	LSize grabbingFrameSize() const;
	LData *grabbedData() const { return _grabbedData; }
	unsigned int grabbedDataRowSize() const;
	bool grabFrameAt(double time, double *frameTime, double *frameDuration);
	LData *simpleGrabFrameAtTime(double time, int width, int height, int *rowBytes);

	CScene *_scene;
	CCaptureSession *_session;
	LTimer *_timer;
	bool _paused;
	bool _loop;
	double _position;
	double _lastTime;
	LargeBBEmulator *_grabber;
	LargeBBEmulator **_grabbing;	// holds the grabber started
	LData *_grabbedData;
	CScenePropertiesSet *_shownFrame;
};

// ---------------------------------------------------------------------------
// CSceneControl: a button drawn over the scene as a billboard.

enum { kControlStateNormal = 0, kControlStateOver = 1, kControlStatePressed = 2 };

extern LConstantString kControlBillboardModelFHKey;

class CSceneControl : public LObject {
	LF_DECLARE_CLASS(CSceneControl)
public:
	CSceneControl();
	virtual void dealloc();
	// The point is relative to the view rect.
	virtual bool hitTest(LPoint) { return true; }
	virtual void performAction();
	virtual void beginTracking();
	virtual void endTracking();
	virtual void mouseEnter();
	virtual void mouseLeave();
	virtual void wasAddedToScene(CScene *scene) { _scene = scene; }
	virtual void willBeRemovedFromScene(CScene *scene)
	{
		if (_scene == scene)
			_scene = 0;
	}
	virtual void render(CFrameDrawer *drawer, CFontDrawableData *font);
	virtual void _createModelData();

	CSceneControl *initWithPaths(LString *normal, LString *over, LString *alt, CHolder *holder);
	// The control's center at nrm * the view size + off.
	void setNrmPosWithPtOff(LPoint nrm, LPoint off);
	void setViewRect(LRect rect)
	{
		_viewRect = rect;
		_useNrmPos = false;
	}
	LRect viewRect() const { return _viewRect; }
	void evalViewRectForViewFrameWithSize(LSize size);
	void setTransparency(float transparency) { _transparency = transparency; }
	int controlState() const { return _state; }
	void setTagTargetSelector(int tag, LObject *target, const char *selector)
	{
		_tag = tag;
		_target = target;
		_selector = selector;
	}
	int tag() const { return _tag; }
	bool value() const { return _value; }
	void setValue(unsigned char value) { _value = value; }
	void setIsToggle(bool toggle) { _isToggle = toggle; }
	void setTitle(LString *title);
	C2DTexture *normalTexture();
	C2DTexture *overTexture();
	C2DTexture *altTexture();
	LSize size();

	CHolder *_holder;
	CScene *_scene;
	LString *_title;
	C2DTexture *_normalTexture;
	C2DTexture *_overTexture;
	C2DTexture *_altTexture;
	CBillboardModelData *_modelData;
	bool _altIsNormal;	// no alternate picture: the normal one is dimmed
	bool _useNrmPos;
	LPoint _nrmPos;
	LPoint _ptOff;
	LRect _viewRect;
	int _state;
	int _tag;
	LObject *_target;
	const char *_selector;
	bool _isToggle;
	bool _value;
	float _transparency;
	LString *_normalPath, *_overPath, *_altPath;
	bool _tracking;
	bool _inside;
};

// ---------------------------------------------------------------------------
// CScene.

class CScene : public LObject {
	LF_DECLARE_CLASS(CScene)
public:
	CScene();
	virtual void dealloc();
	virtual void setCamera(CContextCamera *camera);
	virtual void refreshView();
	virtual CSceneCapturer *detachCapturer();
	virtual void addControl(CSceneControl *control);
	virtual void removeControl(CSceneControl *control);
	virtual LRect viewRectForControl(CSceneControl *control);
	virtual void performControlsLayoutIfNeeded();
	virtual CSceneControl *controlHitTest(LPoint point);
	virtual void invalControlsLayout() { _layoutValid = false; }
	virtual void setControlsVisibility(unsigned char visible) { _controlsVisible = visible; }
	virtual void playbackStartedWithCapturer(CSceneCapturer *) {}
	virtual void playbackStoppedWithCapturer(CSceneCapturer *) {}
	virtual void capturingStartedWithCapturer(CSceneCapturer *) {}
	virtual void capturingStoppedWithCapturer(CSceneCapturer *) {}
	virtual void storeCaptureSessionConstants(CCaptureSession *) {}
	virtual void setUseConstantsFromSession(CCaptureSession *) {}
	virtual void setGrabbedFrameSource(CScenePropertiesSet *source);
	virtual void adjustEnvBeforeDraw() {}
	virtual void renderAllDrawables(CFrameDrawer *) {}
	virtual void frameWillBeRendered() {}
	virtual void drawScene();
	virtual bool processMessage(unsigned int, unsigned int, unsigned int) { return false; }
	virtual void mouseDown(unsigned int flags, LPoint point);
	virtual void mouseMove(unsigned int flags, LPoint point);
	virtual void mouseUp(unsigned int flags, LPoint point);
	virtual void onMouseLeave(unsigned int, LPoint) {}
	virtual void mouseDblClick(unsigned int, LPoint) {}
	virtual void mouseWheel(short, unsigned int, LPoint) {}
	virtual void mouseDragged(unsigned int flags, LPoint point);
	virtual void dragStarted(unsigned int, LPoint) {}
	virtual void dragFinished(unsigned int, LPoint) {}
	virtual bool canBecameInFocus() { return false; }
	virtual bool becameInFocus() { return true; }
	virtual bool resignFocus() { return true; }
	virtual bool onKeyDown(unsigned int, unsigned int) { return false; }
	virtual bool onKeyUp(unsigned int, unsigned int) { return false; }
	virtual CSceneStatePropertyOwner *stateOwnerWithName(const char *name);
	virtual void _renderControls(CFrameDrawer *drawer);
	virtual void _performControlsLayout(LSize size);

	void initWithHolder(CHolder *holder) { _holder = holder; }
	CHolder *holder() const { return _holder; }
	CContextCamera *camera() const { return _camera; }
	CFrameDrawer *frameDrawer() const { return _frameDrawer; }
	void setFrameDrawer(CFrameDrawer *drawer);
	void setControlsFont(CFontDrawable *font);
	void addLight(CContextLight *light);
	void removeLight(CContextLight *light);
	LArray *lights() const { return _lights; }
	void addClipPlane(CClipPlane *plane) { _clipPlanes->addObject(plane); }
	void removeClipPlane(CClipPlane *plane) { _clipPlanes->removeObject(plane); }
	LArray *clipPlanes() const { return _clipPlanes; }
	void registerStateOwner(CSceneStatePropertyOwner *owner);
	void unregisterStateOwner(CSceneStatePropertyOwner *owner);
	void unregisterSceneCapturer(CSceneCapturer *capturer);
	void storePropertyOwnerIfNeeded(CSceneStatePropertyOwner *owner, unsigned int flags)
	{
		if (_capturedProperties)
			_capturedProperties->storePropertyOwner(owner, flags);
	}
	bool recordsState() const { return _recordsState; }
	void setRecordsState(bool records) { _recordsState = records; }

	CHolder *_holder;
	CSceneControl *_hoveredControl;
	CSceneControl *_pressedControl;
	bool _controlsVisible;
	TArray<CSceneCapturer *> *_capturers;
	CScenePropertiesSet *_capturedProperties;	// while a frame is captured
	CScenePropertiesSet *_grabbedFrameSource;	// a frame played back instead of the drawables
	TStrMap<CSceneStatePropertyOwner *, 128> *_stateOwners;
	LArray *_controls;
	CFontDrawable *_controlsFont;
	bool _recordsState;	// the camera and lights register as state owners
	CContextCamera *_camera;
	int _layoutWidth, _layoutHeight;
	bool _layoutValid;
	CFrameDrawer *_frameDrawer;
	CSharableResource _lightsLock;
	LArray *_lights;
	LArray *_clipPlanes;
};

// The names the applications store their scene camera and lights under.
extern const char *kStateObjNameSceneCamera;
extern const char *kStateObjNameSceneLight0;
extern const char *kStateObjNameSceneLight1;
extern const char *kStateObjNameSceneLight2;
extern const char *kStateObjNameSceneLight3;

#endif
