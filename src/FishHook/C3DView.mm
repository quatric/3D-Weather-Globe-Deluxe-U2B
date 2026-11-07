// C3DView: a 3D view's context, holder and scene, drawn in a C3DViewWrapper.

#include "C3DViewWrapper.h"
#include "FHScene.h"
#include "LKeyRemap.h"
#include "LDate.h"

LF_IMPLEMENT_CLASS(C3DView, 0)

#define WRAPPER ((C3DViewWrapper *)_sysView)

static NSWindow *_sCapturingWindow = nil;

C3DView::C3DView()
    : _deviceLost(false), _sysView(0), _contextView(0), _context(0), _holder(0), _scene(0), _framesCount(0),
      _lastFPSTime(-1.0), _fps(0), _mpps(0), _unused(0), _savedSuperview(0), _cachedImageView(0), _lastPrintTime(0)
{
	_savedSize.width = _savedSize.height = 0;
	_savedOrigin.x = _savedOrigin.y = 0;
}

C3DView *C3DView::initWithParams(void *systemView, CDeviceFormat *format)
{
	if (LObject::init() == 0)
		return 0;
	_holder = CHolder::_alloc()->initWithView(this);
	adjustToSystemView(systemView, format);
	return this;
}

void C3DView::dealloc()
{
	adjustToSystemView(0, 0);
	LRelease(_scene);
	_scene = 0;
	if (_holder)
		_holder->adoptToContext(0);
	delete _context;
	_context = 0;
	LRelease(_holder);
	_holder = 0;
	LObject::dealloc();
}

void C3DView::drawInPreparedEnvironment()
{
	if (_scene)
		_scene->drawScene();
}

void C3DView::setScene(CScene *scene)
{
	LRetain(scene);
	LRelease(_scene);
	_scene = scene;
}

// Frames and primitives a second, measured over a second or more.
void C3DView::updateFPS()
{
	double now = LDate::currentTimeIntervalSince1970();
	if (_lastFPSTime < 0) {
		_lastFPSTime = now;
		return;
	}
	double elapsed = now - _lastFPSTime;
	if (elapsed > 1.0) {
		_fps = (float)(_framesCount / elapsed);
		_framesCount = 0;
		_lastFPSTime = now;
		_mpps = (float)((_context->_primitivesCount / 1000000.0) / elapsed);
		_context->_drawCallsCount = 0;
		_context->_primitivesCount = 0;
	}
}

// Events go to the scene with the modifiers and key codes made portable.
bool C3DView::mouseDown(unsigned int modifiers, LPoint where)
{
	if (_scene)
		_scene->mouseDown(LKeyRemap::modifiersFromSystemModifiers(modifiers), where);
	return true;
}

bool C3DView::mouseMove(unsigned int modifiers, LPoint where)
{
	if (_scene)
		_scene->mouseMove(LKeyRemap::modifiersFromSystemModifiers(modifiers), where);
	return true;
}

bool C3DView::mouseUp(unsigned int modifiers, LPoint where)
{
	if (_scene)
		_scene->mouseUp(LKeyRemap::modifiersFromSystemModifiers(modifiers), where);
	return true;
}

bool C3DView::mouseWheel(short delta, unsigned int modifiers, LPoint where)
{
	if (_scene)
		_scene->mouseWheel(delta, LKeyRemap::modifiersFromSystemModifiers(modifiers), where);
	return true;
}

bool C3DView::mouseDblClick(unsigned int modifiers, LPoint where)
{
	if (_scene)
		_scene->mouseDblClick(modifiers, where);
	return true;
}

bool C3DView::mouseDragged(unsigned int modifiers, LPoint where)
{
	if (_scene)
		_scene->mouseDragged(LKeyRemap::modifiersFromSystemModifiers(modifiers), where);
	return true;
}

bool C3DView::dragStarted(unsigned int modifiers, LPoint where)
{
	if (_scene)
		_scene->dragStarted(LKeyRemap::modifiersFromSystemModifiers(modifiers), where);
	return true;
}

bool C3DView::dragFinished(unsigned int modifiers, LPoint where)
{
	if (_scene)
		_scene->dragFinished(LKeyRemap::modifiersFromSystemModifiers(modifiers), where);
	return true;
}

bool C3DView::onMouseLeave(unsigned int modifiers, LPoint where)
{
	if (_scene)
		_scene->onMouseLeave(LKeyRemap::modifiersFromSystemModifiers(modifiers), where);
	return true;
}

bool C3DView::canBecameInFocus()
{
	return _scene ? _scene->canBecameInFocus() : false;
}

bool C3DView::becameInFocus()
{
	return _scene ? _scene->becameInFocus() : false;
}

bool C3DView::resignFocus()
{
	return _scene ? _scene->resignFocus() : false;
}

bool C3DView::onKeyDown(unsigned int keycode, unsigned int modifiers)
{
	return _scene ? _scene->onKeyDown(LKeyRemap::keycodeFromSystemKeycode(keycode), modifiers) : false;
}

bool C3DView::onKeyUp(unsigned int keycode, unsigned int modifiers)
{
	return _scene ? _scene->onKeyUp(LKeyRemap::keycodeFromSystemKeycode(keycode), modifiers) : false;
}

// A new context when the view changed or the context cannot take the
// format; otherwise the lost device is reset when it can be.
void C3DView::recreateContext(CDeviceFormat *format)
{
	bool recreate = _contextView != _sysView;
	if (!recreate && _context)
		recreate = !_context->canBeResetToFormat(format);
	if (recreate) {
		if (_context) {
			_holder->adoptToContext(0);
			delete _context;
			_context = 0;
		}
		_contextView = _sysView;
	}
	if (_context == 0) {
		if (_sysView) {
			_deviceLost = false;
			_context = new CContext(_holder, format);
			_holder->adoptToContext(_context);
		}
	} else if (_context->testCooperativeLevel() != D3DERR_DEVICELOST) {
		LAutoreleasePool::addNewPoolToTop();
		_holder->resetCurrentContext(0);
		LAutoreleasePool::removeTopPool();
	} else
		_deviceLost = true;
}

// Draws a frame: the scene renders into the context's target, then the
// frame is ended and presented when asked.
void C3DView::drawAccelerated(bool postNotification, bool endFrame, bool present)
{
	if (_context == 0)
		return;
	if (_deviceLost) {
		HRESULT hr = _context->testCooperativeLevel();
		if (hr == D3DERR_DEVICELOST)
			return;
		if (hr == D3DERR_DEVICENOTRESET) {
			LAutoreleasePool::addNewPoolToTop();
			_holder->resetCurrentContext(0);
			LAutoreleasePool::removeTopPool();
		}
	}
	if (postNotification)
		LNotificationCenter::center()->postNotification(&FrameWillBeRenderedIn3DView, this, 0);
	LAutoreleasePool::addNewPoolToTop();
	CDrawCache::beginFrame();
	if (!postNotification || _context->beginFrame() >= 0) {
		if (_scene) {
			_context->setMainCamera(_scene->camera());
			_scene->frameWillBeRendered();
		}
		_context->setRenderTarget(0);
		_context->clearCurrentRenderTarget();
		drawInPreparedEnvironment();
		_holder->onBackbufferReady();
		if (endFrame) {
			double now = LTimer::currentTimeValue();
			if (_lastPrintTime + 2.0 < now)
				_lastPrintTime = now;
			_context->endFrame();
			if (present)
				_deviceLost = _context->presentFrame() == D3DERR_DEVICELOST;
			CDrawCache::endFrame();
		}
	}
	_framesCount++;
	updateFPS();
	LAutoreleasePool::removeTopPool();
}

bool C3DView::isDoubleBuffered()
{
	GLint value = 0;
	[[WRAPPER pixelFormat] getValues:&value forAttribute:NSOpenGLPFADoubleBuffer forVirtualScreen:0];
	return (value & 0xff) != 0;
}

void C3DView::flushBuffer()
{
	if (isDoubleBuffered())
		[[WRAPPER openGLContext] flushBuffer];
	else
		glFlush();
}

bool C3DView::setCurrentGLContext()
{
	NSOpenGLContext *context = [(C3DViewWrapper *)_contextView openGLContext];
	if (context == nil) {
		[NSOpenGLContext clearCurrentContext];
		return false;
	}
	if (context != [NSOpenGLContext currentContext])
		[context makeCurrentContext];
	return true;
}

float C3DView::userSpaceScaleFactor()
{
	return [WRAPPER _userSpaceScaleFactor];
}

void C3DView::adjustToSystemView(void *systemView, CDeviceFormat *format)
{
	[WRAPPER _adjustToView:0];
	if (systemView && ![(id)systemView isKindOfClass:[C3DViewWrapper class]])
		systemView = 0;
	_sysView = systemView;
	recreateContext(format);
	[WRAPPER _adjustToView:this];
}

LRect C3DView::frame()
{
	NSRect bounds = [WRAPPER bounds];
	LRect r = {{(float)bounds.origin.x, (float)bounds.origin.y}, {(float)bounds.size.width, (float)bounds.size.height}};
	return r;
}

void C3DView::refresh()
{
	[WRAPPER setNeedsDisplay:YES];
}

bool C3DView::_windowIsVisible()
{
	return [(C3DViewWrapper *)_contextView window] != nil;
}

// Moves the view into an offscreen window of the size, leaving its cached
// image in its place.
void C3DView::enterCaptureModeWithSize(LSize size)
{
	NSRect frame = [WRAPPER frame];
	_savedOrigin.x = frame.origin.x;
	_savedOrigin.y = frame.origin.y;
	_savedSize.width = frame.size.width;
	_savedSize.height = frame.size.height;
	_savedSuperview = [WRAPPER superview];
	if (_sCapturingWindow == nil)
		_sCapturingWindow = [[NSWindow alloc] initWithContentRect:NSZeroRect
		                                                styleMask:NSWindowStyleMaskBorderless
		                                                  backing:NSBackingStoreBuffered
		                                                    defer:NO];
	[_sCapturingWindow setContentSize:NSMakeSize(size.width, size.height)];
	[WRAPPER createCachedImage];
	[WRAPPER removeFromSuperviewWithoutNeedingDisplay];
	[[_sCapturingWindow contentView] addSubview:WRAPPER];
	[WRAPPER setFrame:NSMakeRect(0, 0, size.width, size.height)];
	[_sCapturingWindow setViewsNeedDisplay:NO];
	_C3DCachedImageView *cached = [[_C3DCachedImageView alloc] initWithFrame:frame image:[WRAPPER _cachedImage]];
	_cachedImageView = cached;
	[(NSView *)_savedSuperview addSubview:cached];
	[cached setNeedsDisplay:YES];
}

void C3DView::exitCaptureMode()
{
	[(NSView *)_cachedImageView removeFromSuperviewWithoutNeedingDisplay];
	[(NSView *)_cachedImageView release];
	_cachedImageView = 0;
	[WRAPPER removeFromSuperview];
	[(NSView *)_savedSuperview addSubview:WRAPPER];
	[WRAPPER setFrame:NSMakeRect(_savedOrigin.x, _savedOrigin.y, _savedSize.width, _savedSize.height)];
	[WRAPPER discardCachedImage];
	_savedSuperview = 0;
}
