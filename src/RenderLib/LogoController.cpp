// The logo's panel controller.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(LogoController, LF_CLASSOF(AtlasPanelController))

LogoController::LogoController() : _scene(0)
{
}

void LogoController::dealloc()
{
	if (_scene)
		_scene->invalidate();
	_scene = 0;
	AtlasPanelController::dealloc();
}

BaseScene *LogoController::scene()
{
	return _scene;
}

void LogoController::_terminateCinematics()
{
	AtlasPanelController::_terminateCinematics();
	_scene->stopDemo();
}

void LogoController::sceneWillBeInstalled()
{
	AtlasPanelController::sceneWillBeInstalled();
	_animFlags |= kAnimScaleTimer;
	_setupTimer();
	_scene->rewind();
}

void LogoController::sceneWasUninstalled()
{
	AtlasPanelController::sceneWasUninstalled();
	_animFlags &= ~kAnimScaleTimer;
	_setupTimer();
}

LogoController *LogoController::initWithMainWindowInterface(IWindowInterface *wnd)
{
	if (!AtlasPanelController::initWithMainWindowInterface(wnd))
		return 0;
	_scene = LogoScene::_alloc()->initWithController(this, wnd->acceleratedView()->holder());
	return this;
}
