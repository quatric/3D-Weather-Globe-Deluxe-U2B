// Demo 1: a night flight, coming down as it goes.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PerspSceneDemo1, LF_CLASSOF(CinematicSequence))

PerspSceneDemo1::PerspSceneDemo1()
{
	_duration = 70.0f;
}

void PerspSceneDemo1::_onPlayback(float)
{
	AtlasPanelController *controller = _scene->_controller;
	float scale = _progress * 1.05f + 0.1f;
	controller->setScale(scale < 0.0f ? 0.0f : (scale <= 1.0f ? scale : 1.0f));
	if (!controller->getClass()->isSubclassOf(PerspPanelController::classObject()))
		return;
	PerspPanelController *persp = static_cast<PerspPanelController *>(controller);
	persp->setVertAngle(_progress * 0.8f - 0.8f);
	persp->setHorizAngle(_progress * -0.075f + 0.775f);
	float f = 1.0f - _progress;
	f = 1.0f - f * f;
	Vector2 position = Vector2(0.16f, 0.7f) * f + Vector2(0.414f, 0.671f) * (1.0f - f);
	LPoint p = {position.x, position.y};
	persp->setPosition(p);
}

void PerspSceneDemo1::_onPlaybackFinished()
{
	TimeController *time = TimeController::instance();
	time->setTimeZone(0);
	time->setUseCustomTime(false);
	CinematicSequence::_onPlaybackFinished();
}

void PerspSceneDemo1::_onPlaybackStarted()
{
	CinematicSequence::_onPlaybackStarted();
	TimeController *time = TimeController::instance();
	time->setUseCustomTime(true);
	time->setTimeZone(LTimeZone::timeZoneForSecondsFromGMT(0));
	time->setYear(2000);
	time->setDayOfYear(10);
	time->setNormalizedDayTime(0.035f);
	if (WeatherTracer *tracer = weatherTracer()) {
		tracer->setUseCustomWeather(true);
		tracer->setCustomSkyState((TSkyCode)5, (TSkyCode)1, 0.1f);
	}
}
