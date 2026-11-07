// Demo 3: low over the mountains at dawn, the relief raised; clouds
// gather.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PerspSceneDemo3, LF_CLASSOF(CinematicSequence))

PerspSceneDemo3::PerspSceneDemo3()
{
	_duration = 40.0f;
}

void PerspSceneDemo3::_onPlaybackFinished()
{
	TimeController *time = TimeController::instance();
	time->setTimeZone(0);
	time->setUseCustomTime(false);
	Preferences::setVerticalScaleFactor(1.0f);
	CinematicSequence::_onPlaybackFinished();
}

void PerspSceneDemo3::_onPlayback(float)
{
	TimeController *time = TimeController::instance();
	{
		BaseScene::StDisableRefreshOnTimeChange noRefresh(*_scene);
		time->setNormalizedDayTime(_progress * 0.13f + 0.1f);
	}
	float scale = _progress * 0.6f + 0.8f;
	AtlasPanelController *controller = _scene->_controller;
	controller->setScale(scale < 0.0f ? 0.0f : (scale <= 1.0f ? scale : 1.0f));
	if (controller->getClass()->isSubclassOf(PerspPanelController::classObject())) {
		PerspPanelController *persp = static_cast<PerspPanelController *>(controller);
		persp->setVertAngle(-0.2f);
		persp->setTiltAngle((_progress - 0.7f) * 0.1f);
		persp->setHorizAngle(0.94f);
		Vector2 position = Vector2(0.162f, 0.741f) * _progress + Vector2(0.1796f, 0.676f) * (1.0f - _progress);
		LPoint p = {position.x, position.y};
		persp->setPosition(p);
	}
	float clouds = (_progress - 0.2f) * 4.0f;
	clouds = clouds < 0.0f ? 0.0f : (clouds <= 1.0f ? clouds : 1.0f);
	if (WeatherTracer *tracer = weatherTracer())
		tracer->setCustomSkyState((TSkyCode)1, (TSkyCode)2, clouds);
}

void PerspSceneDemo3::_onPlaybackStarted()
{
	CinematicSequence::_onPlaybackStarted();
	TimeController *time = TimeController::instance();
	time->setUseCustomTime(true);
	time->setTimeZone(LTimeZone::timeZoneForSecondsFromGMT(0));
	time->setDayMonthYearFromDate(LDate::dateWithTime(2007, 6, 21, 12, 0, 0, LTimeZone::timeZoneForSecondsFromGMT(0)));
	time->setNormalizedDayTime(0.35f);
	Preferences::setVerticalScaleFactor(1.7f);
	if (WeatherTracer *tracer = weatherTracer()) {
		SWeatherInfo weather;
		weather.temperature = 0.0f;
		weather.pressure = 0.0f;
		weather.humidity = 0.0f;
		weather.windSpeed = 30.0f;
		weather.wind = Vector2(30.0f, -30.0f);
		tracer->setCustomWeatherInfo(&weather);
	}
}
