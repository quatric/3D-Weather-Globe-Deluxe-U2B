// The intro flight.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CinematicIntro, LF_CLASSOF(CinematicSequence))

CinematicIntro::CinematicIntro()
{
	_duration = 10.0f;
	_homeMapPoint = mapPointFromGeoPoint(Preferences::homeCity()->geoPoint(0));
}

PreferencesOverloader *CinematicIntro::prefsOverloader()
{
	return 0;
}

void CinematicIntro::_onPlaybackStarted()
{
	CinematicSequence::_onPlaybackStarted();
}

void CinematicIntro::_onPlaybackFinished()
{
	CinematicSequence::_onPlaybackFinished();
}

// Down from high above, then turning as the horizon comes up; only the
// start fades.
void CinematicIntro::_onPlayback(float)
{
	_fade = _progress * 5.0f;
	if (_fade > 1.0f)
		_fade = 1.0f;
	AtlasPanelController *controller = _scene->_controller;
	controller->setPosition(_homeMapPoint);
	controller->setScale(_progress * 0.9f + 0.1f);
	if (!controller->getClass()->isSubclassOf(PerspPanelController::classObject()))
		return;
	PerspPanelController *persp = static_cast<PerspPanelController *>(controller);
	persp->setVertAngle(_progress * 0.6f - 0.99f);
	float t = (_progress + _progress) - 1.0f;
	if (!(t >= 0.0f))
		t = 0.0f;
	persp->setHorizAngle(t * t * (t * -2.0f + 3.0f));
}
