// Demo 0: a turn around a fixed point while the clouds and the rain come
// in.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PerspSceneDemo0, LF_CLASSOF(CinematicSequence))

PerspSceneDemo0::PerspSceneDemo0()
{
	_duration = 70.0f;
}

void PerspSceneDemo0::_onPlayback(float)
{
	AtlasPanelController *controller = _scene->_controller;
	LPoint position = {0.2799f, 0.6379f};
	controller->setPosition(position);
	controller->setScale(_progress * 0.29f + 0.71f);
	if (controller->getClass()->isSubclassOf(PerspPanelController::classObject())) {
		PerspPanelController *persp = static_cast<PerspPanelController *>(controller);
		persp->setVertAngle(_progress * 0.7f - 0.7f);
		persp->setHorizAngle(_progress * 2.2f + 0.3f);
	}
	float clouds = (_progress - 0.7f) * 10.0f;
	clouds = clouds < 0.0f ? 0.0f : (clouds <= 1.0f ? clouds : 1.0f);
	float rain = (_progress - 0.8f) * 10.0f;
	rain = rain < 0.0f ? 0.0f : (rain <= 1.0f ? rain : 1.0f);
	if (clouds > 0.0f) {
		if (WeatherTracer *tracer = weatherTracer()) {
			tracer->setCustomSkyState((TSkyCode)0, (TSkyCode)7, clouds);
			SPrecipitationDescr none((TPrecipitationCode)0, 0.0f);
			SPrecipitationDescr precip((TPrecipitationCode)2, 1.0f);
			tracer->setCustomPrecipitations(&none, &precip, rain);
		}
	}
}

void PerspSceneDemo0::_onPlaybackFinished()
{
	TimeController *time = TimeController::instance();
	time->setTimeZone(0);
	time->setUseCustomTime(false);
	CinematicSequence::_onPlaybackFinished();
}

void PerspSceneDemo0::_onPlaybackStarted()
{
	CinematicSequence::_onPlaybackStarted();
	TimeController *time = TimeController::instance();
	time->setUseCustomTime(true);
	time->setTimeZone(LTimeZone::timeZoneForSecondsFromGMT(0));
	time->setYear(2000);
	time->setDayOfYear(10);
	time->setNormalizedDayTime(0.575f);
	if (WeatherTracer *tracer = weatherTracer()) {
		SWeatherInfo weather;
		weather.temperature = 0.0f;
		weather.pressure = 0.0f;
		weather.humidity = 0.0f;
		weather.windSpeed = 20.0f;
		weather.wind = Vector2(0.0f, 0.0f);
		tracer->setCustomWeatherInfo(&weather);
	}
}
