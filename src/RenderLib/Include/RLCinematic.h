// Cinematic sequences: the scripted camera flights the scenes play, in the
// screen saver, the demos and the intro.

#ifndef RLCINEMATIC_H
#define RLCINEMATIC_H

#include "AppModel.h"

class BaseScene;
class WeatherTracer;

class CinematicSequence : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(CinematicSequence)
public:
	static LString *kPlaybackStarted;
	static LString *kPlaybackFinished;

	CinematicSequence();
	virtual CinematicSequence *init();
	virtual void dealloc();
	// Attaches itself to the scene and starts.
	virtual void play(BaseScene *scene);
	virtual void stop();
	virtual PreferencesOverloader *prefsOverloader();
	virtual void sceneWillDie();
	virtual void wasAttachedToScene(BaseScene *scene);
	virtual void willDetachFromScene();
	// Moves the sequence on with the time; called each frame.
	virtual void adjustHostScene();
	virtual void _onPlaybackStarted();
	virtual void _onPlaybackFinished();
	// Moves the scene on by `dt` seconds (_progress and _fade are set).
	virtual void _onPlayback(float dt) = 0;

	// The scene's weather, set by the sequence while it plays.
	WeatherTracer *weatherTracer();

	BaseScene *_scene;	// retained
	float _duration;	// seconds
	float _progress;	// 0..1
	float _fade;	// 0 at the ends, 1 in the middle
	float _unused;
	double _lastTime;
	bool _started;
};

// The intro: the camera comes down from space over the home city.
class CinematicIntro : public CinematicSequence {
	LF_DECLARE_CLASS(CinematicIntro)
public:
	CinematicIntro();
	virtual PreferencesOverloader *prefsOverloader();
	virtual void _onPlaybackStarted();
	virtual void _onPlaybackFinished();
	virtual void _onPlayback(float dt);
	virtual CinematicIntro *getOwnerLObject() { return this; }

	LPoint _homeMapPoint;
};

// The screen saver's endless flight: an orbit over the globe that climbs
// over the sea and comes down over land, slowly rolling, or (the other
// point of view) a turn along the equator.
class CinematicSaver : public CinematicSequence {
	LF_DECLARE_CLASS(CinematicSaver)
public:
	CinematicSaver();
	virtual PreferencesOverloader *prefsOverloader() { return 0; }
	virtual void _onPlaybackStarted();
	virtual void _onPlaybackFinished() {}
	virtual void _onPlayback(float dt);
	virtual CinematicSaver *getOwnerLObject() { return this; }
	// `reset` puts the camera on the orbit instead of moving it along.
	virtual void _doSaverAnimation(float dt, unsigned char reset);

	float _orbitAngle;	// 0..1, the orbit's turn around the axis
	float _phase;	// 0..1 along the orbit
	float _spin;	// -0.5..0.5, the camera's roll
	float _spinDirection;	// -1, 0 or 1
	float _scale;	// 0.6..0.95
	TScreenSaverPointOfViewMode _pointOfView;
};

// The globe's demo flights, played in turn by the saver.
#define PERSP_SCENE_DEMO(Name)                                                  \
	class Name : public CinematicSequence {                                  \
		LF_DECLARE_CLASS(Name)                                           \
	public:                                                                  \
		Name();                                                          \
		virtual void _onPlaybackStarted();                               \
		virtual void _onPlaybackFinished();                              \
		virtual void _onPlayback(float dt);                              \
		virtual Name *getOwnerLObject() { return this; }                 \
	};
PERSP_SCENE_DEMO(PerspSceneDemo0)
PERSP_SCENE_DEMO(PerspSceneDemo1)
PERSP_SCENE_DEMO(PerspSceneDemo2)
PERSP_SCENE_DEMO(PerspSceneDemo3)
PERSP_SCENE_DEMO(PerspSceneDemo4)
#undef PERSP_SCENE_DEMO

#endif
