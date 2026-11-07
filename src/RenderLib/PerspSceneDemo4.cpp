// Demo 4: a storm over the mountains: the clouds thicken, then the
// precipitation changes.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PerspSceneDemo4, LF_CLASSOF(CinematicSequence))

PerspSceneDemo4::PerspSceneDemo4()
{
	_duration = 50.0f;
}

void PerspSceneDemo4::_onPlayback(float)
{
	TimeController *time = TimeController::instance();
	{
		BaseScene::StDisableRefreshOnTimeChange noRefresh(*_scene);
		time->setNormalizedDayTime(0.1f);
	}
	float scale = _progress * 0.6f + 0.8f;
	AtlasPanelController *controller = _scene->_controller;
	controller->setScale(scale < 0.0f ? 0.0f : (scale <= 1.0f ? scale : 1.0f));
	if (controller->getClass()->isSubclassOf(PerspPanelController::classObject())) {
		PerspPanelController *persp = static_cast<PerspPanelController *>(controller);
		persp->setVertAngle(powf(_progress, 3.0f) * 0.5f - 0.3f);
		persp->setTiltAngle(0.0f);
		persp->setHorizAngle(_progress * 1.2f);
		LPoint p = {0.163f, 0.758f};
		persp->setPosition(p);
	}
	float first = (_progress - 0.2f) * 10.0f;
	first = first < 0.0f ? 0.0f : (first <= 1.0f ? first : 1.0f);
	float second = (_progress - 0.4f) * 6.0f;
	second = second < 0.0f ? 0.0f : (second <= 1.0f ? second : 1.0f);
	WeatherTracer *tracer = weatherTracer();
	if (!tracer)
		return;
	if (first < 0.999f) {
		tracer->setCustomSkyState((TSkyCode)7, (TSkyCode)9, first);
		SPrecipitationDescr from((TPrecipitationCode)1, 0.0f);
		SPrecipitationDescr to((TPrecipitationCode)0, 1.0f);
		tracer->setCustomPrecipitations(&from, &to, first);
	} else {
		if (second >= 0.999f)
			return;
		tracer->setCustomSkyState((TSkyCode)9, (TSkyCode)4, second);
		SPrecipitationDescr from((TPrecipitationCode)0, 1.0f);
		SPrecipitationDescr to((TPrecipitationCode)4, 0.7f);
		tracer->setCustomPrecipitations(&from, &to, second);
	}
}

void PerspSceneDemo4::_onPlaybackFinished()
{
	TimeController *time = TimeController::instance();
	time->setTimeZone(0);
	time->setUseCustomTime(false);
	Preferences::setVerticalScaleFactor(1.0f);
	CinematicSequence::_onPlaybackFinished();
}

void PerspSceneDemo4::_onPlaybackStarted()
{
	CinematicSequence::_onPlaybackStarted();
	TimeController *time = TimeController::instance();
	time->setUseCustomTime(true);
	time->setTimeZone(LTimeZone::timeZoneForSecondsFromGMT(0));
	time->setDayMonthYearFromDate(LDate::dateWithTime(2007, 6, 21, 12, 0, 0, LTimeZone::timeZoneForSecondsFromGMT(0)));
	time->setNormalizedDayTime(0.35f);
	Preferences::setVerticalScaleFactor(1.1f);
	if (WeatherTracer *tracer = weatherTracer()) {
		SWeatherInfo weather;
		weather.temperature = 0.0f;
		weather.pressure = 0.0f;
		weather.humidity = 0.0f;
		weather.windSpeed = 10.0f;
		weather.wind = Vector2(-10.0f, -10.0f);
		tracer->setCustomWeatherInfo(&weather);
	}
}
