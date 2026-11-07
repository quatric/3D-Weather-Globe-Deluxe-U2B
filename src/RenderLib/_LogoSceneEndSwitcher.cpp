// Ends the intro and the saver's demo.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(_LogoSceneEndSwitcher, 0)
LF_SELECTOR(_LogoSceneEndSwitcher, _cinematicWasInterrupted);
LF_SELECTOR(_LogoSceneEndSwitcher, _doToggleFS);
LF_SELECTOR(_LogoSceneEndSwitcher, _demoSequenceWasEnded);

_LogoSceneEndSwitcher *_LogoSceneEndSwitcher::initWithBaseInterface(BaseWindowInterface *wnd)
{
	if (!LObject::init())
		return 0;
	_intro = true;
	_toggleFullScreen = false;
	_window = wnd;
	LNotificationCenter *center = LNotificationCenter::center();
	center->addObserver(this, "_cinematicWasInterrupted", AtlasPanelController::kCinematicStoppedNotification, 0);
	center->addObserver(this, "_demoSequenceWasEnded", AtlasPanelController::kDemoSequenceWasEndedNotification,
	                    0);
	return this;
}

void _LogoSceneEndSwitcher::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	LObject::dealloc();
}

intptr_t _LogoSceneEndSwitcher::_cinematicWasInterrupted(char *args)
{
	if (_toggleFullScreen)
		invokeSelectorInMainThreadWithObjects("_doToggleFS", false, false, *(LObject **)args);
	return 0;
}

intptr_t _LogoSceneEndSwitcher::_doToggleFS(char *)
{
	_window->toogleFullScreen();
	return 0;
}

intptr_t _LogoSceneEndSwitcher::_demoSequenceWasEnded(char *)
{
	_window->_panel->startDemoSequence();
	return 0;
}
