// LThread
//
// LF::LThread: an abstract thread object. _executeThread() starts a pthread
// that calls run() and then drops the reference the thread held. All
// threads are listed so currentThread() can find the caller's.

#include "LRunLoop.h"

namespace LF {

LF_IMPLEMENT_CLASS(LThread, 0)

static CSharableResource sAllThreadsMutex;
static TArray<LThread *> *sAllThreads;
// Created during static initialisation, so it is the main thread.
LThread *LThread::sMainThread = new LMainThread;

LThread::LThread() : _thread(0), _runLoop(0), _poster(0)
{
	StSharableLocker lock(&sAllThreadsMutex);
	if (sAllThreads == 0)
		sAllThreads = new TArray<LThread *>;
	LThread *self = this;
	sAllThreads->addObject(self);
}

LRunLoop *LThread::runLoop()
{
	if (_runLoop == 0)
		_runLoop = LRunLoop::_alloc()->initWithMessagePoster(_poster, this);
	return _runLoop;
}

// The original then broadcast a copy of a never-initialised condition
// variable, which did nothing.
void *LThread::_threadEnterFunc(void *param)
{
	LThread *self = (LThread *)param;
	intptr_t result = self->run();
	self->release();
	return (void *)result;
}

void LThread::_executeThread()
{
	retain();
	pthread_create(&_thread, 0, _threadEnterFunc, this);
}

LThread *LThread::currentThread()
{
	pthread_t self = pthread_self();
	StSharableLocker lock(&sAllThreadsMutex);
	int n = sAllThreads ? (int)sAllThreads->count() : 0;
	for (int i = 0; i < n; i++) {
		LThread *t = (*sAllThreads)[i];
		if (pthread_equal(t->_thread, self))
			return t;
	}
	return 0;
}

void LThread::performSingleAction(LObject *target, const char *sel, LObject *param)
{
	LSingleActionThread::_alloc()->initWithParams(target, sel, param)->release();
}

void LThread::dealloc()
{
	StSharableLocker lock(&sAllThreadsMutex);
	LThread *self = this;
	sAllThreads->removeObject(self);
	if (_runLoop) {
		_runLoop->release();
		_runLoop = 0;
	}
	LObject::dealloc();
}

} // namespace LF
