// LRunLoop
//
// LF::LRunLoop: posted messages (invocations) go into a locked array and a
// CFRunLoop source is signalled; timers are CFRunLoopTimers that send a timer
// message. While the app tracks the mouse or a menu, messages are put aside
// and replayed on exit.
//
// A waiting postInvocation() blocks on the invocation's condition. The
// original waited without locking its mutex or checking a predicate, which
// could miss the wakeup; here the mutex is held and LInvocation::_finished
// is the predicate.

#include "LRunLoop.h"
#include "LInvocation.h"
#include "LString.h"
#include "LUndoManager.h"

namespace LF {

LF_IMPLEMENT_CLASS(LRunLoop, 0)

namespace {

// A timer and the mode it was installed with, by CFRunLoopTimer.
class _LTimerWithModeWrapper : public LObject {
public:
	_LTimerWithModeWrapper() : _timer(0), _mode(0) {}
	virtual void dealloc()
	{
		if (_timer) {
			_timer->release();
			_timer = 0;
		}
		LObject::dealloc();
	}

	LTimer *_timer;
	unsigned int _mode;
};

class _timersMap : public TLIntPtrMap<_LTimerWithModeWrapper *> {
public:
	void setTimerForID(LTimer *timer, unsigned int mode, CFRunLoopTimerRef timerID)
	{
		_LTimerWithModeWrapper *w = new _LTimerWithModeWrapper;
		w->_timer = LRetain(timer);
		w->_mode = mode;
		setObjectForKey(w, (intptr_t)timerID);
		w->release();
	}
	void getIDsForTimer(LTimer *timer, TArray<CFRunLoopTimerRef> *ids)
	{
		hashTableIterator it(this);
		while (it.next()) {
			Entry *e = it.entry();
			if (e->value->_timer == timer)
				ids->addObject((CFRunLoopTimerRef)e->key);
		}
	}
};

_timersMap *sTimersMap;
CSharableResource sTimersMapLocker;

} // namespace

LRunLoop *LRunLoop::initWithMessagePoster(LThread::IMessagePoster *poster, LThread *thread)
{
	if (init() == 0)
		return 0;
	_poster = poster;
	_thread = thread;
	CFRunLoopRef loop = CFRunLoopGetCurrent();
	_cfRunLoop = loop;
	CFRunLoopSourceContext ctx;
	memset(&ctx, 0, sizeof ctx);
	ctx.info = this;
	ctx.perform = _processSignal;
	_source = CFRunLoopSourceCreate(0, 0, &ctx);
	CFRunLoopAddSource(loop, _source, kCFRunLoopCommonModes);
	return this;
}

void LRunLoop::processInvocation(LInvocation *inv)
{
	inv->invoke();
	pthread_mutex_t *mutex = inv->_mutex;
	pthread_cond_t *cond = inv->_cond;
	if (cond) {
		pthread_mutex_lock(mutex);
		inv->_finished = true;
		inv->release();
		pthread_cond_broadcast(cond);
		pthread_mutex_unlock(mutex);
	} else {
		inv->release();
	}
}

void LRunLoop::_processTimer(CFRunLoopTimerRef timer, void *info)
{
	((LRunLoop *)info)->processRunLoopMessage(kTimerMessage, (uintptr_t)timer, 0);
}

void LRunLoop::onEnterTrackLoop()
{
	_trackLoopMessages = new MessageArray;
}

void LRunLoop::onExitTrackLoop()
{
	MessageArray *messages = _trackLoopMessages;
	_trackLoopMessages = 0;
	for (int i = 0; i < (int)(messages ? messages->count() : 0); i++) {
		SMessageInfo &m = (*messages)[i];
		processRunLoopMessage(m.msg, m.param1, m.param2);
	}
	delete messages;
}

void LRunLoop::dealloc()
{
	invalidateAllPostedInvocations();
	if (_trackLoopMessages) {
		delete _trackLoopMessages;
		_trackLoopMessages = 0;
	}
	LObject::dealloc();
}

void LRunLoop::invalidateAllPostedInvocations()
{
	// The original released the source here and again on dealloc.
	if (_source) {
		CFRunLoopSourceInvalidate(_source);
		CFRelease(_source);
		_source = 0;
	}
	StSharableLocker lock(&_postedLock);
	int n = _posted ? (int)_posted->count() : 0;
	for (int i = 0; i < n; i++) {
		SMessageInfo &m = (*_posted)[i];
		if (m.msg == kInvocationMessage && m.param1 != 0)
			((LInvocation *)m.param1)->release();
	}
	if (_posted) {
		delete _posted;
		_posted = 0;
	}
}

void LRunLoop::_processSignal(void *info)
{
	LRunLoop *self = (LRunLoop *)info;
	MessageArray *messages = new MessageArray;
	{
		StSharableLocker lock(&self->_postedLock);
		if (self->_posted == 0) {
			delete messages;
			return;
		}
		messages->addObjects(self->_posted->items(), self->_posted->count());
		self->_posted->removeAllObjects(0);
	}
	int n = messages->count();
	for (int i = 0; i < n; i++) {
		SMessageInfo &m = (*messages)[i];
		self->processRunLoopMessage(m.msg, m.param1, m.param2);
	}
	delete messages;
}

bool LRunLoop::postThreadMessage(LThread *thread, unsigned int msg, uintptr_t param1, uintptr_t param2)
{
	if (_poster)
		return _poster->postThreadMessage(thread, msg, param1, param2);
	SMessageInfo m = { msg, param1, param2 };
	{
		StSharableLocker lock(&_postedLock);
		if (_posted == 0)
			_posted = new MessageArray;
		_posted->addObject(m);
	}
	if (_source) {
		CFRunLoopSourceSignal(_source);
		// The run loop may be asleep; the original waited for its next event.
		CFRunLoopWakeUp(_cfRunLoop);
	}
	return true;
}

void LRunLoop::postInvocation(LInvocation *inv)
{
	inv->retain();
	pthread_cond_t *cond = inv->_cond;
	pthread_mutex_t *mutex = inv->_mutex;
	if (mutex && cond) {
		pthread_mutex_lock(mutex);
		postThreadMessage(_thread, kInvocationMessage, (uintptr_t)inv, 0);
		while (!inv->_finished)
			pthread_cond_wait(cond, mutex);
		pthread_mutex_unlock(mutex);
		pthread_mutex_destroy(mutex);
		free(mutex);
		pthread_cond_destroy(cond);
		free(cond);
		inv->_mutex = 0;
		inv->_cond = 0;
	} else {
		postThreadMessage(_thread, kInvocationMessage, (uintptr_t)inv, 0);
	}
}

bool LRunLoop::processRunLoopMessage(unsigned int msg, uintptr_t param1, uintptr_t param2)
{
	bool handled = false;
	if (msg == kTimerMessage) {
		if (param2 == 0) {
			LTimer *timer = 0;
			unsigned int mode = 0;
			bool haveMap;
			{
				StSharableLocker lock(&sTimersMapLocker);
				haveMap = sTimersMap != 0;
				if (haveMap) {
					_LTimerWithModeWrapper *w = sTimersMap->objectForKey((intptr_t)param1);
					if (w) {
						timer = w->_timer;
						mode = w->_mode;
					}
				}
			}
			if (haveMap && timer) {
				if (_trackLoopMessages && (mode & kTimerModeTracking) == 0) {
					// Keep one firing per timer for after the tracking loop.
					for (int i = 0; i < (int)_trackLoopMessages->count(); i++)
						if ((*_trackLoopMessages)[i].param1 == param1)
							return false;
					SMessageInfo m = { kTimerMessage, param1, 0 };
					_trackLoopMessages->addObject(m);
				} else if (mode & kTimerModeDefault) {
					timer->retain();
					LAutoreleasePool::addNewPoolToTop();
					timer->fireImmediately();
					if (!timer->_repeats)
						timer->invalidate();
					LAutoreleasePool::removeTopPool();
					timer->release();
					handled = true;
				}
			}
		}
	} else if (msg == kInvocationMessage) {
		if (_trackLoopMessages == 0) {
			LAutoreleasePool::addNewPoolToTop();
			processInvocation((LInvocation *)param1);
			LAutoreleasePool::removeTopPool();
			handled = true;
		} else {
			SMessageInfo m = { kInvocationMessage, param1, param2 };
			_trackLoopMessages->addObject(m);
		}
	}
	if (_undoManager)
		_undoManager->runLoopProcessedEvent();
	return handled;
}

void LRunLoop::installTimer(LTimer *timer, unsigned int mode)
{
	CFRunLoopTimerContext ctx;
	memset(&ctx, 0, sizeof ctx);
	ctx.info = this;
	double interval = timer->_interval / 1000.0;
	CFRunLoopTimerRef t = CFRunLoopTimerCreate(0, interval + CFAbsoluteTimeGetCurrent(),
	                                           timer->_repeats ? interval : 0.0, 0, 0, _processTimer, &ctx);
	StSharableLocker lock(&sTimersMapLocker);
	if (sTimersMap == 0)
		sTimersMap = new _timersMap;
	sTimersMap->setTimerForID(timer, mode, t);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), t, kCFRunLoopCommonModes);
}

void LRunLoop::uninstallTimer(LTimer *timer)
{
	StSharableLocker lock(&sTimersMapLocker);
	if (sTimersMap == 0)
		return;
	TArray<CFRunLoopTimerRef> ids;
	sTimersMap->getIDsForTimer(timer, &ids);
	for (int i = 0; i < (int)ids.count(); i++) {
		CFRunLoopTimerRef t = ids[i];
		if (CFRunLoopTimerIsValid(t))
			CFRunLoopTimerInvalidate(t);
		sTimersMap->removeObjectForKey((intptr_t)t);
		CFRelease(t);
	}
}

} // namespace LF
