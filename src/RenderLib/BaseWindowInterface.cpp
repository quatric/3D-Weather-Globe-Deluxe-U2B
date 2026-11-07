// The window's base: the 3D view and the panel controllers.

#include "RenderLib.h"

BaseWindowInterface::BaseWindowInterface(void *nativeView, CDeviceFormat *format)
    : _fullScreen(false), _coordinatesPicker(false), _markerVisible(false), _markerPosition(LZeroPoint)
{
	_view = Custom3DView::_alloc()->initWithParams(this, nativeView, format);
	_panels = LArray::_alloc()->init();
	_panel = 0;
	_logoSwitcher = _LogoSceneEndSwitcher::_alloc()->initWithBaseInterface(this);
}

BaseWindowInterface::~BaseWindowInterface()
{
	releaseAndClear(_logoSwitcher);
	if (_panel) {
		_panel->sceneWasUninstalled();
		_panel->invalidate();
		_panel = 0;
	}
	releaseAndClear(_panels);
	releaseAndClear(_view);
}

// The buttons differ in full screen.
void BaseWindowInterface::enterFullScreenMode()
{
	_fullScreen = true;
	if (_panel) {
		_panel->uninstallButtonsIfNeeded();
		_panel->installButtonsIfNeeded();
	}
}

void BaseWindowInterface::exitFullScreenMode()
{
	_fullScreen = false;
	if (_panel) {
		_panel->uninstallButtonsIfNeeded();
		_panel->installButtonsIfNeeded();
	}
}

void BaseWindowInterface::setCoordinatesPickerMode(unsigned char on)
{
	_coordinatesPicker = on;
	_panel->updateCursor();
}

void BaseWindowInterface::setAnimatedMarkerPosition(LPoint geoPoint)
{
	_markerVisible = true;
	_markerPosition = geoPoint;
}

LPoint BaseWindowInterface::animatedMarkerPosition(unsigned char *visible)
{
	if (visible)
		*visible = _markerVisible;
	return _markerPosition;
}

C3DView *BaseWindowInterface::acceleratedView()
{
	return _view;
}

LSize BaseWindowInterface::acceleratedViewSize()
{
	if (!_view)
		return LZeroSize;
	return _view->frame().size;
}

void BaseWindowInterface::refreshAcceleratedView()
{
	if (_view)
		_view->refresh();
}

SharedContextObjectsContainer *BaseWindowInterface::contextObjectsContainer()
{
	return _view ? _view->_objectsContainer : 0;
}

CContextCaps *BaseWindowInterface::contextCaps()
{
	if (!_view || !_view->_context)
		return 0;
	return _view->_context->caps();
}

bool BaseWindowInterface::isVideoCardSupported()
{
	CContextCaps *caps = contextCaps();
	return caps && caps->CompressedTexturesAreSupported();
}

// The controllers are kept once made; the saver modes play the saver's
// flight in the globe's or the flat map's.
void BaseWindowInterface::openPanel(int mode)
{
	LAutoreleasePool::addNewPoolToTop();
	int current = Preferences::mapMode();
	LClass *panelClass;
	LClass *sequenceClass = 0;
	switch (mode) {
	case 0:
		panelClass = PerspPanelController::classObject();
		break;
	case 1:
		panelClass = FlatMapController::classObject();
		break;
	case 2:
		panelClass = PerspPanelController::classObject();
		sequenceClass = CinematicSaver::classObject();
		break;
	case 3:
		panelClass = FlatMapController::classObject();
		sequenceClass = CinematicSaver::classObject();
		break;
	case 4:
		panelClass = LogoController::classObject();
		break;
	default:
		LF_RAISE(NilPtrException());
		panelClass = 0;
		break;
	}

	if (current != mode || !_panel || _panel->getClass() != panelClass) {
		Preferences::setMapMode((TMapMode)mode);
		stopAnyFlying();
		if (_panel) {
			_panel->sceneWasUninstalled();
			_panel->invalidate();
		}
		AtlasPanelController *panel = 0;
		int count = _panels->count();
		for (int i = 0; i < count; i++) {
			LObject *made = _panels->objectAtIndex(i);
			if (made->getClass() == panelClass) {
				panel = static_cast<AtlasPanelController *>(LRetain(made));
				break;
			}
		}
		if (!panel) {
			panel = static_cast<AtlasPanelController *>(panelClass->alloc())->initWithMainWindowInterface(this);
			_panels->addObject(panel);
		}
		_panel = panel;

		BaseScene *scene = _panel->scene();
		if ((unsigned int)(mode - 2) < 2) {
			scene->setRenderOptions(RenderOptionsSaver::options());
			if (sequenceClass) {
				LObject *sequence = LAutorelease(sequenceClass->alloc()->init());
				if (sequence && sequence->getClass()->isSubclassOf(CinematicSequence::classObject()))
					static_cast<CinematicSequence *>(sequence)->play(scene);
			}
		}
		_panel->sceneWillBeInstalled();
		acceleratedView()->setScene(scene);
		acceleratedView()->refresh();
		_panel->setScale(_panel->scale());
	}
	LAutoreleasePool::removeTopPool();
}

void BaseWindowInterface::startDemo(unsigned char saver)
{
	if (_logoSwitcher) {
		_logoSwitcher->_intro = false;
		_logoSwitcher->_toggleFullScreen = saver;
	}
	openPanel(0);
	_panel->startDemoSequence();
	_panel->scene()->setControlsVisibility(0);
}

void BaseWindowInterface::startIntro()
{
	if (_logoSwitcher) {
		_logoSwitcher->_intro = true;
		_logoSwitcher->_toggleFullScreen = false;
	}
	openPanel(0);
	_panel->startIntro();
	_panel->scene()->setControlsVisibility(0);
}
