// Cinematic sequences: the base of the intro, the demos and the saver.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CinematicSequence, 0)

// What the sequences give Preferences::setOverloader while they play.
static PreferencesOverloader sDefaultPrefsOverloader;

CinematicSequence::CinematicSequence()
    : _scene(0), _duration(0.1f), _progress(0.0f), _fade(0.0f), _unused(1.0f), _lastTime(-1.0), _started(false)
{
}

CinematicSequence *CinematicSequence::init()
{
	if (!LObject::init())
		return 0;
	return this;
}

void CinematicSequence::dealloc()
{
	releaseAndClear(_scene);
	LObject::dealloc();
}

PreferencesOverloader *CinematicSequence::prefsOverloader()
{
	return &sDefaultPrefsOverloader;
}

void CinematicSequence::willDetachFromScene()
{
	Preferences::setOverloader(0);
	releaseAndClear(_scene);
}

void CinematicSequence::sceneWillDie()
{
	releaseAndClear(_scene);
}

void CinematicSequence::play(BaseScene *scene)
{
	_lastTime = LTimer::currentTimeValue();
	scene->installCinematicSequence(this);
	_progress = 0.0f;
	_fade = 0.0f;
	_started = false;
}

void CinematicSequence::adjustHostScene()
{
	double now = LTimer::currentTimeValue();
	float dt = (float)(now - _lastTime);
	_lastTime = now;
	if (dt < 0.0f)
		dt = 0.0f;
	else if (dt > 0.1f)
		dt = 0.1f;

	if (!_started) {
		_onPlaybackStarted();
		_started = true;
	}

	float progress = dt / _duration + _progress;
	_progress = progress;
	if (progress > 1.0f) {
		// Detaching releases the scene; _onPlaybackFinished still sees it.
		BaseScene *scene = _scene;
		_progress = 1.0f;
		_fade = 0.0f;
		retain();
		scene->installCinematicSequence(0);
		BaseScene *saved = _scene;
		_scene = scene;
		_onPlaybackFinished();
		_scene = saved;
		release();
		return;
	}

	// The fade covers the first and last 5 seconds (or halves).
	float edge = _duration > 10.0f ? 5.0f / _duration : 0.5f;
	float fade = 1.0f;
	if (progress < edge)
		fade = progress / edge;
	else if (progress > 1.0f - edge)
		fade = (1.0f - progress) / edge;
	_fade = fade;
	_onPlayback(dt);
}

WeatherTracer *CinematicSequence::weatherTracer()
{
	AtlasPanelController *controller = _scene->_controller;
	if (controller && controller->getClass()->isSubclassOf(PerspPanelController::classObject()))
		return static_cast<PerspPanelController *>(controller)->_weatherTracer;
	return 0;
}

void CinematicSequence::_onPlaybackFinished()
{
	LNotificationCenter::center()->postNotification(kPlaybackFinished, this, 0);
	if (WeatherTracer *tracer = weatherTracer())
		tracer->setUseCustomWeather(false);
}

// The sequences play in a clear sky.
void CinematicSequence::_onPlaybackStarted()
{
	LNotificationCenter::center()->postNotification(kPlaybackStarted, this, 0);
	if (WeatherTracer *tracer = weatherTracer()) {
		tracer->setUseCustomWeather(true);
		tracer->setCustomSkyState((TSkyCode)0, (TSkyCode)0, 0.0f);
		SPrecipitationDescr precip((TPrecipitationCode)0, 0.0f);
		tracer->setCustomPrecipitations(&precip, &precip, 0.0f);
		SWeatherInfo info;
		info.temperature = 0.0f;
		info.pressure = 0.0f;
		info.humidity = 0.0f;
		info.windSpeed = 0.0f;
		info.wind = Vector2(0.0f, 0.0f);
		tracer->setCustomWeatherInfo(&info);
	}
}

void CinematicSequence::wasAttachedToScene(BaseScene *scene)
{
	LRetain(scene);
	releaseAndClear(_scene);
	_scene = scene;
	Preferences::setOverloader(prefsOverloader());
}

void CinematicSequence::stop()
{
	BaseScene *scene = _scene;
	_lastTime = -1.0;
	retain();
	scene->installCinematicSequence(0);
	BaseScene *saved = _scene;
	_scene = scene;
	_onPlaybackFinished();
	_scene = saved;
	release();
}
