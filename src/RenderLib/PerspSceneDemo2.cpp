// Demo 2: the sun comes up while the camera comes down.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PerspSceneDemo2, LF_CLASSOF(CinematicSequence))

PerspSceneDemo2::PerspSceneDemo2()
{
	_duration = 60.0f;
}

void PerspSceneDemo2::_onPlaybackFinished()
{
	TimeController *time = TimeController::instance();
	time->setTimeZone(0);
	time->setUseCustomTime(false);
	CinematicSequence::_onPlaybackFinished();
}

void PerspSceneDemo2::_onPlayback(float)
{
	TimeController *time = TimeController::instance();
	{
		BaseScene::StDisableRefreshOnTimeChange noRefresh(*_scene);
		time->setNormalizedDayTime(_progress * 0.17f + 0.38f);
	}
	AtlasPanelController *controller = _scene->_controller;
	controller->setScale(_progress * _progress * 0.9f + 0.1f);
	if (!controller->getClass()->isSubclassOf(PerspPanelController::classObject()))
		return;
	PerspPanelController *persp = static_cast<PerspPanelController *>(controller);
	persp->setVertAngle(_progress * _progress * 1.3f - 0.99f);
	persp->setHorizAngle(_progress * -0.08f + 0.856f);
	float f = 1.0f - _progress;
	f = 1.0f - f * f;
	Vector2 position = Vector2(0.74f, 0.61f) * f + Vector2(0.4f, 0.95f) * (1.0f - f);
	LPoint p = {position.x, position.y};
	persp->setPosition(p);
}

void PerspSceneDemo2::_onPlaybackStarted()
{
	CinematicSequence::_onPlaybackStarted();
	TimeController *time = TimeController::instance();
	time->setUseCustomTime(true);
	time->setTimeZone(LTimeZone::timeZoneForSecondsFromGMT(0));
	time->setDayMonthYearFromDate(LDate::dateWithTime(1999, 8, 11, 12, 0, 0, LTimeZone::timeZoneForSecondsFromGMT(0)));
	time->setNormalizedDayTime(0.35f);
}
