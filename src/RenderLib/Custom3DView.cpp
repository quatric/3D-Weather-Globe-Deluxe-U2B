// The window's 3D view.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(Custom3DView, LF_CLASSOF(C3DView))
LF_SELECTOR(Custom3DView, _texturesSetWasUpdated);

Custom3DView::Custom3DView() : _objectsContainer(0), _unused64(0)
{
}

void Custom3DView::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	_holder->removeObjectForKey(kContextObjectsContainerKey);
	releaseAndClear(_objectsContainer);
	releaseAndClear(_unused64);
	C3DView::dealloc();
}

Custom3DView *Custom3DView::initWithParams(IWindowInterface *wnd, void *systemView, CDeviceFormat *format)
{
	LAutoreleasePool::addNewPoolToTop();
	C3DView::initWithParams(systemView, format);
	_objectsContainer = SharedContextObjectsContainer::_alloc()->initWithHolderAndWndInterface(_holder, wnd);
	_holder->setObjectForKey(_objectsContainer, kContextObjectsContainerKey);
	LNotificationCenter::center()->addObserver(this, "_texturesSetWasUpdated",
	                                           &sLoadedSetWasUpdatedNotification,
	                                           (LObject *)_objectsContainer->_textureLoader);	// LObject is its first base
	LAutoreleasePool::removeTopPool();
	return this;
}

intptr_t Custom3DView::_texturesSetWasUpdated(char *)
{
	refresh();
	return 0;
}

// The new render target takes the scene's fog.
void Custom3DView::recreateContext(CDeviceFormat *format)
{
	C3DView::recreateContext(format);
	if (!_holder || !_holder->context())
		return;
	if (BaseScene *scene = lobj_cast<BaseScene>(_scene))
		_holder->context()->_contextRenderTarget->setFogColor(scene->fogColor());
}

// Not reentered: drawing can pump the run loop.
void Custom3DView::drawAccelerated(bool postNotification, bool endFrame, bool present)
{
	static bool isDrawing = false;
	if (isDrawing)
		return;
	isDrawing = true;
	C3DView::drawAccelerated(postNotification, endFrame, present);
	isDrawing = false;
}
